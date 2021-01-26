//////////////////////////////////////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2021-01-17
//////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef PE_H
#define PE_H

namespace exy {
//--Begin forward declarations

#define DeclarePeSections(ZM)   \
    ZM(CodeSection)             \
    ZM(DataSection)             \
    ZM(ImportSection)           \
    ZM(ExportSection)
struct PeSection;

#define DeclarePeSymbols(ZM)    \
    DeclarePeSections(ZM)       \
    ZM(Module)
struct PeSymbol;

#define DeclarePeNodes(ZM)      \
    DeclarePeSymbols(ZM)

enum class PeKind {
#define ZM(zName) zName,
    DeclarePeNodes(ZM)
#undef ZM
};

#define ZM(zName) struct Pe##zName;
DeclarePeNodes(ZM)
#undef ZM

//----End forward declarations 

struct PeTree {
    Mem             mem;
    List<PeModule*> modules;

    PeTree();
    void dispose();
};

struct PeNode {
    using Kind = PeKind;

    SourceLocation loc;
    Kind           kind;

    PeNode(Loc loc, Kind kind) : loc(loc), kind(kind) {}
    virtual void dispose() {}
};

struct PeSymbol : PeNode {
    Identifier name;

    PeSymbol(Loc loc, Kind kind, Identifier name) : PeNode(loc, kind), name(name) {}
};

struct PeBuffer {
    BYTE *data{};
    int   length{};
    int   capacity{};

    void dispose();
};

struct PeSection : PeSymbol {
    PeBuffer buffer;

    PeSection(Loc loc, Kind kind, Identifier name) : PeSymbol(loc, kind, name) {}
    void dispose();
};

struct PeCodeSection : PeSection {
    PeCodeSection(Loc loc) : PeSection(loc, Kind::CodeSection, ids.dot.text) {}
};

struct PeDataSection : PeSection {
    PeDataSection(Loc loc) : PeSection(loc, Kind::DataSection, ids.dot.data) {}
};

struct PeImportSection : PeSection {
    PeImportSection(Loc loc) : PeSection(loc, Kind::ImportSection, ids.dot.idata) {}
};

struct PeExportSection : PeSection {
    PeExportSection(Loc loc) : PeSection(loc, Kind::ExportSection, ids.dot.edata) {}
};

//------------------------------------------------------------------------------------------------
struct PeModule : PeSymbol {
    PeCodeSection   *text;
    PeDataSection   *data;
    PeImportSection *idata;
    PeExportSection *edata;

    PeModule(Loc loc, Identifier name);
    void dispose();
};
} // namespace exy

#endif // PE_H