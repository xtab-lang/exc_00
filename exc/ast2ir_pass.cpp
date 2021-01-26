//////////////////////////////////////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2020-12-23
//////////////////////////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "ast2ir_pass.h"

#include "ast2ir.h"

namespace exy {
namespace ast2ir_pass {
bool run() {
    MemNew<IrTree>(); // Sets comp.ir
    translateAstTree();
    comp.ast = MemDispose(comp.ast);
    return comp.errors == 0;
}
} // namespace irg_pass
} // namespace exy