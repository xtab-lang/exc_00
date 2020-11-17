module;
#include "stdafx.h"
export module syntax;

//--Forward declarations.
struct SyntaxPos;
struct SyntaxNode;
    struct SyntaxFolder;
    struct SyntaxFile;
    struct SyntaxModule;

export struct SyntaxPos final {
    SyntaxFile *file;
};

export struct SyntaxNode {

};

export struct SyntaxFolder final : SyntaxNode {

};

export struct SyntaxFile final : SyntaxNode {

};

export struct SyntaxModule final : SyntaxNode {

};