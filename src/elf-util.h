#include "debug.h"
#include "myld.h"
#include <cassert>
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
        .p_flags = PF_R | PF_X,
        .p_offset = DUMMY,
        .p_vaddr = vaddr,
        .p_paddr = paddr,
        .p_filesz = DUMMY,
        .p_memsz = DUMMY,
        .p_align = align,
    };
    return program_header_entry_load;
}

static void finalize_pheader_load(Elf64_Phdr *pheader, u64 text_offset, u64 text_size) {
    pheader->p_offset = text_offset;
    pheader->p_filesz = text_size;
    pheader->p_memsz = text_size;
}

static std::shared_ptr<Elf64_Shdr> create_sheader_null() {
    return std::make_shared<Elf64_Shdr>(Elf64_Shdr{
        .sh_name = 0,
        .sh_type = SHT_NULL,
        .sh_flags = 0,
        .sh_addr = 0,
        .sh_offset = 0,
        .sh_size = 0,
        .sh_link = 0,
        .sh_info = 0,
        .sh_addralign = 0,
        .sh_entsize = 0,
    });
}

static std::shared_ptr<Elf64_Shdr> create_dummy_sheader_text(u32 name_index, Elf64_Xword align) {
    return std::make_shared<Elf64_Shdr>(Elf64_Shdr{
        .sh_name = name_index,
        .sh_type = SHT_PROGBITS,
        .sh_flags = SHF_ALLOC | SHF_EXECINSTR,
        .sh_addr = 0, // TODO:
        .sh_offset = DUMMY,
        .sh_size = DUMMY,
        .sh_link = 0,
        .sh_info = 0,
        .sh_addralign = align,
        .sh_entsize = 0,
    });
}

static void finalize_sheader(std::shared_ptr<Elf64_Shdr> sheader, Elf64_Off offset) {
    sheader->sh_offset = offset;
    assert((sheader->sh_offset % sheader->sh_addralign) == 0);
}

static std::shared_ptr<Elf64_Shdr> create_dummy_sheader_strtab(u32 name_index, u64 align) {
    return std::make_shared<Elf64_Shdr>(Elf64_Shdr{
        .sh_name = name_index,
        .sh_type = SHT_STRTAB,
        .sh_flags = 0,
        .sh_addr = 0,
        .sh_offset = DUMMY,
        .sh_size = DUMMY,
        .sh_link = 0,
        .sh_info = 0,
        .sh_addralign = align,
        .sh_entsize = 0,
    });
}

static std::shared_ptr<Elf64_Shdr> create_dummy_sheader_symtab(u32 name_index, Elf64_Xword align) {
    // TODO: linkとinfo
    return std::make_shared<Elf64_Shdr>(Elf64_Shdr{
        .sh_name = name_index,
        .sh_type = SHT_SYMTAB,
        .sh_flags = 0,
        .sh_addr = 0,
        .sh_offset = DUMMY,
        .sh_size = DUMMY,
        .sh_link = 3, // TODO: .strtabのsheader index
        .sh_info = 0, // TODO: 最後のローカルシンボルのindex + 1
        .sh_addralign = align,
        .sh_entsize = sizeof(Elf64_Sym),
    });
}

} // namespace Utils
} // namespace Myld
