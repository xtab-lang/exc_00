//////////////////////////////////////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2020-12-18
//////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef TP_LITERAL_H_
#define TP_LITERAL_H_

namespace exy {
namespace typ_pass {
//--Begin forward declarations
struct Typer;
//----End forward declarations
struct Literal {
    Typer &tp;

    Literal(Typer *tp) : tp(*tp) {}

    AstLiteral* visit(SyntaxLiteral*);
};
} // namespace typ_pass
} // namespace exy

#endif // TP_VARIABLES_H_