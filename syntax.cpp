#include "pch.h"
#include "syntax.h"

#include "src.h"
#include "parser.h"

#pragma warning(disable: 26495)

namespace exy {
using Pos = const SourceToken&;

bool SyntaxTree::initialize() {
    parse(nullptr, compiler.source->folders);
    if (compiler.errors == 0) {
        discoverModules();
    }
    return compiler.errors == 0;
}
void SyntaxTree::dispose() {
    ldispose(folders);
    ldispose(modules);
    mem.dispose();
}

void SyntaxTree::parse(SyntaxFolder *parent, List<SourceFolder*>& list) {
    for (auto i = 0; i < list.length; i++) {
        parse(parent, list.items[i]);
    }
}

void SyntaxTree::parse(SyntaxFolder *parent, List<SourceFile>& list) {
    for (auto i = 0; i < list.length; i++) {
        parse(parent, list.items[i]);
    }
}

void SyntaxTree::parse(SyntaxFolder *parent, SourceFolder *srcFolder) {
    auto folder = mem.New<SyntaxFolder>(*srcFolder, parent);
    if (parent == nullptr) {
        folders.append(folder);
    } else {
        parent->folders.append(folder);
    }
    parse(folder, srcFolder->folders);
    parse(folder, srcFolder->files);
}

void SyntaxTree::parse(SyntaxFolder *parent, SourceFile &srcFile) {
    auto file = mem.New<SyntaxFile>(srcFile, parent);
    parent->files.append(file);
    Parser parser{ *file };
    parser.run();
    parser.dispose();
}
//----------------------------------------------------------
SyntaxFolder::SyntaxFolder(SourceFolder &src, SyntaxFolder *parent) 
    : SyntaxNode(src.pos(), Kind::Folder), src(src), parent(parent) {}

void SyntaxFolder::dispose() {
    ldispose(folders);
    ldispose(files);
    __super::dispose();
}
//----------------------------------------------------------
SyntaxFile::SyntaxFile(SourceFile &src, SyntaxFolder *parent)
    : SyntaxNode(src.pos(), Kind::File), src(src), parent(parent) {}

void SyntaxFile::dispose() {
    ldispose(nodes);
    __super::dispose();
}
//----------------------------------------------------------
SyntaxModule::SyntaxModule(SyntaxFile *firstFile) : pos(firstFile->src.pos()) {}

SyntaxModule::SyntaxModule(SyntaxFile *main, SyntaxFile *init) : 
    pos(main == nullptr ? init->src.pos() : main->src.pos()), main(main), init(init) {}

void SyntaxModule::dispose() {
    files.dispose();
    main = init = nullptr;
}
//----------------------------------------------------------
ModifierSyntax::ModifierSyntax(Pos pos)
    : SyntaxNode(pos, Kind::Modifier), value(pos.keyword) {}
//----------------------------------------------------------
ModifierListSyntax::ModifierListSyntax(ModifierSyntax *first)
    : SyntaxNode(first->pos, Kind::ModifierList) {
    nodes.append(first);
}

void ModifierListSyntax::dispose() {
    ldispose(nodes);
    __super::dispose();
}

Pos ModifierListSyntax::lastPos() const {
    if (nodes.isNotEmpty()) {
        return nodes.last()->lastPos();
    }
    return __super::lastPos();
}
//----------------------------------------------------------
ModuleSyntax::ModuleSyntax(Pos pos)
    : SyntaxNode(pos, Kind::Module) {}

void ModuleSyntax::dispose() {
    name = ndispose(name);
    system = ndispose(system);
    __super::dispose();
}

Pos ModuleSyntax::lastPos() const {
    if (system != nullptr) {
        return system->lastPos();
    }
    if (kwAs != nullptr) {
        return *kwAs;
    }
    if (name != nullptr) {
        return name->lastPos();
    }
    return __super::lastPos();
}
//----------------------------------------------------------
ImportSyntax::ImportSyntax(Pos pos)
    : SyntaxNode(pos, pos.keyword == Keyword::Import ? Kind::Import : Kind::Export) {}

void ImportSyntax::dispose() {
    name = ndispose(name);
    alias = ndispose(alias);
    source = ndispose(source);
    __super::dispose();
}

Pos ImportSyntax::lastPos() const {
    if (kwFrom != nullptr && kwAs != nullptr) {
        if (kwFrom->pos < kwAs->pos) { // 'from' then 'as'
            if (source != nullptr) {
                return source->lastPos();
            }
            return *kwFrom;
        } // else 'as' then 'from'
        if (alias != nullptr) {
            return alias->lastPos();
        }
        return *kwAs;
    }
    if (source != nullptr) {
        return source->lastPos();
    }
    if (kwFrom != nullptr) {
        return *kwFrom;
    }
    if (alias != nullptr) {
        return alias->lastPos();
    }
    if (kwAs != nullptr) {
        return *kwAs;
    }
    if (name != nullptr) {
        return name->lastPos();
    }
    return __super::lastPos();
}
//----------------------------------------------------------
DefineSyntax::DefineSyntax(Node modifiers, Pos pos)
    : SyntaxNode(pos, Kind::Define), modifiers(modifiers) {}

void DefineSyntax::dispose() {
    modifiers = ndispose(modifiers);
    name = ndispose(name);
    value = ndispose(value);
    __super::dispose();
}

Pos DefineSyntax::lastPos() const {
    if (value != nullptr) {
        return value->lastPos();
    }
    if (name != nullptr) {
        return name->lastPos();
    }
    return __super::lastPos();
}
//----------------------------------------------------------
ExternBlockSyntax::ExternBlockSyntax(Pos pos, Node modifiers)
    : SyntaxNode(pos, Kind::ExternBlock), modifiers(modifiers) {}

void ExternBlockSyntax::dispose() {
    modifiers = ndispose(modifiers);
    name = ndispose(name);
    ldispose(nodes);
    __super::dispose();
}

Pos ExternBlockSyntax::lastPos() const {
    if (close != nullptr) {
        return *close;
    }
    if (nodes.isNotEmpty()) {
        return nodes.last()->lastPos();
    }
    if (open != nullptr) {
        return *open;
    }
    if (name != nullptr) {
        return name->lastPos();
    }
    return __super::lastPos();
}
//----------------------------------------------------------
StructureSyntax::StructureSyntax(Node modifiers, Pos pos)
    : SyntaxNode(pos, Kind::Structure), modifiers(modifiers) {}

void StructureSyntax::dispose() {
    modifiers = ndispose(modifiers);
    name = ndispose(name);
    attributes = ndispose(attributes);
    supers = ndispose(supers);
    body = ndispose(body);
    __super::dispose();
}

Pos StructureSyntax::lastPos() const {
    if (body != nullptr) {
        return body->lastPos();
    }
    if (supers != nullptr) {
        return supers->lastPos();
    }
    if (supersOp != nullptr) {
        return *supersOp;
    }
    if (attributes != nullptr) {
        return attributes->lastPos();
    }
    if (name != nullptr) {
        return name->lastPos();
    }
    return __super::lastPos();
}
//----------------------------------------------------------
FunctionSyntax::FunctionSyntax(Node modifiers, Pos pos)
    : SyntaxNode(pos, Kind::Structure), modifiers(modifiers) {}

void FunctionSyntax::dispose() {
    modifiers = ndispose(modifiers);
    httpVerb = ndispose(httpVerb);
    name = ndispose(name);
    parameters = ndispose(parameters);
    fnreturn = ndispose(fnreturn);
    body = ndispose(body);
    __super::dispose();
}

Pos FunctionSyntax::lastPos() const {
    if (body != nullptr) {
        return body->lastPos();
    }
    if (bodyOp != nullptr) {
        return *bodyOp;
    }
    if (fnreturn != nullptr) {
        return fnreturn->lastPos();
    }
    if (fnreturnOp != nullptr) {
        return *fnreturnOp;
    }
    if (parameters != nullptr) {
        return parameters->lastPos();
    }
    if (kwName != nullptr) {
        return *kwName;
    }
    if (opName != nullptr) {
        return *opName;
    }
    if (name != nullptr) {
        return name->lastPos();
    }
    if (httpVerb != nullptr) {
        return httpVerb->lastPos();
    }
    return __super::lastPos();
}
//----------------------------------------------------------
BlockSyntax::BlockSyntax(Pos pos)
    : SyntaxNode(pos, Kind::Block) {}

BlockSyntax::BlockSyntax(Node modifiers, Pos pos)
    : SyntaxNode(pos, Kind::Block), modifiers(modifiers) {}

void BlockSyntax::dispose() {
    modifiers = ndispose(modifiers);
    ldispose(nodes);
    __super::dispose();
}

Pos BlockSyntax::lastPos() const {
    if (close != nullptr) {
        return *close;
    }
    if (nodes.isNotEmpty()) {
        return nodes.last()->lastPos();
    }
    return __super::lastPos();
}
//----------------------------------------------------------
FlowControlSyntax::FlowControlSyntax(Pos pos)
    : SyntaxNode(pos, Kind::FlowControl) {}

void FlowControlSyntax::dispose() {
    expression = ndispose(expression);
    __super::dispose();
}

Pos FlowControlSyntax::lastPos() const {
    if (expression != nullptr) {
        return expression->lastPos();
    }
    if (kwIf != nullptr) {
        return *kwIf;
    }
    return __super::lastPos();
}
//----------------------------------------------------------
IfSyntax::IfSyntax(Node modifiers, Pos pos)
    : SyntaxNode(pos, Kind::If), modifiers(modifiers) {}

void IfSyntax::dispose() {
    condition = ndispose(condition);
    iftrue    = ndispose(iftrue);
    ifalse    = ndispose(ifalse);
    __super::dispose();
}

Pos IfSyntax::lastPos() const {
    if (ifalse != nullptr) {
        return ifalse->lastPos();
    }
    if (kwElse != nullptr) {
        return *kwElse;
    }
    if (iftrue != nullptr) {
        return iftrue->lastPos();
    }
    if (condition != nullptr) {
        return condition->lastPos();
    }
    return __super::lastPos();
}
//----------------------------------------------------------
SwitchSyntax::SwitchSyntax(Pos pos)
    : SyntaxNode(pos, Kind::Switch) {}

void SwitchSyntax::dispose() {
    condition = ndispose(condition);
    body = ndispose(body);
    __super::dispose();
}

Pos SwitchSyntax::lastPos() const {
    if (body != nullptr) {
        return body->lastPos();
    }
    if (condition != nullptr) {
        return condition->lastPos();
    }
    return __super::lastPos();
}
//----------------------------------------------------------
CaseSyntax::CaseSyntax(Pos pos)
    : SyntaxNode(pos, Kind::Case) {}

void CaseSyntax::dispose() {
    condition = ndispose(condition);
    body = ndispose(body);
    __super::dispose();
}

Pos CaseSyntax::lastPos() const {
    if (body != nullptr) {
        return body->lastPos();
    }
    if (condition != nullptr) {
        return condition->lastPos();
    }
    return __super::lastPos();
}
//----------------------------------------------------------
ForInSyntax::ForInSyntax(Pos pos, Node variables)
    : SyntaxNode(pos, Kind::ForIn), variables(variables) {}

void ForInSyntax::dispose() {
    variables = ndispose(variables);
    expression = ndispose(expression);
    body = ndispose(body);
    ifalse = ndispose(ifalse);
    __super::dispose();
}

Pos ForInSyntax::lastPos() const {
    if (ifalse != nullptr) {
        return ifalse->lastPos();
    }
    if (kwElse != nullptr) {
        return *kwElse;
    }
    if (body != nullptr) {
        return body->lastPos();
    }
    if (expression != nullptr) {
        return expression->lastPos();
    }
    if (kwIn != nullptr) {
        return *kwIn;
    }
    if (variables != nullptr) {
        return variables->lastPos();
    }
    if (kwAwait != nullptr) {
        return *kwAwait;
    }
    return __super::lastPos();
}
//----------------------------------------------------------
ForSyntax::ForSyntax(Pos pos, Node initializer)
    : SyntaxNode(pos, Kind::For), initializer(initializer) {}

void ForSyntax::dispose() {
    initializer = ndispose(initializer);
    condition = ndispose(condition);
    increment = ndispose(increment);
    body = ndispose(body);
    ifalse = ndispose(ifalse);
    __super::dispose();
}

Pos ForSyntax::lastPos() const {
    if (ifalse != nullptr) {
        return ifalse->lastPos();
    }
    if (kwElse != nullptr) {
        return *kwElse;
    }
    if (body != nullptr) {
        return body->lastPos();
    }
    if (increment != nullptr) {
        return increment->lastPos();
    }
    if (condition != nullptr) {
        return condition->lastPos();
    }
    if (initializer != nullptr) {
        return initializer->lastPos();
    }
    return __super::lastPos();
}
//----------------------------------------------------------
WhileSyntax::WhileSyntax(Pos pos)
    : SyntaxNode(pos, Kind::DoWhile) {}

void WhileSyntax::dispose() {
    condition = ndispose(condition);
    body = ndispose(body);
    ifalse = ndispose(ifalse);
    __super::dispose();
}

Pos WhileSyntax::lastPos() const {
    if (ifalse != nullptr) {
        return ifalse->lastPos();
    }
    if (kwElse != nullptr) {
        return *kwElse;
    }
    if (body != nullptr) {
        return body->lastPos();
    }
    if (condition != nullptr) {
        return condition->lastPos();
    }
    return __super::lastPos();
}
//----------------------------------------------------------
DoWhileSyntax::DoWhileSyntax(Pos pos)
    : SyntaxNode(pos, Kind::DoWhile) {}

void DoWhileSyntax::dispose() {
    body = ndispose(body);
    condition = ndispose(condition);
    ifalse = ndispose(ifalse);
    __super::dispose();
}

Pos DoWhileSyntax::lastPos() const {
    if (ifalse != nullptr) {
        return ifalse->lastPos();
    }
    if (kwElse != nullptr) {
        return *kwElse;
    }
    if (condition != nullptr) {
        return condition->lastPos();
    }
    if (body != nullptr) {
        return body->lastPos();
    }
    return __super::lastPos();
}
//----------------------------------------------------------
DeferSyntax::DeferSyntax(Pos pos)
    : SyntaxNode(pos, Kind::Defer) {}

void DeferSyntax::dispose() {
    expression = ndispose(expression);
    __super::dispose();
}

Pos DeferSyntax::lastPos() const {
    if (expression != nullptr) {
        return expression->lastPos();
    }
    return __super::lastPos();
}
//----------------------------------------------------------
UsingSyntax::UsingSyntax(Pos pos)
    : SyntaxNode(pos, Kind::Using) {}

void UsingSyntax::dispose() {
    expression = ndispose(expression);
    statement = ndispose(statement);
    __super::dispose();
}

Pos UsingSyntax::lastPos() const {
    if (statement != nullptr) {
        return statement->lastPos();
    }
    if (expression != nullptr) {
        return expression->lastPos();
    }
    return __super::lastPos();
}
//----------------------------------------------------------
VariableSyntax::VariableSyntax(Node modifiers, Pos pos)
    : SyntaxNode(pos, Kind::Variable), modifiers(modifiers) {}

void VariableSyntax::dispose() {
    modifiers = ndispose(modifiers);
    name = ndispose(name);
    rhs = ndispose(rhs);
    __super::dispose();
}

Pos VariableSyntax::lastPos() const {
    if (rhs != nullptr) {
        return rhs->lastPos();
    }
    if (op != nullptr) {
        return *op;
    }
    if (name != nullptr) {
        return name->lastPos();
    }
    if (modifiers != nullptr) {
        return modifiers->lastPos();
    }
    return __super::lastPos();
}
//----------------------------------------------------------
BinarySyntax::BinarySyntax(Node lhs, Pos op, Node rhs)
    : SyntaxNode(lhs->pos, Kind::Binary), lhs(lhs), op(op), rhs(rhs) {}

void BinarySyntax::dispose() {
    lhs = ndispose(lhs);
    rhs = ndispose(rhs);
    __super::dispose();
}

Pos BinarySyntax::lastPos() const {
    if (rhs != nullptr) {
        return rhs->lastPos();
    }
    return op;
}
//----------------------------------------------------------
TernarySyntax::TernarySyntax(Node condition, Pos question)
    : SyntaxNode(condition->pos, Kind::Ternary), condition(condition), question(question) {}

void TernarySyntax::dispose() {
    condition = ndispose(condition);
    iftrue = ndispose(iftrue);
    ifalse = ndispose(ifalse);
    __super::dispose();
}

Pos TernarySyntax::lastPos() const {
    if (ifalse != nullptr) {
        return ifalse->lastPos();
    }
    if (colon != nullptr) {
        return *colon;
    }
    if (iftrue != nullptr) {
        return iftrue->lastPos();
    }
    return question;
}
//----------------------------------------------------------
IfExpressionSyntax::IfExpressionSyntax(Node iftrue, Pos kwIf)
    : SyntaxNode(iftrue->pos, Kind::IfExpression), iftrue(iftrue), kwIf(kwIf) {}

void IfExpressionSyntax::dispose() {
    iftrue = ndispose(iftrue);
    condition = ndispose(condition);
    ifalse = ndispose(ifalse);
    __super::dispose();
}

Pos IfExpressionSyntax::lastPos() const {
    if (ifalse != nullptr) {
        return ifalse->lastPos();
    }
    if (kwElse != nullptr) {
        return *kwElse;
    }
    if (condition != nullptr) {
        return condition->lastPos();
    }
    return kwIf;
}
//----------------------------------------------------------
UnaryPrefixSyntax::UnaryPrefixSyntax(Pos op)
    : SyntaxNode(op, Kind::UnaryPrefix) {}

void UnaryPrefixSyntax::dispose() {
    with = ndispose(with);
    expression = ndispose(expression);
    __super::dispose();
}

Pos UnaryPrefixSyntax::lastPos() const {
    if (with != nullptr) {
        return with->lastPos();
    }
    if (kwWith != nullptr) {
        return *kwWith;
    }
    if (expression != nullptr) {
        return expression->lastPos();
    }
    if (kwFrom != nullptr) {
        return *kwFrom;
    }
    return __super::lastPos();
}
//----------------------------------------------------------
UnarySuffixSyntax::UnarySuffixSyntax(Node expression, Pos op)
    : SyntaxNode(expression->pos, Kind::UnarySuffix), expression(expression), op(op) {}

void UnarySuffixSyntax::dispose() {
    expression = ndispose(expression);
    __super::dispose();
}

Pos UnarySuffixSyntax::lastPos() const {
    return op;
}
//----------------------------------------------------------
DotSyntax::DotSyntax(Node lhs, Pos dot, Node rhs)
    : SyntaxNode(lhs->pos, Kind::Dot), lhs(lhs), dot(dot), rhs(rhs) {}

void DotSyntax::dispose() {
    lhs = ndispose(lhs);
    rhs = ndispose(rhs);
    __super::dispose();
}

Pos DotSyntax::lastPos() const {
    if (rhs != nullptr) {
        return rhs->lastPos();
    }
    return dot;
}
//----------------------------------------------------------
CallSyntax::CallSyntax(Node name, ParenthesizedSyntax *arguments)
    : PostfixEnclosedSyntax(name, arguments, Kind::Call) {}

void CallSyntax::dispose() {
    with = ndispose(with);
    __super::dispose();
}

Pos CallSyntax::lastPos() const {
    if (with != nullptr) {
        return with->lastPos();
    }
    return __super::lastPos();;
}
//----------------------------------------------------------
IndexSyntax::IndexSyntax(Node name, BracketedSyntax *arguments)
    : PostfixEnclosedSyntax(name, arguments, Kind::Index) {}
//----------------------------------------------------------
TypeNameSyntax::TypeNameSyntax(Node name, AngledSyntax *arguments)
    : PostfixEnclosedSyntax(name, arguments, Kind::TypeName) {}
//----------------------------------------------------------
InitializerSyntax::InitializerSyntax(Node name, BracedSyntax *arguments)
    : PostfixEnclosedSyntax(name, arguments, Kind::Initializer) {}
//----------------------------------------------------------
void EnclosedSyntax::dispose() {
    node = ndispose(node);
    __super::dispose();
}

EnclosedSyntax::Pos EnclosedSyntax::lastPos() const {
    if (close != nullptr) {
        return *close;
    }
    if (node != nullptr) {
        return node->lastPos();
    }
    return __super::lastPos();
}
//----------------------------------------------------------
InterpolationSyntax::InterpolationSyntax(Pos pos, Node first)
    : SyntaxNode(pos, Kind::Interpolation) {
    nodes.append(first);
}

void InterpolationSyntax::dispose() {
    ldispose(nodes);
    __super::dispose();
}

Pos InterpolationSyntax::lastPos() const {
    if (close != nullptr) {
        return *close;
    }
    if (nodes.isNotEmpty()) {
        return nodes.last()->lastPos();
    }
    return __super::lastPos();
}
//----------------------------------------------------------
CodeBlockSyntax::CodeBlockSyntax(Pos pos)
    : SyntaxNode(pos, Kind::CodeBlock) {}

void CodeBlockSyntax::dispose() {
    node = ndispose(node);
    __super::dispose();
}

Pos CodeBlockSyntax::lastPos() const {
    if (close != nullptr) {
        return *close;
    }
    if (node != nullptr) {
        return node->lastPos();
    }
    return __super::lastPos();
}
//----------------------------------------------------------
CommaSeparatedSyntax::CommaSeparatedSyntax(Node first)
    : SyntaxNode(first->pos, Kind::CommaSeparated) {
    nodes.append(first);
}

void CommaSeparatedSyntax::dispose() {
    ldispose(nodes);
    __super::dispose();
}

Pos CommaSeparatedSyntax::lastPos() const {
    if (nodes.isNotEmpty()) {
        return nodes.last()->lastPos();
    }
    return __super::lastPos();
}
//----------------------------------------------------------
NameValueSyntax::NameValueSyntax(Name name, Pos op)
    : SyntaxNode(name->pos, Kind::NameValue), name(name), op(op) {}

void NameValueSyntax::dispose() {
    name = ndispose(name);
    value = ndispose(value);
    __super::dispose();
}

Pos NameValueSyntax::lastPos() const {
    if (value != nullptr) {
        return value->lastPos();
    }
    return op;
}
//----------------------------------------------------------
RestSyntax::RestSyntax(Pos ellipsis)
    : SyntaxNode(ellipsis, Kind::Rest) {}

void RestSyntax::dispose() {
    name = ndispose(name);
    __super::dispose();
}

Pos RestSyntax::lastPos() const {
    if (name != nullptr) {
        return name->lastPos();
    }
    return __super::lastPos();
}
//----------------------------------------------------------
RestParameterSyntax::RestParameterSyntax(Node modifiers, Pos ellipsis)
    : SyntaxNode(ellipsis, Kind::RestParameter), modifiers(modifiers), ellipsis(ellipsis) {}

RestParameterSyntax::RestParameterSyntax(Node modifiers, Name name, Pos ellipsis)
    : SyntaxNode(name->pos, Kind::RestParameter), modifiers(modifiers), name(name), ellipsis(ellipsis) {}

void RestParameterSyntax::dispose() {
    modifiers = ndispose(modifiers);
    name = ndispose(name);
    __super::dispose();
}

Pos RestParameterSyntax::lastPos() const {
    return ellipsis;
}
//----------------------------------------------------------
TextSyntax::TextSyntax(Pos pos, const String &value, Pos end)
    : SyntaxNode(pos, Kind::Text), value(value), end(end) {}

Pos TextSyntax::lastPos() const {
    return end;
}
//----------------------------------------------------------
IdentifierSyntax::IdentifierSyntax(Pos pos)
    : SyntaxNode(pos, Kind::Identifier), value(ids.get(pos.sourceValue()))  {}
//----------------------------------------------------------
SingleQuotedSyntax::SingleQuotedSyntax(Pos pos, const String &value, Pos close)
    : SyntaxNode(pos, Kind::SingleQuoted), value(value), close(close) {}

Pos SingleQuotedSyntax::lastPos() const {
    return close;
}
//----------------------------------------------------------
DoubleQuotedSyntax::DoubleQuotedSyntax(Pos pos, const String &value, Pos close)
    : SyntaxNode(pos, Kind::DoubleQuoted), value(value), close(close) {}

Pos DoubleQuotedSyntax::lastPos() const {
    return close;
}
//----------------------------------------------------------
BooleanSyntax::BooleanSyntax(Pos pos)
    : SyntaxNode(pos, Kind::Boolean), value(pos.keyword == Keyword::True) {}
//----------------------------------------------------------
NumberSyntax::NumberSyntax(Pos pos)
    : SyntaxNode(pos, Kind::Number) {}
} // namespace exy
