#pragma once

namespace exy {
struct tp_unary_prefix {
    Typer &tp;
    UnaryPrefixSyntax *syntax;

    tp_unary_prefix(UnaryPrefixSyntax *syntax);
    void dispose();

    TpNode* plusOrMinus();
    TpNode* incrementOrDecrement();
    TpNode* dereference();
    TpNode* addressOf();
    TpNode* logicalNot();
    TpNode* bitwiseNot();

    TpNode* kwSizeOf();
    TpNode* kwTypeOf();
    TpNode* kwDelete();
    TpNode* kwAtomic();

    TpNode* kwAwait();
};

//----------------------------------------------------------
struct tp_unary_suffix {
    Typer &tp;
    UnarySuffixSyntax *syntax;

    tp_unary_suffix(UnarySuffixSyntax *syntax);
    void dispose();

    TpNode* pointerType();
    TpNode* referenceType();
    TpNode* incrementOrDecrement();
};
} // namespace exy