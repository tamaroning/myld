#ifndef ELF_PARSED_H
#define ELF_PARSED_H

#include "myld.h"
#include <cassert>
#include <elf.h>
#include <fmt/core.h>
#include <fmt/format.h>
#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

// TODO: 生ポインタを扱ってる箇所が十分なサイズのデータを指しているか確かめる必要がある

namespace Myld {
namespace Parse {

class SymTableEntry {
  public:
    SymTableEntry(Raw raw_) : name(std::nullopt), raw(raw_) {
        // check size
        assert(raw_.get_size() == sizeof(Elf64_Sym));
    }

    void set_name(std::string s) { name = s; }

    std::string get_name() const {
        assert(name != std::nullopt);
        return name.value();
    }

    // TODO: change to const Elf64_Sym*?
    Elf64_Sym *get_sym() const { return (Elf64_Sym *)raw.to_pointer(); }

    Raw get_raw() const { return raw; }

    u8 get_bind() const { return ELF64_ST_BIND(get_sym()->st_info); }

    u8 get_type() const { return ELF64_ST_TYPE(get_sym()->st_info); }

  private:
    std::optional<std::string> name;
    Raw raw;
};

class SymTable {
  public:
    SymTable(Elf64_Shdr *sheader, Raw raw) : sheader(sheader), entries({}) {
        assert(sheader->sh_entsize == sizeof(Elf64_Sym));
        symbol_num = sheader->sh_size / sheader->sh_entsize;
        fmt::print("found .symtab section (symbol num = {})\n", symbol_num);

        for (int i = 0; i < symbol_num; i++) {
            u32 start = i * sizeof(Elf64_Sym);
            entries.push_back(std::make_shared<SymTableEntry>(SymTableEntry(raw.get_sub(start, sizeof(Elf64_Sym)))));
        }
    }

    u64 get_symbol_num() const { return symbol_num; }
    Elf64_Shdr *get_sheader() const { return sheader; }
    std::vector<std::shared_ptr<SymTableEntry>> get_entries() const { return entries; }

    std::shared_ptr<SymTableEntry> get_symbol_by_name(std::string name) {
        for (auto entry : entries) {
            if (entry->get_name() == name) {
                return entry;
            }
        }
        return nullptr;
    }

  private:
    // section header of .symtab
    Elf64_Shdr *sheader;
    // number of symbols
    u64 symbol_num;
    std::vector<std::shared_ptr<SymTableEntry>> entries;
};

class RelaTextEntry {
  public:
    RelaTextEntry(Raw raw_) : name(std::nullopt), raw(raw_) {}

    void set_name(std::string s) { name = s; }

    std::string get_name() const {
        assert(name != std::nullopt);
        return name.value();
    }

    // TODO: change to const Elf64_Rela*?
    Elf64_Rela *get_rela() const { return (Elf64_Rela *)raw.to_pointer(); }

    u32 get_type() const { return ELF64_R_TYPE(get_rela()->r_info); }

    u32 get_sym() const { return ELF64_R_SYM(get_rela()->r_info); }

  private:
    std::optional<std::string> name;
    Raw raw;
};

class Rela {
  public:
    Rela(Elf64_Shdr *sheader, Raw raw) : sheader(sheader), entries({}) {
        assert(sheader->sh_entsize == sizeof(Elf64_Rela));
        reloc_num = sheader->sh_size / sheader->sh_entsize;
        fmt::print("found .rela.text section (relocation num = {})\n", reloc_num);

        for (int i = 0; i < reloc_num; i++) {
            u32 start = i * sizeof(Elf64_Rela);
            entries.push_back(std::make_shared<RelaTextEntry>(RelaTextEntry(raw.get_sub(start, sizeof(Elf64_Sym)))));
        }
    }

    Elf64_Shdr *get_sheader() const { return sheader; }
    u64 get_reloc_num() const { return reloc_num; }
    std::vector<std::shared_ptr<RelaTextEntry>> get_entries() const { return entries; }

  private:
    // section header of .rela.text
    Elf64_Shdr *sheader;
    // number of relocations
    u64 reloc_num;
    std::vector<std::shared_ptr<RelaTextEntry>> entries;
};

// struct represents a section. this contains section header
class Section {
  public:
    Section(Elf64_Shdr *header, Raw raw) : name(std::nullopt), header(header), raw(raw) {
        assert((header->sh_addralign == 0 && header->sh_type == SHT_NULL) ||
               ((header->sh_offset % header->sh_addralign) == 0));
    }

    void set_name(std::string s) { name = s; }

    std::string get_name() const {
        assert(name != std::nullopt);
        return name.value();
    }
    Raw get_raw() const { return raw; }
    Elf64_Shdr *get_header() const { return header; }

  private:
    std::optional<std::string> name;
    // section header
    Elf64_Shdr *header;
    // content of the section
    Raw raw;
};

class Elf {
  public:
    // create from raw data
    Elf(std::string filename, std::shared_ptr<const std::vector<u8>> raw_bytes)
        : filename(filename), raw(Raw(raw_bytes)), sections({}), sym_table(std::nullopt), relas({}) {
        fmt::print("parsing elf header\n");
        // get elf header
        eheader = (Elf64_Ehdr *)&((*raw_bytes)[0]);

        // get program header
        fmt::print("parsing program header\n");
        if (get_elf_type() == ET_EXEC) {
            for (int i = 0; i < get_program_header_num(); i++) {
                pheader.push_back((Elf64_Phdr *)raw.get_sub(0, sizeof(Elf64_Phdr)).to_pointer());
            }
        }

        // get section header
        std::vector<Elf64_Shdr *> section_headers;
        fmt::print("parsing section header\n");
        for (int i = 0; i < get_section_num(); i++) {
            u64 sheader_elem_offset = eheader->e_shoff + eheader->e_shentsize * i;
            section_headers.push_back((Elf64_Shdr *)raw.get_sub(sheader_elem_offset, sizeof(Elf64_Ehdr)).to_pointer());
        }

        // get raw data of section body
        fmt::print("parsing elf body\n");
        for (int i = 0; i < get_section_num(); i++) {
            fmt::print("parsing sections[{}]\n", i);
            Elf64_Shdr *section_header = section_headers[i];
            Raw section_raw = raw.get_sub(section_header->sh_offset, section_header->sh_size);

            std::shared_ptr<Section> section = std::make_shared<Section>(Section(section_header, section_raw));
            sections.push_back(section);
        }

        // get section name from .shstrtab
        Raw shstrtab_raw = sections[eheader->e_shstrndx]->get_raw();
        for (int i = 0; i < get_section_num(); i++) {
            u64 name_index = section_headers[i]->sh_name;
            // FIXME: とりあえず20
            std::string name(shstrtab_raw.to_pointer() + name_index, shstrtab_raw.to_pointer() + name_index + 20);
            sections[i]->set_name(name.c_str());
        }

        // parse section data
        for (auto section : sections) {
            Elf64_Shdr *sheader = section->get_header();
            // found symbol table
            if (sheader->sh_type == SHT_SYMTAB) {
                assert(section->get_name() == ".symtab");
                sym_table = std::make_optional<SymTable>(SymTable(sheader, section->get_raw()));
            } else if (sheader->sh_type == SHT_RELA) {
                std::string section_name = section->get_name();
                std::string referent_section_name(section_name.begin() + 5, section_name.end());
                // .rela*name*があるのに*name*セクションがない
                assert(get_section_by_name(referent_section_name) != nullptr);

                relas[referent_section_name] = std::make_shared<Rela>(Rela(sheader, section->get_raw()));
            }
        }

        // make sure elf contains .symtab, .strtab, and .shstrtab
        assert(get_section_by_name(".symtab") != nullptr && sym_table.has_value());
        assert(get_section_by_name(".strtab") != nullptr);
        assert(get_section_by_name(".shstrtab") != nullptr);

        fmt::print("looking up symbol names\n");

        // lookup name of each symbol table entry
        auto strtab = get_section_by_name(".strtab");
        auto symtab_entries = sym_table->get_entries();
        for (int i = 0; i < sym_table->get_symbol_num(); i++) {
            std::string symbol_name;
            if (symtab_entries[i]->get_type() == STT_SECTION) {
                u16 shndx = symtab_entries[i]->get_sym()->st_shndx;
                symbol_name = sections[shndx]->get_name();
            } else {
                auto name_index = symtab_entries[i]->get_sym()->st_name;
                // FIXME: とりあえず20
                symbol_name = std::string(strtab->get_raw().to_pointer() + name_index,
                                          strtab->get_raw().to_pointer() + name_index + 20)
                                  .c_str();
            }
            symtab_entries[i]->set_name(symbol_name);
        }

        // lookup name of each rela entry
        for (auto &[_, rela] : relas) {
            auto symtab = get_section_by_name(".symtab");
            auto rela_entries = rela->get_entries();
            for (int i = 0; i < rela->get_reloc_num(); i++) {
                // FIXME: これってほんとにsymbol table entryのインデックスを表してるの?
                auto symbol_index = rela_entries[i]->get_sym();
                // FIXME: とりあえず20
                std::string symbol_name = symtab_entries[symbol_index]->get_name();
                // std::string(&(symtab->get_raw()[name_index]), &(symtab->get_raw()[name_index + 20])).c_str();
                rela_entries[i]->set_name(symbol_name);
            }
        }
    }

    std::string get_filename() const { return filename; }

    u8 get_elf_type() { return eheader->e_type; }

    bool is_supported_type() {
        if (get_elf_type() != ET_REL && get_elf_type() != ET_EXEC)
            return false;
        return true;
    }

    Raw get_raw() { return raw; }

    u64 get_section_num() { return eheader->e_shnum; }

    u64 get_program_header_num() { return eheader->e_phnum; }

    std::shared_ptr<Section> get_section_by_name(std::string name) {
        for (auto section : sections) {
            if (section->get_name() == name)
                return std::shared_ptr<Section>(section);
        }
        return nullptr;
    }

    std::vector<std::shared_ptr<Section>> get_section_starts_with(std::string s) {
        std::vector<std::shared_ptr<Section>> ret({});
        for (auto section : sections) {
            if (section->get_name().starts_with(s))
                ret.push_back(std::shared_ptr<Section>(section));
        }
        return ret;
    }

    std::optional<SymTable> get_sym_table() const { return sym_table; }

    std::shared_ptr<Rela> get_rela_by_name(std::string referent_section) const {
        if (auto iter = relas.find(referent_section); iter != relas.end()) {
            return iter->second;
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
            fmt::print("name : \"{}\", ", symtab_entries[i]->get_name());
            fmt::print("value : 0x{:x}, ", symtab_entries[i]->get_sym()->st_value);
            fmt::print("info : 0x{:x}\n", symtab_entries[i]->get_sym()->st_info);
        }

        // dump relocation table (.rela.text)
        for (auto &[referent_section_name, rela] : relas) {
            fmt::print("relocation info of \"{}\" (.rela{})\n", referent_section_name, referent_section_name);
            auto rela_entries = rela->get_entries();
            for (int i = 0; i < rela->get_reloc_num(); i++) {
                fmt::print("rela[{}]:\n  ", i);
                fmt::print("name : \"{}\", ", rela_entries[i]->get_name());
                fmt::print("offset : \"{}\", ", rela_entries[i]->get_rela()->r_offset);
                fmt::print("info : 0x{:x}, ", rela_entries[i]->get_rela()->r_info);
                fmt::print("addend : {}\n  ", rela_entries[i]->get_rela()->r_addend);
                fmt::print("sym(in info) : 0x{:x}, ", rela_entries[i]->get_sym());
                fmt::print("type(in info) : 0x{:x}, \n", rela_entries[i]->get_type());
            }
        }
    }

  private:
    // use filename as identifier of this structure
    std::string filename;
    // raw data
    Raw raw;
    // elf header
    Elf64_Ehdr *eheader;
    // program header
    std::vector<Elf64_Phdr *> pheader;
    // sections
    std::vector<std::shared_ptr<Section>> sections;
    // symbol table
    // this fielf has some value when the elf contains .symtab section
    std::optional<SymTable> sym_table;
    // relocation info
    std::map<std::string, std::shared_ptr<Rela>> relas;
};

} // namespace Parse
} // namespace Myld

#endif
