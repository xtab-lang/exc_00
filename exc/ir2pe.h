//////////////////////////////////////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2021-01-21
//////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef IR2PE_H_
#define IR2PE_H_

#include "ir.h"
#include "pe.h"

namespace exy {
namespace ir2pe_pass {
void translateIrTree();

struct Symbol {
    IrSymbol *ir;
    PeSymbol *pe;
};

struct SymbolTable {
    Dict<Symbol, UINT64> list{};

    void dispose();

    void build();
};
using SymTab = SymbolTable&;
} // namespace ir2pe_pass
} // namespace exy

#endif // IR2PE_H_