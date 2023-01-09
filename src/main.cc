#include "config.h"
#include "gen-elf.h"
#include "parse-elf.h"
#include <fmt/core.h>
#include <memory>
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

    std::shared_ptr<Myld::Elf::Elf> obj;

    // Myld::Elf::output_exe();

    Myld::Elf::Writer writer("a.o", obj);
    writer.write_file();
    fmt::print("generated a.o\n");

    return 0;
}
