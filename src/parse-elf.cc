#include "parse-elf.h"
#include "context.h"
#include "reader.h"

namespace Myld {

void Context::parse_objects() {
    for (auto file : this->config.get_input_filenames()) {
        Myld::Elf::Reader reader(file);
        // dump object file
        reader.get_elf()->dump();
        this->objs.push_back(reader.get_elf());
    }
}

} // namespace Myld
