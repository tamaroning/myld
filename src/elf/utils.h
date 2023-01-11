
#include <elf.h>

namespace Myld {
namespace Elf {

static Elf64_Ehdr *create_header(Elf64_Half program_header_entry_num, Elf64_Half section_num, Elf64_Off section_header_start) {
    Elf64_Off program_header_offset = (program_header_entry_num == 0) ? 0 : sizeof(Elf64_Ehdr);

    // .shstrtabセクションのindex (.shstrtabは最後のセクションに配置)
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
        .e_shstrndx = shstrtab_index,
    };
    return header;
}

static Elf64_Phdr *create_program_header_entry_load() {
    // FIXME: use dummy for now
    Elf64_Phdr *program_header_entry_load = new Elf64_Phdr{
        // FIXME:
        .p_type = PT_LOAD,
        .p_flags = PF_R | PF_X, 
        .p_offset = 0x1000, // TODO: .textの_startまでのオフセット
        .p_vaddr = 0x80000,     
        .p_paddr = 0x80000,     
        .p_filesz = 0x18,       
        .p_memsz = 0x18,
        .p_align = 0x1000,
    };
    return program_header_entry_load;
}

static Elf64_Shdr *create_section_header_entry_null(Elf64_Word section_name_index) {
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

static Elf64_Shdr *create_section_header_entry_text(Elf64_Word section_name_index, Elf64_Xword section_size,
                                             Elf64_Xword offset) {
    Elf64_Shdr *shdr = new Elf64_Shdr;
    shdr->sh_name = section_name_index;
    shdr->sh_type = SHT_PROGBITS;
    shdr->sh_flags = SHF_ALLOC | SHF_EXECINSTR;
    shdr->sh_addr = 0x80000; // TODO:
    shdr->sh_offset = offset;
    shdr->sh_size = section_size;
    shdr->sh_link = 0;
    shdr->sh_info = 0;
    shdr->sh_addralign = 1;
    shdr->sh_entsize = 0;

    return shdr;
}

static Elf64_Shdr *create_section_header_entry_strtab(Elf64_Word section_name_index, Elf64_Xword section_size,
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

} // namespace Elf
} // namespace Myld