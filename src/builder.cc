#include "builder.h"
#include "myld.h"

namespace Myld {

void Context::build_and_output(std::string filename) {
    Myld::Build::Builder builder{};
    builder.build(*this);
    builder.output(filename);
};

} // namespace Myld
