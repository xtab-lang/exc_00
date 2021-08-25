#include "pch.h"
#include "syntax.h"

#include "src.h"

#define err(pos, msg, ...) diagnostic("Module", pos, msg, __VA_ARGS__)

namespace exy {
using Path = List<Identifier>;

struct Module {
    Module           *parent; // The immediate parent {Module} of {this} module.
    Identifier        name;   // Single string name of module.
    Identifier        system; // Module output i.e. 'exe', 'dll' etc.
    Dict<Module*>     modules;// All the child {Module}s of {this} module.
    List<SyntaxFile*> files;  // All the {SyntaxFile}s contributing to {this} module.
    SyntaxFile       *main;   // The 1 and only {SyntaxFile} named 'main.exy' in {this} module.
    SyntaxFile       *init;   // The 1 and only {SyntaxFile} with the same name as {this} module's term.
    SyntaxModule     *syntax;

    Module(Module *parent, Identifier name) : parent(parent), name(name) {}

    void dispose() {
        modules.dispose([](auto x) { x->dispose(); MemFree(x); });
        files.dispose();
    }

    Module* contains(Identifier id) {
        auto idx = modules.indexOf(id);
        if (idx >= 0) {
            return modules.items[idx].value;
        }
        return nullptr;
    }
};

struct Visitor {
    Module *root;

    void initialize() {
        root = MemNew<Module>(nullptr, ids.kw_star);
    }

    void dispose() {
        if (root != nullptr) {
            root->dispose();
            root = MemFree(root);
        }
    }

    void visitFolder(Module *parent, SyntaxFolder *folder) {
        auto   name = folder->src.name;
        auto    idx = parent->modules.indexOf(name);
        Module *mod = nullptr;
        if (idx >= 0) {
            mod = parent->modules.items[idx].value;
        } else {
            mod = parent->modules.append(name, MemNew<Module>(parent, name));
        }
        for (auto i = 0; i < folder->folders.length; i++) {
            visitFolder(mod, folder->folders.items[i]);
        }
        for (auto i = 0; i < folder->files.length; i++) {
            visitFile(mod, folder->files.items[i]);
        }
    }

    void visitFile(Module *parent, SyntaxFile *file) {
        if (file->moduleStatement != nullptr) {
            visitModuleStatementOfFile(parent, file);
        } else {
            appendFile(parent, file, nullptr);
        }
    }

    void visitModuleStatementOfFile(Module *parent, SyntaxFile *file) {
        Path path{};
        getPathFromModuleName(path, file->moduleStatement->name);
        auto system = file->moduleStatement->system;
        auto    mod = findOrAppendModule(parent, path, 0);
        appendFile(mod, file, system);
        path.dispose();
    }

    Module* findOrAppendModule(Module *parent, Path &path, INT j) {
        const auto isFirst = j == 0;
        if (isFirst) {
            const auto name = path.first();
            for (auto p = parent; p != nullptr; p = p->parent) {
                if (p->name == name) {
                    return findOrAppendModule(p, path, j + 1);
                }
                if (auto found = p->contains(name)) {
                    return findOrAppendModule(found, path, j + 1);
                }
            }
            auto mod = MemNew<Module>(root, name);
            root->modules.append(name, mod);
            return findOrAppendModule(mod, path, j + 1);
        }
        const auto isLast = j == path.length;
        if (isLast) {
            return parent;
        }
        Assert(j < path.length);
        const auto name = path.items[j];
        const auto  idx = parent->modules.indexOf(name);
        if (idx < 0) { // Not found.
            auto mod = MemNew<Module>(parent, name);
            parent->modules.append(name, mod);
            return findOrAppendModule(mod, path, j + 1);
        }
        auto mod = parent->modules.items[idx].value;
        return findOrAppendModule(mod, path, j + 1);
    }

    void appendFile(Module *parent, SyntaxFile *file, IdentifierSyntax *system) {
        parent->files.append(file);
        if (file->src.name == ids.kw_main) {
            if (parent->main != nullptr) {
                err(file, "a module can only contain 1 file named 'main.exy'");
            } else {
                parent->main = file;
            }
        } else if (file->src.name == parent->name) {
            if (parent->init != nullptr) {
                err(file, "a module can only contain 1 file with the same name as the module's term");
            } else {
                parent->init = file;
            }
        }
        if (system != nullptr) {
            if (parent->system == nullptr) {
                parent->system = system->value;
            } else if (parent->system != system->value) {
                err(system, "conflicting module sub-system declarations; was previously %s#<green> and is now %s#<red>",
                    parent->system, system->value);
            }
        }
    }

    void getPathFromModuleName(List<Identifier> &path, SyntaxNode *syntax) {
        if (syntax->kind == SyntaxKind::Identifier) {
            path.append(((IdentifierSyntax*)syntax)->value);
        } else if (syntax->kind == SyntaxKind::Dot) {
            auto dotSyntax = (DotSyntax*)syntax;
            getPathFromModuleName(path, dotSyntax->lhs);
            getPathFromModuleName(path, dotSyntax->rhs);
        } else {
            syntax_error(syntax, "expected an identifier or dot syntax");
        }
    }
};
//----------------------------------------------------------
static INT createSyntaxModules(String &dotName, Module *parent) {
    auto mains = 0;
    auto   eos = dotName.length;
    if (dotName.isNotEmpty()) {
        dotName.append(S("."));
    }
    auto &tree = *compiler.syntaxTree;
    auto   eod = dotName.length;
    auto  &mem = tree.mem;
    for (auto i = 0; i < parent->modules.length; i++) {
        auto mod = parent->modules.items[i].value;
        if (dotName.isNotEmpty()) {
            dotName.text[dotName.length = eod] = '\0';
        }
        dotName.append(mod->name);
        if (mod->files.isNotEmpty()) {
            SyntaxModule *node = nullptr;
            if (mod->main != nullptr || mod->init != nullptr) {
                node = mem.New<SyntaxModule>(mod->main, mod->init);
            } else {
                node = mem.New<SyntaxModule>(mod->files.first());
            }
            for (auto j = 0; j < mod->files.length; j++) {
                node->files.append(mod->files.items[j]);
            }
            node->dotName = ids.get(dotName);
            node->name    = mod->name;
            if (mod->system == nullptr) {
                node->system = ids.kw_dll;
            } else {
                node->system = mod->system;
            }
            if (parent->syntax == nullptr) {
                tree.modules.append(node);
            } else {
                parent->syntax->modules.append(node);
            }
            mod->syntax = node;
            if (mod->main != nullptr) {
                ++mains;
            }
            mains += createSyntaxModules(dotName, mod);
        } else {
            traceln("%c#<red>: folder %s#<yellow> is empty (all sub-folders and their files ignored)", "warning", dotName);
        }
    }
    dotName.text[dotName.length = eos] = '\0';
    return mains;
}

static auto createSyntaxModules(Visitor &visitor) {
    String dotName{};
    auto &tree = *compiler.syntaxTree;
    auto mains = createSyntaxModules(dotName, visitor.root);
    dotName.dispose();
    if (mains == 0) {
        err(tree.modules.first()->pos, "no 'main' file in any module");
    }
}
//----------------------------------------------------------

void SyntaxTree::discoverModules() {
    Visitor visitor{};
    visitor.initialize();
    for (auto i = 0; i < folders.length; i++) {
        visitor.visitFolder(visitor.root, folders.items[i]);
    }
    if (compiler.errors == 0) {
        createSyntaxModules(visitor);
    }
    visitor.dispose();
    if (compiler.errors == 0) {
        printModules();
    }
}

void SyntaxTree::printModules() {
    traceln("Module tree:");
    for (auto i = 0; i < modules.length; i++) {
        printModule(1, modules.items[i]);
    }
}

void SyntaxTree::printModule(INT indent, SyntaxModule *mod) {
    for (auto i = 0; i < indent; ++i) trace("  ");
    traceln("%c#<cyan> %s#<magenta> %c#<cyan> %s", "module", mod->dotName, "as", mod->system);
    for (auto i = 0; i < mod->files.length; i++) {
        auto file = mod->files.items[i];
        for (auto j = 0; j < indent; ++j) trace("  ");
        trace("  %s#<yellow underline>", file->src.dotName);
        if (file->src.name == mod->name) {
            trace(" ; %c#<darkyellow>", "init");
        } else if (file->src.name == ids.kw_main) {
            trace(" ; %c#<darkyellow>", "main");
        }
        traceln("");
    }
    for (auto i = 0; i < mod->modules.length; i++) {
        printModule(indent + 1, mod->modules.items[i]);
    }
}
} // namespace exy