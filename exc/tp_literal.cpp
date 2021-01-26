//////////////////////////////////////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2020-12-18
//////////////////////////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "typer.h"

#include "tp_literal.h"

#define err(token, msg, ...) print_error("Literal", token, msg, __VA_ARGS__)

namespace exy {
namespace stx2ast_pass {
AstConstant* Literal::visit(SyntaxLiteral *syntax) {
    if (auto modifiers = syntax->modifiers) {
        err(modifiers, "modifiers not allowed on literals");
    }
    auto pos = tp.mkpos(syntax);
    switch (syntax->literalKind) {
        case SyntaxLiteralKind::Void:
            return tp.mk.void_(pos);

        case SyntaxLiteralKind::Null:
            return tp.mk.null_(pos);

        case SyntaxLiteralKind::Char:
            return tp.mk.char_(pos, syntax->ch);

        case SyntaxLiteralKind::Bool:
            return tp.mk.bool_(pos, syntax->b);

        case SyntaxLiteralKind::WChar:
            return tp.mk.wchar_(pos, syntax->wch);

        case SyntaxLiteralKind::Utf8:
            return tp.mk.utf8(pos, syntax->utf8);

        case SyntaxLiteralKind::Int8:
            return tp.mk.int8(pos, syntax->i8);

        case SyntaxLiteralKind::Int16:
            return tp.mk.int16(pos, syntax->i16);

        case SyntaxLiteralKind::Int32:
            return tp.mk.int32(pos, syntax->i32);

        case SyntaxLiteralKind::Int64:
            return tp.mk.int64(pos, syntax->i64);

        case SyntaxLiteralKind::UInt8:
            return tp.mk.uint8(pos, syntax->u8);

        case SyntaxLiteralKind::UInt16:
            return tp.mk.uint16(pos, syntax->u16);

        case SyntaxLiteralKind::UInt32:
            return tp.mk.uint32(pos, syntax->u32);

        case SyntaxLiteralKind::UInt64:
            return tp.mk.uint64(pos, syntax->u64);

        case SyntaxLiteralKind::Float: 
            return tp.mk.float32(pos, syntax->f32);

        case SyntaxLiteralKind::Double: 
            return tp.mk.float64(pos, syntax->f64);
    }
    Unreachable();
}
} // namespace stx2ast_pass
} // namespace exy