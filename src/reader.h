#include "myld.h"
#include "parse-elf.h"
#include <cassert>
#include <elf.h>
#include <fmt/core.h>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

namespace Myld::Elf {

class Reader {
  public:
    Reader(std::string filename) : filename(filename), elf(nullptr) {
        std::ifstream file(filename);
        if (!file) {
            fmt::print("Couldn't open elf file\n");
            std::exit(1);
        }
        auto raw_bytes = std::make_shared<std::vector<u8>>(std::vector<u8>(std::istreambuf_iterator<char>(file), {}));

        elf = std::make_shared<Parse::Elf>(Parse::Elf(raw_bytes));
    }

    std::string get_filename() { return filename; }

    std::shared_ptr<Parse::Elf> get_elf() { return elf; }

    void dump() {
        fmt::print("content of {}\n", filename);
        elf->dump();
    }

  private:
    const std::string filename;
    std::shared_ptr<Parse::Elf> elf;
};

} // namespace Myld::Elf
