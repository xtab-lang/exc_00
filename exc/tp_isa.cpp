//////////////////////////////////////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2020-12-19
//////////////////////////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "typer.h"

namespace exy {
namespace stx2ast_pass {
AstPointerType* Isa::PointerType(const AstType &type) {
	return type.isaPointer();
}

AstReferenceType* Isa::ReferenceType(const AstType &type) {
	return type.isaReference();
}
} // namespace stx2ast_pass
} // namespace exy