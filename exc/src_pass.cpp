//////////////////////////////////////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2020-12-08
//////////////////////////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "src_pass.h"

#include "source.h"

namespace exy {
namespace src_pass {
bool run() {
    comp.source = MemAlloc<SourceTree>();
    comp.source = new(comp.source) SourceTree{};
    return comp.source->build();
}
} // namespace src_pass
} // namespace exy