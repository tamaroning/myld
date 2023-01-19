#include "builder.h"
#include "myld.h"

namespace Myld {

void Context::build_and_output() {
    Myld::Build::Builder builder{};
    builder.build(*this);
    builder.output(this->config.get_output_filename());
};

} // namespace Myld
