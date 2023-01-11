#include "parsed.h"
#include "myld.h"
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
        std::vector<u8> raw = std::vector<u8>(std::istreambuf_iterator<char>(file), {});
        elf = std::make_shared<Parsed::Elf>(Parsed::Elf(raw));
    }

    std::string get_filename() { return filename; }

    std::shared_ptr<Parsed::Elf> get_elf() { return elf; }

    void dump() {
        fmt::print("content of {}\n", filename);
        elf->dump();
    }

  private:
    const std::string filename;
    std::shared_ptr<Parsed::Elf> elf;
};

} // namespace Myld::Elf
