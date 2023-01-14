#include "elf-util.h"
#include "parse-elf.h"
#include <cassert>
#include <elf.h>
#include <fstream>
#include <optional>

namespace Myld {

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

class Linker {
  public:
    Linker(std::shared_ptr<Parse::Elf> obj) : obj(obj), config(Config()) {}

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
        // resolve symbols
        // f : 0x0->0x80000
        // _start: 0x18 -> 0x80018
        for (auto sym_entry : obj->get_sym_table().value().get_entries()) {
            std::string sym_name = sym_entry->get_name();
            u8 sym_type = sym_entry->get_type();
            u64 sym_current_value = sym_entry->get_sym()->st_value;
            switch (sym_type) {
            case STT_NOTYPE: {
                // TODO:
            } break;
            case STT_FILE: {
                // TODO:
            } break;
            case STT_FUNC: {
                u64 resolved_addr = sym_current_value + config.get_text_load_addr();
                sym_entry->set_value(resolved_addr);
                if (sym_name == "_start") {
                    _start_addr = resolved_addr;
                }
            } break;
            default: {
                fmt::print("Not implemented: symbol type = 0x{:x}\n", sym_type);
                exit(1);
            } break;
            }
        }

        /*
        // resolve rela
        if (obj->get_rela_text().has_value()) {
            Parse::RelaText rela_text = obj->get_rela_text().value();
            for (auto rela_entry : rela_text.get_entries()) {
                std::string rela_name = rela_entry->get_name();
                u32 rela_type = rela_entry->get_type();
                u64 rela_offset = rela_entry->get_rela()->r_offset;

                fmt::print("resolving rela: \"{}\"\n", rela_name);
                fmt::print("  rela type = 0x{:x}\n", rela_type);
                switch (rela_type) {
                case R_X86_64_PLT32: {
                    fmt::print("found relocation R_X86_64_PLT32\n");
                    // PLT32という名前だがpc relativeとして計算 (St_valu    e + Addend - P) P:
                    // 再配置されるメモリ位置のアドレス ref:
                    // https://stackoverflow.com/questions/64424692/how-does-the-address-of-r-x86-64-plt32-computed
                    // TODO:
                    // (sym->st_value) + (Addend) - (0x80000 + rela->r_offset)
                    std::shared_ptr<Parse::SymTableEntry> symbol = obj->get_sym_table()->get_symbol_by_name(rela_name);
                    assert(symbol != nullptr);
                    i32 resolved_rel32 = symbol->get_sym()->st_value + rela_entry->get_rela()->r_addend -
                                         (config.get_text_load_addr() + rela_entry->get_rela()->r_offset);

                    fmt::print("  resolved rel32= {}\n", resolved_rel32);
                    // FIXME:
                    // obj->get_section_by_name(".text")->embed_raw_i32(rela_offset, resolved_rel32);
                } break;
                default: {
                    fmt::print("  Not implemented: rela type = 0x{:x}\n", rela_type);
                    exit(1);
                } break;
                }
            }
        }
        */

        // --------------- build elf ---------------
        // elf header
        eheader = Utils::create_dummy_eheader();
        // program header
        pheader = Utils::create_dummy_pheader_load(config.get_text_load_addr(), config.get_text_load_addr(), 0x1000);

        // null
        Section null_section = Section(Utils::create_sheader_null());
        sections.push_back(null_section);

        // .text
        fmt::print("creating .text section\n");
        Section text_section = Section(Utils::create_dummy_sheader_text(27, 0x1));
        // NOTE: relocationはobjのSectionでin-placeに行うので、このタイミングでrawを取る
        std::shared_ptr<Parse::Section> obj_text_section = obj->get_section_by_name(".text");
        u64 obj_text_section_size = obj_text_section->get_header()->sh_size;
        text_section.set_raw(std::vector<u8>((u8 *)obj_text_section->get_raw().to_pointer(),
                                             (u8 *)(obj_text_section->get_raw().to_pointer() + obj_text_section_size)));
        sections.push_back(text_section);

        // .symtab
        fmt::print("creating .symtab section\n");
        Section symtab_section = Section(Utils::create_dummy_sheader_symtab(1, 8));
        // NOTE: relocationはobjのSectionでin-placeに行うので、このタイミングでrawを取る
        std::shared_ptr<Parse::Section> obj_symtab_section = obj->get_section_by_name(".symtab");
        u64 obj_symtab_section_size = obj_symtab_section->get_header()->sh_size;
        symtab_section.set_raw(
            std::vector<u8>((u8 *)obj_symtab_section->get_raw().to_pointer(),
                            (u8 *)(obj_symtab_section->get_raw().to_pointer() + obj_symtab_section_size)));
        sections.push_back(symtab_section);

        // .strtab
        fmt::print("creating .strtab section\n");
        Section strtab_section = Section(Utils::create_dummy_sheader_strtab(9, 1));
        // NOTE: relocationはobjのSectionでin-placeに行うので、このタイミングでrawを取る
        std::shared_ptr<Parse::Section> obj_strtab_section = obj->get_section_by_name(".strtab");
        u64 obj_strtab_section_size = obj_strtab_section->get_header()->sh_size;
        fmt::print(".strtab size = 0x{:x}\n", obj_strtab_section_size);
        strtab_section.set_raw(
            std::vector<u8>((u8 *)obj_strtab_section->get_raw().to_pointer(),
                            (u8 *)(obj_strtab_section->get_raw().to_pointer() + obj_strtab_section_size)));
        sections.push_back(strtab_section);

        // .shstrtab
        fmt::print("creating .shstrtab section\n");
        Section shstrtab_section = Section(Utils::create_dummy_sheader_strtab(17, 0x1));
        std::vector<u8> shstrtab_raw = {
            '\0',                                                 // null
            '.',  's', 'y', 'm', 't', 'a',  'b', '\0',            // .symtab
            '.',  's', 't', 'r', 't', 'a',  'b', '\0',            //.strtab
            '.',  's', 'h', 's', 't', 'r',  't', 'a',  'b', '\0', //.shstrtab
            '.',  't', 'e', 'x', 't', '\0',                       // .text
        };
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
        // TODO: .text内の_startのオフセット + 0x8000とする
        Utils::finalize_eheader(&eheader, _start_addr, 1, sections.size(), sheader_start_offset);
        // とりあえず、.textセクションのみロードする
        fmt::print("finalize program header\n");
        Utils::finalize_pheader_load(&pheader, text_section.sheader->sh_offset, text_section.sheader->sh_size);
    }

  private:
    std::shared_ptr<Myld::Parse::Elf> obj;
    Config config;

    // output
    Elf64_Ehdr eheader;
    Elf64_Phdr pheader;

    std::vector<Section> sections;

    u64 padding_after_pheader;

    // 解決した_startのアドレス
    u64 _start_addr;
};

} // namespace Myld