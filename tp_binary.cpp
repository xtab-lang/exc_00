#include "pch.h"
#include "typer.h"

#define illegal_operation() do { \
    diagnostic("BinaryOperation", syntax, "illegal operation: %tptype %tk %tptype", \
        &lhs->type, op, &rhs->type); \
    return tp.throwAway(lhs, rhs); \
} while (0)

namespace exy {
tp_binary::tp_binary(BinarySyntax *syntax) : tp(*typer), syntax(syntax) {}

void tp_binary::dispose() {}

TpNode* tp_binary::equivalence() {
    impl_error(syntax, "%c", __FUNCTION__);
    return nullptr;
}

TpNode* tp_binary::relational() {
    auto  op = syntax->op.kind;
    auto rhs = tp.bindExpression(syntax->rhs);
    auto lhs = tp.bindExpression(syntax->lhs);
    if (rhs == nullptr || lhs == nullptr) {
        return tp.throwAway(rhs, lhs);
    }
    // Handle T& != null or T& == null
    if (op == Tok::NotEqual || op == Tok::Equal) {
        if (lhs->type.isaReference() && tp.isa.Null(rhs)) {
            if (rhs = tp.cast(syntax->rhs, rhs, lhs->type, tp_cast_reason::ImplicitCast)) {
                // Return T& RELATIONAL_OP null → Bool
                return tp.mk.Condition(syntax, lhs, op, rhs);
            }
            return tp.throwAway(lhs);
        }
        if (tp.isa.Null(lhs) && rhs->type.isaReference()) {
            if (lhs = tp.cast(syntax->lhs, lhs, rhs->type, tp_cast_reason::ImplicitCast)) {
                // Return T& RELATIONAL_OP null → Bool
                return tp.mk.Condition(syntax, rhs, op, lhs);
            }
            return tp.throwAway(rhs);
        }
    }
    // T& → T
    rhs = tp.mk.DereferenceIfReference(syntax->rhs, rhs);
    lhs = tp.mk.DereferenceIfReference(syntax->lhs, lhs);
    // Find and call fn RELATIONAL_OP(this: T, U) → V
    tp_site site{ syntax };
    auto result = site.tryCallOperator(lhs, op, rhs);
    site.dispose();
    if (result != nullptr) {
        return result;
    }
    // If no user-defined function exists, we are falling back to the compiler.
    // We only know how to do RELATIONAL_OP for Builtins, Packed and T*.
    // (1) Find a D such that both L and R can be implicitly cast to it.
    auto dst = tp.upperBound(lhs->type, rhs->type, op);
    if (dst.isUnknown()) {
        illegal_operation(); // Failed to find such a D.
    }
    // (2) D should be either a Builtin, Packed or T*.
    if (dst.isNotABuiltin() && dst.isNotAPointer() && dst.isNotPacked()) {
        illegal_operation();
    }
    // (3) Cast L and R to D implicitly.
    rhs = tp.cast(syntax->rhs, rhs, dst, tp_cast_reason::ImplicitCast);
    lhs = tp.cast(syntax->lhs, lhs, dst, tp_cast_reason::ImplicitCast);
    if (rhs == nullptr || lhs == nullptr) {
        return tp.throwAway(lhs, rhs); // Either or both L and R failed to cast to D implicitly.
    }
    // (4) Return Builtin RELATIONAL_OP Builtin → Bool
    //      or Packed RELATIONAL_OP Packed → Bool
    //      or T* RELATIONAL_OP T* → Bool
    return tp.mk.Condition(syntax, lhs, op, rhs);
}

TpNode* tp_binary::logical() {
    auto  op = syntax->op.kind;
    auto rhs = tp.bindExpression(syntax->rhs);
    auto lhs = tp.bindExpression(syntax->lhs);
    if (rhs == nullptr || lhs == nullptr) {
        return tp.throwAway(rhs, lhs);
    }
    // T& → T
    rhs = tp.mk.DereferenceIfReference(syntax->rhs, rhs);
    lhs = tp.mk.DereferenceIfReference(syntax->lhs, lhs);
    // Find and call fn LOGICAL_OP(this: T, U) → V
    tp_site site{ syntax };
    auto result = site.tryCallOperator(lhs, op, rhs);
    site.dispose();
    if (result != nullptr) {
        return result;
    }
    // T → Bool
    rhs = tp.cast(syntax->rhs, rhs, tp.tree.tyBool, tp_cast_reason::ImplicitCast);
    lhs = tp.cast(syntax->lhs, lhs, tp.tree.tyBool, tp_cast_reason::ImplicitCast);
    if (rhs == nullptr || lhs == nullptr) {
        return tp.throwAway(rhs, lhs);
    }
    // Bool LOGICAL_OP Bool → Bool
    return tp.mk.Condition(syntax, lhs, op, rhs);
}

TpNode* tp_binary::nullCoalescing() {
    auto  op = syntax->op.kind;
    auto rhs = tp.bindExpression(syntax->rhs);
    auto lhs = tp.bindExpression(syntax->lhs);
    if (rhs == nullptr || lhs == nullptr) {
        return tp.throwAway(rhs, lhs);
    }
    // T& → T
    rhs = tp.mk.DereferenceIfReference(syntax->rhs, rhs);
    lhs = tp.mk.DereferenceIfReference(syntax->lhs, lhs);
    // Find and call fn ??(this: T, U) → V
    tp_site site{ syntax };
    auto result = site.tryCallOperator(lhs, op, rhs);
    site.dispose();
    if (result != nullptr) {
        return result;
    }
    // If no user-defined function exists, we are falling back to the compiler.
    // We only know how to do '??' on numerics, T* and T&.
    if (lhs->type.isNotNumericOrIndirect() || rhs->type.isNotNumericOrIndirect()) {
        illegal_operation();
    }
    // Find a D such that both L and R can be cast to it.
    auto dst = tp.upperBound(lhs->type, rhs->type, op);
    if (dst.isUnknown()) {
        illegal_operation();
    }
    rhs = tp.cast(syntax->rhs, rhs, dst, tp_cast_reason::ImplicitCast);
    lhs = tp.cast(syntax->lhs, lhs, dst, tp_cast_reason::ImplicitCast);
    if (rhs == nullptr || lhs == nullptr) {
        return tp.throwAway(lhs, rhs);
    }
    // lhs 'if' lhs != null else rhs
    auto condition = tp.mk.Condition(syntax, lhs, Tok::NotEqual, tp.mk.ZeroOf(syntax, dst));
    return tp.mk.Ternary(syntax, condition, lhs, rhs);
}

TpNode* tp_binary::bitwise(bool isAssignment) {
    auto  op = syntax->op.kind;
    auto rhs = tp.bindExpression(syntax->rhs);
    auto lhs = tp.bindExpression(syntax->lhs);
    if (rhs == nullptr || lhs == nullptr) {
        return tp.throwAway(rhs, lhs);
    }
    // T& → T
    rhs = tp.mk.DereferenceIfReference(syntax->rhs, rhs);
    lhs = tp.mk.DereferenceIfReference(syntax->lhs, lhs);
    // Find and call fn BITWISE_OP(this: T, U) → V
    tp_site site{ syntax };
    auto result = site.tryCallOperator(lhs, op, rhs);
    site.dispose();
    if (result != nullptr) {
        return result;
    }
    // If no user-defined function exists, we are falling back to the compiler.
    // We only know how to BITWISE_OP Integrals and PackedIntegrals.
    if (lhs->type.isNotIntegralOrPackedIntegrals() || rhs->type.isNotIntegral()) {
        illegal_operation();
    }
    // Handle bitwise operations on PackedIntegrals.
    if (auto  symbol = lhs->type.isPacked()) {
        auto builtin = (TpBuiltin*)symbol->node;
        auto packing = builtin->packing();
        if (rhs = tp.cast(syntax->rhs, rhs, packing.type, tp_cast_reason::ImplicitCast)) {
            // Packed BITWISE_OP Integral → Packed
            if (isAssignment) {
                return tp.mk.CompoundArithmetic(syntax, lhs, op, rhs);
            }
            return tp.mk.Arithmetic(syntax, lhs, op, rhs);
        }
        return tp.throwAway(lhs);
    }
    // Handle bitwise operations on Integrals.
    if (rhs = tp.cast(syntax->rhs, rhs, lhs->type, tp_cast_reason::ImplicitCast)) {
        // Integral BITWISE_OP Integral → Integral
        if (isAssignment) {
            return tp.mk.CompoundArithmetic(syntax, lhs, op, rhs);
        }
        return tp.mk.Arithmetic(syntax, lhs, op, rhs);
    }
    return tp.throwAway(lhs);
}

TpNode* tp_binary::shift(bool isAssignment) {
    auto  op = syntax->op.kind;
    auto rhs = tp.bindExpression(syntax->rhs);
    auto lhs = tp.bindExpression(syntax->lhs);
    if (rhs == nullptr || lhs == nullptr) {
        return tp.throwAway(rhs, lhs);
    }
    // T& → T
    rhs = tp.mk.DereferenceIfReference(syntax->rhs, rhs);
    lhs = tp.mk.DereferenceIfReference(syntax->lhs, lhs);
    // Find and call fn SHIFT_OP(this: T, U) → V
    tp_site site{ syntax };
    auto result = site.tryCallOperator(lhs, op, rhs);
    site.dispose();
    if (result != nullptr) {
        return result;
    }
    // If no user-defined function exists, we are falling back to the compiler.
    // We only know how to SHIFT_OP Integrals and PackedIntegrals.
    if (lhs->type.isNotIntegralOrPackedIntegrals() || rhs->type.isNotIntegral()) {
        illegal_operation();
    }
    // Handle shift operations on PackedIntegrals.
    if (auto  symbol = lhs->type.isPacked()) {
        auto builtin = (TpBuiltin*)symbol->node;
        auto packing = builtin->packing();
        if (packing.type.isSigned()) {
            if (rhs = tp.cast(syntax->rhs, rhs, tp.tree.tyInt8, tp_cast_reason::ImplicitCast)) {
                // PackedSigned SHIFT_OP Int8 → PackedSigned
                if (isAssignment) {
                    return tp.mk.CompoundShift(syntax, lhs, op, rhs);
                }
                return tp.mk.Shift(syntax, lhs, op, rhs);
            }
            return tp.throwAway(lhs);
        } 
        if (packing.type.isUnsigned()) {
            if (rhs = tp.cast(syntax->rhs, rhs, tp.tree.tyUInt8, tp_cast_reason::ImplicitCast)) {
                // PackedUnsigned SHIFT_OP UInt8 → PackedUnsigned
                if (isAssignment) {
                    return tp.mk.CompoundShift(syntax, lhs, op, rhs);
                }
                return tp.mk.Shift(syntax, lhs, op, rhs);
            }
            return tp.throwAway(lhs);
        }
        illegal_operation();
    }
    // Handle shift operations on Integrals.
    if (lhs->type.isSigned()) {
        if (rhs = tp.cast(syntax->rhs, rhs, tp.tree.tyInt8, tp_cast_reason::ImplicitCast)) {
            // Signed SHIFT_OP Int8 → Signed
            if (isAssignment) {
                return tp.mk.CompoundShift(syntax, lhs, op, rhs);
            }
            return tp.mk.Shift(syntax, lhs, op, rhs);
        }
        return tp.throwAway(lhs);
    }
    if (rhs = tp.cast(syntax->rhs, rhs, tp.tree.tyUInt8, tp_cast_reason::ImplicitCast)) {
        // Unsigned SHIFT_OP UInt8 → Unsigned
        if (isAssignment) {
            return tp.mk.CompoundShift(syntax, lhs, op, rhs);
        }
        return tp.mk.Shift(syntax, lhs, op, rhs);
    }
    return tp.throwAway(lhs);
}

TpNode* tp_binary::minus() {
    auto  op = syntax->op.kind;
    auto rhs = tp.bindExpression(syntax->rhs);
    auto lhs = tp.bindExpression(syntax->lhs);
    if (rhs == nullptr || lhs == nullptr) {
        return tp.throwAway(rhs, lhs);
    }
    // T& → T
    rhs = tp.mk.DereferenceIfReference(syntax->rhs, rhs);
    lhs = tp.mk.DereferenceIfReference(syntax->lhs, lhs);
    // T* - U → T* where U is U/Int64
    if (lhs->type.isaPointer()) {
        if (rhs->type.isSigned()) {
            if (rhs = tp.cast(syntax->rhs, rhs, tp.tree.tyInt64, tp_cast_reason::ImplicitCast)) {
                // T* - Int64 → T*
                return tp.mk.PointerMinusInt(syntax, lhs, rhs);
            }
            return tp.throwAway(lhs);
        }
        if (rhs->type.isUnsigned()) {
            if (rhs = tp.cast(syntax->rhs, rhs, tp.tree.tyUInt64, tp_cast_reason::ImplicitCast)) {
                // T* - UInt64 → T*
                return tp.mk.PointerMinusInt(syntax, lhs, rhs);
            }
            return tp.throwAway(lhs);
        }
        if (rhs->type.isaPointer()) {
            auto upper = tp.upperBound(lhs->type, rhs->type, op);
            if (upper.isUnknown()) {
                illegal_operation();
            }
            lhs = tp.cast(syntax->lhs, lhs, upper, tp_cast_reason::ImplicitCast);
            rhs = tp.cast(syntax->rhs, rhs, upper, tp_cast_reason::ImplicitCast);
            if (lhs == nullptr || rhs == nullptr) {
                return tp.throwAway(lhs, rhs);
            }
            // T* - T* → Int64
            return tp.mk.PointerMinusPointer(syntax, lhs, rhs);
        }
        illegal_operation();
    }
    if (rhs->type.isaPointer()) {
        if (lhs->type.isSigned()) {
            if (lhs = tp.cast(syntax->rhs, lhs, tp.tree.tyInt64, tp_cast_reason::ImplicitCast)) {
                // Int64 - T* → T*
                return tp.mk.IntMinusPointer(syntax, lhs, rhs);
            }
            return tp.throwAway(lhs);
        }
        if (lhs->type.isUnsigned()) {
            if (lhs = tp.cast(syntax->rhs, lhs, tp.tree.tyUInt64, tp_cast_reason::ImplicitCast)) {
                // Int64 - T* → T*
                return tp.mk.IntMinusPointer(syntax, lhs, rhs);
            }
            return tp.throwAway(lhs);
        }
        illegal_operation();
    }
    // Find and call fn -(this: T, U) → V
    tp_site site{ syntax };
    auto result = site.tryCallOperator(lhs, op, rhs);
    site.dispose();
    if (result != nullptr) {
        return result;
    }
    // If no user-defined function exists, we are falling back to the compiler.
    // We only know how to MINUS for builtins.
    if (lhs->type.isNotABuiltin() || rhs->type.isNotABuiltin()) {
        illegal_operation();
    }
    // Find a D such that both L and R can be cast to it.
    auto dst = tp.upperBound(lhs->type, rhs->type, op);
    if (dst.isUnknown()) {
        illegal_operation();
    }
    rhs = tp.cast(syntax->rhs, rhs, dst, tp_cast_reason::ImplicitCast);
    lhs = tp.cast(syntax->lhs, lhs, dst, tp_cast_reason::ImplicitCast);
    if (rhs == nullptr || lhs == nullptr) {
        return tp.throwAway(lhs, rhs);
    }
    // Builtin - Builtin → Builtin
    return tp.mk.Arithmetic(syntax, lhs, op, rhs);
}

TpNode* tp_binary::minusAssign() {
    auto  op = syntax->op.kind;
    auto rhs = tp.bindExpression(syntax->rhs);
    auto lhs = tp.bindExpression(syntax->lhs);
    if (rhs == nullptr || lhs == nullptr) {
        return tp.throwAway(rhs, lhs);
    }
    // T& → T
    rhs = tp.mk.DereferenceIfReference(syntax->rhs, rhs);
    lhs = tp.mk.DereferenceIfReference(syntax->lhs, lhs);
    // T* -= U where U is U/Int64
    if (lhs->type.isaPointer()) {
        if (rhs->type.isSigned()) {
            if (rhs = tp.cast(syntax->rhs, rhs, tp.tree.tyInt64, tp_cast_reason::ImplicitCast)) {
                // T* -= Int64
                return tp.mk.CompoundPointerArithmetic(syntax, lhs, op, rhs);
            }
            return tp.throwAway(lhs);
        }
        if (rhs->type.isUnsigned()) {
            if (rhs = tp.cast(syntax->rhs, rhs, tp.tree.tyUInt64, tp_cast_reason::ImplicitCast)) {
                // T* -= UInt64
                return tp.mk.CompoundPointerArithmetic(syntax, lhs, op, rhs);
            }
            return tp.throwAway(lhs);
        }
        illegal_operation();
    }
    // Find and call fn -=(this: T, U) → V
    tp_site site{ syntax };
    auto result = site.tryCallOperator(lhs, op, rhs);
    site.dispose();
    if (result != nullptr) {
        return result;
    }
    // If no user-defined function exists, we are falling back to the compiler.
    // We only know how to MINUS for builtins.
    if (lhs->type.isNotABuiltin() || rhs->type.isNotABuiltin()) {
        illegal_operation();
    }
    // Find a D such that both L and R can be cast to it.
    auto dst = tp.upperBound(lhs->type, rhs->type, op);
    if (dst.isUnknown()) {
        illegal_operation();
    }
    rhs = tp.cast(syntax->rhs, rhs, dst, tp_cast_reason::ImplicitCast);
    lhs = tp.cast(syntax->lhs, lhs, dst, tp_cast_reason::ImplicitCast);
    if (rhs == nullptr || lhs == nullptr) {
        return tp.throwAway(lhs, rhs);
    }
    // Builtin -= Builtin
    return tp.mk.CompoundArithmetic(syntax, lhs, op, rhs);
}

TpNode* tp_binary::plus() {
    auto  op = syntax->op.kind;
    auto rhs = tp.bindExpression(syntax->rhs);
    auto lhs = tp.bindExpression(syntax->lhs);
    if (rhs == nullptr || lhs == nullptr) {
        return tp.throwAway(rhs, lhs);
    }
    // T& → T
    rhs = tp.mk.DereferenceIfReference(syntax->rhs, rhs);
    lhs = tp.mk.DereferenceIfReference(syntax->lhs, lhs);
    // T* + U → T* where U is U/Int64
    if (lhs->type.isaPointer()) {
        if (rhs->type.isSigned()) {
            if (rhs = tp.cast(syntax->rhs, rhs, tp.tree.tyInt64, tp_cast_reason::ImplicitCast)) {
                // T* + Int64 → T*
                return tp.mk.PointerPlusInt(syntax, lhs, rhs);
            }
            return tp.throwAway(lhs);
        }
        if (rhs->type.isUnsigned()) {
            if (rhs = tp.cast(syntax->rhs, rhs, tp.tree.tyUInt64, tp_cast_reason::ImplicitCast)) {
                // T* + UInt64 → T*
                return tp.mk.PointerPlusInt(syntax, lhs, rhs);
            }
            return tp.throwAway(lhs);
        }
        illegal_operation();
    }
    // U + T* → T* where U is U/Int64
    if (rhs->type.isaPointer()) {
        if (lhs->type.isSigned()) {
            // Signed '+' Pointer → Pointer
            if (lhs = tp.cast(syntax->lhs, lhs, tp.tree.tyInt64, tp_cast_reason::ImplicitCast)) {
                // Int64 '+' Pointer → Pointer
                return tp.mk.IntPlusPointer(syntax, lhs, rhs);
            }
            return tp.throwAway(rhs);
        }
        if (lhs->type.isUnsigned()) {
            // Unsigned '+' Pointer → Pointer
            if (lhs = tp.cast(syntax->lhs, lhs, tp.tree.tyUInt64, tp_cast_reason::ImplicitCast)) {
                // UInt64 '+' Pointer → Pointer
                return tp.mk.IntPlusPointer(syntax, lhs, rhs);
            }
            return tp.throwAway(rhs);
        }
        illegal_operation();
    }
    // Find and call fn +(this: T, U) → V
    tp_site site{ syntax };
    auto result = site.tryCallOperator(lhs, op, rhs);
    site.dispose();
    if (result != nullptr) {
        return result;
    }
    // If no user-defined function exists, we are falling back to the compiler.
    // We only know how to PLUS for builtins.
    if (lhs->type.isNotABuiltin() || rhs->type.isNotABuiltin()) {
        illegal_operation();
    }
    // Find a D such that both L and R can be cast to it.
    auto dst = tp.upperBound(lhs->type, rhs->type, op);
    if (dst.isUnknown()) {
        illegal_operation();
    }
    rhs = tp.cast(syntax->rhs, rhs, dst, tp_cast_reason::ImplicitCast);
    lhs = tp.cast(syntax->lhs, lhs, dst, tp_cast_reason::ImplicitCast);
    if (rhs == nullptr || lhs == nullptr) {
        return tp.throwAway(lhs, rhs);
    }
    // Builtin + Builtin → Builtin
    return tp.mk.Arithmetic(syntax, lhs, op, rhs);
}

TpNode* tp_binary::plusAssign() {
    auto  op = syntax->op.kind;
    auto rhs = tp.bindExpression(syntax->rhs);
    auto lhs = tp.bindExpression(syntax->lhs);
    if (rhs == nullptr || lhs == nullptr) {
        return tp.throwAway(rhs, lhs);
    }
    // T& → T
    rhs = tp.mk.DereferenceIfReference(syntax->rhs, rhs);
    lhs = tp.mk.DereferenceIfReference(syntax->lhs, lhs);
    // T* += U where U is U/Int64
    if (lhs->type.isaPointer()) {
        if (rhs->type.isSigned()) {
            if (rhs = tp.cast(syntax->rhs, rhs, tp.tree.tyInt64, tp_cast_reason::ImplicitCast)) {
                // T* += Int64
                return tp.mk.CompoundPointerArithmetic(syntax, lhs, op, rhs);
            }
            return tp.throwAway(lhs);
        }
        if (rhs->type.isUnsigned()) {
            if (rhs = tp.cast(syntax->rhs, rhs, tp.tree.tyUInt64, tp_cast_reason::ImplicitCast)) {
                // T* += UInt64
                return tp.mk.CompoundPointerArithmetic(syntax, lhs, op, rhs);
            }
            return tp.throwAway(lhs);
        }
        illegal_operation();
    }
    // Find and call fn +=(this: T, U) → V
    tp_site site{ syntax };
    auto result = site.tryCallOperator(lhs, op, rhs);
    site.dispose();
    if (result != nullptr) {
        return result;
    }
    // If no user-defined function exists, we are falling back to the compiler.
    // We only know how to += for builtins.
    if (lhs->type.isNotABuiltin() || rhs->type.isNotABuiltin()) {
        illegal_operation();
    }
    // Find a D such that both L and R can be cast to it.
    auto dst = tp.upperBound(lhs->type, rhs->type, op);
    if (dst.isUnknown()) {
        illegal_operation();
    }
    rhs = tp.cast(syntax->rhs, rhs, dst, tp_cast_reason::ImplicitCast);
    lhs = tp.cast(syntax->lhs, lhs, dst, tp_cast_reason::ImplicitCast);
    if (rhs == nullptr || lhs == nullptr) {
        return tp.throwAway(lhs, rhs);
    }
    // Builtin += Builtin
    return tp.mk.CompoundArithmetic(syntax, lhs, op, rhs);
}

TpNode* tp_binary::multiplicative(bool isAssignment) {
    auto  op = syntax->op.kind;
    auto rhs = tp.bindExpression(syntax->rhs);
    auto lhs = tp.bindExpression(syntax->lhs);
    if (rhs == nullptr || lhs == nullptr) {
        return tp.throwAway(rhs, lhs);
    }
    // T& → T
    rhs = tp.mk.DereferenceIfReference(syntax->rhs, rhs);
    lhs = tp.mk.DereferenceIfReference(syntax->lhs, lhs);
    if (lhs->type.isaPointer() || rhs->type.isaPointer()) {
        illegal_operation();
    }
    // Find and call fn MULTIPLICATIVE_OP(this: T, U) → V
    tp_site site{ syntax };
    auto result = site.tryCallOperator(lhs, op, rhs);
    site.dispose();
    if (result != nullptr) {
        return result;
    }
    // If no user-defined function exists, we are falling back to the compiler.
    // We only know how to do MULTIPLICATIVE_OP for builtins.
    if (lhs->type.isNotABuiltin() || rhs->type.isNotABuiltin()) {
        illegal_operation();
    }
    // Find a D such that both L and R can be cast to it.
    auto dst = tp.upperBound(lhs->type, rhs->type, op);
    if (dst.isUnknown()) {
        illegal_operation();
    }
    rhs = tp.cast(syntax->rhs, rhs, dst, tp_cast_reason::ImplicitCast);
    lhs = tp.cast(syntax->lhs, lhs, dst, tp_cast_reason::ImplicitCast);
    if (rhs == nullptr || lhs == nullptr) {
        return tp.throwAway(lhs, rhs);
    }
    // Builtin MULTIPLICATIVE_OP Builtin → Builtin
    if (isAssignment) {
        return tp.mk.CompoundArithmetic(syntax, lhs, op, rhs);
    }
    return tp.mk.Arithmetic(syntax, lhs, op, rhs);
}

TpNode* tp_binary::assign() {
    auto  op = syntax->op.kind;
    auto rhs = tp.bindExpression(syntax->rhs);
    auto lhs = tp.bindExpression(syntax->lhs);
    if (rhs == nullptr || lhs == nullptr) {
        return tp.throwAway(rhs, lhs);
    }
    // T& → T
    rhs = tp.mk.DereferenceIfReference(syntax->rhs, rhs);
    lhs = tp.mk.DereferenceIfReference(syntax->lhs, lhs);
    // Find and call fn =(this: T, U) → V
    tp_site site{ syntax };
    auto result = site.tryCallOperator(lhs, op, rhs);
    site.dispose();
    if (result != nullptr) {
        return result;
    }
    // If no user-defined function exists, we are falling back to the compiler.
    // We know how to do '=' for everything.
    if (rhs = tp.cast(syntax->rhs, rhs, lhs->type, tp_cast_reason::ExplicitCast)) {
        return tp.mk.Assignment(syntax, lhs, rhs);
    }
    return tp.throwAway(lhs);
}

TpNode* tp_binary::kwAs() {
    auto rhs = tp.bindExpression(syntax->rhs);
    auto lhs = tp.bindExpression(syntax->lhs);
    if (rhs == nullptr || lhs == nullptr) {
        return tp.throwAway(rhs, lhs);
    }
    auto tpname = tp.isa.TypeName(rhs);
    if (tpname == nullptr) {
        type_error(syntax->rhs, "expected a typename, not a value of %tptype", &rhs->type);
        return tp.throwAway(rhs, lhs);
    }
    auto dst = tpname->type;
    tp.throwAway(tpname);
    return tp.cast(syntax, lhs, dst, tp_cast_reason::ExplicitCast);
}

TpNode* tp_binary::kwTo() {
    auto rhs = tp.bindExpression(syntax->rhs);
    auto lhs = tp.bindExpression(syntax->lhs);
    if (rhs == nullptr || lhs == nullptr) {
        return tp.throwAway(rhs, lhs);
    }
    auto tpname = tp.isa.TypeName(rhs);
    if (tpname == nullptr) {
        type_error(syntax->rhs, "expected a typename, not a value of %tptype", &rhs->type);
        return tp.throwAway(rhs, lhs);
    }
    auto dst = tpname->type;
    tp.throwAway(tpname);
    return tp.bitCast(syntax, lhs, dst, tp_cast_reason::ExplicitCast);
}
} // namespace exy