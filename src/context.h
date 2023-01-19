#ifndef CONTEXT_H
#define CONTEXT_H

#include "elf-util.h"
#include "myld.h"
#include "parse-elf.h"
#include <cassert>
#include <map>
#include <optional>

namespace Myld {
using namespace Myld;

class Config {
  public:
    Config() : text_load_addr(0x80000) {}
    u64 get_text_load_addr() const { return text_load_addr; }

  private:
    u64 text_load_addr;
};

class LinkedSymTableEntry {
  public:
    LinkedSymTableEntry(std::string name, std::vector<u8> bytes, std::string obj_file_name)
        : name(name), bytes(bytes), obj_file_name(obj_file_name) {
        // check data size
        assert(bytes.size() == sizeof(Elf64_Sym));
    }

    static LinkedSymTableEntry from(std::shared_ptr<Parse::SymTableEntry> entry, std::string obj_file_name) {
        LinkedSymTableEntry linked_sym(entry->get_name(), entry->get_raw().to_vec(), obj_file_name);
        // name indexは意味をなさなくなるので0にセットしておく
        linked_sym.get_sym()->st_name = 0;
        return linked_sym;
    }

    std::string get_name() const { return name; }

    std::string get_obj_file_name() const { return obj_file_name; }

    // clone and return its raw data
    std::vector<u8> get_bytes() const { return std::vector<u8>(bytes); }

    Elf64_Sym *get_sym() {
        // maybe unsafe operation?
        return (Elf64_Sym *)(&bytes[0]);
    }

    u8 get_bind() const { return ELF64_ST_BIND(get_const_sym()->st_info); }

    u8 get_type() const { return ELF64_ST_TYPE(get_const_sym()->st_info); }

  private:
    std::string name;
    std::vector<u8> bytes;
    // "" in case of null symbol
    std::string obj_file_name;

    const Elf64_Sym *get_const_sym() const {
        // maybe unsafe operation?
        return (Elf64_Sym *)(&bytes[0]);
    }
};

// class represents a new symbol table whose symbols are gathered from multiple object files
class LinkedSymTable {
  public:
    LinkedSymTable() : entries({}) {}

    std::map<std::string, std::shared_ptr<LinkedSymTableEntry>> get_entries() const { return entries; }

    // initialize symbol table. Especially, push null symbol to the table
    void init() {
        // push null symbol as the first symbol
        auto null_sym_bytes = std::vector<u8>(to_bytes(Utils::create_null_sym()));
        LinkedSymTableEntry null_sym = LinkedSymTableEntry("", null_sym_bytes, "");
        insert(std::make_shared<LinkedSymTableEntry>(null_sym));
    }

    bool insert(std::shared_ptr<LinkedSymTableEntry> sym_entry) {
        // TODO: elaborate
        bool ret = entries.find(sym_entry->get_name()) == entries.end();
        entries.insert(std::make_pair(sym_entry->get_name(), sym_entry));
        return ret;
    }

    std::shared_ptr<LinkedSymTableEntry> get_symbol_by_name(std::string name) const {
        if (entries.find(name) == entries.end()) {
            return nullptr;
        } else {
            return entries.at(name);
        }
    }

    // convert to .symtab section data
    std::vector<u8> to_symtab_section_body() {
        u64 name_index = 0;
        std::vector<u8> bytes;
        bytes.reserve(entries.size() * sizeof(Elf64_Sym));
        for (int i = 0; i < 2; i++) {
            for (auto entry : entries) {
                // To locate FILE symbols front of symbol table entry, we take the following measure:
                // We scan entries linearly twice.
                // In first scan, only looks for FILE
                // In second scan. looks for other symbols
                // FIXME: buggy. 単純にエントリーをtypeでソートするほうがいい
                if (i == 0 && entry.second->get_type() != STT_FILE) {
                    continue;
                } else if (i == 1 && entry.second->get_type() == STT_FILE) {
                    continue;
                }

                // set name index
                entry.second->get_sym()->st_name = name_index;
                std::vector<u8> entry_bytes = entry.second->get_bytes();
                bytes.insert(bytes.end(), entry_bytes.begin(), entry_bytes.end());
                // update name index (plus 1 because of "\0")
                name_index += entry.second->get_name().length() + 1;
            }
        }
        return bytes;
    }

    // convert to .strtab section data
    // これを呼んだあとに、シンボルを追加すると.strtabの一貫性が失われる
    std::vector<u8> to_strtab_section_body() {
        std::vector<u8> bytes;
        bytes.reserve(entries.size() * sizeof(Elf64_Sym));
        for (int i = 0; i < 2; i++) {
            for (auto entry : entries) {
                // To locate FILE symbols front of symbol table entry, we take the following measure:
                // We scan entries linearly twice.
                // In first scan, only looks for FILE
                // In second scan. looks for other symbols
                // FIXME: buggy. 単純にエントリーをtypeでソートするほうがいい
                if (i == 0 && entry.second->get_type() != STT_FILE) {
                    continue;
                } else if (i == 1 && entry.second->get_type() == STT_FILE) {
                    continue;
                }

                std::string name = entry.second->get_name();
                bytes.insert(bytes.end(), name.begin(), name.end());
                bytes.push_back('\0');
            }
        }
        return bytes;
    }

    u64 get_local_symbol_num() {
        u64 ret = 0;
        for (auto entry : entries) {
            if (entry.second->get_bind() == STB_LOCAL) {
                ret++;
            }
        }
        return ret;
    }

  private:
    // map from symbol name to symbol
    std::map<std::string, std::shared_ptr<LinkedSymTableEntry>> entries;
};

class Context {
  public:
    Context(std::vector<std::shared_ptr<Myld::Parse::Elf>> objs)
        : objs(objs), config(Config()), layout(), _start_addr(std::nullopt) {
        linked_sym_table.init();
    }

    std::vector<std::shared_ptr<Myld::Parse::Elf>> objs;
    Config config;
    LinkedSymTable linked_sym_table;
    std::map<ObjAndSection, u64> layout;

    // resolved address of `_start`
    std::optional<u64> _start_addr;

    std::map<std::string, std::vector<u8>> section_raws;

    void build_and_output(std::string filename);
};

} // namespace Myld

#endif
