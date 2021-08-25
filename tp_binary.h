#pragma once

namespace exy {
struct tp_binary {
    Typer &tp;
    BinarySyntax *syntax;

    tp_binary(BinarySyntax *syntax);
    void dispose();

    // NotEquivalent '!==' and Equivalent '==='
    TpNode* equivalence();

    // NotEqual '!=', Equal '==', Less '<', LessOrEqual '<=', Greater '>' and GreaterOrEqual '>='
    TpNode* relational();

    // OrOr '||' and AndAnd '&&'
    TpNode* logical();

    // QuestionQuestion '??'
    TpNode* nullCoalescing();

    // Or '|', XOr '^' and And '&'
    // OrAssign '|=', XOrAssign '^=' and AndAssign '&='
    TpNode* bitwise(bool isAssignment);

    // LeftShift '<<', RightShift '>>' and UnsignedRightShift '>>>'
    // LeftShiftAssign '<<=', RightShiftAssign '>>=' and UnsignedRightShiftAssign '>>>='
    TpNode* shift(bool isAssignment);

    // Minus '-'
    TpNode* minus();
    // MinusAssign '-='
    TpNode* minusAssign();

    // Plus '+'
    TpNode* plus();
    // PlusAssign '+='
    TpNode* plusAssign();

    // Multiply '*', Divide '/', Remainder '%', DivRem '%%' and Exponentiation '**'
    // MultiplyAssign '*=', DivideAssign '/=', RemainderAssign '%=', DivRemAssign '%%=' and ExponentiationAssign '**='
    TpNode* multiplicative(bool isAssignment);

    // Assign '='
    TpNode* assign();

    // As 'as'
    TpNode* kwAs();

    // To 'to'
    TpNode* kwTo();
};
} // namespace exy