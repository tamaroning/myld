#include "parsed.h"
#include "utils.h"
#include "myld.h"
#include <cassert>
#include <elf.h>
#include <fmt/core.h>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace Myld {
namespace Elf {
using namespace Myld::Elf;

/*
class SectionBin {
  public:
    SectionBin(std::vector<u8> raw_data) : raw(raw_data) {}

  private:
    const std::vector<u8> raw;
};

class ElfBin {
  public:
    ElfBin() {}

  private:
    Elf64_Ehdr eheader;
    std::vector<Elf64_Phdr> pheader_entries;
    std::vector<Elf64_Shdr> sheader_entries;
    std::vector<SectionBin> sections;
};
*/

// -----
// elf header
// -----
// program header table
// -----
// segment 1
// -----
// ...
// -----
// segment N
// -----
// section header table
//
// segmentは以下の順で並べる
// .text .strtab .shstrtab

using string_table = std::vector<std::pair<const char *, u32>>;
using section_data_raw = std::vector<u8>;

static u64 calc_section_size(string_table str_table) {
    u64 sum = 0;
    for (auto e : str_table) {
        sum += e.second;
    }
    return sum;
}

static u64 calc_section_size(section_data_raw section_raw) { return section_raw.size(); }

static section_data_raw string_table_to_raw(string_table str_table) {
    std::vector<u8> raw;
    for (auto e : str_table) {
        std::vector<u8> s(e.first, e.first + e.second);
        raw.insert(raw.end(), s.begin(), s.end());
    }
    return raw;
}

class Writer {
  public:
    Writer(std::string filename, std::shared_ptr<Parsed::Elf> obj) : filename(filename), obj(obj) {
        fmt::print("preparing elf writer\n");
        stream = std::ofstream(filename, std::ios::binary | std::ios::trunc);

        // hardcode for now
        // seection: .text. .strtab, .shstrtab
        init_info(1, 4);
        init_section_data();
        calculate_offset();
    }

    //~Writer() { stream.close(); }

    void init_info(u64 ph_num, u64 sh_num) {
        fmt::print("initializing elf info\n");
        pheader_entry_num = ph_num;
        sheader_entry_num = sh_num;
    }

    void init_section_data() {
        fmt::print("initializing section data\n");
        str_table = {
            std::pair("\0", 1),
        };

        shstr_table = {
            std::pair("\0", 1),
            std::pair(".strtab\0", 8),
            std::pair(".shstrtab\0", 10),
            std::pair(".text\0", 6),
        };
        sh_name_null_index = 0;
        sh_name_strtab_index = 1;
        sh_name_shstrtab_index = 1 + 8;
        sh_name_text_index = 1 + 8 + 10;
        fmt::print("created string tables\n");

        auto text_section = obj->get_section_by_name(".text");
        if (text_section == nullptr) {
            fmt::print("Couldn't find .text section\n");
            std::exit(1);
        }
        text_section_size = obj->get_section_by_name(".text")->get_size();
        fmt::print("initialized section data\n");
    }

    void calculate_offset() {
        // calc size
        str_table_size = calc_section_size(str_table);
        shstr_table_size = calc_section_size(shstr_table);

        // calc offset
        program_header_ofs = sizeof(Elf64_Ehdr);

        // segmentのスタートを4kに揃える
        segment_ofs = program_header_ofs + sizeof(Elf64_Phdr) * pheader_entry_num;
        if (segment_ofs % 0x1000 != 0) {
            padding_before_null_section = 0x1000 - (segment_ofs % 0x1000);
            segment_ofs += padding_before_null_section;
        }

        section_text_ofs = segment_ofs;
        section_strtab_ofs = section_text_ofs + text_section_size;
        section_shstrtab_ofs = section_strtab_ofs + calc_section_size(str_table);
        section_header_ofs = section_shstrtab_ofs + calc_section_size(shstr_table);
        fmt::print("calculated offset\n");
    }

    void write_file() {
        fmt::print("enter write_file\n");
        std::shared_ptr<Parsed::Section> text_section = obj->get_section_by_name(".text");

        // elf header
        Elf64_Ehdr *elf_header = create_header(pheader_entry_num, sheader_entry_num, section_header_ofs);
        stream.write((char *)elf_header, sizeof(Elf64_Ehdr));

        // program header
        Elf64_Phdr *program_header_entry_load = create_program_header_entry_load();
        stream.write((char *)program_header_entry_load, sizeof(Elf64_Phdr) * pheader_entry_num);

        // padding before null section
        std::fill_n(std::ostream_iterator<char>(stream), padding_before_null_section, '\0');

        fmt::print("writing section data\n");

        // null section
        // do nothing

        // .text
        stream.write(reinterpret_cast<char *>(text_section->get_raw().data()), text_section_size);

        // .strtab
        stream.write(reinterpret_cast<char *>(string_table_to_raw(str_table).data()), str_table_size);
        // .shstrtab
        stream.write(reinterpret_cast<char *>(string_table_to_raw(shstr_table).data()), shstr_table_size);

        // section header
        // null
        Elf64_Shdr *section_header_entry_null = create_section_header_entry_null(sh_name_null_index);
        stream.write(reinterpret_cast<const char *>(section_header_entry_null), sizeof(Elf64_Shdr));
        // .text
        Elf64_Shdr *section_header_entry_text =
            create_section_header_entry_text(sh_name_text_index, text_section->get_size(), section_text_ofs);
        stream.write(reinterpret_cast<const char *>(section_header_entry_text), sizeof(Elf64_Shdr));
        // .strtab
        Elf64_Shdr *section_header_entry_strtab =
            create_section_header_entry_strtab(sh_name_strtab_index, str_table_size, section_strtab_ofs);
        stream.write(reinterpret_cast<const char *>(section_header_entry_strtab), sizeof(Elf64_Shdr));
        // .shstrtab
        Elf64_Shdr *section_header_entry_shstrtab =
            create_section_header_entry_strtab(sh_name_shstrtab_index, shstr_table_size, section_shstrtab_ofs);
        stream.write((char *)section_header_entry_shstrtab, sizeof(Elf64_Shdr));
    }

  private:
    std::string filename;
    std::ofstream stream;

    // number of program header entries
    u64 pheader_entry_num;
    // number of section header entries
    u64 sheader_entry_num;

    // section which has type of SHT_STRTAB
    string_table str_table;
    string_table shstr_table;
    u64 str_table_size;
    u64 shstr_table_size;

    // other sections
    std::vector<section_data_raw> raw_sections;
    u64 text_section_size;

    // section name index
    u64 sh_name_null_index;
    u64 sh_name_strtab_index;
    u64 sh_name_shstrtab_index;
    u64 sh_name_text_index;

    // offsets
    u64 program_header_ofs;
    u64 segment_ofs;
    u64 section_text_ofs;
    u64 section_strtab_ofs;
    u64 section_shstrtab_ofs;
    u64 section_header_ofs;

    u64 padding_before_null_section = 0;

    // object files
    std::shared_ptr<Parsed::Elf> obj;
};

} // namespace Elf
} // namespace Myld
