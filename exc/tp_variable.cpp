//////////////////////////////////////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2020-12-17
//////////////////////////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "typer.h"

#include "tp_variable.h"

#define err(token, msg, ...) print_error("Variable", token, msg, __VA_ARGS__)

namespace exy {
namespace stx2ast_pass {
Variable::Node Variable::visit(Decl decl) {
    tp.mods.validateVariableModifiers(decl->modifiers);
    if (decl->name->kind == SyntaxKind::Identifier) {
        return visit(decl, (Name)decl->name);
    } else if (decl->name->kind == SyntaxKind::CommaList) {
        return visit(decl, (Tuple)decl->name);
    }
    err(decl->name, "expected an identifier or a comma-separated list of identifiers");
    return nullptr;
}

Variable::Node Variable::visit(Decl decl, Name name) {
    auto pos = tp.mkpos(decl);
    if (decl->typeName) {
        if (auto node = tp.visitExpression(decl->typeName)) {
            if (auto tpname = tp.isa.TypeName(node)) {
                if (auto value = decl->value) {
                    if (auto rhs = tp.visitExpression(value)) {
                        auto res = make(pos, decl->modifiers, name, tpname->type, rhs);
                        tp.throwAway(tpname);
                        return res;
                    }
                    return tp.throwAway(tpname);
                }
                auto res = make(pos, decl->modifiers, name, tpname->type);
                tp.throwAway(tpname);
                return res;
            } 
            err(decl->typeName, "expected a typename");
            return tp.throwAway(node);
        }
        return nullptr;
    } if (auto rhs = tp.visitExpression(decl->value)) {
        return make(pos, decl->modifiers, name, rhs);
    }
    Unreachable();
}

Variable::Node Variable::visit(Decl decl, Tuple tuple) {
    if (decl->typeName) {
        err(tuple, "visit with typename");
        return nullptr;
    } if (auto rhs = tp.visitExpression(decl->value)) {
        return make(tp.mkpos(decl), decl->modifiers, tuple, rhs);
    }
    Unreachable();
}

Variable::Node Variable::make(Loc loc, Mods modifiers, Name name, Node rhs) {
    auto kind = getKind(modifiers);
    switch (kind) {
        case AstKind::Global: {
            if (auto  global = tp.mk.global(tp.mkpos(name), name->value, rhs->type)) {
                return tp.mk.definition(loc, global, rhs);
            }
        } break;
        default: {
            err(name, "make: not implemented");
        } break;
    }
    return tp.throwAway(rhs);
}

Variable::Node Variable::make(Loc loc, Mods modifiers, Name name, Type type) {
    auto kind = getKind(modifiers);
    switch (kind) {
        case AstKind::Global: {
            if (auto global = tp.mk.global(tp.mkpos(name), name->value, type)) {
                return tp.mk.name(loc, global);
            }
        } return nullptr;
        default: {
            err(name, "make: not implemented");
        } break;
    }
    return nullptr;
}

Variable::Node Variable::make(Loc loc, Mods modifiers, Name name, Type type, Node rhs) {
    auto kind = getKind(modifiers);
    switch (kind) {
        case AstKind::Global: {
            if (auto  global = tp.mk.global(tp.mkpos(name), name->value, type)) {
                return tp.mk.definition(loc, global, rhs);
            }
        } break;
        default: {
            err(name, "make: not implemented");
        } break;
    }
    return tp.throwAway(rhs);
}

Variable::Node Variable::make(Loc loc, Mods modifiers, Tuple tuple, Node rhs) {
    auto  kind = getKind(modifiers);
    switch (kind) {
        case AstKind::Global: {
            auto errors = 0;
            for (auto i = 0; i < tuple->nodes.length; ++i) {
                auto name = (Name)tuple->nodes.items[i];
                if (auto global = tp.mk.global(tp.mkpos(name), name->value, rhs->type)) {
                    return tp.mk.definition(loc, global, rhs);
                }
                ++errors;
            } if (!errors) {
                return nullptr;
            }
        } break;
        default: {
            err(tuple, "not implemented");
        } break;
    }
    return tp.throwAway(rhs);
}

AstKind Variable::getKind(Mods modifiers) {
    if (Modifiers::isStatic(modifiers)) {
        return AstKind::Global;
    } if (auto owner = tp.currentScope()->owner) {
        if (tp.isa.Module(owner)) {
            return AstKind::Global;
        }
    }
    Assert(0);
    return AstKind::Global;
}
} // namespace stx2ast_pass
} // namespace exy