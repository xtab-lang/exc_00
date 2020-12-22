//////////////////////////////////////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2020-12-19
//////////////////////////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "typer.h"

#define err(token, msg, ...) print_error("Make", token, msg, __VA_ARGS__)

namespace exy {
namespace typ_pass {
AstAlias* Isa::alias(AstNode *node) {
	switch (node->kind) {
		case AstKind::TypeAlias:
		case AstKind::ValueAlias:
		case AstKind::ConstAlias:
			return (AstAlias*)node;
	}
	return nullptr;
}
} // namespace typ_pass
} // namespace exy