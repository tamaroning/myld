#ifndef ELF_UTIL_H
#define ELF_UTIL_H

#include "myld.h"
#include <elf.h>
#include <memory>

namespace Myld {
namespace Utils {

#define DUMMY 0

static Elf64_Ehdr create_dummy_eheader() {
    Elf64_Ehdr header = Elf64_Ehdr{
        // 16 byte
        .e_ident = {ELFMAG0, ELFMAG1, ELFMAG2, ELFMAG3, ELFCLASS64, ELFDATA2LSB, EV_CURRENT, ELFOSABI_SYSV},
        // 2 byte
        .e_type = ET_EXEC,
        // 2 byte (0x3e)
        .e_machine = EM_X86_64,
        // 4 byte
        .e_version = EV_CURRENT,
        // 8 byte
        .e_entry = DUMMY,
        // 8 byte
        .e_phoff = DUMMY,
        // 8 byte
        .e_shoff = DUMMY,
        // 4 byte
        .e_flags = 0x0, // TODO: ここ何？
        // 2 byte
        .e_ehsize = sizeof(Elf64_Ehdr),
        // 2 byte
        .e_phentsize = sizeof(Elf64_Phdr),
        // 2 byte
        .e_phnum = DUMMY,
        // 2 byte
        .e_shentsize = sizeof(Elf64_Shdr),
        // 2 byte (number of section header entry)
        .e_shnum = DUMMY,
        // 2 byted
        .e_shstrndx = DUMMY,
    };
    return header;
}

static void finalize_eheader(Elf64_Ehdr *eheader, Elf64_Addr entry_point_addr, u64 pheader_entry_num,
                             u64 sheader_entry_num, u64 sheader_start_offset) {
    Elf64_Off pheader_offset = (pheader_entry_num == 0) ? 0 : sizeof(Elf64_Ehdr);
    Elf64_Off sheader_offset = (pheader_entry_num == 0) ? 0 : sheader_start_offset;

    Elf64_Half shstrtab_index = (sheader_entry_num == 0) ? 0 : sheader_entry_num - 1;

    eheader->e_entry = entry_point_addr;
    eheader->e_phoff = pheader_offset;
    eheader->e_shoff = sheader_offset;
    eheader->e_phnum = pheader_entry_num;
    eheader->e_shnum = sheader_entry_num;
    eheader->e_shstrndx = shstrtab_index;
}

static Elf64_Phdr create_dummy_pheader_load(u64 vaddr, u64 paddr, u64 align) {
    Elf64_Phdr program_header_entry_load = Elf64_Phdr{
        .p_type = PT_LOAD,
        // TODO: ldは特定のセクションの有無に応じて変えている
        // e.g. ldでは .textのみならRX. .text & .rodataならRWX.
        .p_flags = PF_R | PF_W | PF_X,
        .p_offset = DUMMY,
        .p_vaddr = vaddr,
        .p_paddr = paddr,
        .p_filesz = DUMMY,
        .p_memsz = DUMMY,
        .p_align = align,
    };
    return program_header_entry_load;
}

static void finalize_pheader_load(Elf64_Phdr *pheader, u64 offset, u64 size) {
    pheader->p_offset = offset;
    pheader->p_filesz = size; // TODO: correct?
    pheader->p_memsz = size;  // TODO: correct>
}

static std::shared_ptr<Elf64_Shdr> create_dummy_sheader(u32 name, u32 type, u64 flags, u64 addr, u32 link, u32 info,
                                                        u64 addralign, u64 entsize) {
    return std::make_shared<Elf64_Shdr>(Elf64_Shdr{
        .sh_name = name,
        .sh_type = type,
        .sh_flags = flags,
        .sh_addr = addr,
        .sh_offset = DUMMY,
        .sh_size = DUMMY,
        .sh_link = link,
        .sh_info = info,
        .sh_addralign = addralign,
        .sh_entsize = entsize,
    });
}

static Elf64_Sym create_null_sym() {
    return Elf64_Sym{
        .st_name = 0,  // "\0"
        .st_info = 0,  // TODO:
        .st_other = 0, // TODO:
        .st_shndx = SHN_UNDEF,
        .st_value = 0,
        .st_size = 0,
    };
}

} // namespace Utils
} // namespace Myld

#endif
