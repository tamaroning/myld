#include "myld.h"
#include <cassert>
#include <elf.h>
#include <fstream>
#include <memory>
#include <optional>
#include <string>
#include <vector>

class Section {
  public:
    Section(u64 offset, u64 size, std::vector<u8> raw) : name("dummy"), offset(offset), size(size), raw(raw) {}

    void set_name(std::string name) { name = name; }

    std::vector<u8> get_raw() { return raw; }

    std::string get_name() { return name; }

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
            std::shared_ptr<Section> section(new Section(offset, size, raw_data));
            sections.push_back(section);
        }

        // set section name
        std::vector<u8> shstrtab_raw = sections[get_section_num() - 1]->get_raw();
        for (int i = 0; i < get_section_num(); i++) {
            u64 name_index = sheader[i]->sh_name;
            // FIXME: null文字はどうなるの？
            std::string name(shstrtab_raw[name_index], shstrtab_raw[name_index + 20]);
            sections[i]->set_name(name);
        }
    }

    u8 get_elf_type() { return eheader->e_type; }

    bool is_supported_type() {
        if (get_elf_type() != ET_REL && get_elf_type() != ET_EXEC)
            return false;
        return true;
    }

    u8 get_section(u64 index) {
        if (index >= get_section_num()) {
            fmt::print("section[{}] does not exist", index);
            std::exit(1);
        }
        // TODO:
        return 0;
    }

    std::vector<u8> get_raw(u64 offset, u64 size) {
        std::vector sub(&raw[offset], &raw[offset + size]);
        assert(sub.size() == size);
        return sub;
    }

    u64 get_section_num() { return eheader->e_shnum; }

    u64 get_program_header_num() { return eheader->e_phnum; }

    void dump() {
        for (int i = 0; i < sections.size(); i++) {
            std::shared_ptr<Section> section = sections[i];
            fmt::print("section[{}]: name={}\n", i, section->get_name());
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

    std::vector<std::shared_ptr<Section>> sections;
};

class ElfReader {
  public:
    ElfReader(std::string filename) : filename(filename), elf(nullptr) {
        std::ifstream file(filename);
        if (!file) {
            fmt::print("Couldn't open elf file\n");
            std::exit(1);
        }
        std::vector<u8> raw = std::vector<u8>(std::istreambuf_iterator<char>(file), {});
        elf = std::make_shared<Elf>(new Elf(raw));
    }

    std::string get_filename() { return filename; }

    std::shared_ptr<Elf> get_elf() { return elf; }

  private:
    const std::string filename;
    std::shared_ptr<Elf> elf;
};
