//////////////////////////////////////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2020-12-18
//////////////////////////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "typer.h"

#define err(token, msg, ...) print_error("Make", token, msg, __VA_ARGS__)

namespace exy {
namespace typ_pass {
AstGlobal* Make::global(Loc loc, Identifier name, const AstType &type) {
    auto scope = tp.currentScope();
    if (auto found = scope->find(name)) {
        err(loc, "%s#<red> already defined in the current scope", name);
        return nullptr;
    }
    return tp.mem.New<AstGlobal>(loc, type, scope, name);
}

AstName* Make::name(Loc loc, AstSymbol *symbol) {
    if (tp._names.length) {
        auto last = tp._names.pop();
        return new(last) AstName{ loc, symbol };
    }
    return tp.mem.New<AstName>(loc, symbol);
}
} // namespace typ_pass
} // namespace exy