#include "myld.h"
#include <vector>

namespace Myld {
namespace Elf {

// TODO:
// parseしたelfをリンカで扱いやすい形(ObjBody)にする
// そのため、不必要なelf/program/section headerは取り除く
// 一部headerの情報も保持しておいたほうがいいのかもしれない (entry pointとか?)

enum SectionType { StringTable };

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
