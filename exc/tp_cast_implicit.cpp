//////////////////////////////////////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2020-12-22
//////////////////////////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "typer.h"

#define err(token, msg, ...) print_error("ImplicitCast", token, msg, __VA_ARGS__)

namespace exy {
namespace typ_pass {
AstNode* Cast::implicitCast(Loc loc, AstNode *node, const AstType &type) {
	if (!node) {
		return nullptr;
	} if (node->type == type) {
		return node;
	}
	err(loc, "%type ← %type", &type, &node->type);
	return tp.throwAway(node);
}
} // namespace typ_pass
} // namespace exy