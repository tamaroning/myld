#include <elf.h>

namespace Myld::Elf {
using namespace Myld::Elf;

Elf64_Ehdr *create_header(Elf64_Half program_header_entry_num, Elf64_Half section_num, Elf64_Off section_header_start);

Elf64_Phdr *create_program_header_entry_load();

Elf64_Shdr *create_section_header_entry_null(Elf64_Word section_name_index);

Elf64_Shdr *create_section_header_entry_strtab(Elf64_Word section_name_index, Elf64_Xword section_size,
                                               Elf64_Xword offset);

void output_exe();

} // namespace Myld::Elf
