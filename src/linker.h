#include "builder.h"
#include "elf-util.h"
#include "elf/parse.h"
#include <cassert>
#include <elf.h>
#include <fstream>
#include <optional>

namespace Myld {

using namespace Myld::Elf;

class Config {
  public:
    Config() {}
    u64 entry_addr;
};

class Section {
  public:
    Section(Elf64_Shdr sheader) : sheader(sheader), padding_size(std::nullopt), raw({}) {}

    void set_padding_size(u64 size) {
        assert(!padding_size.has_value());
        padding_size = std::make_optional(size);
    }
    u64 get_padding_size() const {
        assert(padding_size.has_value());
        return padding_size.value();
    }

    void set_raw(std::vector<u8> raw_) { raw = raw_; }
    std::vector<u8> get_raw() const { return raw; }

    Elf64_Shdr sheader;

  private:
    // padding just before the section
    std::optional<u64> padding_size;
    std::vector<u8> raw;
};

class Linker {
  public:
    Linker(Parsed::Elf obj) : obj(obj), config(Config()) {}

    void output(std::string filename) {
        std::ofstream stream = std::ofstream(filename, std::ios::binary | std::ios::trunc);

        // elf header
        fmt::print("writing elf header\n");
        stream.write((char *)&eheader, sizeof(Elf64_Ehdr));

        // program header
        fmt::print("writing program header\n");
        stream.write((char *)&pheader, sizeof(Elf64_Phdr));

        // section bodies
        fmt::print("writing section bodies\n");
        for (Section section : sections) {
            fmt::print("padding size 0x{:x}\n", section.get_padding_size());
            std::fill_n(std::ostream_iterator<char>(stream), section.get_padding_size(), '\0');
            stream.write((char *)&(section.get_raw()[0]), section.sheader.sh_size);
        }

        // section headers
        fmt::print("writing section header\n");
        for (Section section : sections) {
            stream.write((char *)&(section.sheader), sizeof(Elf64_Shdr));
        }
    }

    void link() {
        eheader = Utils::create_dummy_eheader();
        pheader = Utils::create_dummy_pheader_load();

        Section null_section = Section(Utils::create_sheader_null());
        sections.push_back(null_section);

        // .text
        fmt::print("creating .text section\n");
        Section text_section = Section(Utils::create_dummy_sheader_text(0x1000));
        fmt::print("type {:x}\n", text_section.sheader.sh_type);
        fmt::print("size {:x}\n", text_section.sheader.sh_size);
        fmt::print("offset {:x}\n", text_section.sheader.sh_offset);

        auto obj_text_section = obj.get_section_by_name(".text");
        auto obj_text_size = obj_text_section->get_header()->sh_size;
        text_section.set_raw(
            std::vector<u8>(&(obj_text_section->get_raw()[0]), &(obj_text_section->get_raw()[obj_text_size])));
        sections.push_back(text_section);

        // .strtab
        fmt::print("creating .strtab section\n");
        Section strtab_section = Section(Utils::create_dummy_sheader_strtab(0x1));
        std::string strtab = "\0";
        strtab_section.set_raw(std::vector<u8>(strtab.begin(), strtab.end()));
        sections.push_back(strtab_section);

        // .shstrtab
        fmt::print("creating .shstrtab section\n");
        Section shstrtab_section = Section(Utils::create_dummy_sheader_strtab(0x1));
        std::string shstrtab = "\0.strtab\0.shstrtab\0.text\0";
        shstrtab_section.set_raw(std::vector<u8>(shstrtab.begin(), shstrtab.end()));
        sections.push_back(shstrtab_section);

        Utils::finalize_sheader_text(&text_section.sheader, 19, 0x1000, 0x18);
        Utils::finalize_sheader_strtab(&strtab_section.sheader, 1, 0x1018, 1);
        Utils::finalize_sheader_strtab(&shstrtab_section.sheader, 9, 0x1019, 0x19);

        // calculate padding before each section
        fmt::print("calculating padding\n");
        u64 section_start = sizeof(Elf64_Ehdr) + sizeof(Elf64_Phdr);
        for (int i = 0; i < sections.size(); i++) {
            fmt::print("section [{}]: ", i);
            u64 align = sections[i].sheader.sh_addralign;
            fmt::print("offset = 0x{:x}, ", section_start);
            fmt::print("align = 0x{:x}, ", align);

            u64 padding_size;
            if (sections[i].sheader.sh_type == SHT_NULL) {
                padding_size = 0;
            } else {
                if ((section_start % align) != 0) {
                    padding_size = align - (section_start % align);
                    section_start += padding_size;
                } else {
                    padding_size = 0;
                }
            }
            fmt::print("padding = 0x{:x}\n", padding_size);
            sections[i].set_padding_size(padding_size);
        }

        // calulate range from first section start to last section end
        fmt::print("calucating section size\n");
        u64 all_sections_size_sum = 0;
        for (int i = 0; i < sections.size(); i++) {
            fmt::print("section [{}]: ", i);
            fmt::print("padding = 0x{:x}\n", sections[i].get_padding_size());
            all_sections_size_sum += sections[i].get_padding_size();
            all_sections_size_sum += sections[i].sheader.sh_size;
        }

        fmt::print("finalize elf header\n");
        Utils::finalize_eheader(&eheader, 0x80000, 1, sections.size(), all_sections_size_sum);
        fmt::print("finalize program header\n");
        Utils::finalize_pheader_load(&pheader, 0x1000, 0x18);
    }

  private:
    Parsed::Elf obj;
    Config config;

    // output
    Elf64_Ehdr eheader;
    Elf64_Phdr pheader;

    std::vector<Section> sections;
};

} // namespace Myld