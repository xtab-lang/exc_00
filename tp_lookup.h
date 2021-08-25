#pragma once

namespace exy {
struct tp_lookup {
    using Pos = SyntaxNode*;

    Typer &tp;

    tp_lookup();
    void dispose();

    bool initialize(Pos pos);

    TpScope* searchableScopeOf(Type);

    TpNode* find(Pos pos, Identifier name);
    TpNode* find(Pos pos, TpScope*, Identifier name);
    TpNode* find(Pos pos, TpNode *base, Identifier name);
private:
    struct CaptureFrame {
        TpScope      *scope;
        CaptureFrame *next;
        CaptureFrame *prev;
    };
    struct CaptureStack {
        Typer &tp;
        CaptureFrame *top;
        CaptureFrame *bottom;
        CaptureStack();
        bool crossesIllegalBoundaries();
        TpNode* propagate(Pos pos, TpSymbol *found);
    private:
        TpNode* propagateLocalOrParameter(Pos, TpSymbol *found);
        TpNode* propagateField(Pos, TpSymbol *found);
        TpNode* propagateFieldToLambdaFunction(Pos, CaptureFrame*, TpSymbol *field);
        TpNode* propagateFieldToLambdaStruct(Pos, CaptureFrame*, TpSymbol *thisParameter, TpSymbol *field);
    };
    struct CaptureList {
        Typer &tp;
        CaptureList();
        void dispose();
        CaptureStack make(TpSymbol *found);
        auto isaOneStackFrameCapture() { return list.length == 1; }
    private:
        void place(TpScope *scope);
        List<CaptureFrame> list{};
    };

    static Identifier kw__MODULE__;
    static Identifier kw__FOLDER__;
    static Identifier kw__FILE__;
    static Identifier kw__FUNCTION__;
    static Identifier kw__LINE__;
    static Identifier kw__COL__;
};
} // namespace exy
