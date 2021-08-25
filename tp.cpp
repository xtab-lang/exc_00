#include "pch.h"
#include "typer.h"

namespace exy {
TpType TpTree::tyUnknown{};
TpType TpTree::tyVoidPointer{};
#define ZM(zName, zSize) TpType TpTree::ty##zName{};
DeclareBuiltinTypeKeywords(ZM)
#undef ZM

bool TpTree::initialize() {
    scope = mem.New<TpScope>(/* parent = */ nullptr, /* owner = */ nullptr);
    initializeBuiltins();
    if (initializeModules()) {
        Typer tp{};
        tp.run();
        tp.dispose();
    }
    return compiler.errors == 0;
}

void TpTree::dispose() {
    scope = ndispose(scope);
    modules.dispose();
    mem.dispose();
}

void TpTree::initializeBuiltins() {
    auto firstFile = compiler.syntaxTree->modules.first()->files.first();
    auto      &pos = firstFile->pos.pos;
#define ZM(zName, zSize) do { \
        auto name =  ids.get(S(#zName)); \
        auto node = mem.New<TpBuiltin>(pos, Keyword::zName, name); \
        auto  sym = mem.New<TpSymbol>(nullptr, name, node); \
        node->type = sym; \
        scope->symbols.append(sym->name, sym); \
        ty##zName = node->type; \
        sym->bindStatus.value = Status::Done; \
    } while (0);
    DeclareBuiltinTypeKeywords(ZM)
#undef ZM
}
 
bool TpTree::initializeModules() {
    auto &tree = *compiler.syntaxTree;
    for (auto i = 0; i < tree.modules.length; i++) {
        auto syntax = tree.modules.items[i];
        auto   name = syntax->name;
        if (auto dup = scope->contains(syntax->name)) {
            dup_error(syntax->pos, dup);
        } else {
            auto node = mem.New<TpModule>(syntax);
            auto  sym = mem.New<TpSymbol>(nullptr, name, node);
            node->type  = sym;
            node->scope = mem.New<TpScope>(/* parent = */ scope, /* owner = */ sym);
            scope->symbols.append(name, sym);
            modules.append(sym);
            initializeModule(node);
        }
    }
    return compiler.errors == 0;
}

void TpTree::initializeModule(TpModule *node) {
    auto sc = node->scope;
    node->modifiers.isStatic = true;
    for (auto i = 0; i < node->syntax->modules.length; i++) {
        auto syntax = node->syntax->modules.items[i];
        auto   name = syntax->name;
        if (auto dup = sc->contains(name)) {
            dup_error(syntax->pos, dup);
        } else {
            auto child = mem.New<TpModule>(syntax);
            auto   sym = mem.New<TpSymbol>(nullptr, name, child);
            child->type = sym;
            child->scope = mem.New<TpScope>(/* parent = */ sc, /* owner = */ sym);
            sc->symbols.append(name, sym);
            modules.append(sym);
            initializeModule(child);
        }
    }
}

//----------------------------------------------------------
TpSymbol::TpSymbol(ParentScope scope, Identifier name, TpSymbolNode *node)
    : scope(scope), name(name), node(node) {}

void TpSymbol::dispose() {
    node = ndispose(node);
}

//----------------------------------------------------------
TpScope::TpScope(TpScope *parent, TpSymbol *owner) 
    : parent(parent), owner(owner) {}

void TpScope::dispose() {
    ldispose(symbols);
    ldispose(statements);
}

TpSymbol* TpScope::contains(Identifier name) {
    auto idx = symbols.indexOf(name);
    if (idx >= 0) {
        return symbols.items[idx].value;
    }
    return nullptr;
}

TpSymbol* TpScope::append(TpSymbol *symbol) {
    symbols.append(symbol->name, symbol);
    return symbol;
}

//----------------------------------------------------------
TpNode::TpNode(Pos pos, Type type, Kind kind) 
    : pos(pos), type(type), kind(kind) {}

String TpNode::kindName() const {
    return kindName(kind);
}

String TpNode::kindName(Kind k) {
    switch (k) {
    #define ZM(zName) case Kind::zName: return String{ S(#zName) };
        DeclareTpNodes(ZM)
        #undef ZM
    }
    UNREACHABLE();
}

//----------------------------------------------------------
TpSymbolNode::TpSymbolNode(Pos pos, Type type, Kind kind, Identifier dotName) 
    : TpNode(pos, type, kind), dotName(dotName) {}

//----------------------------------------------------------
TpTypeNode::TpTypeNode(Pos pos, Kind kind, Identifier dotName)
    : TpSymbolNode(pos, TpType(), kind, dotName) {}

void TpTypeNode::dispose() {
    scope = ndispose(scope);
    __super::dispose();
}

//----------------------------------------------------------
TpBuiltin::TpBuiltin(Pos pos, Keyword keyword, Identifier dotName)
    : TpTypeNode(pos, Kind::Builtin, dotName), keyword(keyword) {
    switch (keyword) {
    #define ZM(zName, zSize) case Keyword::zName: size = zSize; break;
        DeclareBuiltinTypeKeywords(ZM)
        #undef ZM
        default: UNREACHABLE();
    }
}

TpType TpPacking::packedType() const {
    auto  &tree = typer->tree;
    auto symbol = type.isaBuiltin();
    auto   node = (TpBuiltin*)symbol->node;
    switch (node->keyword) {
        case Keyword::Char:
        case Keyword::Int8: switch (count) {
            case 16: return tree.tyInt8x16;
            case 32: return tree.tyInt8x32;
            case 64: return tree.tyInt8x64;
        } break;
        case Keyword::Int16: switch (count) {
            case  8: return tree.tyInt16x8;
            case 16: return tree.tyInt16x16;
            case 32: return tree.tyInt16x32;
        } break;
        case Keyword::Int32: switch (count) {
            case  4: return tree.tyInt32x4;
            case  8: return tree.tyInt32x8;
            case 16: return tree.tyInt32x16;
        } break;
        case Keyword::Int64: switch (count) {
            case 2: return tree.tyInt64x2;
            case 4: return tree.tyInt64x4;
            case 8: return tree.tyInt64x8;
        } break;
        case Keyword::Bool:
        case Keyword::UInt8: switch (count) {
            case 16: return tree.tyUInt8x16;
            case 32: return tree.tyUInt8x32;
            case 64: return tree.tyUInt8x64;
        } break;
        case Keyword::UInt16: switch (count) {
            case  8: return tree.tyUInt16x8;
            case 16: return tree.tyUInt16x16;
            case 32: return tree.tyUInt16x32;
        } break;
        case Keyword::UInt32: switch (count) {
            case  4: return tree.tyUInt32x4;
            case  8: return tree.tyUInt32x8;
            case 16: return tree.tyUInt32x16;
        } break;
        case Keyword::UInt64: switch (count) {
            case 2: return tree.tyUInt64x2;
            case 4: return tree.tyUInt64x4;
            case 8: return tree.tyUInt64x8;
        } break;
        case Keyword::Float: switch (count) {
            case  4: return tree.tyFloatx4;
            case  8: return tree.tyFloatx8;
            case 16: return tree.tyFloatx16;
        } break;
        case Keyword::Double: switch (count) {
            case 2: return tree.tyDoublex2;
            case 4: return tree.tyDoublex4;
            case 8: return tree.tyDoublex8;
        } break;
    }
    Assert(0);
    return tree.tyUnknown;
}

TpPacking TpBuiltin::packing() const {
    auto &tree = typer->tree;
    switch (keyword) {
        case Keyword::Floatx4:  return { tree.tyFloat,  4 };
        case Keyword::Floatx8:  return { tree.tyFloat,  8 };
        case Keyword::Floatx16: return { tree.tyFloat, 16 };

        case Keyword::Doublex2: return { tree.tyDouble, 2 };
        case Keyword::Doublex4: return { tree.tyDouble, 4 };
        case Keyword::Doublex8: return { tree.tyDouble, 8 };

        case Keyword::Int8x16: return { tree.tyInt8, 16 };
        case Keyword::Int8x32: return { tree.tyInt8, 32 };
        case Keyword::Int8x64: return { tree.tyInt8, 64 };

        case Keyword::UInt8x16: return { tree.tyUInt8, 16 };
        case Keyword::UInt8x32: return { tree.tyUInt8, 32 };
        case Keyword::UInt8x64: return { tree.tyUInt8, 64 };

        case Keyword::Int16x8:  return { tree.tyInt16,  8 };
        case Keyword::Int16x16: return { tree.tyInt16, 16 };
        case Keyword::Int16x32: return { tree.tyInt16, 32 };

        case Keyword::UInt16x8:  return { tree.tyUInt16,  8 };
        case Keyword::UInt16x16: return { tree.tyUInt16, 16 };
        case Keyword::UInt16x32: return { tree.tyUInt16, 32 };

        case Keyword::Int32x4:  return { tree.tyInt32,  4 };
        case Keyword::Int32x8:  return { tree.tyInt32,  8 };
        case Keyword::Int32x16: return { tree.tyInt32, 16 };

        case Keyword::UInt32x4:  return { tree.tyUInt32,  4 };
        case Keyword::UInt32x8:  return { tree.tyUInt32,  8 };
        case Keyword::UInt32x16: return { tree.tyUInt32, 16 };

        case Keyword::Int64x2: return { tree.tyInt64, 2 };
        case Keyword::Int64x4: return { tree.tyInt64, 4 };
        case Keyword::Int64x8: return { tree.tyInt64, 8 };

        case Keyword::UInt64x2: return { tree.tyUInt64, 26 };
        case Keyword::UInt64x4: return { tree.tyUInt64, 4 };
        case Keyword::UInt64x8: return { tree.tyUInt64, 8 };
    }
    Assert(0);
    return TpPacking{ tree.tyUnknown, 0 };
}

//----------------------------------------------------------
TpModule::TpModule(SyntaxModule *syntax)
    : TpTypeNode(syntax->pos.pos, Kind::Module, syntax->dotName), syntax(syntax), 
    system(syntax->system) {}

void TpModule::dispose() {
    ldispose(urlHandlers);
    __super::dispose();
}

//----------------------------------------------------------
TpOverloadSet::TpOverloadSet(TpSymbol *first)
    : TpTypeNode(((TpTemplate*)first->node)->pos, Kind::OverloadSet, ((TpTemplate*)first->node)->dotName) {
    auto firstTemplate = (TpTemplate*)first->node;
    firstTemplate->parentOv = this;
    list.append(first);
}

void TpOverloadSet::dispose() {
    ldispose(list);
    __super::dispose();
}

//----------------------------------------------------------
TpTemplate::TpTemplate(StructureSyntax *syntax, const TpArity &arity, Identifier dotName)
    : TpTypeNode(typer->mkPos(syntax), Kind::Template, dotName), syntax(syntax), arity(arity) {}

TpTemplate::TpTemplate(FunctionSyntax *syntax, const TpArity &arity, Identifier dotName)
    : TpTypeNode(typer->mkPos(syntax), Kind::Template, dotName), syntax(syntax), arity(arity) {}

void TpTemplate::dispose() {
    ldispose(instances);
    __super::dispose();
}

//----------------------------------------------------------
TpStruct::TpStruct(Pos pos, Identifier dotName, StructKind structKind)
    : TpTypeNode(pos, Kind::Struct, dotName), structKind(structKind) {
    isCompilerGenerated = structKind > TupleStruct;
}

void TpStruct::dispose() {
    bases.dispose();
    derived.dispose();
    parameters.dispose();
    __super::dispose();
}

//----------------------------------------------------------
TpUnion::TpUnion(Pos pos, Identifier dotName)
    : TpTypeNode(pos, Kind::Union, dotName) {}

//----------------------------------------------------------
TpArray::TpArray(Pos pos, Identifier dotName, INT length)
    : TpTypeNode(pos, Kind::Array, dotName), length(length) {}

//----------------------------------------------------------
TpEnum::TpEnum(Pos pos, Identifier dotName)
    : TpTypeNode(pos, Kind::Enum, dotName) {}

//----------------------------------------------------------
TpFunction::TpFunction(Pos pos, Keyword keyword, Identifier dotName)
    : TpTypeNode(pos, Kind::Function, dotName), keyword(keyword) {
}

void TpFunction::dispose() {
    parameters.dispose();
    __super::dispose();
}

TpSymbol* TpFunction::isaConstructor() const {
    auto &tp = *typer;
    auto parentScope = scope->parent;
    if (parentScope == nullptr) {
        return nullptr;
    }
    if (auto stSymbol = tp.isa.StructSymbol(parentScope->owner)) {
        auto fnSymbol = (TpSymbol*)scope->owner;
        if (stSymbol->name == fnSymbol->name) {
            return fnSymbol;
        }
    }
    return nullptr;
}

TpSymbol* TpFunction::isaDisposeFunction() const {
    auto &tp = *typer;
    auto parentScope = scope->parent;
    if (parentScope == nullptr) {
        return nullptr;
    }
    if (auto stSymbol = tp.isa.StructSymbol(parentScope->owner)) {
        auto fnSymbol = (TpSymbol*)scope->owner;
        auto id = ids.kw_dispose;
        if ((fnSymbol->name == ids.kw_dispose || fnSymbol->name->isRandomOf(id->text, id->length)) && parameters.length == 1) {
            auto parameter = parameters.first();
            if (parameter->name == ids.kw_this) {
                const auto &parameterType = parameter->node->type;
                if (auto ptr = parameterType.isIndirect()) {
                    if (ptr->pointee == stSymbol->node->type) {
                        return fnSymbol;
                    }
                }
            }
        }
    }
    return nullptr;
}

TpSymbol* TpFunction::isaGeneratorFunction() const {
    auto &tp = *typer;
    auto parentScope = scope->parent;
    if (parentScope == nullptr) {
        return nullptr;
    }
    if (auto stSymbol = tp.isa.StructSymbol(parentScope->owner)) {
        auto   stNode = (TpStruct*)stSymbol->node;
        if (stNode->structKind != TpStruct::ResumableStruct) {
            return nullptr;
        }
        auto fnSymbol = (TpSymbol*)scope->owner;
        auto id = ids.kw_next;
        if (fnSymbol->name->isRandomOf(id->text, id->length) && parameters.length == 1) {
            auto parameter = parameters.first();
            if (parameter->name == ids.kw_this) {
                const auto &parameterType = parameter->node->type;
                if (auto ptr = parameterType.isIndirect()) {
                    if (ptr->pointee == stSymbol->node->type) {
                        return fnSymbol;
                    }
                }
            }
        }
    }
    return nullptr;
}

TpSymbol* TpFunction::isaLambdaFunction() const {
    auto &tp = *typer;
    auto parentScope = scope->parent;
    if (parentScope == nullptr) {
        return nullptr;
    }
    if (auto stSymbol = tp.isa.StructSymbol(parentScope->owner)) {
        auto   stNode = (TpStruct*)stSymbol->node;
        if (stNode->structKind != TpStruct::LambdaStruct) {
            return nullptr;
        }
        auto fnSymbol = (TpSymbol*)scope->owner;
        if (fnSymbol->name == ids.kw_open_close_parens && parameters.length > 0) {
            auto parameter = parameters.first();
            if (parameter->name == ids.kw_this) {
                const auto &parameterType = parameter->node->type;
                if (auto ptr = parameterType.isIndirect()) {
                    if (ptr->pointee == stSymbol->node->type) {
                        return fnSymbol;
                    }
                }
            }
        }
    }
    return nullptr;
}

//----------------------------------------------------------
TpLabel::TpLabel(Pos pos, Identifier dotName)
    : TpTypeNode(pos, Kind::Label, dotName) {}

//----------------------------------------------------------
TpAliasNode::TpAliasNode(Pos pos, Type type, Kind kind, Identifier dotName, TpAliasKind aliasKind)
    : TpSymbolNode(pos, type, kind, dotName), aliasKind(aliasKind) {}

//----------------------------------------------------------
TpTypeAlias::TpTypeAlias(Pos pos, Type type, Identifier dotName, TpAliasKind aliasKind) 
    : TpAliasNode(pos, type, Kind::TypeAlias, dotName, aliasKind) {}

//----------------------------------------------------------
TpConstAlias::TpConstAlias(Pos pos, TpConstExpr *value, Identifier dotName, TpAliasKind aliasKind)
    : TpAliasNode(pos, value->type, Kind::ConstAlias, dotName, aliasKind), value(value) {}

void TpConstAlias::dispose() {
    value = ndispose(value);
    __super::dispose();
}

//----------------------------------------------------------
TpValueAlias::TpValueAlias(Pos pos, TpSymbol *value, Identifier dotName, TpAliasKind aliasKind)
    : TpAliasNode(pos, value->node->type, Kind::ValueAlias, dotName, aliasKind), value(value) {}

//----------------------------------------------------------
TpValueNode::TpValueNode(Pos pos, Type type, Kind kind, Identifier dotName)
    : TpSymbolNode(pos, type, kind, dotName) {}

//----------------------------------------------------------
TpParameter::TpParameter(Pos pos, Type type, Identifier dotName)
    : TpValueNode(pos, type, Kind::Parameter, dotName) {}

//----------------------------------------------------------
TpLocal::TpLocal(Pos pos, Type type, Identifier dotName)
    : TpValueNode(pos, type, Kind::Local, dotName) {}

//----------------------------------------------------------
TpGlobal::TpGlobal(Pos pos, Type type, Identifier dotName)
    : TpValueNode(pos, type, Kind::Global, dotName) {
    modifiers.isStatic = true;
}

TpGlobal::TpGlobal(Pos pos, TpNode* rhs, Identifier dotName) 
    : TpValueNode(pos, rhs->type, Kind::Global, dotName), rhs(rhs) {
    modifiers.isStatic = true;
}

void TpGlobal::dispose() {
    rhs = ndispose(rhs);
    __super::dispose();
}

//----------------------------------------------------------
TpField::TpField(Pos pos, Type type, Identifier dotName, FieldKind fieldKind)
    : TpValueNode(pos, type, Kind::Field, dotName), fieldKind(fieldKind) {}

TpField::TpField(Pos pos, TpNode* rhs, Identifier dotName, FieldKind fieldKind)
    : TpValueNode(pos, rhs->type, Kind::Field, dotName), fieldKind(fieldKind), rhs(rhs) {}

void TpField::dispose() {
    rhs = ndispose(rhs);
    __super::dispose();
}

//----------------------------------------------------------
TpBlock::TpBlock(Pos pos, TpLabel *label)
    : TpNode(pos, typer->tree.tyVoid, Kind::Block), scope(label->scope) {}

TpBlock::TpBlock(Pos pos, TpLabel *label, Kind kind)
    : TpNode(pos, typer->tree.tyVoid, kind), scope(label->scope) {}

void TpBlock::dispose() {
    scope = ndispose(scope);
    __super::dispose();
}

//----------------------------------------------------------
TpIfBlock::TpIfBlock(Pos pos, TpLabel *label)
    : TpBlock(pos, label, Kind::IfBlock) {}

void TpIfBlock::dispose() {
    condition = ndispose(condition);
    __super::dispose();
}

//----------------------------------------------------------
TpLoop::TpLoop(Pos pos, TpLabel *label)
    : TpBlock(pos, label, Kind::Loop) {}

void TpLoop::dispose() {
    body = ndispose(body);
    __super::dispose();
}

//----------------------------------------------------------
TpYield::TpYield(Pos pos, TpNode *value)
    : TpNode(pos, typer->tree.tyVoid, Kind::Yield_), value(value) {}

void TpYield::dispose() {
    value = ndispose(value);
    __super::dispose();
}

//----------------------------------------------------------
TpYieldFrom::TpYieldFrom(Pos pos, TpNode *value)
    : TpNode(pos, typer->tree.tyVoid, Kind::YieldFrom), value(value) {}

void TpYieldFrom::dispose() {
    value = ndispose(value);
    __super::dispose();
}

//----------------------------------------------------------
TpReturn::TpReturn(Pos pos)
    : TpNode(pos, typer->tree.tyVoid, Kind::Return) {}

TpReturn::TpReturn(Pos pos, TpNode *value)
    : TpNode(pos, value->type, Kind::Return), value(value) {}

void TpReturn::dispose() {
    value = ndispose(value);
    __super::dispose();
}

//----------------------------------------------------------
TpBreak::TpBreak(Pos pos)
    : TpNode(pos, typer->tree.tyVoid, Kind::Break) {}

//----------------------------------------------------------
TpContinue::TpContinue(Pos pos)
    : TpNode(pos, typer->tree.tyVoid, Kind::Continue) {}

//----------------------------------------------------------
TpDefer::TpDefer(Pos pos, TpNode *value)
    : TpNode(pos, value->type, Kind::Defer), value(value) {}

void TpDefer::dispose() {
    value = ndispose(value);
    __super::dispose();
}

//----------------------------------------------------------
TpUnary::TpUnary(Pos pos, TpNode *value, Tok op, Kind kind)
    : TpNode(pos, value->type, kind), op(op), value(value) {}

void TpUnary::dispose() {
    value = ndispose(value);
    __super::dispose();
}

//----------------------------------------------------------
TpUnaryPrefix::TpUnaryPrefix(Pos pos, Tok op, TpNode *value)
    : TpUnary(pos, value, op, Kind::UnaryPrefix) {}

//----------------------------------------------------------
TpUnarySuffix::TpUnarySuffix(Pos pos, TpNode *value, Tok op)
    : TpUnary(pos, value, op, Kind::UnarySuffix) {}

//----------------------------------------------------------
TpPointerUnaryPrefix::TpPointerUnaryPrefix(Pos pos, Tok op, TpNode *value)
    : TpUnary(pos, value, op, Kind::PointerUnaryPrefix) {}

//----------------------------------------------------------
TpPointerUnarySuffix::TpPointerUnarySuffix(Pos pos, TpNode *value, Tok op)
    : TpUnary(pos, value, op, Kind::PointerUnarySuffix) {}

//----------------------------------------------------------
TpDefinition::TpDefinition(Pos pos, TpNode *lhs, TpNode *rhs)
    : TpNode(pos, lhs->type, Kind::Definition), lhs(lhs), rhs(rhs) {}

void TpDefinition::dispose() {
    lhs = ndispose(lhs);
    rhs = ndispose(rhs);
    __super::dispose();
}

//----------------------------------------------------------
TpAssignment::TpAssignment(Pos pos, TpNode *dst, TpNode *src)
    : TpNode(pos, dst->type, Kind::Assignment), dst(dst), op(Tok::Assign), src(src) {}

TpAssignment::TpAssignment(Pos pos, TpNode *dst, Tok op, TpNode *src, TpKind kind)
    : TpNode(pos, dst->type, kind), dst(dst), op(op), src(src) {}

void TpAssignment::dispose() {
    dst = ndispose(dst);
    src = ndispose(src);
    __super::dispose();
}

//----------------------------------------------------------
TpCompoundPointerArithmetic::TpCompoundPointerArithmetic(Pos pos, TpNode *dst, Tok op, TpNode *src)
    : TpAssignment(pos, dst, op, src, Kind::CompoundPointerArithmetic) {}

//----------------------------------------------------------
TpCompoundShift::TpCompoundShift(Pos pos, TpNode *dst, Tok op, TpNode *src)
    : TpAssignment(pos, dst, op, src, Kind::CompoundShift) {}

//----------------------------------------------------------
TpCompoundArithmetic::TpCompoundArithmetic(Pos pos, TpNode *dst, Tok op, TpNode *src)
    : TpAssignment(pos, dst, op, src, Kind::CompoundArithmetic) {}

//----------------------------------------------------------
TpBinary::TpBinary(Pos pos, Type type, TpNode *lhs, Tok op, TpNode *rhs, TpKind kind)
    : TpNode(pos, type, kind), lhs(lhs), op(op), rhs(rhs) {}

void TpBinary::dispose() {
    lhs = ndispose(lhs);
    rhs = ndispose(rhs);
    __super::dispose();
}

//----------------------------------------------------------
TpPointerArithmetic::TpPointerArithmetic(Pos pos, Type type, TpNode *lhs, Tok op, TpNode *rhs)
    : TpBinary(pos, type, lhs, op, rhs, Kind::PointerArithmetic) {}

//----------------------------------------------------------
TpShift::TpShift(Pos pos, TpNode *lhs, Tok op, TpNode *rhs)
    : TpBinary(pos, lhs->type, lhs, op, rhs, Kind::Shift) {}

//----------------------------------------------------------
TpArithmetic::TpArithmetic(Pos pos, TpNode *lhs, Tok op, TpNode *rhs)
    : TpBinary(pos, lhs->type, lhs, op, rhs, Kind::Arithmetic) {}

//----------------------------------------------------------
TpCondition::TpCondition(Pos pos, TpNode *lhs, Tok op, TpNode *rhs)
    : TpBinary(pos, typer->tree.tyBool, lhs, op, rhs, Kind::Condition) {}

//----------------------------------------------------------
TpTernary::TpTernary(Pos pos, TpNode *condition, TpNode *iftrue, TpNode *ifalse)
    : TpNode(pos, iftrue->type, Kind::Ternary), condition(condition), iftrue(iftrue), ifalse(ifalse) {}

void TpTernary::dispose() {
    condition = ndispose(condition);
    iftrue = ndispose(iftrue);
    ifalse = ndispose(ifalse);
    __super::dispose();
}

//----------------------------------------------------------
TpCall::TpCall(Pos pos, Type type, TpNode *name)
    : TpNode(pos, type, Kind::Call), name(name) {}

void TpCall::dispose() {
    name = ndispose(name);
    ldispose(arguments);
    __super::dispose();
}

//----------------------------------------------------------
TpInitializer::TpInitializer(Pos pos, Type type)
    : TpNode(pos, type, Kind::Initializer) {}

void TpInitializer::dispose() {
    ldispose(arguments);
    __super::dispose();
}

//----------------------------------------------------------
TpAwait::TpAwait(Pos pos, Type type, TpNode *value)
    : TpNode(pos, type, Kind::Await), value(value) {}

void TpAwait::dispose() {
    value = ndispose(value);
    __super::dispose();
}

//----------------------------------------------------------
TpSizeOf::TpSizeOf(Pos pos, Type value)
    : TpNode(pos, typer->tree.tyInt32, Kind::SizeOf), value(value) {}

//----------------------------------------------------------
TpNew::TpNew(Pos pos, Type type)
    : TpNode(pos, type, Kind::New) {}

void TpNew::dispose() {
    __super::dispose();
}

//----------------------------------------------------------
TpDelete::TpDelete(Pos pos, TpNode *value)
    : TpNode(pos, value->type, Kind::Delete), value(value) {}

void TpDelete::dispose() {
    value = ndispose(value);
    __super::dispose();
}

//----------------------------------------------------------
TpAtomic::TpAtomic(Pos pos, TpNode *expression)
    : TpNode(pos, expression->type, Kind::Atomic), value(value) {}

void TpAtomic::dispose() {
    value = ndispose(value);
    __super::dispose();
}

//----------------------------------------------------------
TpAddressOf::TpAddressOf(Pos pos, TpNode *value)
    : TpNode(pos, value->type.mkPointer(), Kind::ReferenceOf), value(value) {}

void TpAddressOf::dispose() {
    value = ndispose(value);
    __super::dispose();
}

//----------------------------------------------------------
TpReferenceOf::TpReferenceOf(Pos pos, TpNode *value)
    : TpNode(pos, value->type.mkReference(), Kind::ReferenceOf), value(value) {}

void TpReferenceOf::dispose() {
    value = ndispose(value);
    __super::dispose();
}

//----------------------------------------------------------
TpDereference::TpDereference(Pos pos, TpNode *value)
    : TpNode(pos, value->type.pointee(), Kind::Dereference), value(value) {}

void TpDereference::dispose() {
    value = ndispose(value);
    __super::dispose();
}

//----------------------------------------------------------
TpName::TpName(Pos pos, Type type, Kind kind)
    : TpNode(pos, type, kind) {}

//----------------------------------------------------------
TpTypeName::TpTypeName(Pos pos, Type type)
    : TpName(pos, type, Kind::TypeName) {}

//----------------------------------------------------------
TpValueName::TpValueName(Pos pos, TpSymbol *symbol)
    : TpName(pos, symbol->node->type, Kind::ValueName), symbol(symbol) {
    Assert(tp_isa::ValueSymbol(symbol));
}

//----------------------------------------------------------
TpFieldName::TpFieldName(Pos pos, TpNode *base, TpSymbol *symbol)
    : TpName(pos, symbol->node->type, Kind::FieldName), base(base), symbol(symbol) {
    Assert(tp_isa::FieldSymbol(symbol));
}

void TpFieldName::dispose() {
    base = ndispose(base);
    __super::dispose();
}

//----------------------------------------------------------
TpIndexName::TpIndexName(Pos pos, Type type, TpNode *base, TpNode *index)
    : TpName(pos, type, Kind::IndexName), base(base), index(index) {
    Assert(index->type.isInt64() || index->type.isUInt64());
}

void TpIndexName::dispose() {
    base = ndispose(base);
    index = ndispose(index);
    __super::dispose();
}

//----------------------------------------------------------
TpConstExpr::TpConstExpr(Pos pos, TpNode *value)
    : TpName(pos, value->type, Kind::ConstExpr), value(value) {
    Assert(value->kind != Kind::ConstExpr);
}

//----------------------------------------------------------
TpCast::TpCast(Pos pos, Type type, TpNode *value, CastKind castKind, Reason reason)
    : TpNode(pos, type, Kind::Cast), value(value), castKind(castKind), reason(reason) {
    Assert(type != value->type);
}

void TpCast::dispose() {
    value = ndispose(value);
    __super::dispose();
}

//----------------------------------------------------------
TpLiteral::TpLiteral(Pos pos, Type type, UINT64 u64)
    : TpNode(pos, type, Kind::Literal), u64(u64) {}
} // namespace exy