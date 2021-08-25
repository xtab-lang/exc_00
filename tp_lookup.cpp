#include "pch.h"
#include "typer.h"

#define member_find_error(pos, name, type) diagnostic("Lookup", pos, "identifier %s#<red> not found in %tptype", name, &type)

#define find_error(pos, name) diagnostic("Lookup", pos, "identifier %s#<red> not found", name)
#define access_error(pos, name) diagnostic("Lookup", pos, "identifier %s#<red> found but cannot be accessed from the current scope", name)

namespace exy {
Identifier tp_lookup::kw__MODULE__   = nullptr;
Identifier tp_lookup::kw__FOLDER__   = nullptr;
Identifier tp_lookup::kw__FILE__     = nullptr;
Identifier tp_lookup::kw__FUNCTION__ = nullptr;
Identifier tp_lookup::kw__LINE__     = nullptr;
Identifier tp_lookup::kw__COL__      = nullptr;

tp_lookup::tp_lookup() 
    : tp(*typer) {}

void tp_lookup::dispose() {
}

static TpSymbol *extractModule(SyntaxNode *pos, TpNode *found) {
    auto      &tp = *typer;
    TpSymbol *mod = nullptr;
    if (auto tpname = tp.isa.TypeName(found)) {
        mod = tpname->type.isaModule();
        if (mod == nullptr) {
            type_error(pos, "expected a module typename, not %tpk", found->kind);
        }
    } else {
        type_error(pos, "expected a module typename, not %tpk", found->kind);
    }
    tp.throwAway(found);
    return mod;
}

static TpSymbol *extractStruct(SyntaxNode *pos, TpNode *found) {
    auto     &tp = *typer;
    TpSymbol *st = nullptr;
    if (auto tpname = tp.isa.TypeName(found)) {
        st = tpname->type.isaStruct();
        if (st == nullptr) {
            type_error(pos, "expected a struct typename, not %tpk", found->kind);
        }
    } else {
        type_error(pos, "expected a struct typename, not %tpk", found->kind);
    }
    tp.throwAway(found);
    return st;
}

static TpSymbol *extractFunction(SyntaxNode *pos, TpNode *found) {
    auto     &tp = *typer;
    TpSymbol *fn = nullptr;
    if (auto tpname = tp.isa.TypeName(found)) {
        fn = tpname->type.isaFunction();
        if (fn == nullptr) {
            type_error(pos, "expected a template function typename, not %tpk", found->kind);
        }
    } else {
        type_error(pos, "expected a template function typename, not %tpk", found->kind);
    }
    tp.throwAway(found);
    return fn;
}

bool tp_lookup::initialize(Pos pos) {
    kw__MODULE__ = ids.get(S("__MODULE__"));
    kw__FOLDER__ = ids.get(S("__FOLDER__"));
    kw__FILE__ = ids.get(S("__FILE__"));
    kw__FUNCTION__ = ids.get(S("__FUNCTION__"));
    kw__LINE__ = ids.get(S("__LINE__"));
    kw__COL__ = ids.get(S("__COL__"));

    const auto errors = compiler.errors;
    if (auto found = find(pos, ids.kw_std)) {
        if (auto mod = extractModule(pos, found)) {
            tp.mod_std = mod;
            auto scope = ((TpModule*)mod->node)->scope;
            if (found  = find(pos, scope, ids.kw_string)) {
                tp.sym_std_string = extractStruct(pos, found);
            }
            if (found = find(pos, scope, ids.kw_String)) {
                tp.sym_std_String = extractStruct(pos, found);
            }
            if (found = find(pos, scope, ids.kw_wstring)) {
                tp.sym_std_wstring = extractStruct(pos, found);
            }
            if (found = find(pos, scope, ids.kw_WString)) {
                tp.sym_std_WString = extractStruct(pos, found);
            }
            if (found = find(pos, scope, ids.kw_collections)) {
                tp.mod_collections = extractModule(pos, found);
            }
            if (found = find(pos, scope, ids.kw_startup)) {
                tp.sym_std_startup = extractFunction(pos, found);
            }
            if (found = find(pos, scope, ids.kw_shutdown)) {
                tp.sym_std_shutdown = extractFunction(pos, found);
            }
        }
    }
    if (auto    found = find(pos, ids.kw_aio)) {
        if (auto mod = extractModule(pos, found)) {
            tp.mod_aio = mod;
            auto scope = ((TpModule*)mod->node)->scope;
            if (found = find(pos, scope, ids.kw_OVERLAPPED)) {
                tp.sym_aio_OVERLAPPED = extractStruct(pos, found);
            }
            if (found = find(pos, scope, ids.kw_SRWLOCK)) {
                tp.sym_aio_SRWLOCK = extractStruct(pos, found);
            }
            if (found = find(pos, scope, ids.kw_startup)) {
                tp.sym_aio_startup = extractFunction(pos, found);
            }
            if (found = find(pos, scope, ids.kw_shutdown)) {
                tp.sym_aio_shutdown = extractFunction(pos, found);
            }
        }
    }
    return errors == compiler.errors;
}

TpScope* tp_lookup::searchableScopeOf(Type type) {
    if (auto ptr = type.isIndirect()) {
        if (auto symbol = ptr->pointee.isaStructOrUnionOrEnum()) {
            auto   node = (TpTypeNode*)symbol->node;
            return node->scope;
        }
    } else if (auto symbol = type.isaStructOrUnionOrEnum()) {
        auto node = (TpTypeNode*)symbol->node;
        return node->scope;
    }
    return nullptr;
}

 
TpNode* tp_lookup::find(Pos pos, Identifier name) {
#define not_found()       do { find_error(pos, name); capture.dispose(); return nullptr; } while (0)
#define find_fail()       do { access_error(pos, name); capture.dispose(); return nullptr; } while (0)
#define find_succeed()    do { capture.dispose(); return tp.mk.Name(pos, found); } while (0)
#define propagate_found() do { auto res = stack.propagate(pos, found); capture.dispose(); return res; } while (0)

    if (name == kw__FUNCTION__) {
        if (auto fn = tp.current->function()) {
            return tp.mk.Literal(pos, tp.sym_std_string->node->type, UINT64(fn->node->dotName));
        }
        find_error(pos, name);
        return nullptr;
    }
    if (name == kw__LINE__) {
        return tp.mk.Literal(pos, tp.tree.tyInt32, pos->pos.pos.range.start.line);
    }
    if (name == kw__COL__) {
        return tp.mk.Literal(pos, tp.tree.tyInt32, pos->pos.pos.range.start.col);
    }

    CaptureList capture{};
    TpSymbol *found = nullptr;
    auto          p = tp.current->scope;
    while (p != nullptr) {
        if (found = p->contains(name)) {
            break;
        }
        p = p->parent;
    }
    if (found == nullptr) {
        not_found();
    }
    if (p->owner == nullptr) {
        find_succeed();
    }
    if (tp.isa.LocalOrParameter(found)) { // Found a local or parameter after crossing boundaries.
        auto stack = capture.make(found);
        if (capture.isaOneStackFrameCapture()) {
            find_succeed();
        }
        if (stack.crossesIllegalBoundaries()) {
            find_fail();
        }
        if (tp.isa.LambdaFunction(stack.bottom->scope->owner)) {
            propagate_found();
        }
        find_succeed();
    } 
    if (tp.isa.Field(found)) {
        auto stack = capture.make(found);
        if (stack.crossesIllegalBoundaries()) {
            find_fail();
        }
        if (tp.isa.LambdaFunction(stack.bottom->scope->owner)) {
            Assert(!capture.isaOneStackFrameCapture());
            propagate_found();
        }
        find_fail();
    }
    find_succeed();
}

TpNode* tp_lookup::find(Pos pos, TpScope *scope, Identifier name) {
    if (auto found = scope->contains(name)) {
        return tp.mk.Name(pos, found);
    }
    find_error(pos, name);
    return nullptr;
}

TpNode* tp_lookup::find(Pos pos, TpNode *base, Identifier name) {
    if (auto scope = searchableScopeOf(base->type)) {
        if (auto found = scope->contains(name)) {
            return tp.mk.Name(pos, base, found);
        }
    }
    member_find_error(pos, name, base->type);
    return tp.throwAway(base);
}

//----------------------------------------------------------
tp_lookup::CaptureStack::CaptureStack() : tp(*typer) {}

bool tp_lookup::CaptureStack::crossesIllegalBoundaries() {
    for (auto frame = bottom; frame != nullptr; frame = frame->prev) {
        auto  owner = frame->scope->owner;
        if (tp.isa.LambdaFunction(owner)) {
            // Do nothing.
        } else if (tp.isa.LambdaStruct(owner)) {
            // Do nothing.
        } else if (tp.isa.Function(owner)) {
            return frame->prev != nullptr;
        } else {
            return true;
        }
    }
    return false;
}

TpNode* tp_lookup::CaptureStack::propagate(Pos pos, TpSymbol *found) {
    if (tp.isa.LocalOrParameter(found)) {
        return propagateLocalOrParameter(pos, found);
    }
    return propagateField(pos, found);
}

TpNode* tp_lookup::CaptureStack::propagateLocalOrParameter(Pos pos, TpSymbol *found) {
    auto frame = top;
    auto  next = frame->next;
    // {frame} is either an ordinary function frame or a lambda function frame.
    if (tp.isa.LambdaStruct(next->scope->owner)) {
        // {next} is a lambda struct frame.
        // Setup a lambda capture field in {next} then propagate.
        auto name = found->name;
        if (auto dup = next->scope->contains(name)) {
            dup_error(pos, dup);
            return nullptr;
        }
        auto rhs = (TpNode*)tp.mk.Name(pos, found);
        if (tp.isa.CaptureByReference(rhs->type)) {
            rhs = tp.mk.ReferenceOf(pos, rhs);
        }
        auto field = tp.mk.CapturedField(pos, next->scope, name, rhs);
        return propagateFieldToLambdaFunction(pos, next->next, field);
    }
    UNREACHABLE();
}

TpNode* tp_lookup::CaptureStack::propagateField(Pos pos, TpSymbol *found) {
    auto frame = top;
    auto  next = frame->next;
    // {frame} is a lambda struct frame and {next} is a lambda function frame.
    if (tp.isa.LambdaStruct(frame->scope->owner)) {
        return propagateFieldToLambdaFunction(pos, next, found);
    }
    UNREACHABLE();
}

TpNode* tp_lookup::CaptureStack::propagateFieldToLambdaFunction(Pos pos, CaptureFrame *frame,
                                                                TpSymbol *field) {
    auto  lambdaFnNode = tp.isa.LambdaFunction(frame->scope->owner);
    auto thisParameter = lambdaFnNode->parameters.first();
    auto          next = frame->next;
    if (next == nullptr) {
        auto base = tp.mk.Name(pos, thisParameter);
        return tp.mk.FieldName(pos, base, field);
    }
    // {next} is a lambda struct frame.
    Assert(tp.isa.LambdaStruct(next->scope->owner));
    return propagateFieldToLambdaStruct(pos, next, thisParameter, field);
}

TpNode* tp_lookup::CaptureStack::propagateFieldToLambdaStruct(Pos pos, CaptureFrame *frame,
                                                              TpSymbol *thisParameter,
                                                              TpSymbol *field) {
    auto  name = field->name;
    auto scope = frame->scope;
    if (auto dup = scope->contains(name)) {
        dup_error(pos, dup);
        return nullptr;
    }
    auto  base = tp.mk.Name(pos, thisParameter);
    auto   rhs = tp.mk.FieldName(pos, base, field);
    field = tp.mk.CapturedField(pos, scope, name, rhs);
    return propagateFieldToLambdaFunction(pos, frame->next, field);
}

//----------------------------------------------------------
tp_lookup::CaptureList::CaptureList() : tp(*typer) {}

void tp_lookup::CaptureList::dispose() {
    list.dispose();
}

static TpScope* typeOwnerScopeOf(TpScope *scope) {
    auto &tp = *typer;
    while (scope != nullptr) {
        if (!tp.isa.Label(scope->owner)) {
            return scope;
        }
        scope = scope->parent;
    }
    UNREACHABLE();
}

tp_lookup::CaptureStack tp_lookup::CaptureList::make(TpSymbol *found) {
    CaptureStack stack{};
    auto scope = tp.current->scope;
    while (scope != found->scope) {
        auto parent = typeOwnerScopeOf(scope);
        place(parent);
        scope = scope->parent;
    }
    Assert(scope != nullptr);
    place(scope);

    const auto    last = list.length - 1;
    CaptureFrame *prev = nullptr;
    for (auto i = list.length; --i >= 0; ) {
        auto capture = &list.items[i];
        if (i == 0) {
            stack.bottom = capture;
        } 
        if (i == last) {
            stack.top = capture;
        }
        if (prev != nullptr) {
            prev->next = capture;
            capture->prev = prev;
        }
        prev = capture;
    }
    return stack;
}

void tp_lookup::CaptureList::place(TpScope *scope) {
    auto parent = typeOwnerScopeOf(scope);
    if (list.isEmpty()) {
        list.place(parent);
    } else if (list.last().scope != parent) {
        list.place(parent);
    }
}

//----------------------------------------------------------
TpNode* Typer::bindIdentifier(IdentifierSyntax *syntax) {
    tp_lookup lookup{};
    auto found = lookup.find(syntax, syntax->value);
    lookup.dispose();
    return found;
}

TpNode* Typer::bindIdentifier(TpNode *base, IdentifierSyntax *syntax) {
    auto name = syntax->value;
    if (isa.TypeName(base)) {
        auto type = base->type.dereferenceIfReference();
        throwAway(base);
        if (auto symbol = type.isDirect()) {
            if (auto node = isa.TypeNode(symbol->node)) {
                if (auto scope = node->scope) {
                    tp_lookup lookup{};
                    auto found = lookup.find(syntax, scope, name);
                    lookup.dispose();
                    return found;
                }
            }
        }
        member_find_error(syntax, name, type);
    } else {
        tp_lookup lookup{};
        auto found = lookup.find(syntax, base, name);
        lookup.dispose();
        return found;
    }
    return throwAway(base);
}

} // namespace exy