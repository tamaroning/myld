#include "builder.h"
#include "myld.h"
#include "context.h"
#include <cassert>
#include <elf.h>
#include <fstream>
#include <map>
#include <optional>

namespace Myld {

static void embed_raw_i32(std::vector<u8> &raw, u64 offset, i32 value) {
    // little endian
    raw[offset + 0] = (value >> 0) & 0xff;
    raw[offset + 1] = (value >> 8) & 0xff;
    raw[offset + 2] = (value >> 16) & 0xff;
    raw[offset + 3] = (value >> 24) & 0xff;
}

class Linker {
  public:
    Linker(std::vector<std::shared_ptr<Parse::Elf>> objs) : ctx(Context(objs)) {}

    void link() {
        fmt::print("collecting symbols\n");
        // push all symbols to linked symbol table
        for (auto obj : ctx.objs) {
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
                    if (!ctx.linked_sym_table.insert(sym)) {
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
        for (auto &[symbol_name, symbol] : ctx.linked_sym_table.get_entries()) {
            fmt::print(" name: \"{}\"\n", symbol_name);
        }

        // decide layout of `.text` here
        // Generate .text section by just concatinating all .text sections (alignment = 1byte)
        {
            std::vector<u8> text_raw({});
            for (auto obj : ctx.objs) {
                std::shared_ptr<Parse::Section> obj_text_section = obj->get_section_by_name(".text");
                Raw obj_text_raw = obj_text_section->get_raw();
                ctx.layout[obj_and_section(obj->get_filename(), ".text")] = text_raw.size();
                text_raw.insert(text_raw.end(), obj_text_raw.begin(), obj_text_raw.end());
            }
            ctx.section_raws[".text"] = text_raw;
        }

        // decide layout of `.rodata` here
        // Generate .text section by just concatinating all .rodata sections (alignment = 1byte)
        // TODO: STT_SECTIONかつ名前が.rodataであるシンボルが含まれているセクションのrodataだけ集めればいいっぽい
        {
            std::vector<u8> rodata_raw({});
            for (auto obj : ctx.objs) {
                std::shared_ptr<Parse::Section> obj_rodata_section = obj->get_section_by_name(".rodata");
                if (obj_rodata_section != nullptr) {
                    Raw obj_rodata_raw = obj_rodata_section->get_raw();
                    ctx.layout[obj_and_section(obj->get_filename(), ".rodata")] = rodata_raw.size();
                    rodata_raw.insert(rodata_raw.end(), obj_rodata_raw.begin(), obj_rodata_raw.end());
                }
            }
            ctx.section_raws[".rodata"] = rodata_raw;
        }

        // decide layout of `.data` here
        // TODO: .shstrtabを自動生成するようにしたい

        // resolve symbol address
        for (auto &[symbol_name, symbol] : ctx.linked_sym_table.get_entries()) {
            switch (symbol->get_type()) {
            case STT_NOTYPE:
                break;
            case STT_FILE:
                break;
            case STT_FUNC: {
                u64 resolved_addr = symbol->get_sym()->st_value + ctx.config.get_text_load_addr() +
                                    ctx.layout[obj_and_section(symbol->get_obj_file_name(), ".text")];
                symbol->get_sym()->st_value = resolved_addr;
                if (symbol_name == "_start") {
                    ctx._start_addr = resolved_addr;
                }
            } break;
            default: {
                fmt::print("Not implemented: symbol type = 0x{:x}\n", symbol->get_type());
                exit(1);
            } break;
            }
        }

        if (!ctx._start_addr.has_value()) {
            fmt::print("symbol `_start` not found. Could not create executable\n");
            std::exit(1);
        }

        // resolve rela
        fmt::print("resolving address\n");
        for (auto obj : ctx.objs) {
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
                        std::shared_ptr<LinkedSymTableEntry> symbol =
                            ctx.linked_sym_table.get_symbol_by_name(rela_name);
                        assert(symbol != nullptr);
                        i32 resolved_rel32 = symbol->get_sym()->st_value + rela_entry->get_rela()->r_addend -
                                             (ctx.config.get_text_load_addr() + rela_entry->get_rela()->r_offset);

                        fmt::print("  resolved rel32= {}\n", resolved_rel32);
                        // FIXME:
                        embed_raw_i32(ctx.section_raws[".text"],
                                      rela_offset + ctx.layout[obj_and_section(obj->get_filename(), ".text")],
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
        // TODO: fix
        Build::Builder::build_and_output(ctx, "myld-a.out");
    }

  private:
    Context ctx;
    /*
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
    */
};

} // namespace Myld