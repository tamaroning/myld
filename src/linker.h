#include "elf-util.h"
#include "myld.h"
#include "parse-elf.h"
#include <cassert>
#include <elf.h>
#include <fstream>
#include <map>
#include <optional>
#include <unordered_set>
// use ""s
using namespace std::literals::string_literals;

namespace Myld {

static void embed_raw_i32(std::vector<u8> &raw, u64 offset, i32 value) {
    // little endian
    raw[offset + 0] = (value >> 0) & 0xff;
    raw[offset + 1] = (value >> 8) & 0xff;
    raw[offset + 2] = (value >> 16) & 0xff;
    raw[offset + 3] = (value >> 24) & 0xff;
}

class Config {
  public:
    Config() : text_load_addr(0x80000) {}
    u64 get_text_load_addr() const { return text_load_addr; }

  private:
    u64 text_load_addr;
};

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

class Linker {
  public:
    Linker(std::vector<std::shared_ptr<Parse::Elf>> objs)
        : objs(objs), config(Config()), layout(), _start_addr(std::nullopt) {}

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

    void link() {
        linked_sym_table.init();

        fmt::print("collecting symbols\n");
        // push all symbols to linked symbol table
        for (auto obj : objs) {
            for (auto sym_entry : obj->get_sym_table().value().get_entries()) {
                switch (sym_entry->get_type()) {
                case STT_NOTYPE: {
                    // null or symbol defined in another file
                } break;
                case STT_SECTION: {
                    if (sym_entry->get_name() == ".rodata") {
                        // TODO: ここで指定されたセクションだけrodataをあつめればいいっぽい
                    }
                } break;
                default: {
                    // FIXME: ２つのファイルにvoid foo(); が含まれていたとき、シンボルを二重に登録してしまう
                    // vectorの代わりにhash setを使う
                    auto sym = std::make_shared<LinkedSymTableEntry>(
                        LinkedSymTableEntry::from(sym_entry, obj->get_filename()));
                    // insert symbol
                    if (!linked_sym_table.insert(sym)) {
                        // duplicated symbol
                        fmt::print("found duplicated symbol: \"{}\"\n", sym->get_name());
                        std::exit(1);
                    }
                } break;
                }
            }
        }

        // debug
        // dump symbols
        fmt::print("linked sym table:\n");
        for (auto &[symbol_name, symbol] : linked_sym_table.get_entries()) {
            fmt::print(" name: \"{}\"\n", symbol_name);
        }

        // decide layout of `.text` here
        // Generate .text section by just concatinating all .text sections (alignment = 1byte)
        {
            std::vector<u8> text_raw({});
            for (auto obj : objs) {
                std::shared_ptr<Parse::Section> obj_text_section = obj->get_section_by_name(".text");
                Raw obj_text_raw = obj_text_section->get_raw();
                layout[obj_and_section(obj->get_filename(), ".text")] = text_raw.size();
                text_raw.insert(text_raw.end(), obj_text_raw.begin(), obj_text_raw.end());
            }
            section_raws[".text"] = text_raw;
        }

        // decide layout of `.rodata` here
        // Generate .text section by just concatinating all .rodata sections (alignment = 1byte)
        // TODO: STT_SECTIONかつ名前が.rodataであるシンボルが含まれているセクションのrodataだけ集めればいいっぽい
        {
            std::vector<u8> rodata_raw({});
            for (auto obj : objs) {
                std::shared_ptr<Parse::Section> obj_rodata_section = obj->get_section_by_name(".rodata");
                if (obj_rodata_section != nullptr) {
                    Raw obj_rodata_raw = obj_rodata_section->get_raw();
                    layout[obj_and_section(obj->get_filename(), ".rodata")] = rodata_raw.size();
                    rodata_raw.insert(rodata_raw.end(), obj_rodata_raw.begin(), obj_rodata_raw.end());
                }
            }
            section_raws[".rodata"] = rodata_raw;
        }

        // decide layout of `.data` here
        // TODO: .shstrtabを自動生成するようにしたい

        // resolve symbol address
        for (auto &[symbol_name, symbol] : linked_sym_table.get_entries()) {
            switch (symbol->get_type()) {
            case STT_NOTYPE:
                break;
            case STT_FILE:
                break;
            case STT_FUNC: {
                u64 resolved_addr = symbol->get_sym()->st_value + config.get_text_load_addr() +
                                    layout[obj_and_section(symbol->get_obj_file_name(), ".text")];
                symbol->get_sym()->st_value = resolved_addr;
                if (symbol_name == "_start") {
                    _start_addr = resolved_addr;
                }
            } break;
            default: {
                fmt::print("Not implemented: symbol type = 0x{:x}\n", symbol->get_type());
                exit(1);
            } break;
            }
        }

        if (!_start_addr.has_value()) {
            fmt::print("symbol `_start` not found. Could not create executable\n");
            std::exit(1);
        }

        // resolve rela
        fmt::print("resolving address\n");
        for (auto obj : objs) {
            auto rela_text = obj->get_rela_by_name(".text");
            if (rela_text != nullptr) {
                for (auto rela_entry : rela_text->get_entries()) {
                    std::string rela_name = rela_entry->get_name();
                    u32 rela_type = rela_entry->get_type();
                    u64 rela_offset = rela_entry->get_rela()->r_offset;

                    fmt::print("resolving rela: \"{}\"\n", rela_name);
                    fmt::print("  rela type = 0x{:x}\n", rela_type);
                    switch (rela_type) {
                    case R_X86_64_PLT32: {
                        fmt::print("found relocation R_X86_64_PLT32\n");
                        // ELF spec (L + A - P)
                        // ref:
                        // https://stackoverflow.com/questions/64424692/how-does-the-address-of-r-x86-64-plt32-computed
                        // (sym->st_value) + (Addend) - (0x80000 + rela->r_offset)
                        std::shared_ptr<LinkedSymTableEntry> symbol = linked_sym_table.get_symbol_by_name(rela_name);
                        assert(symbol != nullptr);
                        i32 resolved_rel32 = symbol->get_sym()->st_value + rela_entry->get_rela()->r_addend -
                                             (config.get_text_load_addr() + rela_entry->get_rela()->r_offset);

                        fmt::print("  resolved rel32= {}\n", resolved_rel32);
                        // FIXME:
                        embed_raw_i32(section_raws[".text"],
                                      rela_offset + layout[obj_and_section(obj->get_filename(), ".text")],
                                      resolved_rel32);
                    } break;
                    default: {
                        fmt::print("  Not implemented: rela type = 0x{:x}\n", rela_type);
                        exit(1);
                    } break;
                    }
                }
            }
        }

        // build elf
        // TODO: move this to another class
        // elf header
        eheader = Utils::create_dummy_eheader();
        // program header
        pheader = Utils::create_dummy_pheader_load(config.get_text_load_addr(), config.get_text_load_addr(), 0x1000);

        // null
        Section null_section = Section(Utils::create_sheader_null());
        sections.push_back(null_section);

        // .text
        fmt::print("creating .text section\n");
        Section text_section = Section(Utils::create_dummy_sheader_text(27, 1, config.get_text_load_addr()));
        text_section.set_raw(section_raws[".text"]);
        sections.push_back(text_section);

        // .rodata
        std::optional<Section> rodata_section = std::nullopt;
        if (section_raws[".rodata"].size() > 0) {
            fmt::print("creating .rodata section\n");
            // TODO: アドレスの計算方法をもっとスマートにしたい
            rodata_section = Section(
                Utils::create_dummy_sheader_rodata(33, 1, config.get_text_load_addr() + text_section.sheader->sh_size));
            rodata_section.value().set_raw(section_raws[".rodata"]);
            sections.push_back(rodata_section.value());
        }

        // .data, .data*name*
        // TODO:

        // .symtab
        // create_section(".symtab", linked_sym_table.to_symtab_section_body(), 0);
        fmt::print("creating .symtab section\n");
        Section symtab_section =
            Section(Utils::create_dummy_sheader_symtab(1, 8, 3, linked_sym_table.get_local_symbol_num()));
        symtab_section.set_raw(linked_sym_table.to_symtab_section_body());
        sections.push_back(symtab_section);

        // .strtab
        // create_section(".strtab", linked_sym_table.to_strtab_section_body(), 0);
        fmt::print("creating .strtab section\n");
        Section strtab_section = Section(Utils::create_dummy_sheader_strtab(9, 1));
        strtab_section.set_raw(linked_sym_table.to_strtab_section_body());
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
        Utils::finalize_eheader(&eheader, _start_addr.value(), 1, sections.size(), sheader_start_offset);
        // とりあえず、.textセクションのみロードする
        fmt::print("finalize program header\n");
        u64 load_offset = text_section.sheader->sh_offset;
        u64 load_size = text_section.sheader->sh_size;
        Utils::finalize_pheader_load(&pheader, load_offset, load_size);
    }

  private:
    std::vector<std::shared_ptr<Myld::Parse::Elf>> objs;
    Config config;
    LinkedSymTable linked_sym_table;
    std::map<ObjAndSection, u64> layout;

    // resolved address of `_start`
    std::optional<u64> _start_addr;

    std::map<std::string, std::vector<u8>> section_raws;

    // output
    Elf64_Ehdr eheader;
    Elf64_Phdr pheader;

    std::vector<Section> sections;

    u64 padding_after_pheader;

    std::string shstrtab_content;

    void create_section(std::string section_name, std::vector<u8> raw, u64 addr) {
        fmt::print("creating section {}\n", section_name);
        u32 type = 0;
        u64 flags = 0;
        // addr provided by arg
        u32 link = 0;
        u32 info = 0;
        u64 addralign = 0;
        u64 entsize = 0;
        if (section_name == "") {
            // do nothing
        } else if (section_name == ".symtab") {
            type = SHT_SYMTAB;
            addralign = 8;
            entsize = sizeof(Elf64_Sym);
            link = 3;                                       // TODO: strtabのindex
            info = linked_sym_table.get_local_symbol_num(); // TODO: 最後のローカルシンボルのindex + 1とするのが正しい
        } else if (section_name == ".strtab" || section_name == ".shstrtab") {
            type = SHT_STRTAB;
            addralign = 1;
        } else if (section_name == ".text") {
            type = SHT_PROGBITS;
            addralign = 1;
            flags = SHF_ALLOC | SHF_EXECINSTR; // AX
        } else if (section_name == ".rodata") {
            type = SHT_PROGBITS;
            addralign = 1;
            flags = SHF_ALLOC; // A
        } else if (section_name.starts_with(".data")) {
            type = SHT_PROGBITS;
            addralign = 1;
            flags = SHF_WRITE | SHF_ALLOC; // WA
        } else {
            fmt::print("unknown section name {}\n", section_name);
            std::exit(1);
        }
        auto sheader =
            Utils::create_dummy_sheader(shstrtab_content.size(), type, flags, addr, link, info, addralign, entsize);
        shstrtab_content += section_name + "\0"s;

        Section section = Section(sheader);
        section.set_raw(raw);
        sections.push_back(section);
    }
};

} // namespace Myld