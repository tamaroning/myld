#include "myld.h"
#include <elf.h>
#include <vector>
#include <memory>

namespace Myld {
namespace Build {
class Section {
  public:
  private:
    std::unique_ptr<Elf64_Shdr> sheader;
    std::vector<u8> raw;
};

class Builder {
  public:
  private:
    std::unique_ptr<Elf64_Ehdr> eheader;
    std::unique_ptr<Elf64_Phdr> pheader;

    std::vector<Section> sections;
};

} // namespace Build
} // namespace Myld