#include "pch.h"
#include "typer.h"

namespace exy {
TpNode* Typer::bindCondition(SyntaxNode *syntax) {
    Assert(syntax != nullptr);
    if (auto value = bindExpression(syntax)) {
        if (auto condition = isa.Condition(value)) {
            return condition;
        }
        if (auto expr = isa.ConstExpr(value)) {
            expr->value = mk.Condition(syntax, expr->value);
            return expr->value != nullptr ? expr : throwAway(expr);
        }
        if (auto expr = isa.Atomic(value)) {
            return expr->value != nullptr ? expr : throwAway(expr);
        }
        if (auto zero = mk.ZeroOf(syntax, value->type)) {
            return mk.Condition(syntax, value, Tok::NotEqual, zero);
        }
        throwAway(value);
    }
    return nullptr;
}

TpNode* Typer::bindExpression(SyntaxNode *syntax) {
    if (syntax == nullptr) {
        return nullptr;
    }
    switch (syntax->kind) {
        case SyntaxKind::Structure:
            return bindStructureExpression((StructureSyntax*)syntax);

        case SyntaxKind::Function:
            return bindFunctionExpression((FunctionSyntax*)syntax);

        case SyntaxKind::Variable:
            return bindVariableExpression((VariableSyntax*)syntax);

        case SyntaxKind::Ternary:
            return bindTernaryExpression((TernarySyntax*)syntax);

        case SyntaxKind::IfExpression:
            return bindIfExpression((IfExpressionSyntax*)syntax);

        case SyntaxKind::Binary:
            return bindBinaryExpression((BinarySyntax*)syntax);

        case SyntaxKind::UnaryPrefix:
            return bindUnaryPrefixExpression((UnaryPrefixSyntax*)syntax);

        case SyntaxKind::UnarySuffix:
            return bindUnarySuffixExpression((UnarySuffixSyntax*)syntax);

        case SyntaxKind::Dot:
            return bindDotExpression((DotSyntax*)syntax);

        case SyntaxKind::Index:
            return bindIndexExpression((IndexSyntax*)syntax);

        case SyntaxKind::Call:
            return bindCallExpression((CallSyntax*)syntax);

        case SyntaxKind::Initializer:
            return bindInitializerExpression((InitializerSyntax*)syntax);

        case SyntaxKind::Parenthesized:
            return bindParenthesizedExpression((ParenthesizedSyntax*)syntax);

        case SyntaxKind::Identifier:
            return bindIdentifier((IdentifierSyntax*)syntax);

        case SyntaxKind::Null:
            return bindNullExpression((NullSyntax*)syntax);

        case SyntaxKind::Void:
            return bindVoidExpression((VoidSyntax*)syntax);

        case SyntaxKind::Boolean:
            return bindBooleanExpression((BooleanSyntax*)syntax);

        case SyntaxKind::Number:
            return bindNumberExpression((NumberSyntax*)syntax);
    }
    syntax_error(syntax, "%sk not expected in an expression context", syntax->kind);
    return nullptr;
}

TpNode* Typer::bindStructureExpression(StructureSyntax *syntax) {
    if (auto templateSymbol = collectTemplate(syntax)) {
        return mk.Name(syntax, templateSymbol);
    }
    return nullptr;
}

TpNode* Typer::bindFunctionExpression(FunctionSyntax *syntax) {
    if (auto templateSymbol = collectTemplate(syntax)) {
        return mk.Name(syntax, templateSymbol);
    }
    return nullptr;
}

TpNode* Typer::bindVariableExpression(VariableSyntax *syntax) {
    if (current->isaLocalsScope()) {
        Assert(0);
    }
    syntax_error(syntax, "variable expressions only allowed in function scope");
    return nullptr;
}

TpNode* Typer::bindTernaryExpression(TernarySyntax *syntax) {
    auto condition = bindExpression(syntax->condition);
    auto    iftrue = bindExpression(syntax->iftrue);
    auto    ifalse = bindExpression(syntax->ifalse);
    if (condition == nullptr || iftrue == nullptr || ifalse == nullptr) {
        return throwAway(condition, iftrue, ifalse);
    }
    condition = cast(syntax->condition, condition, tree.tyBool, tp_cast_reason::ImplicitCast);
    if (condition == nullptr) {
        return throwAway(condition, iftrue, ifalse);
    }
    iftrue = mk.DereferenceIfReference(syntax->iftrue, iftrue);
    ifalse = mk.DereferenceIfReference(syntax->ifalse, ifalse);
    auto upper = upperBound(iftrue->type, ifalse->type);
    if (upper.isUnknown()) {
        type_error(syntax, "different types for the branches of a '?:' operation: %tptype ? %tptype : %tptype",
                   &condition->type, &iftrue->type, &ifalse->type);
    }
    iftrue = cast(syntax->iftrue, iftrue, upper, tp_cast_reason::ImplicitCast);
    ifalse = cast(syntax->ifalse, ifalse, upper, tp_cast_reason::ImplicitCast);
    if (iftrue == nullptr || ifalse == nullptr) {
        return throwAway(condition, iftrue, ifalse);
    }
    return mk.Ternary(syntax, condition, iftrue, ifalse);
}

TpNode* Typer::bindIfExpression(IfExpressionSyntax *syntax) {
    auto condition = bindExpression(syntax->condition);
    auto    iftrue = bindExpression(syntax->iftrue);
    TpNode *ifalse = nullptr;
    if (condition == nullptr || iftrue == nullptr) {
        return throwAway(condition, iftrue);
    }
    if (syntax->ifalse != nullptr) {
        ifalse = bindExpression(syntax->ifalse);
        if (ifalse == nullptr) {
            return throwAway(condition, iftrue, ifalse);
        }
    } else {
        ifalse = mk.ZeroOf(syntax, iftrue->type);
    }
    condition = cast(syntax->condition, condition, tree.tyBool, tp_cast_reason::ImplicitCast);
    if (condition == nullptr) {
        return throwAway(condition, iftrue, ifalse);
    }
    iftrue = mk.DereferenceIfReference(syntax->iftrue, iftrue);
    ifalse = mk.DereferenceIfReference(syntax->ifalse == nullptr ? syntax : syntax->ifalse, ifalse);
    auto upper = upperBound(iftrue->type, ifalse->type);
    if (upper.isUnknown()) {
        type_error(syntax, "different types for the branches of a '?:' operation");
    }
    iftrue = cast(syntax->iftrue, iftrue, upper, tp_cast_reason::ImplicitCast);
    ifalse = cast(syntax->ifalse == nullptr ? syntax : syntax->ifalse, ifalse, upper, tp_cast_reason::ImplicitCast);
    if (condition == nullptr || iftrue == nullptr || ifalse == nullptr) {
        return throwAway(condition, iftrue, ifalse);
    }
    return mk.Ternary(syntax, condition, iftrue, ifalse);
}

TpNode* Typer::bindBinaryExpression(BinarySyntax *syntax) {
    tp_binary bin{ syntax };
    TpNode *result = nullptr;
    auto checkKeyword = false;
    switch (syntax->op.kind) {
        case Tok::OrAssign:
        case Tok::XOrAssign:
        case Tok::AndAssign: {
            result = bin.bitwise(/* isAssignment = */ true);
        } break;
        case Tok::LeftShiftAssign:
        case Tok::RightShiftAssign:
        case Tok::UnsignedRightShiftAssign: {
            result = bin.shift(/* isAssignment = */ true);
        } break;
        case Tok::MinusAssign: {
            result = bin.minusAssign();
        } break;
        case Tok::PlusAssign: {
            result = bin.plusAssign();
        } break;
        case Tok::MultiplyAssign:
        case Tok::DivideAssign:
        case Tok::RemainderAssign:
        case Tok::ExponentiationAssign: {
            result = bin.multiplicative(/* isAssignment = */ true);
        } break;
        case Tok::Assign: {
            result = bin.assign();
        } break;
        case Tok::OrOr:
        case Tok::AndAnd: {
            result = bin.logical();
        } break;
        case Tok::QuestionQuestion: {
            result = bin.nullCoalescing();
        } break;
        case Tok::Or:
        case Tok::XOr:
        case Tok::And: {
            result = bin.bitwise(/* isAssignment = */ false);
        } break;
        case Tok::NotEquivalent:
        case Tok::Equivalent: {
            result = bin.equivalence();
        } break;
        case Tok::NotEqual:
        case Tok::Equal:
        case Tok::Less:
        case Tok::LessOrEqual:
        case Tok::Greater:
        case Tok::GreaterOrEqual: {
            result = bin.relational();
        } break;
        case Tok::LeftShift:
        case Tok::RightShift:
        case Tok::UnsignedRightShift: {
            result = bin.shift(/* isAssignment = */ false);
        } break;
        case Tok::Minus: {
            result = bin.minus();
        } break;
        case Tok::Plus: {
            result = bin.plus();
        } break;
        case Tok::Multiply:
        case Tok::Divide:
        case Tok::Remainder:
        case Tok::Exponentiation: {
            result = bin.multiplicative(/* isAssignment = */ false);
        } break;
        default: {
            checkKeyword = true;
        } break;
    }
    if (checkKeyword) {
        switch (syntax->op.keyword) {
            case Keyword::As: {
                result = bin.kwAs();
            } break;
            case Keyword::To: {
                result = bin.kwTo();
            } break;
            default: {
                impl_error(syntax, "bindBinaryExpression(%tok)", syntax->op);
            } break;
        }
    }
    bin.dispose();
    return result;
}

TpNode* Typer::bindUnaryPrefixExpression(UnaryPrefixSyntax *syntax) {
    tp_unary_prefix unary{ syntax };
    TpNode *result = nullptr;
    switch (syntax->pos.kind) {
        case Tok::UnaryMinus:
        case Tok::UnaryPlus: {
            result = unary.plusOrMinus();
        } break;
        case Tok::MinusMinus:
        case Tok::PlusPlus: {
            result = unary.incrementOrDecrement();
        } break;
        case Tok::Dereference: {
            result = unary.dereference();
        } break;
        case Tok::AddressOf: {
            result = unary.addressOf();
        } break;
        case Tok::LogicalNot: {
            result = unary.logicalNot();
        } break;
        case Tok::BitwiseNot: {
            result = unary.bitwiseNot();
        } break;
    }
    if (result != nullptr) {
        return result;
    }
    switch (syntax->pos.keyword) {
        case Keyword::SizeOf: {
            result = unary.kwSizeOf();
        } break;
        case Keyword::TypeOf: {
            result = unary.kwTypeOf();
        } break;
        case Keyword::Delete: {
            result = unary.kwDelete();
        } break;
        case Keyword::Atomic: {
            result = unary.kwAtomic();
        } break;
        case Keyword::Await: {
            result = unary.kwAwait();
        } break;
        default: {
            impl_error(syntax, "bindUnaryPrefixExpression(%tk)", syntax->pos.kind);
        } break;
    }
    return result;
}

TpNode* Typer::bindUnarySuffixExpression(UnarySuffixSyntax *syntax) {
    tp_unary_suffix unary{ syntax };
    TpNode *result = nullptr;
    switch (syntax->op.kind) {
        case Tok::Pointer: {
            result = unary.pointerType();
        } break;
        case Tok::Reference: {
            result = unary.referenceType();
        } break;
        case Tok::MinusMinus:
        case Tok::PlusPlus: {
            result = unary.incrementOrDecrement();
        } break;
        default: {
            impl_error(syntax, "bindUnaryPrefixExpression(%tk)", syntax->pos.kind);
        } break;
    }
    return result;
}

TpNode* Typer::bindDotExpression(DotSyntax *syntax) {
    if (auto base = bindExpression(syntax->lhs)) {
        return bindDotExpression(base, syntax->rhs);
    }
    return nullptr;
}

TpNode* Typer::bindDotExpression(TpNode *base, SyntaxNode *syntax) {
    switch (auto k = syntax->kind) {
        case SyntaxKind::Index:
            return bindIndexExpression(base, (IndexSyntax*)syntax);
        case SyntaxKind::Call:
            return bindCallExpression(base, (CallSyntax*)syntax);
        case SyntaxKind::Initializer:
            return bindInitializerExpression(base, (InitializerSyntax*)syntax);
        case SyntaxKind::Identifier:
            return bindIdentifier(base, (IdentifierSyntax*)syntax);
        default:
            syntax_error(syntax, "%sk not allowed as index in a dot-expression", k);
            break;
    }
    return throwAway(base);
}

TpNode* Typer::bindIndexExpression(TpNode *base, IndexSyntax *syntax) {
    tp_bracketed index{ syntax };
    auto res = index.bind(base);
    index.dispose();
    return res;
}

TpNode* Typer::bindIndexExpression(IndexSyntax *syntax) {
    tp_bracketed index{ syntax };
    auto res = index.bind(/* base = */ nullptr);
    index.dispose();
    return res;
}

TpNode* Typer::bindCallExpression(TpNode *base, CallSyntax *syntax) {
    tp_parenthesized call{ syntax };
    auto res = call.bind(base);
    call.dispose();
    return res;
}

TpNode* Typer::bindCallExpression(CallSyntax *syntax) {
    tp_parenthesized call{ syntax };
    auto res = call.bind(/* base = */ nullptr);
    call.dispose();
    return res;
}

TpNode* Typer::bindInitializerExpression(TpNode *base, InitializerSyntax *syntax) {
    tp_braced braced{ syntax };
    auto res = braced.bind(base);
    braced.dispose();
    return res;
}

TpNode* Typer::bindInitializerExpression(InitializerSyntax *syntax) {
    tp_braced braced{ syntax };
    auto res = braced.bind(/* base = */ nullptr);
    braced.dispose();
    return res;
}

TpNode* Typer::bindParenthesizedExpression(ParenthesizedSyntax *syntax) {
    if (auto list = isa.CommaSeparatedSyntax(syntax)) {
        impl_error(syntax, "%c", __FUNCTION__);
        return nullptr;
    } 
    if (auto value = bindExpression(syntax->value)) {
        return value;
    }
    return nullptr;
}

TpNode* Typer::bindNullExpression(NullSyntax *syntax) {
    return mk.Literal(syntax, tree.tyVoidPointer, 0ui64);
}

TpNode* Typer::bindVoidExpression(VoidSyntax *syntax) {
    return mk.Literal(syntax, tree.tyVoid, 0ui64);
}

TpNode* Typer::bindBooleanExpression(BooleanSyntax *syntax) {
    return mk.Literal(syntax, tree.tyBool, UINT64(syntax->value));
}

TpNode* Typer::bindNumberExpression(NumberSyntax *syntax) {
    switch (syntax->type) {
    #define ZM(zName, zSize) case Keyword::zName: return mk.Literal(syntax, tree.ty##zName, syntax->u64);
        DeclareBuiltinTypeKeywords(ZM)
    #undef ZM
    }
    impl_error(syntax, "bindNumber(%sk)", syntax->kind);
    return nullptr;
}
} // namespace exy