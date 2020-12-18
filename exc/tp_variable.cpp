//////////////////////////////////////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2020-12-17
//////////////////////////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "typer.h"

#define err(token, msg, ...) print_error("Variable", token, msg, __VA_ARGS__)

namespace exy {
namespace typ_pass {
AstNode* Variable::visit(Decl decl) {
    if (decl->name->kind == SyntaxKind::Identifier) {
        return visit(decl, (Name)decl->name);
    }
    err(decl->name, "expected an identifier or a comma-separated list of identifiers");
    return nullptr;
}

AstNode* Variable::visit(Decl decl, Name name) {
    if (decl->typeName) {
        err(name, "visit with typename");
        return nullptr;
    } if (auto rhs = tp.visitExpression(decl->value)) {
        return make(decl->modifiers, name, rhs);
    }
    return nullptr;
}

AstNode* Variable::make(Mods modifiers, Name name, AstNode *rhs) {
    auto scope = tp.currentScope();
    auto  kind = getKind(modifiers);
    auto   pos = tp.mkpos(name);
    tp.mods.validateVariableModifiers(modifiers);
    switch (kind) {
        case AstKind::Global: {
            if (auto global = tp.mk.global(pos, name->value, rhs->type)) {
                return tp.mk.name(pos, global);
            }
        } break;
        default: {
            err(name, "make: not implemented");
        } break;
    }
    return tp.throwAway(rhs);
}

AstKind Variable::getKind(Mods modifiers) {
    if (Modifiers::isStatic(modifiers)) {
        return AstKind::Global;
    } if (auto owner = tp.currentScope()->owner) {
        if (owner->isaModule()) {
            return AstKind::Global;
        }
    }
    return AstKind::Local;
}
} // namespace typ_pass
} // namespace exy