//////////////////////////////////////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2020-12-18
//////////////////////////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "typer.h"

#include "tp_cast.h"

#define err(token, msg, ...) print_error("Make", token, msg, __VA_ARGS__)

namespace exy {
namespace stx2ast_pass {
Make::Symbol Make::importOf(Loc loc, Identifier name, Symbol symbol) {
    return aliasOf(loc, AstAliasKind::Import, name, symbol);
}

Make::Symbol Make::exportOf(Loc loc, Identifier name, Symbol symbol) {
    return aliasOf(loc, AstAliasKind::Export, name, symbol);
}

Make::Symbol Make::aliasOf(Loc loc, AstAliasKind decl, Identifier name, Symbol symbol) {
    auto scope = tp.currentScope();
    switch (symbol->kind) {
        case AstKind::Builtin: {
            auto node = (AstBuiltin*)symbol;
            return tp.mem.New<AstTypeAlias>(loc, decl, scope, name, node->type);
        }

        case AstKind::Module: {
            auto node = (AstModule*)symbol;
            if (tp.visitModule(node)) {
                return tp.mem.New<AstTypeAlias>(loc, decl, scope, name, node->type);
            }
            return nullptr;
        }

        case AstKind::TypeAlias:
            return tp.mem.New<AstTypeAlias>(loc, decl, scope, name, symbol->type);

        case AstKind::ValueAlias:
            return aliasOf(loc, decl, name, ((AstValueAlias*)symbol)->value);

        case AstKind::ConstAlias:
            return tp.mem.New<AstConstAlias>(loc, decl, scope, name, ((AstConstAlias*)symbol)->value);

        case AstKind::Global:
            return tp.mem.New<AstValueAlias>(loc, decl, scope, name, symbol);
    } 

    err(loc, "illegal alias of %ast symbol", symbol->kind);

    return nullptr;
}

Make::Symbol Make::global(Loc loc, Identifier name, Type type) {
    auto     scope = tp.currentScope();
    if (auto found = scope->find(name)) {
        err(loc, "%s#<red> already defined in the current scope", name);
        return nullptr;
    }
    return tp.mem.New<AstGlobal>(loc, type, scope, name);
}

Make::Node Make::name(Loc loc, AstSymbol *symbol) {
    switch (symbol->kind) {
        case AstKind::Builtin: {
            auto node = (AstBuiltin*)symbol;
            return tpname(loc, node->type);
        }

        case AstKind::Module: {
            auto node = (AstModule*)symbol;
            if (tp.visitModule(node)) {
                return tpname(loc, node->type);
            }
        } break;

        case AstKind::TypeAlias:
            return tpname(loc, symbol->type);

        case AstKind::ValueAlias: {
            symbol = ((AstValueAlias*)symbol)->value;
        } break;

        case AstKind::ConstAlias:
        case AstKind::Global:
            break; // OK. Will make name instead of tpname.

        default: {
            err(loc, "cannot make name of %ast symbol", symbol->kind);
            symbol = nullptr;
        } break;
    } 
    
    if (symbol) {
        if (tp._names.length) {
            auto last = tp._names.pop();
            return new(last) AstName{ loc, symbol };
        }
        return tp.mem.New<AstName>(loc, symbol);
    }
    
    return nullptr;
}

Make::Node Make::tpname(Loc loc, Type type) {
    if (tp._tpnames.length) {
        auto last = tp._tpnames.pop();
        return new(last) AstTypeName{ loc, type };
    }
    return tp.mem.New<AstTypeName>(loc, type);
}

Make::Node Make::explicitCast(Loc loc, Node src, Type dst) {
    return Cast(tp).explicitCast(loc, src, dst);
}

Make::Node Make::implicitCast(Loc loc, Node src, Type dst) {
    return Cast(tp).implicitCast(loc, src, dst);
}

Make::Node Make::definition(Loc loc, Symbol symbol, Node rhs) {
    if (auto l = name(symbol->loc, symbol)) {
        if (auto lhs = tp.isa.Name(l)) {
            if (rhs = explicitCast(loc, rhs, lhs->type)) {
                return tp.mem.New<AstDefinition>(loc, lhs, rhs);
            }
        } else {
            err(loc, "expected a name, not %ast", symbol->kind);
        }
    }
    return tp.throwAway(rhs);
}

Make::Constant Make::constant(Loc loc, Type type, UINT64 u64) {
    return tp.mem.New<AstConstant>(loc, type, u64);
}

Make::Constant Make::void_(Loc loc) {
    return constant(loc, tp.tree.tyVoid);
}

Make::Constant Make::null_(Loc loc) {
    return constant(loc, tp.tree.tyNull);
}

Make::Constant Make::char_(Loc loc, char ch) {
    return constant(loc, tp.tree.tyChar, UINT64(UINT8(ch)));
}

Make::Constant Make::bool_(Loc loc, bool b) {
    return constant(loc, tp.tree.tyBool, UINT64(UINT8(b)));
}

Make::Constant Make::wchar_(Loc loc, wchar_t wch) {
    return constant(loc, tp.tree.tyWChar, UINT64(UINT16(wch)));
}

Make::Constant Make::utf8(Loc loc, BYTE u[4]) {
    auto val = constant(loc, tp.tree.tyUtf8);
    for (auto i = 0; i < 4; ++i) {
        val->utf8[i] = u[i];
    }
    return val;
}

Make::Constant Make::int8(Loc loc, INT8 i8) {
    return constant(loc, tp.tree.tyInt8, UINT64(INT64(i8)));
}

Make::Constant Make::int16(Loc loc, INT16 i16) {
    return constant(loc, tp.tree.tyInt16, UINT64(INT64(i16)));
}

Make::Constant Make::int32(Loc loc, INT32 i32) {
    return constant(loc, tp.tree.tyInt32, UINT64(INT64(i32)));
}

Make::Constant Make::int64(Loc loc, INT64 i64) {
    return constant(loc, tp.tree.tyInt64, UINT64(i64));
}

Make::Constant Make::uint8(Loc loc, UINT8 u8) {
    return constant(loc, tp.tree.tyInt8, UINT64(u8));
}

Make::Constant Make::uint16(Loc loc, UINT16 u16) {
    return constant(loc, tp.tree.tyInt16, UINT64(u16));
}

Make::Constant Make::uint32(Loc loc, UINT32 u32) {
    return constant(loc, tp.tree.tyInt32, UINT64(u32));
}

Make::Constant Make::uint64(Loc loc, UINT64 u64) {
    return constant(loc, tp.tree.tyInt64, u64);
}

Make::Constant Make::float32(Loc loc, float f32) {
    return constant(loc, tp.tree.tyInt64, UINT64(meta::reinterpret<UINT32>(f32)));
}

Make::Constant Make::float64(Loc loc, double f64) {
    return constant(loc, tp.tree.tyInt64, meta::reinterpret<UINT64>(f64));
}
} // namespace stx2ast_pass
} // namespace exy