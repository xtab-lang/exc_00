//////////////////////////////////////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2021-01-17
//////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once
#include "pch.h"
#include "pe.h"

namespace exy {
PeTree::PeTree() {
    comp.pe = this;
}

void PeTree::dispose() {
    ldispose(modules);
    mem.dispose();
}
//------------------------------------------------------------------------------------------------
void PeBuffer::dispose() {
    data = MemFree(data);
}
//------------------------------------------------------------------------------------------------
void PeSection::dispose() {
    buffer.dispose();
}
//------------------------------------------------------------------------------------------------
PeModule::PeModule(Loc loc, Identifier name) : PeSymbol(loc, Kind::Module, name) {
    comp.pe->modules.append(this);
    auto &mem = comp.pe->mem;
    text = mem.New<PeCodeSection>(loc);
    data = mem.New<PeDataSection>(loc);
    idata = mem.New<PeImportSection>(loc);
    edata = mem.New<PeExportSection>(loc);
}

void PeModule::dispose() {
    text  = ndispose(text);
    data  = ndispose(data);
    idata = ndispose(idata);
    edata = ndispose(edata);
}
} // namespace exy