////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2020-11-16
////////////////////////////////////////////////////////////////

module;
#include "stdafx.h"
export module syntax.tree;

import lib;

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