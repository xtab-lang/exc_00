#pragma once

namespace exy {
struct tp_template_instance_pair {
    TpSymbol *templateSymbol;
    TpSymbol *instanceSymbol;

    auto syntax() { return ((TpTemplate*)templateSymbol->node)->syntax; }

    template<typename T>
    auto syntaxAs() { return (T*)syntax(); }

    auto& modifiers() { return ((TpSymbolNode*)instanceSymbol->node)->modifiers; }
};

struct tp_mk {
    using  Pos = SyntaxNode*;
    using Type = const TpType&;

    Typer &tp;
    Mem   &mem;

    tp_mk(Typer *t);

    // symbols----------------------------------------------
    TpSymbol* TypeAlias(Pos, Identifier name, Type type, TpAliasKind kind);
    TpSymbol* DefineTypeAlias(Pos, Identifier name, Type type);
    TpSymbol* ImportTypeAlias(Pos, Identifier name, Type type);
    TpSymbol* ExportTypeAlias(Pos, Identifier name, Type type);

    TpSymbol* ConstAlias(Pos, Identifier name, TpConstExpr *value, TpAliasKind kind);
    TpSymbol* DefineConstAlias(Pos, Identifier name, TpConstExpr *value);
    TpSymbol* ImportConstAlias(Pos, Identifier name, TpConstExpr *value);
    TpSymbol* ExportConstAlias(Pos, Identifier name, TpConstExpr *value);

    TpSymbol* ValueAlias(Pos, Identifier name, TpSymbol *value, TpAliasKind kind);
    TpSymbol* DefineValueAlias(Pos, Identifier name, TpSymbol *value);
    TpSymbol* ImportValueAlias(Pos, Identifier name, TpSymbol *value);
    TpSymbol* ExportValueAlias(Pos, Identifier name, TpSymbol *value);

    TpSymbol* Global(Pos, Identifier name, Type);
    TpSymbol* Global(Pos, Identifier name, TpNode *rhs);

    TpSymbol* Field(Pos, Identifier name, Type);
    TpSymbol* Field(Pos, Identifier name, TpNode *rhs);
    TpSymbol* CapturedField(Pos, TpScope*, Identifier name, TpNode *rhs);

    TpSymbol* Local(Pos, Identifier name, Type);

    TpSymbol* Parameter(Pos, Identifier name, Type);

    TpSymbol* OverloadSet(TpSymbol *firstOfOverloadSet);
    TpSymbol* Template(TpSymbol *ovSymbol, StructureSyntax*, const TpArity &arity, Identifier name);
    TpSymbol* Template(TpSymbol *ovSymbol, FunctionSyntax*, const TpArity &arity, Identifier name);
    TpSymbol* Template(StructureSyntax*, const TpArity &arity, Identifier name);
    TpSymbol* Template(FunctionSyntax*, const TpArity &arity, Identifier name);
    TpSymbol* UrlHandlerTemplate(TpScope *moduleScope, FunctionSyntax*, const TpArity &arity, Identifier name);
    TpSymbol* ExternTemplate(FunctionSyntax*, const TpArity &arity, Identifier name, Identifier dllPath);

    tp_template_instance_pair Struct(TpSymbol *templateSymbol);
    tp_template_instance_pair Function(TpSymbol *templateSymbol);
    tp_template_instance_pair Extern(TpSymbol *templateSymbol, Identifier dllPath);
    // Compiler generated symbols.
    TpSymbol* OrdinaryFn(Pos, Identifier name);
    TpSymbol* NextFn(Pos);    // fn `next`(this: T*);
    TpSymbol* DisposeFn(Pos); // fn `dispose`(this: T*) -> T*;
    TpSymbol* ResumableStruct(Pos);
    tp_template_instance_pair LambdaStruct(TpSymbol *templateSymbol);
    TpSymbol* LambdaFunction(Pos); // fn ()(this: T*);

    TpSymbol* Symbol(Identifier name, TpSymbolNode *node);
    TpSymbol* Symbol(TpScope *scope, Identifier name, TpSymbolNode *node);

    // flow control-----------------------------------------
    TpNode* Defer(Pos, TpNode *value);
    TpNode* Return(Pos, TpNode *value);
    TpNode* Break(Pos);
    TpNode* Continue(Pos);

    // blocks-----------------------------------------------
    TpBlock* Block(Pos);
    TpIfBlock* IfBlock(Pos);
    TpLoop* Loop(Pos);

    // unary------------------------------------------------
    TpNode* UnaryPrefix(Pos, Tok op, TpNode*);
    TpNode* PointerUnaryPrefix(Pos, Tok op, TpNode*);
    TpNode* UnarySuffix(Pos, TpNode*, Tok op);
    TpNode* PointerUnarySuffix(Pos, TpNode*, Tok op);
    TpNode* SizeOf(Pos, TpNode*);
    TpNode* TypeOf(Pos, TpNode*);
    TpNode* Delete(Pos, TpNode*);
    TpNode* Atomic(Pos, TpNode*);

    // assignments------------------------------------------
    TpDefinition* Definition(Pos, TpSymbol *lhs, TpNode *rhs);
    TpAssignment* Assignment(Pos, TpNode *dst, TpNode *src);
    TpAssignment* CompoundPointerArithmetic(Pos, TpNode *lhs, Tok op, TpNode *rhs);
    TpAssignment* CompoundShift(Pos, TpNode *dst, Tok op, TpNode *src);
    TpAssignment* CompoundArithmetic(Pos, TpNode *dst, Tok op, TpNode *src);

    // pointer operations-----------------------------------
    TpNode* PointerPlusInt(Pos, TpNode *lhs, TpNode *rhs);
    TpNode* IntPlusPointer(Pos, TpNode *lhs, TpNode *rhs);
    TpNode* PointerMinusInt(Pos, TpNode *lhs, TpNode *rhs);
    TpNode* IntMinusPointer(Pos, TpNode *lhs, TpNode *rhs);
    TpNode* PointerMinusPointer(Pos, TpNode *lhs, TpNode *rhs);
    TpNode* DereferenceIfReference(Pos, TpNode *value);
    TpNode* Dereference(Pos, TpNode *value);
    TpNode* AddressOf(Pos, TpNode *value);
    TpNode* ReferenceOf(Pos, TpNode *value);

    // binary operations------------------------------------
    TpNode* Shift(Pos, TpNode *lhs, Tok op, TpNode *rhs);
    TpNode* Arithmetic(Pos, TpNode *lhs, Tok op, TpNode *rhs);
    TpNode* Condition(Pos, TpNode *lhs, Tok op, TpNode *rhs);
    TpNode* Condition(Pos, TpNode *lhs);

    // ternary operations-----------------------------------
    TpNode* Ternary(Pos, TpNode *condition, TpNode *iftrue, TpNode *ifalse);

    // initializers-----------------------------------------
    TpNode* ZeroOf(Pos, TpTypeName*);
    TpNode* ZeroOf(Pos, Type);

    // generator operations---------------------------------
    TpNode* Yield_(Pos, TpNode *value);
    TpNode* YieldFrom(Pos, TpNode *value);
    TpNode* Await(Pos, TpNode *value);

    // other operations-------------------------------------
    TpCall* FunctionCallFromSite(TpNode *name, TpSymbol *fnSymbol);
    TpInitializer* StructInitializerFromSite(Type);

    // names------------------------------------------------
    TpNode* Name(Pos, TpSymbol*);
    TpNode*  TypeName(Pos, Type);
    TpNode* ValueName(Pos, TpSymbol*);
    TpNode* ConstExpr(Pos, TpNode*);
    TpNode* FieldName(Pos, TpNode *base, TpSymbol*);
    TpNode* IndexName(Pos, TpNode *base, TpNode *index);

    TpNode* Name(Pos, TpNode *base, TpSymbol*);

    // constants--------------------------------------------
    TpConstExpr* Literal(Pos, Type, UINT64 u64);

    Identifier dotName(Identifier rhs);
    Identifier dotName(TpScope *scope, Identifier rhs);
    Identifier dotName(TpSymbol *lhs, Identifier rhs);
    Identifier dotName(TpSymbolNode *lhs, Identifier rhs);
    Identifier dotName(Identifier lhs, Identifier rhs);

private:
    TpSymbol* instantiateIfZeroArity(TpSymbol *templateSymbol);
};
} // namespace exy