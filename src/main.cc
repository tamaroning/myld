#include "config.h"
#include "gen-elf.h"
#include "parse-elf.h"
#include <fmt/core.h>
#include <string>

// ref: https://tyfkda.github.io/blog/2020/04/20/elf-obj.html

int main(int argc, char *argv[]) {
    fmt::print("myld version {}\n\n", MYLD_VERSION);

    if (argc != 2) {
        fmt::print("Usage: myld <FILE>\n");
        return 0;
    }

    std::string elf_file_name = argv[1];

    Myld::Elf::Reader reader(elf_file_name);
    reader.dump();

    Myld::Elf::output_exe();

    return 0;
}
