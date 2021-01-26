//////////////////////////////////////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2020-12-22
//////////////////////////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "typer.h"

#include "tp_cast.h"

#define err(token, msg, ...) print_error("ExplicitCast", token, msg, __VA_ARGS__)

namespace exy {
namespace stx2ast_pass {
AstNode* Cast::explicitCast(Loc loc, AstNode *value, const AstType &dst) {
	if (!value) {
		return nullptr;
	}
	auto &src = value->type;

	if (src == dst) { // OK: explicit S → D where S == D
		return value;
	}

	if (tp.isa.Void(src)) { // OK: explicit Void → D
		return tp.mem.New<AstCast>(loc, value, dst);
	} 
	
	if (tp.isa.Void(dst)) { // OK: explicit S → Void
		return tp.mem.New<AstCast>(loc, value, dst);
	} 
	
	if (auto srcbuiltin = tp.isa.Builtin(src)) {
		if (auto dstbuiltin = tp.isa.Builtin(dst)) {
			// OK: explicit S → D where both S and D are builtins
			return tp.mem.New<AstCast>(loc, value, dst);
		}
		if (auto dstptr = tp.isa.PointerType(dst)) {
			// OK: explicit S → D* where S isa builtin
			return tp.mem.New<AstCast>(loc, value, dst);
		}
	}

	else

	if (auto dstbuiltin = tp.isa.Builtin(dst)) {
		if (auto srcptr = tp.isa.PointerType(src)) {
			// OK: explicit S* → D where D isa builtin
			return tp.mem.New<AstCast>(loc, value, dst);
		}
	}

	err(loc, "%type ← %type", &dst, &src);

	return tp.throwAway(value);
}
} // namespace stx2ast_pass
} // namespace exy