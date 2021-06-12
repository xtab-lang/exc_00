#pragma once

namespace exy {
struct SourceTree;
struct SourceFolder;
struct SourceFile;
//----------------------------------------------------------
struct SourceTree {
    Mem                 mem{};
    List<SourceFolder*> folders{};

    bool initialize();
    void dispose();

private:
    void visitSourceFolder(Identifier);
    void printTree();
    void printFolder(SourceFolder*, INT indent);

    void tokenize();
    void tokenize(SourceFolder*);
    void tokenize(SourceFile&);
};
//----------------------------------------------------------
struct SourceFolder {
    SourceFolder       *parent;
    Identifier          path;
    Identifier          name;
    List<SourceFile>    files;
    List<SourceFolder*> folders;

    SourceFolder(SourceFolder *parent, Identifier path, Identifier name) :
        parent(parent), path(path), name(name) {}

    void initialize();
    void dispose();
};
//----------------------------------------------------------
struct SourceFile {
    List<SourceToken> tokens;
    String            source;
    SourceFolder     *parent;
    Identifier        path;
    Identifier        name;
    INT               lines{};
    INT               characters{};

    SourceFile(SourceFolder *parent, Identifier path, Identifier name) :
        parent(parent), path(path), name(name) {}

    void initialize();
    void dispose();
};
//----------------------------------------------------------
} // namespace exy
