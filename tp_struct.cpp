#include "pch.h"
#include "typer.h"

namespace exy {
tp_struct::tp_struct(tp_site *site, tp_template_instance_pair &pair)
    : tp(*typer), site(*site), stSyntax(pair.syntaxAs<StructureSyntax>()),
    stSymbol(pair.instanceSymbol), stNode(((TpStruct*)pair.instanceSymbol->node)) {}

void tp_struct::dispose() {

}

void tp_struct::bindParameters() {
    if (site.parameters.list.isEmpty()) {
        Assert(site.arguments.list.isEmpty());
        return;
    }
    Assert(site.parameters.list.length == site.arguments.list.length);
    for (auto i = 0; i < site.parameters.list.length; i++) {
        auto &parameter = site.parameters.list.items[i];
        auto &argument = site.arguments.list.items[i];
        Assert(parameter.syntax != nullptr && parameter.name != nullptr && parameter.type.isKnown());
        if (auto dup = tp.current->scope->contains(parameter.name)) {
            dup_error(parameter.syntax, dup);
        } else if (tp.isa.TypeName(argument.value)) {
            auto parameterSymbol = tp.mk.TypeAlias(parameter.syntax, parameter.name, parameter.type, 
                                                   TpAliasKind::Define);
            stNode->parameters.append(parameterSymbol);
        } else if (auto constExpr = tp.isa.ConstExpr(argument.value)) {
            auto parameterSymbol = tp.mk.ConstAlias(parameter.syntax, parameter.name, constExpr,
                                                   TpAliasKind::Define);
            stNode->parameters.append(parameterSymbol);
        } else {
            Assert(0);
        }
    }
}

void tp_struct::bindSupers() {
    Assert(stSyntax->supers == nullptr);
}

void tp_struct::bindBody() {
    if (auto body = tp.isa.BlockSyntax(stSyntax->body)) {
        // ... 'struct' ... '{' ...
        Assert(body->modifiers == nullptr);
        tp.collectTemplates(body->nodes);
        for (auto i = 0; i < body->nodes.length; i++) {
            bindStatement(body->nodes.items[i]);
        }
    } else if (tp.isa.EmptySyntax(stSyntax->body)) {
        // ... 'struct' ... ';'
    } else {
        syntax_error(stSyntax->body, "expected body of struct");
    }
}

void tp_struct::bindStatement(SyntaxNode *statement) {
    switch (statement->kind) {
        case SyntaxKind::Empty:
            break; // Ok but do nothing.
        case SyntaxKind::Module:
            break; // Ok but do nothing.
        case SyntaxKind::Import: {
            tp.bindImportStatement((ImportSyntax*)statement);
        } break;
        case SyntaxKind::Define: {
            tp.bindDefineStatement((DefineSyntax*)statement);
        } break;
        case SyntaxKind::ExternBlock: {
            auto block = (ExternBlockSyntax*)statement;
            auto  prev = tp.current->pushExternBlock(block);
            for (auto i = 0; i < block->nodes.length; i++) {
                bindStatement(block->nodes.items[i]);
            }
            tp.current->popExternBlock(prev, block);
        } break;
        case SyntaxKind::Structure:
        case SyntaxKind::Function: {
            // Do nothing. Already collected as templates.
        } break;
        case SyntaxKind::Block: {
            auto block = (BlockSyntax*)statement;
            tp.current->pushBlockModifiers(block->modifiers);
            for (auto i = 0; i < block->nodes.length; i++) {
                bindStatement(block->nodes.items[i]);
            }
            tp.current->popBlockModifiers(block->modifiers);
        } break;
        case SyntaxKind::Variable: {
            bindVariable((VariableSyntax*)statement);
        } break;
        case SyntaxKind::CommaSeparated: {
            auto node = (CommaSeparatedSyntax*)statement;
            for (auto i = 0; i < node->nodes.length; i++) {
                bindStatement(node->nodes.items[i]);
            }
        } break;
        default:
            syntax_error(statement, "%sk not expected in %kw scope", statement->kind, Keyword::Struct);
            break;
    }
}

void tp_struct::bindVariable(VariableSyntax *statement) {
    if (tp.current->isStatic() || tp.isa.kwStatic(statement->modifiers)) {
        tp.bindGlobal(statement);
    } else {
        tp.bindField(statement);
    }
}
} // namespace exy