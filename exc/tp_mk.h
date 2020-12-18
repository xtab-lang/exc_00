//////////////////////////////////////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2020-12-18
//////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef TP_MK_H_
#define TP_MK_H_

namespace exy {
namespace typ_pass {
//--Begin forward declarations
struct Typer;
//----End forward declarations
struct Make {
    Typer &tp;

    Make(Typer *tp) : tp(*tp) {}

    AstGlobal* global(Loc loc, Identifier name, const AstType &type);

    AstName* name(Loc loc, AstSymbol *symbol);
};
} // namespace typ_pass
} // namespace exy

#endif // TP_MK_H_