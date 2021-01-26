//////////////////////////////////////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2021-01-21
//////////////////////////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "ir2pe.h"

namespace exy {
namespace ir2pe_pass {
void SymbolTable::dispose() {
    list.dispose();
}

void SymbolTable::build() {
    auto &mem = comp.pe->mem;
    for (auto i = 0; i < comp.ir->modules.length; ++i) {
        auto ir = comp.ir->modules.items[i];
        auto pe = mem.New<PeModule>(ir->loc, ir->name);
        list.append(UINT64(ir), { ir, pe });
    }
}
//------------------------------------------------------------------------------------------------
static bool buildSymbolTable(SymTab symtab) {
    traceln("\r\n%cl#<cyan|blue> { phase: %cl#<green>, thread: %u#<green> }",
        S("ir2pe"), S("building the ir → pe symbol table"), GetCurrentThreadId());

    traceln("%cl#<cyan|blue> { errors: %i#<red>, symbols: %i#<green> }",
        S("ir2pe"), comp.errors, symtab.list.length);

    return comp.errors == 0;
}

static bool visitSymbolTable(SymTab symtab) {
    traceln("\r\n%cl#<cyan|blue> { phase: %cl#<green>, threads: %u#<green> }",
        S("ir2pe"), S("ir → pe"), aio::ioThreads());

    traceln("%cl#<cyan|blue> { errors: %i#<red> }",
        S("ir2pe"), comp.errors);

    return comp.errors == 0;
}

void translateIrTree() {
    SymbolTable symtab{};

    if (buildSymbolTable(symtab)) {
        if (visitSymbolTable(symtab)) {

        }
    }

    symtab.dispose();
}
} // namespace ir2pe_pass
} // namespace exy