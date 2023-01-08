#include "config.h"
#include "elf.h"
#include <fstream>
#include <iostream>
#include <vector>
#include <format>

typedef uint8_t u8;

int main(int argc, char *argv[]) {
    std::cout << "myld version " << MYLD_VERSION << std::endl << std::endl;

    std::string elf_file_name = argv[1];
    std::ifstream elf_file(elf_file_name);

    if (!elf_file) {
        std::cout << "Couldn't open elf file" << std::endl;
        return 0;
    }
    
    std::vector<u8> elf_data(std::istreambuf_iterator<char>(elf_file), {});

    Elf64_Ehdr *elf_header;
    elf_header = (Elf64_Ehdr*) &(elf_data[0]);
    
    std::cout << "file name : " << elf_file_name << std::endl;
    std::cout << "type      : " << elf_header->e_type << std::endl;

    if(elf_header->e_type != ET_REL) {
        std::cout << std::format("Hello {}!\n", "world");
    }

    return 0;
}
