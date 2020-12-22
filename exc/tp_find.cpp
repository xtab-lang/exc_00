//////////////////////////////////////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2020-12-20
//////////////////////////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "typer.h"

#define err(token, msg, ...) print_error("Name", token, msg, __VA_ARGS__)

namespace exy {
namespace typ_pass {
AstNode* Find::name(SyntaxIdentifier *syntax) {
    return name(tp.mkpos(syntax), syntax->value);
}

AstNode* Find::name(Loc loc, Identifier id) {
    AstSymbol *found{};
    auto scope = tp.currentScope();
    while (scope) {
        if (found = scope->find(id)) {
            break;
        }
        scope = scope->parent;
    } if (!found) {
        found = tp.tree.find(id);
    } if (found) {
        return tp.mk.name(loc, found);
    }
    err(loc, "identifier %s#<red> not found", id);
    return nullptr;
}
} // namespace typ_pass
} // namespace exy
