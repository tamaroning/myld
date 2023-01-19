#include "config.h"
#include "linker.h"
#include "reader.h"
#include <fmt/core.h>
#include <memory>
#include <string>
#include <vector>

const char *kOutputFileName = "myld-a.out";

int main(int argc, char *argv[]) {
    std::string output_filename = kOutputFileName;
    std::vector<std::string> input_filenames({});

    int arg_index = 1;
    while (true) {
        if (arg_index >= argc)
            break;

        if (std::string(argv[arg_index]) == "-o") {
            output_filename = argv[arg_index + 1];
            arg_index += 2;
            continue;
        }

        if (std::string(argv[arg_index]) == "-T") {
            fmt::print("warning: ignored linker script\n");
            arg_index += 2;
            continue;
        }

        if (std::string(argv[arg_index]) == "-nostdlib") {
            fmt::print("warning: ignored -nostdlib\n");
            arg_index += 1;
            continue;
        }

        input_filenames.push_back(argv[arg_index]);
        arg_index++;
    }

    if (input_filenames.size() == 0) {
        fmt::print("myld (version {})\n", MYLD_VERSION);
        fmt::print("Usage: myld [options] <filename> ...\n");
        fmt::print("Options:\n");
        fmt::print("  -o filename\tSet output filename\n");
        std::exit(0);
    }

    fmt::print("input file: ");
    for (auto name : input_filenames) {
        fmt::print("{}, ", name);
    }
    fmt::print("\n");

    fmt::print("output file: {}\n", output_filename);

    std::vector<std::shared_ptr<Myld::Parse::Elf>> objs({});
    for (auto obj_filename : input_filenames) {
        Myld::Elf::Reader reader(obj_filename);
        reader.dump();
        objs.push_back(reader.get_elf());
    }

    Myld::Linker linker = Myld::Linker(objs);
    linker.link();

    fmt::print("generated {}\n", output_filename);

    return 0;
}
