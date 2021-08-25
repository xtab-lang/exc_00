#include "pch.h"
#include "typer.h"

namespace exy {
struct var {
    static void create(SyntaxNode *pos, SyntaxNode *modifiers, IdentifierSyntax *id, TpNode *rhs,
                       TpKind kind) {
        auto     &tp = *typer;
        auto    name = id->value;
        if (auto dup = tp.current->scope->contains(name)) {
            dup_error(id, dup);
            tp.throwAway(rhs);
            return;
        }
        TpSymbol *symbol = nullptr;
        switch (kind) {
            case TpKind::Global: symbol = tp.mk.Global(pos, name, rhs); break;
            case TpKind::Field:  symbol = tp.mk.Field(pos, name, rhs); break;
            case TpKind::Local:  symbol = tp.mk.Local(pos, name, rhs->type); break;
            default: UNREACHABLE();
        }
        tp.applyBlockModifiers(symbol);
        tp.applyModifiers(modifiers, symbol);
        if (kind == TpKind::Local) {
            // lhs ← rhs
            auto def = tp.mk.Definition(pos, symbol, rhs);
            tp.current->scope->append(def);
        }
    }

    static void create(SyntaxNode *pos, SyntaxNode *modifiers, IdentifierSyntax *id,
                       TpTypeName *tpname, TpKind kind) {
        auto     &tp = *typer;
        auto    name = id->value;
        if (auto dup = tp.current->scope->contains(name)) {
            dup_error(id, dup);
            tp.throwAway(tpname);
            return;
        }
        TpSymbol *symbol = nullptr;
        switch (kind) {
            case TpKind::Global: symbol = tp.mk.Global(pos, name, tpname->type); break;
            case TpKind::Field: symbol = tp.mk.Field(pos, name, tpname->type); break;
            case TpKind::Local: symbol = tp.mk.Local(pos, name, tpname->type); break;
            default: UNREACHABLE();
        }
        tp.applyBlockModifiers(symbol);
        tp.applyModifiers(modifiers, symbol);
        tp.throwAway(tpname);
    }
};

//----------------------------------------------------------
tp_var::tp_var(VariableSyntax *syntax) : tp(*typer), syntax(syntax) {}

void tp_var::dispose() {
}

void tp_var::bindGlobal() {
    bind(TpKind::Global);
}

void tp_var::bindField() {
    bind(TpKind::Field);
}

void tp_var::bindLocal() {
    bind(TpKind::Local);
}

void tp_var::bind(TpKind kind) {
    if (syntax->rhs == nullptr) {
        if (auto nv = tp.isa.NameValueSyntax(syntax->name)) {
            // variable ⟶ modifiers name ':' typename 
            Assert(nv->op.kind == Tok::Colon);
            auto id = nv->name;
            if (auto res = tp.bindExpression(nv->value)) {
                if (auto tpname = tp.isa.TypeName(res)) {
                    return var::create(syntax, syntax->modifiers, id, tpname, kind);
                }
                type_error(nv->value, "expected a typename, not %tpk: %tptype", res->kind,
                           &res->type);
                tp.throwAway(res);
            }
            return;
        }
        syntax_error(syntax, "expected a typename");
        return;
    }
    auto rhs = tp.bindExpression(syntax->rhs);
    if (rhs == nullptr) {
        return;
    }
    if (auto id = tp.isa.IdentifierSyntax(syntax->name)) {
        // variable ⟶ modifiers name '=' expression
        return var::create(syntax, syntax->modifiers, id, rhs, kind);
    }
    if (auto nv = tp.isa.NameValueSyntax(syntax->name)) {
        // variable ⟶ modifiers name ':' typename '=' expression
        Assert(nv->op.kind == Tok::Colon);
        auto id = nv->name;
        if (auto res = tp.bindExpression(nv->value)) {
            if (auto tpname = tp.isa.TypeName(res)) {
                if (rhs = tp.cast(syntax->rhs, rhs, tpname->type, tp_cast_reason::ExplicitCast)) {
                    return var::create(syntax, syntax->modifiers, id, rhs, kind);
                }
                tp.throwAway(tpname);
                return;
            }
            type_error(nv->value, "expected a typename, not %tpk: %tptype", res->kind,
                       &res->type);
            tp.throwAway(rhs, res);
            return;
        }
        tp.throwAway(rhs);
        return;
    }
    if (auto parens = tp.isa.ParenthesizedSyntax(syntax->name)) {
        if (auto id = tp.isa.IdentifierSyntax(parens->value)) {
            // global ⟶ modifiers '(' name ')' '=' expression
            return var::create(id, syntax->modifiers, id, rhs, kind);
        }
        if (auto nv = tp.isa.NameValueSyntax(parens->value)) {
            // global ⟶ modifiers '(' name ':' typename ')' '=' expression
            Assert(nv->op.kind == Tok::Colon);
            auto id = nv->name;
            if (auto res = tp.bindExpression(nv->value)) {
                if (auto tpname = tp.isa.TypeName(res)) {
                    if (rhs = tp.cast(syntax->rhs, rhs, tpname->type, tp_cast_reason::ExplicitCast)) {
                        return var::create(syntax, syntax->modifiers, id, rhs, kind);
                    }
                    tp.throwAway(tpname);
                    return;
                }
                type_error(nv->value, "expected a typename, not %tpk: %tptype", res->kind,
                           &res->type);
                tp.throwAway(rhs, res);
                return;
            }
            tp.throwAway(rhs);
            return;
        }
        if (auto list = tp.isa.CommaSeparatedSyntax(parens->value)) {
            // variable ⟶ modifiers '(' (name [':' typename])+ ')' '=' expression
            Assert(0);
            const auto errors = compiler.errors;
            for (auto i = 0; i < list->nodes.length; i++) {
                auto node = list->nodes.items[i];
                if (auto id = tp.isa.IdentifierSyntax(node)) {
                    var::create(node, syntax->modifiers, id, rhs, kind);
                } else if (auto nv = tp.isa.NameValueSyntax(node)) {
                    Assert(0);
                } else {
                    syntax_error(node, "expected identifier or name-value, not %sk", node->kind);
                }
            }
            if (errors == compiler.errors) {
                return;
            }
            tp.throwAway(rhs);
            return;
        }
    }
    syntax_error(syntax, "invalid %sk declaration", syntax->kind);
}

//----------------------------------------------------------
void Typer::bindGlobal(VariableSyntax *syntax) {
    tp_var var{ syntax };
    var.bindGlobal();
    var.dispose();
}

void Typer::bindField(VariableSyntax *syntax) {
    tp_var var{ syntax };
    var.bindField();
    var.dispose();
}

void Typer::bindLocal(VariableSyntax *syntax) {
    tp_var var{ syntax };
    var.bindLocal();
    var.dispose();
}
} // namespace exy