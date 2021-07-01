#include "pch.h"
#include "syntax.h"

#include "src.h"

#define err(pos, msg, ...) compiler_error("Module", pos, msg, __VA_ARGS__)

namespace exy {
using Path = List<Identifier>;

struct Module {
    Path              path;   // Decomposed '.' separated name of module.
    Identifier        system; // Module output i.e. 'exe', 'dll' etc.
    List<SyntaxFile*> files;  // All the {SyntaxFile}s contributing to {this} module.
    SyntaxFile       *main;   // The 1 and only {SyntaxFile} named 'main.exy' in {this} module.
    SyntaxFile       *init;   // The 1 and only {SyntaxFile} with the same name as {this} module's term.

    Module(Module *parent, Identifier name) { 
        if (parent != nullptr) {
            path.append(parent->path);
        }
        path.append(name); 
    }

    Module(Path &ppath) {
        path.append(ppath);
    }

    void dispose() {
        path.dispose();
        files.dispose();
    }
};

struct Visitor {
    List<Module*> modules{};

    void dispose() {
        modules.dispose([](Module *x) { x->dispose(); MemFree(x); });
    }

    void visitFolder(Module *parent, SyntaxFolder *folder) {
        auto mod = modules.append(MemNew<Module>(parent, folder->src.name));
        for (auto i = 0; i < folder->folders.length; i++) {
            visitFolder(mod, folder->folders.items[i]);
        }
        for (auto i = 0; i < folder->files.length; i++) {
            visitFile(mod, folder->files.items[i]);
        }
    }

    void visitFile(Module *parent, SyntaxFile *file) {
        if (file->moduleStatement != nullptr) {
            visitModuleStatementOfFile(file);
        } else {
            appendFile(parent, file, nullptr);
        }
    }

    void visitModuleStatementOfFile(SyntaxFile *file) {
        Path path{};
        getPathFromModuleName(path, file->moduleStatement->name);
        auto system = file->moduleStatement->system;
        if (auto mod = findModuleFrom(path)) {
            appendFile(mod, file, system);
        } else {
            mod = modules.append(MemNew<Module>(path));
            appendFile(mod, file, system);
        }
        path.dispose();
    }

    void appendFile(Module *parent, SyntaxFile *file, IdentifierSyntax *system) {
        auto parentName = parent->path.last();
        parent->files.append(file);
        if (file->src.name == ids.kw_main) {
            if (parent->main != nullptr) {
                err(file, "a module can only contain 1 file named 'main.exy'");
            } else {
                parent->main = file;
            }
        } else if (file->src.name == parentName) {
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

    Module* findModuleFrom(Path &path) {
        for (auto i = 0; i < modules.length; i++) {
            auto mod = modules.items[i];
            if (isSamePath(mod->path, path)) {
                return mod;
            }
        }
        return nullptr;
    }

    bool isSamePath(Path &a, Path &b) {
        if (a.length == b.length) {
            for (auto i = 0; i < a.length; ++i) {
                if (a.items[i] !=b.items[i]) {
                    return false;
                }
            }
            return true;
        }
        return false;
    }

    void getPathFromModuleName(List<Identifier> &path, SyntaxNode *syntax) {
        if (syntax->kind == SyntaxKind::Identifier) {
            path.append(((IdentifierSyntax*)syntax)->value);
        } else if (syntax->kind == SyntaxKind::Dot) {
            auto dotSyntax = (DotSyntax*)syntax;
            getPathFromModuleName(path, dotSyntax->lhs);
            getPathFromModuleName(path, dotSyntax->rhs);
        } else {
            err(syntax, "expected an identifier or dot syntax");
        }
    }
};
//----------------------------------------------------------
Identifier makeDotName(Path &path) {
    String str{};
    for (auto i = 0; i < path.length; i++) {
        if (i > 0) {
            str.append(S("."));
        }
        str.append(path.items[i]);
    }
    auto id = ids.get(str);
    str.dispose();
    return id;
}

void SyntaxTree::discoverModules() {
    Visitor visitor{};
    for (auto i = 0; i < folders.length; i++) {
        visitor.visitFolder(nullptr, folders.items[i]);
    }
    for (auto i = 0; i < visitor.modules.length; i++) {
        auto mod = visitor.modules.items[i];
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
            node->dotName = makeDotName(mod->path);
            node->name = mod->path.last();
            modules.append(node);
        }
    }
    visitor.dispose();
    if (compiler.errors == 0) {
        printModules();
    }
}

void SyntaxTree::printModules() {
    traceln("Module tree:");
    for (auto i = 0; i < modules.length; i++) {
        printModule(modules.items[i]);
    }
}

void SyntaxTree::printModule(SyntaxModule *mod) {
    traceln("  %c#<cyan> %s#<magenta>", "module", mod->dotName);
    for (auto i = 0; i < mod->files.length; i++) {
        auto file = mod->files.items[i];
        traceln("    %s#<yellow underline>", file->src.dotName);
    }
}
} // namespace exy