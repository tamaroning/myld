#ifndef ELF_PARSED_H
#define ELF_PARSED_H

#include "myld.h"
#include <cassert>
#include <elf.h>
#include <fmt/core.h>
#include <fmt/format.h>
#include <memory>
#include <optional>
#include <string>
#include <vector>

// TODO: 生ポインタを扱ってる箇所が十分なサイズのデータを指しているか確かめる必要がある

namespace Myld {
namespace Parse {

class SymTableEntry {
  public:
    SymTableEntry(Elf64_Sym *raw) : name(std::nullopt), sym(raw) {}

    void set_name(std::string s) { name = s; }

    std::string get_name() const {
        assert(name != std::nullopt);
        return name.value();
    }

    Elf64_Sym *get_sym() const { return sym; }

  private:
    std::optional<std::string> name;
    Elf64_Sym *sym;
};

class SymTable {
  public:
    SymTable(Elf64_Shdr *sheader, u8 *raw)
        : sheader(sheader), entries(std::make_shared<std::vector<SymTableEntry>>(std::vector<SymTableEntry>({}))) {
        assert(sheader->sh_entsize == sizeof(Elf64_Sym));
        symbol_num = sheader->sh_size / sheader->sh_entsize;
        fmt::print("found .symtab section (symbol num = {})\n", symbol_num);

        for (int i = 0; i < symbol_num; i++) {
            u32 start = i * sizeof(Elf64_Sym);
            entries->push_back(SymTableEntry((Elf64_Sym *)&raw[start]));
        }
    }

    u64 get_symbol_num() const { return symbol_num; }
    Elf64_Shdr *get_sheader() const { return sheader; }
    std::shared_ptr<std::vector<SymTableEntry>> get_entries() const { return entries; }

  private:
    // section header of .symtab
    Elf64_Shdr *sheader;
    // number of symbols
    u64 symbol_num;
    std::shared_ptr<std::vector<SymTableEntry>> entries;
};

class RelaTextEntry {
  public:
    RelaTextEntry(Elf64_Rela *raw) : name(std::nullopt), rela(raw) {}

    void set_name(std::string s) { name = s; }

    std::string get_name() const {
        assert(name != std::nullopt);
        return name.value();
    }

    Elf64_Rela *get_rela() const { return rela; }

  private:
    std::optional<std::string> name;
    Elf64_Rela *rela;
};

class RelaText {
  public:
    RelaText(Elf64_Shdr *sheader, u8 *raw)
        : sheader(sheader), entries(std::make_shared<std::vector<RelaTextEntry>>(std::vector<RelaTextEntry>({}))) {
        assert(sheader->sh_entsize == sizeof(Elf64_Rela));
        reloc_num = sheader->sh_size / sheader->sh_entsize;
        fmt::print("found .rela.text section (relocation num = {})\n", reloc_num);

        for (int i = 0; i < reloc_num; i++) {
            u32 start = i * sizeof(Elf64_Rela);
            entries->push_back(RelaTextEntry((Elf64_Rela *)&raw[start]));
        }
    }

    Elf64_Shdr *get_sheader() const { return sheader; }
    u64 get_reloc_num() const { return reloc_num; }
    std::shared_ptr<std::vector<RelaTextEntry>> get_entries() const { return entries; }

  private:
    // section header of .rela.text
    Elf64_Shdr *sheader;
    // number of relocations
    u64 reloc_num;
    std::shared_ptr<std::vector<RelaTextEntry>> entries;
};

// struct represents a section. this contains section header
class Section {
  public:
    Section(Elf64_Shdr *header, u8 *raw) : name(std::nullopt), header(header), raw(raw) {
        assert((header->sh_addralign == 0 && header->sh_type == SHT_NULL) ||
               ((header->sh_offset % header->sh_addralign) == 0));
    }

    void set_name(std::string s) { name = s; }

    std::string get_name() const {
        assert(name != std::nullopt);
        return name.value();
    }
    u8 *get_raw() const { return raw; }
    Elf64_Shdr *get_header() const { return header; }

  private:
    std::optional<std::string> name;
    // section header
    Elf64_Shdr *header;
    // content of the section
    u8 *raw;
};

class Elf {
  public:
    // create from raw data
    Elf(u8 *raw) : raw(raw) {
        fmt::print("parsing elf header\n");
        // get elf header
        eheader = (Elf64_Ehdr *)&(raw[0]);

        // get program header
        fmt::print("parsing program header\n");
        if (get_elf_type() == ET_EXEC) {
            for (int i = 0; i < get_program_header_num(); i++) {
                pheader.push_back((Elf64_Phdr *)&(raw[eheader->e_ehsize]));
            }
        }

        // get section header
        std::vector<Elf64_Shdr *> section_headers;
        fmt::print("parsing section header\n");
        for (int i = 0; i < get_section_num(); i++) {
            u64 sheader_elem_offset = eheader->e_shoff + eheader->e_shentsize * i;
            section_headers.push_back((Elf64_Shdr *)&(raw[sheader_elem_offset]));
        }

        // get section data
        fmt::print("parsing elf body\n");
        for (int i = 0; i < get_section_num(); i++) {
            fmt::print("parsing sections[{}]\n", i);
            Elf64_Shdr *section_header = section_headers[i];
            u8 *section_raw = get_raw(section_header->sh_offset, section_header->sh_size);

            // found symbol table
            if (section_header->sh_type == SHT_SYMTAB) {
                sym_table = std::make_optional<SymTable>(SymTable(section_header, section_raw));
            } else if (section_header->sh_type == SHT_RELA) {
                rela_text = std::make_optional<RelaText>(RelaText(section_header, section_raw));
            }

            std::shared_ptr<Section> section = std::make_shared<Section>(Section(section_header, section_raw));
            sections.push_back(section);
        }
        // checks if parser found symbol table
        assert(sym_table != std::nullopt);

        // get section name from last section (=.shstrtab)
        u8 *shstrtab_raw = sections[get_section_num() - 1]->get_raw();
        for (int i = 0; i < get_section_num(); i++) {
            u64 name_index = section_headers[i]->sh_name;
            // FIXME: とりあえず20
            std::string name(&shstrtab_raw[name_index], &shstrtab_raw[name_index + 20]);
            sections[i]->set_name(name.c_str());
        }

        // make sure elf contains .symtab, .strtab, and .shstrtab
        assert(get_section_by_name(".symtab") != nullptr && sym_table.has_value());
        assert(get_section_by_name(".strtab") != nullptr);
        assert(get_section_by_name(".shstrtab") != nullptr);
        // make sure .rela.text is parsed correctly
        assert((get_section_by_name(".rela.text") == nullptr && !rela_text.has_value()) ||
               (get_section_by_name(".rela.text") != nullptr && rela_text.has_value()));

        fmt::print("looking up symbol names\n");

        // lookup name of each symbol table entry
        auto strtab = get_section_by_name(".strtab");
        auto symtab_entries = sym_table->get_entries();
        for (int i = 0; i < sym_table->get_symbol_num(); i++) {
            auto name_index = (*symtab_entries)[i].get_sym()->st_name;
            // FIXME: とりあえず20
            std::string symbol_name =
                std::string(&(strtab->get_raw()[name_index]), &(strtab->get_raw()[name_index + 20])).c_str();
            (*symtab_entries)[i].set_name(symbol_name);
        }

        // lookup name of each rela entry
        if (rela_text.has_value()) {
            auto symtab = get_section_by_name(".symtab");
            auto rela_entries = rela_text->get_entries();
            for (int i = 0; i < rela_text->get_reloc_num(); i++) {
                // FIXME: これってほんとにsymbol table entryのインデックスを表してるの?
                auto symbol_index = ELF64_R_SYM((*rela_entries)[i].get_rela()->r_info);
                // FIXME: とりあえず20
                std::string symbol_name = (*sym_table->get_entries())[symbol_index].get_name();
                // std::string(&(symtab->get_raw()[name_index]), &(symtab->get_raw()[name_index + 20])).c_str();
                (*rela_entries)[i].set_name(symbol_name);
            }
        }
    }

    u8 get_elf_type() { return eheader->e_type; }

    bool is_supported_type() {
        if (get_elf_type() != ET_REL && get_elf_type() != ET_EXEC)
            return false;
        return true;
    }

    u8 *get_raw(u64 offset, u64 size) { return &raw[offset]; }

    u64 get_section_num() { return eheader->e_shnum; }

    u64 get_program_header_num() { return eheader->e_phnum; }

    std::shared_ptr<Section> get_section_by_name(std::string name) {
        for (auto section : sections) {
            if (section->get_name() == name)
                return section;
        }
        return nullptr;
    }

    void dump() {
        // dump elf header
        fmt::print("ELF type: {}, ", eheader->e_type);
        fmt::print("version: {}\n", eheader->e_version);

        // dump sections
        for (int i = 0; i < sections.size(); i++) {
            std::shared_ptr<Section> section = sections[i];
            fmt::print("section[{}] :\n  ", i);
            fmt::print("name: \"{}\", ", sections[i]->get_name());
            fmt::print("size: 0x{:x}, ", sections[i]->get_header()->sh_size);
            fmt::print("entsize: 0x{:x}, ", sections[i]->get_header()->sh_entsize);
            fmt::print("offset: 0x{:x}, ", sections[i]->get_header()->sh_offset);
            fmt::print("align: 0x{:x}\n", sections[i]->get_header()->sh_addralign);
        }

        // dump symbol table
        auto symtab_entries = sym_table->get_entries();
        for (int i = 0; i < sym_table->get_symbol_num(); i++) {
            fmt::print("symbol[{}]:\n  ", i);
            fmt::print("name : \"{}\", ", (*symtab_entries)[i].get_name());
            fmt::print("value : 0x{:x}, ", (*symtab_entries)[i].get_sym()->st_value);
            fmt::print("info : 0x{:x}\n", (*symtab_entries)[i].get_sym()->st_info);
        }

        // dump relocation table (.rela.text)
        if (rela_text.has_value()) {
            auto rela_entries = rela_text->get_entries();
            for (int i = 0; i < rela_text->get_reloc_num(); i++) {
                fmt::print("rela[{}]:\n  ", i);
                fmt::print("name : \"{}\", ", (*rela_entries)[i].get_name());
                fmt::print("offset : \"{}\", ", (*rela_entries)[i].get_rela()->r_offset);
                fmt::print("info : 0x{:x}, ", (*rela_entries)[i].get_rela()->r_info);
                fmt::print("addend : {}\n  ", (*rela_entries)[i].get_rela()->r_addend);
                fmt::print("sym(in info) : 0x{:x}, ", ELF64_R_SYM((*rela_entries)[i].get_rela()->r_info));
                fmt::print("type(in info) : 0x{:x}, \n", ELF64_R_TYPE((*rela_entries)[i].get_rela()->r_info));
            }
        }
    }

  private:
    // raw data
    u8 *raw;
    // elf header
    Elf64_Ehdr *eheader;
    // program header
    std::vector<Elf64_Phdr *> pheader;
    // sections
    std::vector<std::shared_ptr<Section>> sections;

    // symbol table
    // this fielf has some value when the elf contains .symtab section
    std::optional<SymTable> sym_table;
    // symbol table
    // this fielf has some value when the elf contains .rela.text section
    std::optional<RelaText> rela_text;
};

} // namespace Parse
} // namespace Myld

#endif
