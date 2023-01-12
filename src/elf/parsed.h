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

namespace Myld {
namespace Elf {
namespace Parsed {

class SymTableEntry {
  public:
    SymTableEntry(std::vector<u8> raw_data) : name(std::nullopt), raw(raw_data) {
        sym = (Elf64_Sym *)&(raw[0]);
        auto symtab_r = raw;
        fmt::print("name index of current sym entry: {}\n", sym->st_name);
    }

    void set_name(std::string s) { name = s; }

    std::string get_name() const {
        assert(name != std::nullopt);
        return name.value();
    }

    Elf64_Sym *get_sym() const { return sym; }

  private:
    std::optional<std::string> name;
    Elf64_Sym *sym;
    // FIXME: いらない？
    std::vector<u8> raw;
};

class SymTable {
  public:
    SymTable(u64 symbol_num, u64 entry_size, std::vector<u8> raw)
        : symbol_num(symbol_num),
          symbols(std::make_shared<std::vector<SymTableEntry>>(std::vector<SymTableEntry>({}))) {
        assert(entry_size == sizeof(Elf64_Sym));
        for (int i = 0; i < symbol_num; i++) {
            u32 start = i * sizeof(Elf64_Sym);
            u32 end = start + sizeof(Elf64_Sym);
            symbols->push_back(SymTableEntry(std::vector(&raw[start], &raw[end])));
        }
    }

    u64 get_symbol_num() const { return symbol_num; }

    // returns mutable reference
    std::shared_ptr<std::vector<SymTableEntry>> get_symbols() { return symbols; }

  private:
    u64 symbol_num;
    std::shared_ptr<std::vector<SymTableEntry>> symbols;
};

enum class SectionType : u32 {
    Null = SHT_NULL,
    ProgBits = SHT_PROGBITS, // .text
    SymTab = SHT_SYMTAB,     // .symtab
    StrTab = SHT_STRTAB,     // .strtab, .shstrtab
};

class Section {
  public:
    Section(SectionType type, u64 addr, u64 offset,u64 size, u64 entry_size, u64 align, std::vector<u8> raw)
        : name("[dummy]"), type(type), addr(addr), offset(offset), size(size), entry_size(entry_size), align(align), raw(raw) {
        assert(raw.size() == size);
        assert((align == 0 && type == SectionType::Null) || ((offset % align) == 0));
    }

    void set_name(std::string s) { name = s; }

    std::string get_name() const { return name; }
    SectionType get_type() const { return type; }
    u64 get_addr() const { return addr; }
    u64 get_offset() const { return offset; }
    u64 get_size() const { return size; }
    u64 get_entry_size() const { return entry_size; }
    u64 get_align() const { return align; }
    std::vector<u8> get_raw() const { return raw; }

  private:
    std::string name;
    const SectionType type;
    const u64 addr;
    const u64 offset;
    const u64 size;
    // now only used for .symtab
    const u64 entry_size;
    const u64 align;
    const std::vector<u8> raw;
};

class Elf {
  public:
    // create from raw data
    Elf(std::vector<u8> raw) : raw(raw) {
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
        fmt::print("parsing section header\n");
        for (int i = 0; i < get_section_num(); i++) {
            u64 sheader_elem_offset = eheader->e_shoff + eheader->e_shentsize * i;
            sheader.push_back((Elf64_Shdr *)&(raw[sheader_elem_offset]));
        }

        // get section data
        fmt::print("parsing elf body\n");
        for (int i = 0; i < get_section_num(); i++) {
            fmt::print("parsing sections[{}]\n", i);
            SectionType type = (SectionType)sheader[i]->sh_type;
            u64 addr = sheader[i]->sh_addr;
            u64 offset = sheader[i]->sh_offset;
            u64 size = sheader[i]->sh_size;
            u64 entry_size = sheader[i]->sh_entsize;
            u64 align = sheader[i]->sh_addralign;
            std::vector<u8> raw_data = get_raw(offset, size);

            // found symbol table
            if (type == SectionType::SymTab) {
                u64 symbol_num = size / entry_size;
                fmt::print("found symbol table (symbol num={})\n", symbol_num);
                sym_table = std::make_optional<SymTable>(SymTable(symbol_num, entry_size, raw_data));
            }

            std::shared_ptr<Section> section =
                std::make_shared<Section>(Section(type, addr, offset, size, entry_size, align, raw_data));
            sections.push_back(section);
        }
        // checks if parser found symbol table
        assert(sym_table != std::nullopt);

        // get section name from last section (=.shstrtab)
        std::vector<u8> shstrtab_raw = sections[get_section_num() - 1]->get_raw();
        for (int i = 0; i < get_section_num(); i++) {
            u64 name_index = sheader[i]->sh_name;
            // FIXME: better handling
            std::string name(&shstrtab_raw[name_index], &shstrtab_raw[name_index + 20]);
            // debug print
            // fmt::print("find section named {}\n", name.c_str());
            sections[i]->set_name(name.c_str());
        }

        // make sure elf contains .symtab, .strtab, and .shstrtab
        assert(get_section_by_name(".symtab") != nullptr);
        assert(get_section_by_name(".strtab") != nullptr);
        assert(get_section_by_name(".shstrtab") != nullptr);

        fmt::print("resolving symbol names\n");
        // auto symtab = get_section_by_name(".symtab");
        // auto symtab_r = symtab->get_raw();
        //  fmt::print(".symtab: {:2x}{:2x}{:2x}{:2x}{:2x}{:2x}{:2x}{:2x}\n", symtab_r[0], symtab_r[1], symtab_r[2],
        //             symtab_r[3], symtab_r[4], symtab_r[5], symtab_r[6], symtab_r[7]);
        //   set name for each sym_table entry
        for (int i = 0; i < sym_table->get_symbol_num(); i++) {
            //fmt::print("symbol[{}]\n", i);
            auto entries = sym_table->get_symbols();
            auto strtab = get_section_by_name(".strtab");
            assert(strtab != nullptr);
            auto name_index = (*entries)[i].get_sym()->st_name;
            //fmt::print("name_index : 0x{:x}\n", name_index);
            std::string symbol_name =
                std::string(&(strtab->get_raw()[name_index]), &(strtab->get_raw()[name_index + 20])).c_str();
            (*entries)[i].set_name(symbol_name);
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

    std::shared_ptr<Section> get_section_by_name(std::string name) {
        for (auto section : sections) {
            if (section->get_name() == name)
                return section;
        }
        return nullptr;
    }

    void dump() {
        // dump elf header
        fmt::print("ELF type : {}\n", eheader->e_type);
        fmt::print("ELF version: {}\n", eheader->e_version);
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
            fmt::print("  entsize : 0x{:x}\n", sections[i]->get_entry_size());
            fmt::print("  offset : 0x{:x}\n", sections[i]->get_offset());
            fmt::print("  align : 0x{:x}\n", sections[i]->get_align());
        }

        // dump symbol table
        fmt::print("symbols\n");
        auto entries = sym_table->get_symbols();
        for (int i = 0; i < sym_table->get_symbol_num(); i++) {
            fmt::print("symbol[{}]:\n", i);
            fmt::print("  name : \"{}\"\n", (*entries)[i].get_name());
            fmt::print("  value : 0x{:x}\n", (*entries)[i].get_sym()->st_value);
            fmt::print("  info : 0x{:x}\n", (*entries)[i].get_sym()->st_info);
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

    // symbol table
    std::optional<SymTable> sym_table;
};

} // namespace Parsed
} // namespace Elf
} // namespace Myld

#endif
