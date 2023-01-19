#ifndef BUILDER_H
#define BUILDER_H

#include "context.h"
#include "elf-util.h"
#include "myld.h"
#include <cassert>
#include <elf.h>
#include <fstream>
#include <optional>

// use ""s
using namespace std::literals::string_literals;

namespace Myld {
namespace Build {

using namespace Myld;

class Section {
  public:
    Section(std::shared_ptr<Elf64_Shdr> sheader) : sheader(sheader), padding_size(std::nullopt), raw({}) {}

    void finalize(u64 padding, u64 offset) {
        set_padding_size(padding);
        sheader->sh_offset = offset;
        if (sheader->sh_addralign != 0) {
            assert((sheader->sh_offset % sheader->sh_addralign) == 0);
        }
    }

    u64 get_padding_size() const {
        assert(padding_size.has_value());
        return padding_size.value();
    }

    void set_raw(std::vector<u8> raw_) {
        raw = raw_;
        sheader->sh_size = raw_.size();
    }
    std::vector<u8> get_raw() const { return raw; }

    std::shared_ptr<Elf64_Shdr> sheader;

  private:
    // padding just before the section
    std::optional<u64> padding_size;
    std::vector<u8> raw;

    void set_padding_size(u64 size) {
        assert(!padding_size.has_value());
        padding_size = std::make_optional(size);
    }
};

class Builder {
  public:
    Builder() {}

    static void build_and_output(Context ctx, std::string filename) {
        Builder builder = Builder();
        builder.build(ctx);
        builder.output(filename);
    }

    void output(std::string filename) {
        std::ofstream stream = std::ofstream(filename, std::ios::binary | std::ios::trunc);
        u64 current_offset = 0;

        // elf header
        fmt::print("writing elf header\n");
        stream.write((char *)&eheader, sizeof(Elf64_Ehdr));
        current_offset += sizeof(Elf64_Ehdr);

        // program header
        fmt::print("writing program header\n");
        stream.write((char *)&pheader, sizeof(Elf64_Phdr));
        current_offset += sizeof(Elf64_Phdr);

        // 0x1000からセクション本体が始まるようにpadding
        fmt::print("inserting paddin after program header\n");
        std::fill_n(std::ostream_iterator<char>(stream), padding_after_pheader, '\0');
        current_offset += padding_after_pheader;

        // section bodies
        fmt::print("writing section bodies\n");
        for (int i = 0; i < sections.size(); i++) {
            fmt::print("body of section [{}]\n", i);
            fmt::print("  padding before body:  0x{:x}\n", sections[i].get_padding_size());
            std::fill_n(std::ostream_iterator<char>(stream), sections[i].get_padding_size(), '\0');
            current_offset += sections[i].get_padding_size();

            fmt::print("  body size:  0x{:x}\n", sections[i].sheader->sh_size);
            assert(current_offset == sections[i].sheader->sh_offset);
            stream.write((char *)&(sections[i].get_raw()[0]), sections[i].sheader->sh_size);
            current_offset += sections[i].sheader->sh_size;
        }

        // section headers
        fmt::print("writing section header\n");
        assert(current_offset == eheader.e_shoff);
        for (int i = 0; i < sections.size(); i++) {
            fmt::print("header of section [{}]\n", i);
            stream.write((char *)&(*(sections[i].sheader)), sizeof(Elf64_Shdr));
            current_offset += sizeof(Elf64_Shdr);
        }
    }

    void build(Context ctx) {
        // elf header
        eheader = Utils::create_dummy_eheader();
        // program header
        pheader =
            Utils::create_dummy_pheader_load(ctx.config.get_text_load_addr(), ctx.config.get_text_load_addr(), 0x1000);

        // null
        Section null_section = Section(Utils::create_sheader_null());
        sections.push_back(null_section);

        // .text
        fmt::print("creating .text section\n");
        Section text_section = Section(Utils::create_dummy_sheader_text(27, 1, ctx.config.get_text_load_addr()));
        text_section.set_raw(ctx.section_raws[".text"]);
        sections.push_back(text_section);

        // .rodata
        std::optional<Section> rodata_section = std::nullopt;
        if (ctx.section_raws[".rodata"].size() > 0) {
            fmt::print("creating .rodata section\n");
            // TODO: アドレスの計算方法をもっとスマートにしたい
            rodata_section = Section(Utils::create_dummy_sheader_rodata(
                33, 1, ctx.config.get_text_load_addr() + text_section.sheader->sh_size));
            rodata_section.value().set_raw(ctx.section_raws[".rodata"]);
            sections.push_back(rodata_section.value());
        }

        // .data, .data*name*
        // TODO:

        // .symtab
        // create_section(".symtab", linked_sym_table.to_symtab_section_body(), 0);
        fmt::print("creating .symtab section\n");
        Section symtab_section =
            Section(Utils::create_dummy_sheader_symtab(1, 8, 3, ctx.linked_sym_table.get_local_symbol_num()));
        symtab_section.set_raw(ctx.linked_sym_table.to_symtab_section_body());
        sections.push_back(symtab_section);

        // .strtab
        // create_section(".strtab", linked_sym_table.to_strtab_section_body(), 0);
        fmt::print("creating .strtab section\n");
        Section strtab_section = Section(Utils::create_dummy_sheader_strtab(9, 1));
        strtab_section.set_raw(ctx.linked_sym_table.to_strtab_section_body());
        sections.push_back(strtab_section);

        // .shstrtab
        fmt::print("creating .shstrtab section\n");
        Section shstrtab_section = Section(Utils::create_dummy_sheader_strtab(17, 0x1));
        std::string shstrtab_content = "\0"s + ".symtab\0"s + ".strtab\0"s + ".shstrtab\0" + ".text\0"s + ".rodata\0"s;
        std::vector<u8> shstrtab_raw(shstrtab_content.begin(), shstrtab_content.end());
        shstrtab_section.set_raw(shstrtab_raw);
        sections.push_back(shstrtab_section);

        // calculate padding before each section
        u64 first_section_start_offset = 0x1000;
        u64 section_start_offset = first_section_start_offset;
        padding_after_pheader = section_start_offset - sizeof(Elf64_Ehdr) - sizeof(Elf64_Phdr);
        fmt::print("padding after pheader: 0x{:x}\n", padding_after_pheader);
        fmt::print("finalizing section\n");
        for (int i = 0; i < sections.size(); i++) {
            fmt::print("section [{}]: ", i);
            u64 align = sections[i].sheader->sh_addralign;
            fmt::print("offset = 0x{:x}, ", section_start_offset);
            fmt::print("size = 0x{:x}, ", sections[i].sheader->sh_size);
            fmt::print("align = 0x{:x}, ", align);

            u64 padding_size;
            if (sections[i].sheader->sh_type == SHT_NULL) {
                padding_size = 0;
            } else {
                if ((section_start_offset % align) != 0) {
                    padding_size = align - (section_start_offset % align);
                } else {
                    padding_size = 0;
                }
            }
            fmt::print("padding = 0x{:x}\n", padding_size);
            section_start_offset += padding_size;
            sections[i].finalize(padding_size, section_start_offset);
            section_start_offset += sections[i].sheader->sh_size;
        }

        // calulate distance from first section start to last section end
        u64 dist_from_sections_start_to_sections_end = 0;
        for (int i = 0; i < sections.size(); i++) {
            dist_from_sections_start_to_sections_end += sections[i].get_padding_size();
            dist_from_sections_start_to_sections_end += sections[i].sheader->sh_size;
        }

        fmt::print("finalize elf header\n");
        u64 sheader_start_offset = first_section_start_offset + dist_from_sections_start_to_sections_end;
        fmt::print("start of section header: 0x{:x} = {}\n", sheader_start_offset, sheader_start_offset);
        Utils::finalize_eheader(&eheader, ctx._start_addr.value(), 1, sections.size(), sheader_start_offset);
        // とりあえず、.textセクションのみロードする
        fmt::print("finalize program header\n");
        u64 load_offset = text_section.sheader->sh_offset;
        u64 load_size = text_section.sheader->sh_size;
        Utils::finalize_pheader_load(&pheader, load_offset, load_size);
    }

  private:
    // output
    Elf64_Ehdr eheader;
    Elf64_Phdr pheader;

    std::vector<Section> sections;

    u64 padding_after_pheader;

    std::string shstrtab_content;
};

} // namespace Build
} // namespace Myld

#endif
