#pragma once

namespace exy {
struct tp_cast {
    List<tp_cast> list; // Memberwise casts if {kind} is MemberWise.
    TpType        dst;
    tp_cast_kind  kind;
    tp_cast(Type dst, tp_cast_kind kind) : dst(dst), kind(kind) {}
    void dispose();
};

struct tp_cast_list {
    Type &src, &dst;
    List<tp_cast>  casts{};
    tp_cast_reason reason;
    bool          ok = true;
    tp_cast_list(Type &src, Type &dst, tp_cast_reason reason) : src(src), dst(dst), reason(reason) {}
    void dispose();
    auto failed() { return !ok; }
    auto isImplicit() { return reason == tp_cast_reason::ImplicitCast; }
};
} // namespace exy