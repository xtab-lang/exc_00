#include "pch.h"
#include "typer.h"

#define illegal_prefix_operation() do { \
    diagnostic("UnaryPrefixOperation", syntax, "illegal operation: %tk %tptype", \
        op, &val->type); \
    return tp.throwAway(val); \
} while (0)

namespace exy {
tp_unary_prefix::tp_unary_prefix(UnaryPrefixSyntax *syntax) : tp(*typer), syntax(syntax) {}

void tp_unary_prefix::dispose() {}

TpNode* tp_unary_prefix::plusOrMinus() {
    auto  op = syntax->pos.kind;
    auto val = tp.bindExpression(syntax->expression);
    if (val == nullptr) {
        return nullptr;
    }
    // T& → T
    val = tp.mk.DereferenceIfReference(syntax->expression, val);
    // Find and call fn UNARY_OP(this: T) → U
    tp_site site{ syntax };
    auto result = site.tryCallOperator(op, val);
    site.dispose();
    if (result != nullptr) {
        return result;
    }
    // If no user-defined function exists, we are falling back to the compiler.
    // We only know how to do UNARY_OP for Numbers.
    if (val->type.isNumeric()) {
        return tp.mk.UnaryPrefix(syntax, op, val);
    }
    illegal_prefix_operation();
}

TpNode* tp_unary_prefix::incrementOrDecrement() {
    auto  op = syntax->pos.kind;
    auto val = tp.bindExpression(syntax->expression);
    if (val == nullptr) {
        return nullptr;
    }
    // T& → T
    val = tp.mk.DereferenceIfReference(syntax->expression, val);
    // --T* or ++T* 
    if (val->type.isaPointer()) {
        return tp.mk.PointerUnaryPrefix(syntax, op, val);
    }
    // Find and call fn UNARY_PREFIX_OP(this: T) → U
    tp_site site{ syntax };
    auto result = site.tryCallOperator(op, val);
    site.dispose();
    if (result != nullptr) {
        return result;
    }
    // If no user-defined function exists, we are falling back to the compiler.
    // We only know how to do UNARY_PREFIX_OP for Numbers.
    if (val->type.isNumeric()) {
        return tp.mk.UnaryPrefix(syntax, op, val);
    }
    illegal_prefix_operation();
}

TpNode* tp_unary_prefix::dereference() {
    auto val = tp.bindExpression(syntax->expression);
    if (val == nullptr) {
        return nullptr;
    }
    return tp.mk.Dereference(syntax, val);
}

TpNode* tp_unary_prefix::addressOf() {
    auto val = tp.bindExpression(syntax->expression);
    if (val == nullptr) {
        return nullptr;
    }
    return tp.mk.AddressOf(syntax, val);
}

TpNode* tp_unary_prefix::logicalNot() {
    auto val = tp.bindExpression(syntax->expression);
    if (val == nullptr) {
        return nullptr;
    }
    val = tp.mk.DereferenceIfReference(syntax->expression, val);
    return tp.mk.Condition(syntax, val, Tok::Equal, tp.mk.ZeroOf(syntax->expression, val->type));
}

TpNode* tp_unary_prefix::bitwiseNot() {
    auto  op = syntax->pos.kind;
    auto val = tp.bindExpression(syntax->expression);
    if (val == nullptr) {
        return nullptr;
    }
    // T& → T
    val = tp.mk.DereferenceIfReference(syntax->expression, val);
    // Find and call fn ~(this: T) → V
    tp_site site{ syntax };
    auto result = site.tryCallOperator(op, val);
    site.dispose();
    if (result != nullptr) {
        return result;
    }
    // If no user-defined function exists, we are falling back to the compiler.
    // We only know how to ~Integrals and ~PackedIntegrals.
    if (val->type.isNotIntegralOrPackedIntegrals() || val->type.isNotIntegral()) {
        illegal_prefix_operation();
    }
    return tp.mk.UnaryPrefix(syntax, op, val);
}

TpNode* tp_unary_prefix::kwSizeOf() {
    if (auto value = tp.bindExpression(syntax->expression)) {
        return tp.mk.SizeOf(syntax, value);
    }
    return nullptr;
}

TpNode* tp_unary_prefix::kwTypeOf() {
    if (auto value = tp.bindExpression(syntax->expression)) {
        return tp.mk.TypeOf(syntax, value);
    }
    return nullptr;
}

TpNode* tp_unary_prefix::kwDelete() {
    TpNode *result = nullptr;
    if (auto value = tp.bindExpression(syntax->expression)) {
        if (auto ptr = value->type.isIndirect()) {
            if (ptr->pointee.isaStruct()) {
                if (syntax->with != nullptr) { // 'delete' T*/& with lambda ... where T is a struct
                    Assert(0);
                } else { // 'delete' T*/& where T is a struct
                    Assert(0);
                }
            } else { 
                //  'delete' t: T*/&
                //  Translation 🡓
                //      t.dispose()         // if T.dispose exists
                //      t = mem.free( t )   // if T is T*
                // else t = null            // if T is T&
                result = tp.mk.Delete(syntax, value);
            }
        } else if (value->type.isaStruct()) {
            if (syntax->with != nullptr) {
                //  'delete' t: T
                //  Translation 🡓
                //      t.dispose() with lambda
                if (auto with = tp.bindExpression(syntax->with)) {
                    tp_site site{ syntax };
                    result = site.callDispose(value, with);
                    site.dispose();
                }
            } else {
                //  'delete' t: T
                //  Translation 🡓
                //      t.dispose()
                tp_site site{ syntax };
                result = site.callDispose(value, /* with = */ nullptr);
                site.dispose();
            }
        } else if (syntax->with != nullptr) {
            //  'delete' t: T
            //  Translation 🡓
            //      t = null
            result = tp.mk.Assignment(syntax, value, tp.mk.ZeroOf(syntax, value->type));
        } else {
            Assert(0);
        }
    }
    return result;
}

TpNode* tp_unary_prefix::kwAtomic() {
    if (auto value = tp.bindExpression(syntax->expression)) {
        return tp.mk.Atomic(syntax, value);
    }
    return nullptr;
}

TpNode* tp_unary_prefix::kwAwait() {
    if (auto value = tp.bindExpression(syntax->expression)) {
        return tp.mk.Await(syntax, value);
    }
    return nullptr;
}

//----------------------------------------------------------
tp_unary_suffix::tp_unary_suffix(UnarySuffixSyntax *syntax) : tp(*typer), syntax(syntax) {}

void tp_unary_suffix::dispose() {}

TpNode* tp_unary_suffix::pointerType() {
    if (auto value = tp.bindExpression(syntax->expression)) {
        if (auto tpname = tp.isa.TypeName(value)) {
            tpname->type = tpname->type.mkPointer();
            return tpname;
        }
        type_error(syntax->expression, "expected a typename, not a value of %tptype", &value->type);
        return tp.throwAway(value);
    }
    return nullptr;
}

TpNode* tp_unary_suffix::referenceType() {
    if (auto value = tp.bindExpression(syntax->expression)) {
        if (auto tpname = tp.isa.TypeName(value)) {
            tpname->type = tpname->type.mkReference();
            return tpname;
        }
        type_error(syntax->expression, "expected a typename, not a value of %tptype", &value->type);
        return tp.throwAway(value);
    }
    return nullptr;
}

TpNode* tp_unary_suffix::incrementOrDecrement() {
    auto  op = syntax->pos.kind;
    auto val = tp.bindExpression(syntax->expression);
    if (val == nullptr) {
        return nullptr;
    }
    // T& → T
    val = tp.mk.DereferenceIfReference(syntax->expression, val);
    // T*-- or T*++
    if (val->type.isaPointer()) {
        return tp.mk.PointerUnarySuffix(syntax, val, op);
    }
    // Find and call fn UNARY_SUFFIX_OP(this: T) → U
    tp_site site{ syntax };
    auto result = site.tryCallOperator(op, val);
    site.dispose();
    if (result != nullptr) {
        return result;
    }
    // If no user-defined function exists, we are falling back to the compiler.
    // We only know how to do UNARY_SUFFIX_OP for Numbers.
    if (val->type.isNumeric()) {
        return tp.mk.UnarySuffix(syntax, val, op);
    }
    illegal_prefix_operation();
}
} // namespace exy