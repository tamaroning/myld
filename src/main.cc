#include "config.h"
#include "elf.h"
#include <fmt/core.h>
#include <fstream>
#include <iostream>
#include <vector>

typedef uint8_t u8;

int main(int argc, char *argv[]) {
    fmt::print("myld version {}\n\n", MYLD_VERSION);

    if (argc != 2) {
        fmt::print("Usage: myld <FILE>\n");
        return 0;
    }

    std::string elf_file_name = argv[1];
    std::ifstream elf_file(elf_file_name);

    if (!elf_file) {
        fmt::print("Couldn't open elf file\n");
        return 0;
    }

    std::vector<u8> elf_data(std::istreambuf_iterator<char>(elf_file), {});

    Elf64_Ehdr *elf_header;
    elf_header = (Elf64_Ehdr *)&(elf_data[0]);

    fmt::print("file name : {}\n", elf_file_name);
    fmt::print("type      : {}\n", elf_header->e_type);

    if (elf_header->e_type != ET_REL) {
        fmt::print("Hello {}!\n", "world");
    }

    return 0;
}
