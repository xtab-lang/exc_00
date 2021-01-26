//////////////////////////////////////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2020-12-22
//////////////////////////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "typer.h"

#include "tp_cast.h"

#define err(token, msg, ...) print_error("ImplicitCast", token, msg, __VA_ARGS__)

namespace exy {
namespace stx2ast_pass {
AstNode* Cast::implicitCast(Loc loc, AstNode *node, const AstType &type) {
	if (!node) {
		return nullptr;
	} if (node->type == type) {
		return node;
	}
	err(loc, "%type ← %type", &type, &node->type);
	return tp.throwAway(node);
}
} // namespace stx2ast_pass
} // namespace exy