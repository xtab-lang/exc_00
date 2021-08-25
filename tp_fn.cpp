#include "pch.h"
#include "typer.h"

namespace exy {
tp_fn_body::tp_fn_body() : tp(*typer) {}

void tp_fn_body::bind(SyntaxNode *syntax) {
    if (syntax == nullptr) {
        return;
    }
    if (auto block = tp.isa.BlockSyntax(syntax)) {
        bind(block->nodes);
    } else if (syntax != nullptr) {
        bindStatement(syntax);
    }
}

void tp_fn_body::bind(List<SyntaxNode*> &list) {
    tp.collectTemplates(list);
    for (auto i = 0; i < list.length; i++) {
        bindStatement(list.items[i]);
    }
}

void tp_fn_body::bindStatement(SyntaxNode *syntax) {
    switch (syntax->kind) {
        case SyntaxKind::Empty:
            break; // Ok but do nothing.
        case SyntaxKind::Module: {
            syntax_error(syntax, "unexpected statement in function scope: %sk", syntax->kind);
        } break;
        case SyntaxKind::Import: {
            tp.bindImportStatement((ImportSyntax*)syntax);
        } break;
        case SyntaxKind::Define: {
            tp.bindDefineStatement((DefineSyntax*)syntax);
        } break;
        case SyntaxKind::ExternBlock: {
            auto block = (ExternBlockSyntax*)syntax;
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
            auto block = (BlockSyntax*)syntax;
            if (block->arguments != nullptr) {
                bindBlockWithArguments(block);
                break;
            }
            tp.current->pushBlockModifiers(block->modifiers);
            for (auto i = 0; i < block->nodes.length; i++) {
                bindStatement(block->nodes.items[i]);
            }
            tp.current->popBlockModifiers(block->modifiers);
        } break;
        case SyntaxKind::FlowControl: {
            auto node = (FlowControlSyntax*)syntax;
            switch (node->pos.keyword) {
                case Keyword::Assert: {
                    tp.bindAssertStatement(node);
                } break;
                case Keyword::Return: {
                    tp.bindReturnStatement(node);
                } break;
                case Keyword::Break: {
                    tp.bindBreakStatement(node);
                } break;
                case Keyword::Continue: {
                    tp.bindContinueStatement(node);
                } break;
                default: {
                    syntax_error(syntax, "unexpected flow-control statement in function scope: %sk", syntax->kind);
                } break;
            }
        } break;
        case SyntaxKind::If: {
            tp.bindIfStatement((IfSyntax*)syntax);
        } break;
        case SyntaxKind::Switch: {
            Assert(0);
        } break;
        case SyntaxKind::ForIn: {
            tp.bindForInStatement((ForInSyntax*)syntax);
        } break;
        case SyntaxKind::For: {
            tp.bindForStatement((ForSyntax*)syntax);
        } break;
        case SyntaxKind::While: {
            tp.bindWhileStatement((WhileSyntax*)syntax);
        } break;
        case SyntaxKind::DoWhile: {
            Assert(0);
        } break;
        case SyntaxKind::Defer: {
            Assert(0);
        } break;
        case SyntaxKind::Using: {
            Assert(0);
        } break;
        case SyntaxKind::Variable: {
            tp.bindLocal((VariableSyntax*)syntax);
        } break;
        case SyntaxKind::CommaSeparated: {
            auto node = (CommaSeparatedSyntax*)syntax;
            for (auto i = 0; i < node->nodes.length; i++) {
                bindStatement(node->nodes.items[i]);
            }
        } break;
        case SyntaxKind::UnaryPrefix: {
            auto node = (UnaryPrefixSyntax*)syntax;
            if (node->pos.keyword == Keyword::Yield) {
                tp.bindYieldStatement(node);
            } else if (auto value = tp.bindExpression(syntax)) {
                tp.current->scope->append(value);
            }
        } break;
        default: if (auto value = tp.bindExpression(syntax)) {
            tp.current->scope->append(value);
        } break;
    }
}

void tp_fn_body::bindBlockWithArguments(BlockSyntax *syntax) {
    if (auto modifier = tp.isa.ModifierSyntax(syntax->modifiers)) {
        if (modifier->value == Keyword::Synchronized) {
            auto block = tp.mk.Block(syntax);
            tp.current->scope->append(block);
            tp_current current{ block->scope };
            if (tp.enter(current)) {
                auto argumentsSyntax = syntax->arguments;
                const auto    errors = compiler.errors;
                List<TpNode*> arguments{};
                List<SyntaxNode*> argumentSyntax{};
                if (argumentsSyntax->value == nullptr) {
                    // Do nothing.
                } else if (auto list = tp.isa.CommaSeparatedSyntax(argumentsSyntax->value)) {
                    for (auto i = 0; i < list->nodes.length; i++) {
                        if (auto argument = tp.bindExpression(list->nodes.items[i])) {
                            arguments.append(argument);
                            argumentSyntax.append(list->nodes.items[i]);
                        }
                    }
                } else if (auto argument = tp.bindExpression(argumentsSyntax->value)) {
                    arguments.append(argument);
                    argumentSyntax.append(argumentsSyntax->value);
                }
                if (errors == compiler.errors) {
                    for (auto i = 0; i < arguments.length; i++) {
                        auto argument = arguments.items[i];
                        tp_site site{ argumentSyntax.items[i] };
                        site.synchronize(argument);
                        site.dispose();
                    }
                    arguments.dispose(); // tp_site::synchronize will have consumed all of them.
                    argumentSyntax.dispose();
                    for (auto i = 0; i < syntax->nodes.length; i++) {
                        bindStatement(syntax->nodes.items[i]);
                    }
                } else {
                    tp.throwAway(arguments);
                    argumentSyntax.dispose();
                }
                tp.leave(current);
            }
            return;
        }
    }
    syntax_error(syntax->modifiers, "expected %kw", Keyword::Synchronized);
}

//----------------------------------------------------------
tp_if_block::tp_if_block(IfSyntax *syntax) : tp(*typer), syntax(syntax) {}

void tp_if_block::bind() {
    auto block = tp.mk.IfBlock(syntax);
    tp_current current{ block->scope };
    if (tp.enter(current)) {
        block->condition = tp.bindCondition(syntax->condition);
        tp_fn_body body{};
        body.bind(syntax->iftrue);
        body.dispose();
        tp.leave(current);
    }
    if (auto ifalse = syntax->ifalse) {
        tp_fn_body body{};
        if (tp.isa.BlockMaker(ifalse)) {
            body.bind(ifalse);
        } else {
            auto block2 = tp.mk.Block(ifalse);
            tp_current current2{ block2->scope };
            if (tp.enter(current2)) {
                body.bind(ifalse);
                tp.leave(current2);
            }
        }
        body.dispose();
    }
}

//----------------------------------------------------------
tp_forin_loop::tp_forin_loop(ForInSyntax *syntax) : tp(*typer), syntax(syntax) {}

/*  Syntax → ['await'] 'for' id [',' id]+ 'in' expression statement
*   Translation 🡓
*       NonAwaitLoop for fixed arrays 🡓
*           `loop` {
*               var    `index` ← 0
*               const    `src` ← expression
*               const `length` ← `src`.length
*               const  `items` ← `src`.items
*               var       item ← null
*               'if' `index` < `length` {
*                   item ← `items`[`index`]
*                   ...body...
*                   ++`index`
*               }
*           } 
*           `ifnobreak` {
*               ...body...
*           }
*       NonAwaitLoop for structs 🡓
*           `loop` {
*               var    `index` ← 0
*               const    `src` ← expression
*               const `length` ← `src`.length
*               const  `items` ← `src`.items
*               var       item ← null
*               'if' `index` < `length` {
*                   item ← `items`[`index`]
*                   ...body...
*                   ++`index`
*               }
*           } 
*           `ifnobreak` {
*               ...body...
*           }
*       NonAwaitLoop for resumables 🡓
*           `loop` {
*               const      `src` ← expression
*               var (item, done) ← `src`.next()
*               'if' !`done` {
*                   ...body...
*                   (item, done) ← `src`.next()
*               }
*           } 
*           `ifnobreak` {
*               ...body...
*           }
*       AwaitLoop 🡓
*           `loop` {
*               var        `src` ← expression
*               var (item, done) ← await `src`
*               'if' !`done` {
*                   ...body...
*                   (item, done) ← await `src`
*               }
*           } 
*           `ifnobreak` {
*               ...body...
*           }
*/
void tp_forin_loop::bind() {
    auto block = tp.mk.Loop(syntax);
    tp_current current{ block->scope };
    if (tp.enter(current)) {
        if (auto expr = tp.bindExpression(syntax->expression)) {
            auto type = expr->type.dereferenceIfReference();
            if (auto symbol = type.isDirect()) {
                if (tp.isa.Array(symbol)) {
                    expr = bindArrayLoop(expr, symbol);
                } else if (auto st = tp.isa.Struct(symbol)) {
                    if (st->isaResumableStruct()) {
                        expr = bindResumableLoop(expr, symbol);
                    } else {
                        expr = bindStructLoop(expr, symbol);
                    }
                } else {
                    type_error(expr, "expected a direct type, not %tptype", &expr->type);
                }
            } else {
                type_error(expr, "expected a loopable type, not %tptype", &expr->type);
            }
            tp.throwAway(expr);
        }
        if (syntax->ifnobreak != nullptr) {
            Assert(0);
        }
        tp.leave(current);
    }
}

TpNode* tp_forin_loop::bindArrayLoop(TpNode *expr, TpSymbol *) {
    impl_error(expr, "%c", __FUNCTION__);
    return tp.throwAway(expr);
}

TpNode* tp_forin_loop::bindStructLoop(TpNode *expr, TpSymbol *stSymbol) {
    /*  `loop` {
            var    `index`: Int32
            const `source` ← expression
            const `length` ← `src`.length
            const  `items` ← `src`.items
            var       item: T
            var      index: Int32
            `index` ← 0
            'if' `index` < `length` {
                index ← `index`
                item  ← `items`[`index`]
                ...body...
                ++`index`
            }
        } 
        `ifnobreak` {
            ...body...
        }
    */
    if (syntax->kwAwait != nullptr) {
        type_error(syntax->kwAwait, "%kw is not invalid for %tptype", syntax->kwAwait->keyword, &expr->type);
        return tp.throwAway(expr);
    }
    tp_lookup lookup{};
    // `source` ← expression
    auto srcSymbol = tp.mk.Local(syntax->expression, ids.random(ids.kw_source), expr->type);
    auto    source = tp.mk.Name(syntax->expression, srcSymbol);
    tp.current->scope->append(
        tp.mk.Assignment( syntax->expression, source, expr)
    );
    const auto errors = compiler.errors;
    auto lengthName = lookup.find(syntax->expression, source, ids.kw_length);
    auto  itemsName = lookup.find(syntax->expression, source, ids.kw_items);
    if (lengthName == nullptr || itemsName == nullptr) {
        lookup.dispose();
        return tp.throwAway(source, lengthName, itemsName);
    }
    if (!lengthName->type.isIntegral()) {
        type_error(syntax->expression, "expected %s#<red> to be integral, not %tptype", ids.kw_length,
                   &lengthName->type);
        lookup.dispose();
        return tp.throwAway(source, lengthName, itemsName);
    }
    // `index`
    auto idxSymbol = tp.mk.Local(syntax->expression, ids.random(ids.kw_index), lengthName->type);
    TpType itemType{};
    if (auto ptr = itemsName->type.isaPointer()) {
        itemType = ptr->pointee;
    }
    if (itemType.isUnknown()) {
        lookup.dispose();
        return tp.throwAway(source, lengthName, itemsName);
    }
    TpSymbol    *itemSymbol = nullptr;
    TpSymbol   *indexSymbol = nullptr;
    SyntaxNode  *itemSyntax = nullptr;
    SyntaxNode *indexSyntax = nullptr;
    if (auto list = tp.isa.CommaSeparatedSyntax(syntax->variables)) {
        itemSyntax = list->nodes.items[0];
        if (auto id = tp.isa.IdentifierSyntax(itemSyntax)) {
            itemSymbol = tp.mk.Local(id, id->value, itemType); // var item: T
            if (list->nodes.length == 2) {
                indexSyntax = list->nodes.items[1];
                if (id = tp.isa.IdentifierSyntax(indexSyntax)) {
                    indexSymbol = tp.mk.Local(id, id->value, tp.tree.tyInt32); // var index: Int32
                } else {
                    syntax_error(itemSyntax, "expected an identifier, not %sk", itemSyntax->kind);
                }
            } else {
                syntax_error(list , "expected 1 or 2 variables, not %i#<red>", list->nodes.length);
            }
        } else {
            syntax_error(itemSyntax, "expected an identifier, not %sk", itemSyntax->kind);
        }
    } else if (auto id = tp.isa.IdentifierSyntax(syntax->variables)) {
        itemSyntax = id;
        itemSymbol = tp.mk.Local(id, id->value, itemType); // var item: T
    } else {
        syntax_error(syntax->variables, "expected an identifier or a list of identifiers, not %sk", 
                     syntax->variables->kind);
    }
    if (errors != compiler.errors) {
        lookup.dispose();
        return tp.throwAway(source, lengthName, itemsName);
    }
    auto idxName = tp.mk.Name(syntax->expression, idxSymbol);
    // `index` ← 0
    tp.current->scope->append(
        tp.mk.Assignment(syntax->expression, idxName,
                         tp.mk.ZeroOf(syntax->expression, idxSymbol->node->type))
    );
    // `index` < `length`
    auto condition = tp.mk.Condition(syntax->expression, idxName, Tok::Less, lengthName);
    // if `index` < `length` {
    auto block = tp.mk.IfBlock(syntax->variables);
    auto  loop = (TpLoop*)tp.current->scope->owner->node;
    loop->body = block;
    tp_current cur{ block->scope };
    if (tp.enter(cur)) {
        block->condition = condition; // `index` < `length`
        if (indexSymbol != nullptr) { // index ← `index`
            tp.current->scope->append(
                tp.mk.Assignment(indexSyntax,
                                 tp.mk.Name(indexSyntax, indexSymbol),
                                 tp.mk.Name(syntax->expression, idxSymbol))
            );
        }
        // item ← `items`[`index`]
        tp.current->scope->append(
            tp.mk.Assignment(
                itemSyntax,
                tp.mk.Name(itemSyntax, itemSymbol),
                tp.mk.IndexName(itemSyntax, itemsName, idxName)
            )
        );
        tp_fn_body body{};
        body.bind(syntax->body);
        body.dispose();
        tp.leave(cur);
    } // }
    if (syntax->ifnobreak) {
        Assert(0);
    }
    lookup.dispose();
    return tp.throwAway(source, lengthName, itemsName);
}

TpNode* tp_forin_loop::bindResumableLoop(TpNode *expr, TpSymbol *) {
    impl_error(expr, "%c", __FUNCTION__);
    return tp.throwAway(expr);
}
//----------------------------------------------------------
tp_for_loop::tp_for_loop(ForSyntax *syntax) : tp(*typer), syntax(syntax) {}

void tp_for_loop::bind() {
    /*  Syntax → 'for' [initializer ';'] condition ';' [increment [';']] statement
    *   Translation 🡓
    *       `loop` {
    *           [...initializer...]
    *           'if' condition {
    *               ...body...
    *               [...increment...]
    *           }
    *       }
    *       `ifnobreak` {
    *           ...body...
    *       }
    */
    tp_fn_body body{};
    auto loop = tp.mk.Loop(syntax);
    tp_current current{ loop->scope };
    if (tp.enter(current)) {
        if (syntax->initializer != nullptr) {
            if (auto list = tp.isa.CommaSeparatedSyntax(syntax->initializer)) {
                for (auto i = 0; i < list->nodes.length; i++) {
                    body.bind(list->nodes.items[i]);
                }
            } else {
                body.bind(syntax->initializer);
            }
        }
        loop->body = tp.mk.IfBlock(syntax->body);
        tp_current current2{ loop->body->scope };
        if (tp.enter(current2)) {
            loop->body->condition = tp.bindCondition(syntax->condition);
            body.bind(syntax->body);
            body.bind(syntax->increment);
            tp.leave(current2);
        }
        if (syntax->ifnobreak != nullptr) {
            Assert(0);
        }
        tp.leave(current);
    }
    body.dispose();
}

//----------------------------------------------------------
tp_while_loop::tp_while_loop(WhileSyntax *syntax) : tp(*typer), syntax(syntax) {}

void tp_while_loop::bind() {
    /*  `loop` {
            'if' condition {
                ...body...
            }
        }
        `ifnobreak` {
            ...body...
        }
    */
    tp_fn_body body{};
    auto loop = tp.mk.Loop(syntax);
    tp_current current{ loop->scope };
    if (tp.enter(current)) {
        loop->body = tp.mk.IfBlock(syntax);
        tp_current current2{ loop->body->scope };
        if (tp.enter(current2)) {
            loop->body->condition = tp.bindCondition(syntax->condition);
            body.bind(syntax->body);
            tp.leave(current2);
        }
        if (syntax->ifnobreak != nullptr) {
            Assert(0);
        }
        tp.leave(current);
    }
    body.dispose();
}

//----------------------------------------------------------
tp_fn::tp_fn(tp_site *site, tp_template_instance_pair &pair)
	: tp(*typer), site(*site), fnSyntax(pair.syntaxAs<FunctionSyntax>()),
    fnSymbol(pair.instanceSymbol), fnNode(((TpFunction*)pair.instanceSymbol->node)) {}

tp_fn::tp_fn(tp_site *site, FunctionSyntax *fnSyntax, TpSymbol *fnSymbol)
    : tp(*typer), site(*site), fnSyntax(fnSyntax), fnSymbol(fnSymbol), 
    fnNode((TpFunction*)fnSymbol->node) {}

void tp_fn::dispose() {
}

void tp_fn::bindParameters() {
    if (site.parameters.list.isEmpty()) {
        Assert(site.arguments.list.isEmpty());
        fnNode->modifiers.isStatic = true;
        return;
    }
    for (auto i = 0; i < site.parameters.list.length; i++) {
        auto &parameter = site.parameters.list.items[i];
        Assert(parameter.syntax != nullptr && parameter.name != nullptr && parameter.type.isKnown());
        if (auto dup = tp.current->scope->contains(parameter.name)) {
            dup_error(parameter.syntax, dup);
        } else if (auto parameterSymbol = tp.mk.Parameter(parameter.syntax, parameter.name, parameter.type)) {
            tp.applyModifiers(parameter.modifiers, parameterSymbol);
            fnNode->parameters.append(parameterSymbol);
            if (parameter.name == ids.kw_this) {
                if (i == 0) {
                    auto parentSymbol = fnSymbol->scope->owner;
                    auto   parentNode = parentSymbol->node;
                    if (parentNode->kind == TpKind::Struct || parentNode->kind == TpKind::Union ||
                        parentNode->kind == TpKind::Enum) {
                        // Ok.
                    } else {
                        type_error(parameter.syntax, "%s#<red> can only be declared inside a structure or enum",
                                   parameter.name);
                    }
                } else {
                    type_error(parameter.syntax, "%s#<red> must be the first parameter", parameter.name);
                }
            } else if (i == 0) {
                fnNode->modifiers.isStatic = true;
            }
        }
    }
}

void tp_fn::bindReturn() {
    if (auto fnreturn = tp.bindExpression(fnSyntax->fnreturn)) {
        if (auto tpname = tp.isa.TypeName(fnreturn)) {
            fnNode->fnreturn = fnreturn->type;
        } else if (tp.isa.This(fnreturn)) {
            fnNode->fnreturn = fnreturn->type;
        } else {
            type_error(fnSyntax->fnreturn, "expected a typename");
        }
        tp.throwAway(fnreturn);
    }
}

void tp_fn::bindBody() {
    if (fnNode->modifiers.isResumable()) {
        return bindResumableBody();
    }
    if (fnSyntax->bodyOp) {
        if (auto  retval = tp.bindExpression(fnSyntax->body)) {
            if (auto ret = tp.mk.Return(fnSyntax->body, retval)) {
                tp.current->scope->append(ret);
            }
        }
    } else if (auto block = tp.isa.BlockSyntax(fnSyntax->body)) {
        Assert(block->modifiers == nullptr);
        tp_fn_body body{};
        body.bind(block->nodes);
        body.dispose();
    } else if (tp.isa.EmptySyntax(fnSyntax->body)) {
        // Do nothing.
    } else {
        syntax_error(fnSyntax->body, "expected body of function");
    }
}

void tp_fn::finish() {
    if (fnNode->fnreturn.isUnknown()) {
        fnNode->fnreturn = tp.tree.tyVoid;
    }
}

void tp_fn::bindResumableBody() {
    // We are now in the scope of the {fnNode} which is an 'async' function.
    // All parameters have been   bound and any fnreturn has been resolved.
    const auto errors = compiler.errors;
    // (1) Create a struct for a resumable in the scope of {fnNode}.
    auto stSymbol = tp.mk.ResumableStruct(fnSyntax);
    auto   stNode = (TpStruct*)stSymbol->node;
    fnNode->fnreturn = stNode->type.mkPointer();
    // See resumable.exy in aio for the full definition of the resumable.
    if (stSymbol->bindStatus.begin()) {
        traceln("%depth binding %tptype", &stNode->type);
        tp_current current{ stNode->scope };
        if (tp.enter(current)) { // struct `Resumable` 
            // overlapped: aio.OVERLAPPED
            tp.mk.Field(fnSyntax, ids.kw_overlapped, tp.sym_aio_OVERLAPPED->node->type);
            // next: fn `next`(this) { ... }
            auto nextfnSymbol = tp.mk.NextFn(fnSyntax);
            tp.mk.Field(fnSyntax, ids.kw_next, tp.mk.Name(fnSyntax, nextfnSymbol));
            // dispose: fn `dispose`(this) { ... 
            auto disposefnSymbol = tp.mk.DisposeFn(fnSyntax);
            tp.mk.Field(fnSyntax, ids.kw_dispose, tp.mk.Name(fnSyntax, disposefnSymbol));
            // srw: aio.SRWLOCK
            tp.mk.Field(fnSyntax, ids.kw_srw, tp.sym_aio_SRWLOCK->node->type);
            // __awaiter__: Void*
            tp.mk.Field(fnSyntax, ids.kw__awaiter__, tp.tree.tyVoidPointer);
            // __resume__: Int
            tp.mk.Field(fnSyntax, ids.kw__resume__, tp.tree.tyInt32);
            // __hResult__: UInt
            tp.mk.Field(fnSyntax, ids.kw__hResult__, tp.tree.tyUInt32);
            // __bytesTransferred__: UInt
            tp.mk.Field(fnSyntax, ids.kw__bytesTransferred__, tp.tree.tyUInt32);
            // __done__: Bool
            tp.mk.Field(fnSyntax, ids.kw__done__, tp.tree.tyBool);
            // ...parameters...
            for (auto i = 0; i < site.parameters.list.length; ++i) {
                auto      &parameter = site.parameters.list.items[i];
                auto parameterSymbol = fnNode->parameters.items[i];
                auto     fieldSymbol = tp.mk.Field(parameter.syntax, parameter.name,
                                                   tp.mk.Name(parameter.syntax, parameterSymbol));
                tp.applyModifiers(parameter.modifiers, fieldSymbol);
            }
            //---fn `next`(this) {
            if (nextfnSymbol->bindStatus.begin()) {
                auto nextfnNode = (TpFunction*)nextfnSymbol->node;
                traceln("%depth binding %tptype", &nextfnNode->type);
                tp_current current2{ nextfnNode->scope };
                if (tp.enter(current2)) {
                    if (fnSyntax->bodyOp) {
                        syntax_error(fnSyntax->body, "resumables must define a body");
                    } else if (auto block = tp.isa.BlockSyntax(fnSyntax->body)) {
                        Assert(block->modifiers == nullptr);
                        tp_fn_body body{};
                        body.bind(block->nodes);
                        body.dispose();
                    } else if (tp.isa.EmptySyntax(fnSyntax->body)) {
                        // Do nothing.
                    } else {
                        syntax_error(fnSyntax->body, "expected body of function");
                    }
                    if (nextfnNode->fnreturn.isUnknown()) {
                        nextfnNode->fnreturn = tp.tree.tyVoid;
                    }
                    tp.leave(current2);
                }
                nextfnSymbol->bindStatus.finish();
                traceln("%depth bound   %tptype (with %i#<red> error%c)", &nextfnNode->type,
                        compiler.errors - errors, compiler.errors - errors == 1 ? "" : "s");
            } // } fn `next`(this: `Resumable`*) -> T
            tp.leave(current); // } struct `Resumable`
        } // Back to fnScope.
        stSymbol->bindStatus.finish();
        traceln("%depth bound   %tptype (with %i#<red> error%c)", &stNode->type,
                compiler.errors - errors, compiler.errors - errors == 1 ? "" : "s");
    }
}
} // namespace exy