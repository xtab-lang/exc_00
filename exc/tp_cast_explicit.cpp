//////////////////////////////////////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2020-12-22
//////////////////////////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "typer.h"

#define err(token, msg, ...) print_error("ExplicitCast", token, msg, __VA_ARGS__)

namespace exy {
namespace typ_pass {
AstNode* Cast::explicitCast(Loc loc, AstNode *value, const AstType &dst) {
	if (!value) {
		return nullptr;
	}
	auto &src = value->type;

	if (src == dst) { // OK: explicit T → T
		return value;
	}

	if (tp.isa.Void(src)) { // OK: explicit Void → T
		return tp.mem.New<AstCast>(loc, value, dst);
	} 
	
	if (tp.isa.Void(dst)) { // OK: explicit T → Void
		return tp.mem.New<AstCast>(loc, value, dst);
	} 
	
	if (auto a = tp.isa.Builtin(src)) {
		if (auto b = tp.isa.Builtin(dst)) { // OK: explicit T → U where both T and U are builtins
			return tp.mem.New<AstCast>(loc, value, dst);
		}
	}
	err(loc, "%type ← %type", &dst, &src);
	return tp.throwAway(value);
}
} // namespace typ_pass
} // namespace exy