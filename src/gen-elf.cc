#include <elf.h>
#include <fmt/core.h>
#include <fstream>
#include <iostream>
#include <vector>

namespace Myld::Elf {
using namespace Myld::Elf;

Elf64_Ehdr *create_header(Elf64_Half program_header_entry_num, Elf64_Half section_num, Elf64_Off section_header_start) {
    Elf64_Off program_header_offset = (program_header_entry_num == 0) ? 0 : sizeof(Elf64_Ehdr);

    // .shstrtabセクションのindex
    Elf64_Half shstrtab_index = (section_num == 0) ? 0 : section_num - 1;

    Elf64_Ehdr *header = new Elf64_Ehdr{
        // 16 byte
        .e_ident = {ELFMAG0, ELFMAG1, ELFMAG2, ELFMAG3, ELFCLASS64, ELFDATA2LSB, EV_CURRENT, ELFOSABI_SYSV}, // FIXME:
        // 2 byte
        .e_type = ET_EXEC,
        // 2 byte (0x3e)
        .e_machine = EM_X86_64,
        // 4 byte
        .e_version = EV_CURRENT,
        // 8 byte
        .e_entry = 0x80000, // TODO:
        // 8 byte
        .e_phoff = program_header_offset,
        // 8 byte
        .e_shoff = (section_num == 0) ? 0 : section_header_start,
        // 4 byte
        .e_flags = 0x0, // TODO:
        // 2 byte
        .e_ehsize = sizeof(Elf64_Ehdr),
        // 2 byte
        .e_phentsize = sizeof(Elf64_Phdr),
        // 2 byte
        .e_phnum = program_header_entry_num,
        // 2 byte
        .e_shentsize = sizeof(Elf64_Shdr),
        // 2 byte (number of section header entry)
        .e_shnum = section_num,
        // 2 byted
        // .shstrtabは最後のセクションに配置 (1)
        .e_shstrndx = shstrtab_index,
    };
    return header;
}

Elf64_Phdr *create_program_header_entry_load() {
    // FIXME: use dummy for now
    Elf64_Phdr *program_header_entry_load = new Elf64_Phdr{
        .p_type = PT_LOAD,
        .p_flags = PF_R | PF_X, // TODO:
        .p_offset = 0x1000,     // FIXME:
        .p_vaddr = 0x80000,     // TODO:
        .p_paddr = 0x80000,     // TODO:
        .p_filesz = 0x38,       // TODO:
        .p_memsz = 0x38,
        .p_align = 0x1000,
    };
    return program_header_entry_load;
}

Elf64_Shdr *create_section_header_entry_null(Elf64_Word section_name_index) {
    Elf64_Shdr *section_header_entry_null = new Elf64_Shdr{
        .sh_name = section_name_index,
        .sh_type = SHT_NULL,
        .sh_flags = 0,
        .sh_addr = 0,
        .sh_offset = 0,
        .sh_size = 0,
        .sh_link = 0,
        .sh_info = 0,
        .sh_addralign = 0,
        .sh_entsize = 0,
    };
    return section_header_entry_null;
}

Elf64_Shdr *create_section_header_entry_strtab(Elf64_Word section_name_index, Elf64_Xword section_size,
                                               Elf64_Xword offset) {
    Elf64_Shdr *shdr = new Elf64_Shdr;
    shdr->sh_name = section_name_index;
    shdr->sh_type = SHT_STRTAB;
    shdr->sh_flags = 0;
    shdr->sh_addr = 0;
    shdr->sh_offset = offset;
    shdr->sh_size = section_size;
    shdr->sh_link = 0;
    shdr->sh_info = 0;
    shdr->sh_addralign = 1;
    shdr->sh_entsize = 0;

    return shdr;
}

void output_exe() {
    std::ofstream output_elf("a.out", std::ios::binary | std::ios::trunc);

    // -----
    // elf header
    // -----
    // program header table
    // -----
    // segment 1
    // -----
    // ...
    // -----
    // segment N
    // -----
    // section header table

    // segmentは以下の順で並べる
    // .text .strtab .shstrtab

    const char *section_strtab = "\0";
    Elf64_Off section_strtab_size = 1;

    const char *shstrtab[4] = {
        "\0",
        ".strtab\0",
        ".shstrtab\0",
        ".text\0",
    };
    Elf64_Word name_null_ofs = 0;
    Elf64_Word name_null_len = 1;
    Elf64_Word name_strtab_ofs = name_null_ofs + name_null_len;
    Elf64_Word name_strtab_len = 8;
    Elf64_Word name_shstrtab_ofs = name_strtab_ofs + name_strtab_len;
    Elf64_Word name_shstrtab_len = 10;
    Elf64_Word name_text_ofs = name_shstrtab_ofs + name_shstrtab_len;
    Elf64_Word name_text_len = 6;

    Elf64_Off section_shstrtab_size = name_text_ofs + name_text_len;

    Elf64_Half program_header_entry_num = 1;
    Elf64_Off segment_start = sizeof(Elf64_Ehdr) + sizeof(Elf64_Phdr) * program_header_entry_num;
    Elf64_Off section_strtab_start = segment_start;
    Elf64_Off section_shstrtab_start = section_strtab_start + section_strtab_size;

    Elf64_Off section_header_start = section_shstrtab_start + section_shstrtab_size;

    // elf header
    Elf64_Ehdr *elf_header = create_header(1, 3, section_header_start);
    output_elf.write((char *)elf_header, sizeof(Elf64_Ehdr));

    // program header
    Elf64_Phdr *program_header_entry_load = create_program_header_entry_load();
    output_elf.write((char *)program_header_entry_load, sizeof(Elf64_Phdr) * program_header_entry_num);

    // .strtab
    output_elf.write(section_strtab, section_strtab_size);
    // .shstrtab
    output_elf.write(shstrtab[0], name_null_len);
    output_elf.write(shstrtab[1], name_strtab_len);
    output_elf.write(shstrtab[2], name_shstrtab_len);
    output_elf.write(shstrtab[3], name_text_len);

    // section header
    Elf64_Shdr *section_header_entry_null = create_section_header_entry_null(name_null_ofs);
    output_elf.write(reinterpret_cast<const char *>(section_header_entry_null), sizeof(Elf64_Shdr));

    Elf64_Shdr *section_header_entry_strtab =
        create_section_header_entry_strtab(name_strtab_ofs, section_strtab_size, section_strtab_start);
    output_elf.write(reinterpret_cast<const char *>(section_header_entry_strtab), sizeof(Elf64_Shdr));

    Elf64_Shdr *section_header_entry_shstrtab =
        create_section_header_entry_strtab(name_shstrtab_ofs, section_shstrtab_size, section_shstrtab_start);
    output_elf.write((char *)section_header_entry_shstrtab, sizeof(Elf64_Shdr));

    fmt::print("generated a.out\n");
}
} // namespace Myld::Elf
