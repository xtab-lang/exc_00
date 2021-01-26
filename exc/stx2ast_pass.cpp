//////////////////////////////////////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2020-12-12
//////////////////////////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "stx2ast_pass.h"

#include "typer.h"
#include "source.h"

#define err(token, msg, ...) print_error("Module", token, msg, __VA_ARGS__)

namespace exy {
namespace stx2ast_pass {
struct Module {
    Dict<Module*>     modules;
    List<SyntaxFile*> files;
    Module           *parent;
    Identifier        name;
    Identifier        dotName;
    SyntaxFile       *main;

    void dispose() {
        modules.dispose([](auto x) { x->dispose(); MemFree(x); });
        files.dispose();
    }
};

struct ModuleTree {
    Dict<Module*> modules{};

    void dispose() {
        modules.dispose([](auto x) { x->dispose(); MemFree(x); });
    }

    void append(SyntaxFile *file) {
        auto  mod = file->mod;
        auto path = getModulePath(mod->name);
        append(path, file);
        path.dispose();
    }

    void appendMain() {
        auto     mod = MemAlloc<Module>();
        mod->name    = ids.main;
        mod->dotName = ids.main;
        modules.append(ids.main, mod);
    }

    void append(List<Identifier> &path, SyntaxFile *file) {
        Module *mod{};
        for (auto i = 0; i < path.length; ++i) {
            auto name = path.items[i];
            if (mod) {
                if (mod->name != name) {
                    auto idx = mod->modules.indexOf(name);
                    if (idx >= 0) {
                        mod = mod->modules.items[idx].value;
                    } else {
                        auto child = MemAlloc<Module>();
                        child->name = name;
                        child->dotName = makeDotName(path, i);
                        child->parent = mod;
                        mod->modules.append(name, child);
                        mod = child;
                    }
                }
            } else {
                auto idx = modules.indexOf(name);
                if (idx >= 0) {
                    mod = modules.items[idx].value;
                } else {
                    mod = MemAlloc<Module>();
                    mod->name = name;
                    mod->dotName = makeDotName(path, i);
                    modules.append(name, mod);
                }
            }
        } if (mod->name == file->sourceFile().name) {
            mod->main = file;
        }
        mod->files.append(file);
    }

    List<Identifier> getModulePath(SyntaxNode *node) {
        List<Identifier> list{};
        if (node->kind == SyntaxKind::Identifier) {
            putIdentifier(list, (SyntaxIdentifier*)node);
        } else if (node->kind == SyntaxKind::DotExpression) {
            putDotExpression(list, (SyntaxDotExpression*)node);
        } else {
            Assert(0);
        }
        Assert(list.length);
        return list;
    }

    Identifier makeDotName(List<Identifier> &path, int j) {
        String str{};
        for (auto i = 0; i <= j; ++i) {
            if (str.length) {
                str.append(S("."));
            }
            str.append(path.items[i]);
        }
        auto id = ids.get(str);
        str.dispose();
        return id;
    }

    void putDotExpression(List<Identifier> &path, SyntaxDotExpression *node) {
        auto lhs = node->lhs;
        auto rhs = node->rhs;
        if (lhs) {
            if (lhs->kind == SyntaxKind::Identifier) {
                putIdentifier(path, (SyntaxIdentifier*)lhs);
            } else if (lhs->kind == SyntaxKind::DotExpression) {
                putDotExpression(path, (SyntaxDotExpression*)lhs);
            } else {
                Assert(0);
            }
        } else {
            putFolderPath(path, node->pos.loc.file.parent);
        } if (rhs) {
            if (rhs->kind == SyntaxKind::Identifier) {
                putIdentifier(path, (SyntaxIdentifier*)rhs);
            } else if (rhs->kind == SyntaxKind::DotExpression) {
                putDotExpression(path, (SyntaxDotExpression*)rhs);
            } else {
                Assert(0);
            }
        } else {
            Assert(0);
        }
    }

    void putFolderPath(List<Identifier> &path, const SourceFolder *folder) {
        if (auto parent = folder->parent) {
            putFolderPath(path, parent);
            path.append(folder->name);
        }
    }

    void putIdentifier(List<Identifier> &path, SyntaxIdentifier *node) {
        auto id = (SyntaxIdentifier*)node;
        if (id) {
            if (path.length) {
                auto last = path.last();
                if (last == id->value) {
                    return;
                }
            }
            path.append(id->value);
        } else {
            Assert(path.isEmpty());
            path.append(nullptr);
        }
    }

    void print(Module *mod, int indent) {
        for (auto i = 0; i < indent; ++i) trace("   ");
        traceln("%s#<underline cyan> [%s#<cyan>]", mod->name, mod->dotName);
        for (auto i = 0; i < mod->files.length; ++i) {
            for (auto j = 0; j <= indent; ++j) trace("   ");
            auto    file = mod->files.items[i];
            auto &source = file->sourceFile();
            if (file == mod->main) {
                traceln("%s#<underline darkyellow>", source.path);
            } else {
                traceln("%s#<underline yellow>", source.path);
            }
        } for (auto i = 0; i < mod->modules.length; ++i) {
            print(mod->modules.items[i].value, indent + 1);
        }
    }
};
//------------------------------------------------------------------------------------------------
static ModuleTree* discoverModuleTree() {
    traceln("\r\n%cl#<cyan|blue> { phase: %cl#<green>, files: %i#<green>, thread: %u#<green> }",
            S("typer"), S("discovering the module tree"), comp.syntax->files.length,
            GetCurrentThreadId());
    auto    tree = MemNew<ModuleTree>();
    auto &syntax = *comp.syntax;
    tree->appendMain();
    for (auto i = 0; i < syntax.files.length; ++i) {
        tree->append(syntax.files.items[i]);
    } for (auto i = 0; i < tree->modules.length; ++i) {
        tree->print(tree->modules.items[i].value, 1);
    }
    traceln("%cl#<cyan|blue> { errors: %i#<red>, topLevelModules: %i#<green> }", S("typer"),
            comp.errors, tree->modules.length);
    return tree;
}
//------------------------------------------------------------------------------------------------
static bool modifiersAreTheSame(SyntaxNode *a, SyntaxNode *b) {
    if (a->kind == SyntaxKind::Modifier) {
        if (b->kind != SyntaxKind::Modifier) {
            return false;
        }
        auto am = (SyntaxModifier*)a;
        auto bm = (SyntaxModifier*)b;
        return am->value == bm->value;
    } if (a->kind == SyntaxKind::Modifiers) {
        if (b->kind != SyntaxKind::Modifiers) {
            return false;
        }
        auto am = (SyntaxModifiers*)a;
        auto bm = (SyntaxModifiers*)b;
        for (auto i = 0; i < am->nodes.length; ++i) {
            if (!bm->contains(am->nodes.items[i]->value)) {
                return false;
            }
        } for (auto i = 0; i < bm->nodes.length; ++i) {
            if (!am->contains(bm->nodes.items[i]->value)) {
                return false;
            }
        }
        return true;
    }
    Unreachable();
}
static void validateModule(Module *mod) {
    SyntaxNode *firstModifiers{};
    for (auto i = 0; i < mod->files.length; ++i) {
        auto file = mod->files.items[i];
        if (auto node = file->mod) {
            if (auto modifiers = node->modifiers) {
                if (firstModifiers) {
                    if (!modifiersAreTheSame(firstModifiers, modifiers)) {
                        err(file, "different module modifiers in %s#<underline yellow> and %s#<underline yellow>; all module files must contain the same module statement", 
                            file->sourceFile().dotName, firstModifiers->pos.loc.file.dotName);
                    }
                } else if (i == 0) {
                    firstModifiers = modifiers;
                } else {
                    err(modifiers, "unexpected modifiers for module; all module files must contain the same module statement");
                }
            } else if (firstModifiers) {
                err(file, "%s#<underline yellow> requires a module statement matching %s#<underline yellow>", 
                    file->sourceFile().dotName, firstModifiers->pos.loc.file.dotName);
            } else if (i == 0) {
                // OK. First file. No modifiers.
            } else {
                // OK. Subsequent file. No modifiers still.
            }
        } else {
            Assert(0); // All files must have a 'module' statement.
        }
    } for (auto i = 0; i < mod->modules.length; ++i) {
        validateModule(mod->modules.items[i].value);
    }
}
static bool validateModuleTree(ModuleTree *tree) {
    for (auto i = 0; i < tree->modules.length; ++i) {
        validateModule(tree->modules.items[i].value);
    }
    return comp.errors == 0;
}
//------------------------------------------------------------------------------------------------
static SyntaxFile* findFirstFileOf(Module *mod) {
    if (auto found = mod->main) {
        return found;
    } if (mod->files.length) {
        return mod->files.first();
    } for (auto i = 0; i < mod->modules.length; ++i) {
        if (auto found = findFirstFileOf(mod->modules.items[i].value)) {
            return found;
        }
    }
    Unreachable();
}
static int enterModule(AstModule *parent, Module *mod) {
    auto modules = 0;
    for (auto i = 0; i < mod->modules.length; ++i) {
        auto   child = mod->modules.items[i].value;
        auto    file = findFirstFileOf(child);
        auto &tokens = file->sourceFile().tokens;
        auto  &start = tokens.first().loc.range.start;
        auto    &end = tokens.last().loc.range.end;
        SourceLocation loc{ file->pos.loc.file, start, end };

        auto idx = parent->ownScope->symbols.indexOf(mod->name);
        if (idx >= 0) {
            err(loc, "identifier %s#<red> already defined", child->name);
            continue;
        }

        auto  ast = comp.ast->mem.New<AstModule>(loc, parent->ownScope, child->name, child->dotName, 
                                                 child->files, child->main);

        trace("%s#<cyan> [", ast->dotName);
        for (auto j = 0; j < child->files.length; ++j) {
            if (j) trace(", ");
            trace("%s#<underline yellow>", child->files.items[j]->sourceFile().dotName);
        }
        traceln("]");

        modules += enterModule(ast, child) + 1;
    }
    return modules;
}
static void createAstTree(ModuleTree *moduleTree) {
    traceln("\r\n%cl#<cyan|blue> { phase: %cl#<green>, thread: %u#<green> }",
            S("typer"), S("initializing the AST with modules"), GetCurrentThreadId());
    comp.ast = MemAlloc<AstTree>();
    auto modules = 0;
    auto  global = comp.ast->global;
    for (auto i = 0; i < moduleTree->modules.length; ++i) {
        auto     mod = moduleTree->modules.items[i].value;
        auto    file = findFirstFileOf(mod);
        auto &tokens = file->sourceFile().tokens;
        auto  &start = tokens.first().loc.range.start;
        auto    &end = tokens.last().loc.range.end;
        SourceLocation loc{ file->pos.loc.file, start, end };

        if (i == 0) {
            comp.ast->initialize(loc);
            global = comp.ast->global;
        }
        if (global->name == mod->name) {
            enterModule(global, mod);
            continue;
        }
        auto idx = global->ownScope->symbols.indexOf(mod->name);
        if (idx >= 0) {
            err(loc, "identifier %s#<red> already defined", mod->name);
            continue;
        }

        auto ast = comp.ast->mem.New<AstModule>(loc, global->ownScope, mod->name, mod->dotName, 
                                                mod->files, mod->main);

        trace("%s#<cyan> [", ast->dotName);
        for (auto j = 0; j < mod->files.length; ++j) {
            if (j) trace(", ");
            trace("%s#<underline yellow>", mod->files.items[j]->sourceFile().dotName);
        }
        traceln("]");

        modules += enterModule(ast, mod) + 1;
    }
    traceln("%cl#<cyan|blue> { errors: %i#<red>, modules: %i#<green> }",
            S("typer"), comp.errors, modules);
}
//------------------------------------------------------------------------------------------------
static void disposeSyntaxFilesOfAstModules(AstModule *parent) {
    parent->syntax.dispose();
    for (auto i = 0; i < parent->ownScope->symbols.length; ++i) {
        auto symbol = parent->ownScope->symbols.items[i].value;
        if (symbol->kind == AstKind::Module) {
            disposeSyntaxFilesOfAstModules((AstModule*)symbol);
        }
    }
}
//------------------------------------------------------------------------------------------------
bool run() {
    if (auto moduleTree = discoverModuleTree()) {
        if (validateModuleTree(moduleTree)) {
            createAstTree(moduleTree);

            Typer typer{};
            typer.run();
            typer.dispose();

            disposeSyntaxFilesOfAstModules(comp.ast->global);
        }
        moduleTree->dispose();
        MemFree(moduleTree);

        comp.syntax = MemDispose(comp.syntax);
    }

    return comp.errors == 0;
}
} // namespace stx2ast_pass
} // namespace exy