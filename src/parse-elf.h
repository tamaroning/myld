#include "myld.h"
#include <cassert>
#include <elf.h>
#include <fmt/core.h>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

namespace Myld::Elf {

class Section {
  public:
    Section(u64 offset, u64 size, std::vector<u8> raw) : name("dummy"), offset(offset), size(size), raw(raw) {}

    void set_name(std::string s) { name = s; }

    std::vector<u8> get_raw() { return raw; }

    std::string get_name() { return name; }

    u64 get_size() { return size; }

    u64 get_offset() { return offset; }

  private:
    std::string name;
    const u64 offset;
    const u64 size;
    const std::vector<u8> raw;
};

class Elf {
  public:
    Elf(std::vector<u8> raw) : raw(raw) {
        // set elf header
        eheader = (Elf64_Ehdr *)&(raw[0]);

        // set program header
        if (get_elf_type() == ET_EXEC) {
            for (int i = 0; i < get_program_header_num(); i++) {
                pheader.push_back((Elf64_Phdr *)&(raw[eheader->e_ehsize]));
            }
        }

        // set section header
        for (int i = 0; i < get_section_num(); i++) {
            u64 sheader_elem_offset = eheader->e_shoff + eheader->e_shentsize * i;
            sheader.push_back((Elf64_Shdr *)&(raw[sheader_elem_offset]));
        }

        // set sections
        for (int i = 0; i < get_section_num(); i++) {
            u64 offset = sheader[i]->sh_offset;
            u64 size = sheader[i]->sh_size;
            std::vector<u8> raw_data = get_raw(offset, size);
            std::shared_ptr<Section> section = std::make_shared<Section>(Section(offset, size, raw_data));
            sections.push_back(section);
        }

        // set section name
        std::vector<u8> shstrtab_raw = sections[get_section_num() - 1]->get_raw();
        // debug
        // fmt::print(".shstrtab strats with {:c}{:c}{:c}{:c}{:c}...\n", shstrtab_raw[0], shstrtab_raw[1],
        // shstrtab_raw[2],
        //           shstrtab_raw[3], shstrtab_raw[4]);

        for (int i = 0; i < get_section_num(); i++) {
            u64 name_index = sheader[i]->sh_name;
            // FIXME: better handling
            std::string name(&shstrtab_raw[name_index], &shstrtab_raw[name_index + 20]);
            // debug print
            // fmt::print("find section named {}\n", name.c_str());
            sections[i]->set_name(name.c_str());
        }
    }

    u8 get_elf_type() { return eheader->e_type; }

    bool is_supported_type() {
        if (get_elf_type() != ET_REL && get_elf_type() != ET_EXEC)
            return false;
        return true;
    }

    std::vector<u8> get_raw(u64 offset, u64 size) {
        std::vector sub(&raw[offset], &raw[offset + size]);
        assert(sub.size() == size);
        return sub;
    }

    u64 get_section_num() { return eheader->e_shnum; }

    u64 get_program_header_num() { return eheader->e_phnum; }

    void dump() {
        // dump elf header
        fmt::print("type : {}\n", eheader->e_type);
        fmt::print("version: {}\n", eheader->e_version);
        /*
        // section header
        for (int i = 0; i < eheader->e_shnum; i++) {
            fmt::print("section[{}]:\n", i);
            fmt::print("  name : {}\n", sheader[i]->sh_type);
            fmt::print("  size : {}\n", sheader[i]->sh_size);
        }
        */

        // dump sections
        for (int i = 0; i < sections.size(); i++) {
            std::shared_ptr<Section> section = sections[i];
            fmt::print("section[{}] :\n", i);
            fmt::print("  name : \"{}\"\n", sections[i]->get_name());
            fmt::print("  size : 0x{:x}\n", sections[i]->get_size());
            fmt::print("  offset : 0x{:x}\n", sections[i]->get_offset());
        }
    }

  private:
    const std::vector<u8> raw;
    // elf header
    Elf64_Ehdr *eheader;
    // program header
    std::vector<Elf64_Phdr *> pheader;
    // section header
    std::vector<Elf64_Shdr *> sheader;
    // sections
    std::vector<std::shared_ptr<Section>> sections;
};

class Reader {
  public:
    Reader(std::string filename) : filename(filename), elf(nullptr) {
        std::ifstream file(filename);
        if (!file) {
            fmt::print("Couldn't open elf file\n");
            std::exit(1);
        }
        std::vector<u8> raw = std::vector<u8>(std::istreambuf_iterator<char>(file), {});
        elf = std::make_shared<Elf>(Elf(raw));
    }

    std::string get_filename() { return filename; }

    std::shared_ptr<Elf> get_elf() { return elf; }

    void dump() {
        fmt::print("content of {}\n", filename);
        elf->dump();
    }

  private:
    const std::string filename;
    std::shared_ptr<Elf> elf;
};

} // namespace Myld::Elf
