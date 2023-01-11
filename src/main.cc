#include "config.h"
#include "elf/reader.h"
#include "elf/writer.h"
#include <fmt/core.h>
#include <memory>
#include <string>

// ref: https://tyfkda.github.io/blog/2020/04/20/elf-obj.html

const char *kExeFileName = "a.o";

int main(int argc, char *argv[]) {
    fmt::print("myld version {}\n\n", MYLD_VERSION);

    if (argc != 2) {
        fmt::print("Usage: myld <FILE>\n");
        return 0;
    }

    std::string elf_file_name = argv[1];

    Myld::Elf::Reader reader(elf_file_name);
    reader.dump();

    std::shared_ptr<Myld::Elf::Parsed::Elf> obj(reader.get_elf());

    Myld::Elf::Writer writer(kExeFileName, obj);
    writer.write_file();
    fmt::print("generated {}\n", kExeFileName);

    return 0;
}
