//////////////////////////////////////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2020-12-02
//////////////////////////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "syntax.h"

#include "parser.h"
#include "source.h"

namespace exy {
//------------------------------------------------------------------------------------------------
void SyntaxTree::dispose() {
    ldispose(files);
    mem.dispose();
}
//------------------------------------------------------------------------------------------------
void SyntaxNode::dispose() {
    modifiers = ndispose(modifiers);
}

String SyntaxNode::kindName() const {
    return kindName(kind);
}

String SyntaxNode::kindName(Kind k) {
    switch (k) {
    #define ZM(zName) case SyntaxKind::zName: return { S(#zName), 0u };
        DeclareSyntaxNodes(ZM)
    #undef ZM
    }
    Unreachable();
}
//------------------------------------------------------------------------------------------------
void SyntaxModifiers::dispose() {
    ldispose(nodes);
    __super::dispose();
}
Pos SyntaxModifiers::lastpos() const {
    if (nodes.length) {
        return nodes.last()->lastpos();
    }
    return __super::lastpos();
}
bool SyntaxModifiers::contains(Keyword kw) const {
    for (auto i = 0; i < nodes.length; ++i) {
        auto modifier = nodes.items[i];
        if (modifier->value == kw) {
            return true;
        }
    }
    return false;
}
//------------------------------------------------------------------------------------------------
void SyntaxFile::dispose() {
    mod = ndispose(mod);
    ldispose(nodes);
    __super::dispose();
}

Pos SyntaxFile::lastpos() const {
    if (nodes.length) {
        return nodes.last()->lastpos();
    } if (mod) {
        return mod->lastpos();
    }
    return __super::lastpos();
}

const SourceFile& SyntaxFile::sourceFile() const {
    return pos.loc.file;
}

const List<SourceToken>& SyntaxFile::tokens() const {
    return pos.loc.file.tokens;
}
//------------------------------------------------------------------------------------------------
void SyntaxModule::dispose() {
    name = ndispose(name);
    __super::dispose();
}
Pos SyntaxModule::lastpos() const {
    if (name) {
        return name->lastpos();
    }
    return __super::lastpos();
}
//------------------------------------------------------------------------------------------------
void SyntaxImportOrExport::dispose() {
    name = ndispose(name);
    from = ndispose(from);
    as = ndispose(as);
    __super::dispose();
}
Pos SyntaxImportOrExport::lastpos() const {
    if (from) {
        if (as) {
            auto &fpos = from->lastpos();
            auto &apos = as->lastpos();
            if (fpos.loc.range.end > apos.loc.range.end) {
                return fpos;
            }
            return apos;
        }
        return from->lastpos();
    } if (as) {
        return as->lastpos();
    } if (name) {
        return name->lastpos();
    }
    return __super::lastpos();
}
//------------------------------------------------------------------------------------------------
void SyntaxLet::dispose() {
    name  = ndispose(name);
    value = ndispose(value);
    __super::dispose();
}
Pos SyntaxLet::lastpos() const {
    if (value) {
        return value->lastpos();
    } if (name) {
        return name->lastpos();
    }
    return __super::lastpos();
}
//------------------------------------------------------------------------------------------------
void SyntaxCommaList::dispose() {
    ldispose(nodes);
    __super::dispose();
}
Pos SyntaxCommaList::lastpos() const {
    if (close) {
        return *close;
    } if (nodes.length) {
        return nodes.last()->lastpos();
    }
    return __super::lastpos();
}
//------------------------------------------------------------------------------------------------
void SyntaxUnary::dispose() {
    value = ndispose(value);
    __super::dispose();
}
Pos SyntaxUnaryPrefix::lastpos() const {
    if (value) {
        return value->lastpos();
    }
    return op;
}
Pos SyntaxUnarySuffix::lastpos() const {
    return op;
}
//------------------------------------------------------------------------------------------------
void SyntaxDotExpression::dispose() {
    lhs = ndispose(lhs);
    rhs = ndispose(rhs);
    __super::dispose();
}
Pos SyntaxDotExpression::lastpos() const {
    if (rhs) {
        return rhs->lastpos();
    }
    return dot;
}
//------------------------------------------------------------------------------------------------
void SyntaxArgumentizedExpression::dispose() {
    name = ndispose(name);
    arguments = ndispose(arguments);
    __super::dispose();
}
Pos SyntaxArgumentizedExpression::lastpos() const {
    if (arguments) {
        return arguments->lastpos();
    } if (name) {
        return name->lastpos();
    }
    return __super::lastpos();
}
//------------------------------------------------------------------------------------------------
void SyntaxNameValue::dispose() {
    name     = ndispose(name);
    typeName = ndispose(typeName);
    value    = ndispose(value);
    __super::dispose();
}
Pos SyntaxNameValue::lastpos() const {
    if (value) {
        return value->lastpos();
    } if (assign) {
        return *assign;
    } if (typeName) {
        return typeName->lastpos();
    } if (name) {
        return name->lastpos();
    }
    return __super::lastpos();
}
//------------------------------------------------------------------------------------------------
SyntaxFileProvider::SyntaxFileProvider() 
    : WorkProvider(comp.options.defaultFilesPerThread), files(comp.syntax->files) {}

void SyntaxFileProvider::dispose() {
    pos = 0;
}

bool SyntaxFileProvider::next(List<SyntaxFile*> &batch) {
    AcquireSRWLockExclusive(&srw);
    auto end = min(pos + perBatch, files.length);
    for (; pos < end; ++pos) {
        batch.append(files.items[pos]);
    }
    ReleaseSRWLockExclusive(&srw);
    return batch.isNotEmpty();
}
} // namespace exy