//////////////////////////////////////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2020-11-25
//////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef SOURCE_H_
#define SOURCE_H_

namespace exy {
//--Begin forward declarations
struct SourceToken;
struct SourceFile;
struct SourceFolder;
//----End forward declarations

struct SourceFile {
    String            source;
    List<SourceToken> tokens;
    SourceFolder     *parent;
    Identifier        path;
    Identifier        name;
    Identifier        dotName;

    SourceFile(SourceFolder *parent, Identifier path, Identifier name) : parent(parent), path(path), name(name) {}
    void dispose();
};

struct SourceFolder {
    List<SourceFile*>   files;
    List<SourceFolder*> folders;
    SourceFolder       *parent;
    Identifier          path;
    Identifier          name;
    Identifier          dotName;

    SourceFolder(SourceFolder *parent, Identifier path, Identifier name) : parent(parent), path(path), name(name) {}
    void dispose();
};

struct SourceTree {
    Mem           mem;
    SourceFolder *root;
    UINT64        bytes;
    int           files;
    int           tokens;

    void dispose();
    bool build();
private:
    SourceFolder* build(SourceFolder *parent, String &path, const String &name, int &indent);
    void readFile(SourceFile *file, int size);
    bool makeDotName(String&, SourceFolder*);
    bool makeDotName(String&, SourceFile*);
    bool isaBadFileName(const String&);
};

struct SourceFileProvider : WorkProvider<SourceFile> {
    List<SourceFile*> files{};
    int               pos{};

    SourceFileProvider();
    void dispose();
    bool next(List<SourceFile*> &batch);
private:
    void collectFiles(SourceFolder *folder);
};
} // namespace exy

#endif // SOURCE_H_