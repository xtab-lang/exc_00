//////////////////////////////////////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2020-12-17
//////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef TP_CAST_H_
#define TP_CAST_H_

namespace exy {
namespace typ_pass {
//--Begin forward declarations
//----End forward declarations
struct Cast {
    Typer &tp;

    Cast(Typer &tp) : tp(tp) {}

    AstNode* explicitCast(Loc, AstNode*, const AstType&);
    AstNode* implicitCast(Loc, AstNode*, const AstType&);
private:
};
} // namespace typ_pass
} // namespace exy

#endif // TP_CAST_H_