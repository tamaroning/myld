#include "config.h"
#include "reader.h"
#include <fmt/core.h>
#include <memory>
#include <string>
#include "linker.h"

// ref: https://tyfkda.github.io/blog/2020/04/20/elf-obj.html

const char *kOutputFileName = "myld-a.out";

int main(int argc, char *argv[]) {
    fmt::print("myld version {}\n", MYLD_VERSION);

    if (argc < 2) {
        fmt::print("Usage: myld [options] <filename>\n");
        return 0;
    }

    std::string output_filename = kOutputFileName;
    std::string obj_filename = kOutputFileName;

    int arg_index = 1;
    while (true) {
        if (arg_index >= argc)
            break;

        if (std::string(argv[arg_index]) == "-o") {
            output_filename = argv[arg_index + 1];
            arg_index += 2;
            continue;
        }

        obj_filename = argv[arg_index];
        arg_index++;
    }
    fmt::print("input file {}\n", obj_filename);
    fmt::print("output file {}\n", output_filename);

    Myld::Elf::Reader reader(obj_filename);
    reader.dump();

    Myld::Linker linker = Myld::Linker(reader.get_elf());
    linker.link();
    linker.output(output_filename);

    fmt::print("generated {}\n", output_filename);

    return 0;
}
