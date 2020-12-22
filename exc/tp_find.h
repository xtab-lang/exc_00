//////////////////////////////////////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2020-12-20
//////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef TP_FIND_H_
#define TP_FIND_H_

namespace exy {
namespace typ_pass {
struct Find {
    Typer &tp;

    Find(Typer *tp) : tp(*tp) {}

    AstNode* name(SyntaxIdentifier*);
    AstNode* name(Loc, Identifier);
};
} // namespace typ_pass
} // namespace exy

#endif // TP_FIND_H_