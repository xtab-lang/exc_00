//////////////////////////////////////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2020-12-18
//////////////////////////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "typer.h"

#define err(token, msg, ...) print_error("Literal", token, msg, __VA_ARGS__)

namespace exy {
namespace typ_pass {
AstLiteral* Literal::visit(SyntaxLiteral *syntax) {
    if (auto modifiers = syntax->modifiers) {
        err(modifiers, "modifiers not allowed on literals");
    }
    auto pos = tp.mkpos(syntax);
    switch (syntax->literalKind) {
        case SyntaxLiteralKind::Void:
            return tp.mem.New<AstVoid>(pos);
        case SyntaxLiteralKind::Null:
            return tp.mem.New<AstNull>(pos);
        case SyntaxLiteralKind::Char:
            return tp.mem.New<AstChar>(pos, syntax->ch);
        case SyntaxLiteralKind::Bool:
            return tp.mem.New<AstBool>(pos, syntax->b);
        case SyntaxLiteralKind::WChar:
            return tp.mem.New<AstWChar>(pos, syntax->wch);
        case SyntaxLiteralKind::Utf8:
            return tp.mem.New<AstUtf8>(pos, syntax->u32);

        case SyntaxLiteralKind::Int8:
            return tp.mem.New<AstSignedInt>(pos, tp.tree.tyInt8, syntax->i8);
        case SyntaxLiteralKind::Int16:
            return tp.mem.New<AstSignedInt>(pos, tp.tree.tyInt16, syntax->i16);
        case SyntaxLiteralKind::Int32:
            return tp.mem.New<AstSignedInt>(pos, tp.tree.tyInt32, syntax->i32);
        case SyntaxLiteralKind::Int64:
            return tp.mem.New<AstSignedInt>(pos, tp.tree.tyInt64, syntax->i64);

        case SyntaxLiteralKind::UInt8:
            return tp.mem.New<AstUnsignedInt>(pos, tp.tree.tyUInt8, syntax->u8);
        case SyntaxLiteralKind::UInt16:
            return tp.mem.New<AstUnsignedInt>(pos, tp.tree.tyUInt16, syntax->u16);
        case SyntaxLiteralKind::UInt32:
            return tp.mem.New<AstUnsignedInt>(pos, tp.tree.tyUInt32, syntax->u32);
        case SyntaxLiteralKind::UInt64:
            return tp.mem.New<AstUnsignedInt>(pos, tp.tree.tyUInt64, syntax->u64);

        case SyntaxLiteralKind::Float:  return tp.mem.New<AstFloat>(pos, syntax->f32);

        case SyntaxLiteralKind::Double: return tp.mem.New<AstDouble>(pos, syntax->f64);
    }
    Unreachable();
}
} // namespace typ_pass
} // namespace exy