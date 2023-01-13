#include "elf/parse.h"
#include "builder.h"
#include <elf.h>

namespace Myld {

using namespace Myld::Elf;

class Linker {
  public:
    Linker(Parsed::Elf obj) : obj(obj) {}

    void do_link() {
        // TODO:
        // objの情報を集めながらexecutableをビルドしていく
    }

  private:
    Parsed::Elf obj;

    Builder builder;
};

} // namespace Myld