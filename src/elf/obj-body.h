#include <vector>
#include "myld.h"

namespace Myld {
namespace Elf {

enum SectionType {
    StringTable
};

class ObjSection {
  private:
    u64 align;
};

class ObjBody {
  private:
    std::vector<ObjSection> sections;
};

} // namespace Elf
} // namespace Myld
