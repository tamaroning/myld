#ifndef BUILDER_H
#define BUILDER_H

#include "context.h"
#include "elf-util.h"
#include "myld.h"
#include <cassert>
#include <elf.h>
#include <fstream>
#include <optional>

// use ""s
using namespace std::literals::string_literals;

namespace Myld {
namespace Build {

using namespace Myld;

class Section {
  public:
    Section(std::string name, std::shared_ptr<Elf64_Shdr> sheader)
        : sheader(sheader), name(name), padding_size(std::nullopt), raw({}) {}

    void finalize(u64 padding, u64 offset) {
        set_padding_size(padding);
        sheader->sh_offset = offset;
        if (sheader->sh_addralign != 0) {
            assert((sheader->sh_offset % sheader->sh_addralign) == 0);
        }
    }

    std::string get_name() const { return name; }

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
    std::string name;
    // padding just before the section
    std::optional<u64> padding_size;
    std::vector<u8> raw;

    void set_padding_size(u64 size) {
        assert(!padding_size.has_value());
        padding_size = std::make_optional(size);
    }
};

class Builder {
  public:
    Builder() {}

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
            fmt::print("  padding before body:  0x{:x}\n", sections[i]->get_padding_size());
            std::fill_n(std::ostream_iterator<char>(stream), sections[i]->get_padding_size(), '\0');
            current_offset += sections[i]->get_padding_size();

            fmt::print("  body size:  0x{:x}\n", sections[i]->sheader->sh_size);
            assert(current_offset == sections[i]->sheader->sh_offset);
            stream.write((char *)&(sections[i]->get_raw()[0]), sections[i]->sheader->sh_size);
            current_offset += sections[i]->sheader->sh_size;
        }

        // section headers
        fmt::print("writing section header\n");
        assert(current_offset == eheader.e_shoff);
        for (int i = 0; i < sections.size(); i++) {
            fmt::print("header of section [{}]\n", i);
            stream.write((char *)&(*(sections[i]->sheader)), sizeof(Elf64_Shdr));
            current_offset += sizeof(Elf64_Shdr);
        }
    }

    void build(Context ctx) {
        // elf header
        eheader = Utils::create_dummy_eheader();
        // program header
        pheader =
            Utils::create_dummy_pheader_load(ctx.config.get_text_load_addr(), ctx.config.get_text_load_addr(), 0x1000);

        current_load_addr = ctx.config.get_text_load_addr();

        // null
        create_section(ctx, "", {});

        // .text
        create_section(ctx, ".text", ctx.section_raws[".text"]);

        // .rodata
        // std::optional<Section> rodata_section = std::nullopt;
        if (ctx.section_raws[".rodata"].size() > 0) {
            create_section(ctx, ".rodata", ctx.section_raws[".rodata"]);
        }

        // .data, .data*name*
        // TODO:

        // .symtab
        create_section(ctx, ".symtab", ctx.linked_sym_table.to_symtab_section_body());

        // .strtab
        create_section(ctx, ".strtab", ctx.linked_sym_table.to_strtab_section_body());

        // .shstrtab
        create_section(ctx, ".shstrtab", std::vector<u8>(shstrtab_content.begin(), shstrtab_content.end()));

        // calculate padding before each section
        u64 first_section_start_offset = 0x1000;
        u64 section_start_offset = first_section_start_offset;
        padding_after_pheader = section_start_offset - sizeof(Elf64_Ehdr) - sizeof(Elf64_Phdr);
        fmt::print("padding after pheader: 0x{:x}\n", padding_after_pheader);
        fmt::print("finalizing section\n");
        for (int i = 0; i < sections.size(); i++) {
            fmt::print("section [{}]: ", i);
            u64 align = sections[i]->sheader->sh_addralign;
            fmt::print("offset = 0x{:x}, ", section_start_offset);
            fmt::print("size = 0x{:x}, ", sections[i]->sheader->sh_size);
            fmt::print("align = 0x{:x}, ", align);

            u64 padding_size;
            if (sections[i]->sheader->sh_type == SHT_NULL) {
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
            sections[i]->finalize(padding_size, section_start_offset);
            section_start_offset += sections[i]->sheader->sh_size;
        }

        // calulate distance from first section start to last section end
        u64 dist_from_sections_start_to_sections_end = 0;
        for (int i = 0; i < sections.size(); i++) {
            dist_from_sections_start_to_sections_end += sections[i]->get_padding_size();
            dist_from_sections_start_to_sections_end += sections[i]->sheader->sh_size;
        }

        fmt::print("finalize elf header\n");
        u64 sheader_start_offset = first_section_start_offset + dist_from_sections_start_to_sections_end;
        fmt::print("start of section header: 0x{:x} = {}\n", sheader_start_offset, sheader_start_offset);
        Utils::finalize_eheader(&eheader, ctx._start_addr.value(), 1, sections.size(), sheader_start_offset);
        // とりあえず、.textセクションのみロードする
        fmt::print("finalize program header\n");
        auto text_section = get_section_by_name(".text");
        assert(text_section != nullptr);
        u64 load_offset = text_section->sheader->sh_offset;
        u64 load_size = text_section->sheader->sh_size;
        Utils::finalize_pheader_load(&pheader, load_offset, load_size);
    }

  private:
    // output
    Elf64_Ehdr eheader;
    Elf64_Phdr pheader;
    u64 padding_after_pheader;

    std::vector<std::shared_ptr<Section>> sections;

    std::string shstrtab_content;
    u64 current_load_addr;

    std::shared_ptr<Section> get_section_by_name(std::string name) {
        for (auto section : sections) {
            if (section->get_name() == name) {
                return section;
            }
        }
        return nullptr;
    }

    void create_section(const Context ctx, std::string section_name, std::vector<u8> raw) {
        fmt::print("creating section {}\n", section_name);
        u32 type = 0;
        u64 flags = 0;
        u64 addr = 0;
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
            // .strtabのsection header index
            link = sections.size() + 1;
            info =
                ctx.linked_sym_table.get_local_symbol_num(); // TODO: 最後のローカルシンボルのindex + 1とするのが正しい
        } else if (section_name == ".strtab") {
            type = SHT_STRTAB;
            addralign = 1;
        } else if (section_name == ".shstrtab") {
            type = SHT_STRTAB;
            addralign = 1;
        } else if (section_name == ".text") {
            type = SHT_PROGBITS;
            addralign = 1;
            flags = SHF_ALLOC | SHF_EXECINSTR; // AX
            addr = current_load_addr;
            current_load_addr += raw.size();
        } else if (section_name == ".rodata") {
            type = SHT_PROGBITS;
            addralign = 1;
            flags = SHF_ALLOC; // A
            addr = current_load_addr;
            current_load_addr += raw.size();
        } else if (section_name.starts_with(".data")) {
            type = SHT_PROGBITS;
            addralign = 1;
            flags = SHF_WRITE | SHF_ALLOC; // WA
            addr = current_load_addr;
            current_load_addr += raw.size();
        } else {
            fmt::print("unknown section name {}\n", section_name);
            std::exit(1);
        }
        fmt::print("name index: {} size: {}\n", shstrtab_content.size(), section_name.size());
        auto sheader =
            Utils::create_dummy_sheader(shstrtab_content.size(), type, flags, addr, link, info, addralign, entsize);
        shstrtab_content += section_name + "\0"s;

        Section section = Section(section_name, sheader);
        // FIXME: ここで足しておかないとバグる
        if (section_name == ".shstrtab") {
            std::string n(".shstrtab\0"s);
            raw.insert(raw.end(), n.begin(), n.end());
        }
        section.set_raw(raw);
        sections.push_back(std::make_shared<Section>(section));
    }
};

} // namespace Build
} // namespace Myld

#endif
