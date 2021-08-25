#include "pch.h"
#include "typer.h"

namespace exy {
#define fail_if_implicit() do { if (result.isImplicit()) return fail(); } while (0)
#define fail_if(zExpr) do { if ((zExpr)) return fail(); } while (0)

static const TpType& signedOf(INT size) {
    auto &tree = *compiler.tpTree;
    switch (size) {
        case SIZEOF_BYTE:    return tree.tyInt8;
        case SIZEOF_SHORT:   return tree.tyInt16;
        case SIZEOF_INT:     return tree.tyInt32;
        case SIZEOF_POINTER: return tree.tyInt64;
    }
    UNREACHABLE();
}

static const TpType& unsignedOf(INT size) {
    auto &tree = *compiler.tpTree;
    switch (size) {
        case SIZEOF_BYTE:    return tree.tyUInt8;
        case SIZEOF_SHORT:   return tree.tyUInt16;
        case SIZEOF_INT:     return tree.tyUInt32;
        case SIZEOF_POINTER: return tree.tyUInt64;
    }
    UNREACHABLE();
}

static const TpType& floatOf(INT size) {
    auto &tree = *compiler.tpTree;
    switch (size) {
        case SIZEOF_FLOAT:  return tree.tyFloat;
        case SIZEOF_DOUBLE: return tree.tyDouble;
    }
    UNREACHABLE();
}

//----------------------------------------------------------
using Result = tp_cast_list&;
using   Kind = tp_cast_kind;

struct Caster {
    using Type     = const TpType&;
    using Indirect = TpIndirectType*;
    Result result;
    Typer &tp;

    Caster(Result result) : result(result), tp(*typer) {}

    Result fail() { result.ok = false; return result; }
    Caster& place(Type dst, Kind kind) { result.casts.place(dst, kind); return *this; }

    Caster& fromNull(Type dst) { return place(dst, Kind::FromNullCast); }
    Caster& widen(Type dst) { return place(dst, Kind::WideningCast); }
    Caster& narrow(Type dst) { return place(dst, Kind::NarrowingCast); }
    Caster& bitCast(Type dst) { return place(dst, Kind::BitCast); }
    Caster& addressOf(Type dst) { return place(dst, Kind::AddressOfCast); }
    Caster& referenceOf(Type dst) { return place(dst, Kind::ReferenceOfCast); }
    Caster& dereference(Type dst) { return place(dst, Kind::DereferenceCast); }
    Caster& toBool() { return place(tp.tree.tyBool, Kind::ToBoolCast); }
    Caster& fp(Type dst) { return place(dst, Kind::FloatCast); }
    Caster& packed(Type dst, Kind kind) { return place(dst, kind); }

    Result PointerToPointer(Indirect src, Indirect dst);
    Result PointerToReference(Indirect src, Indirect dst);
    Result ReferenceToPointer(Indirect src, Indirect dst);
    Result ReferenceToReference(Indirect src, Indirect dst);

    Result PointerToIntegral(TpSymbol *dstsym);
    Result IntegralToPointer(TpSymbol *srcsym, Type dst);

    Result PointerToFloatingPoint(TpSymbol *dstsym);
    Result FloatingPointToPointer(TpSymbol *srcsym, Type dst);

    Result SameSign(TpSymbol *srcsym, TpSymbol *dstsym);
    Result SignedToUnsigned(TpSymbol *srcsym, TpSymbol *dstsym);
    Result SignedToFloatingPoint(TpSymbol *srcsym, TpSymbol *dstsym);
    Result UnsignedToSigned(TpSymbol *srcsym, TpSymbol *dstsym);
    Result UnsignedToFloatingPoint(TpSymbol *srcsym, TpSymbol *dstsym);

    Result FloatingPointToSigned(TpSymbol *srcsym, TpSymbol *dstsym);
    Result FloatingPointToUnsigned(TpSymbol *srcsym, TpSymbol *dstsym);
    Result FloatingPointToFloatingPoint(TpSymbol *srcsym, TpSymbol *dstsym);

    Result PackedFloatingPointsToPackedFloatingPoints(TpSymbol *srcsym, TpSymbol *dstsym);
    Result PackedFloatingPointsToPackedIntegers(TpSymbol *srcsym, TpSymbol *dstsym);

    Result PackedIntegersToPackedFloatingPoints(TpSymbol *srcsym, TpSymbol *dstsym);
    Result PackedIntegersToPackedIntegers(TpSymbol *srcsym, TpSymbol *dstsym);

    Result FloatingPointToPackedFloatingPoints(TpSymbol *srcsym, TpSymbol *dstsym);
    Result FloatingPointToPackedIntegers(TpSymbol *srcsym, TpSymbol *dstsym);

    Result IntegerToPackedFloatingPoints(TpSymbol *srcsym, TpSymbol *dstsym);
    Result IntegerToPackedIntegers(TpSymbol *srcsym, TpSymbol *dstsym);
};

//----------------------------------------------------------
void tp_cast::dispose() {
    list.dispose();
}

//----------------------------------------------------------
void tp_cast_list::dispose() {
    casts.dispose([](auto &x) { x.dispose(); });
}

//----------------------------------------------------------
TpNode* Typer::bitCast(SyntaxNode *pos, TpNode *value, const TpType &dst, tp_cast_reason reason) {
    auto result = canBitCast(value, dst, reason);
    value = cast(pos, value, dst, result);
    result.dispose();
    return value;
}

TpNode* Typer::cast(SyntaxNode *pos, TpNode *value, const TpType &dst, tp_cast_reason reason) {
    auto result = canCast(value, dst, reason);
    value = cast(pos, value, dst, result);
    result.dispose();
    return value;
}

TpNode* Typer::cast(SyntaxNode *syntax, TpNode *value, const TpType &dst, tp_cast_list &conversion) {
    auto  pos = mkPos(syntax);
    auto &src = value->type;
    if (conversion.failed()) {
        conversion.dispose();
        cast_error(pos, conversion);
        return throwAway(value);
    }
    if (src == dst) {
        Assert(conversion.casts.isEmpty());
        conversion.dispose();
        return value;
    }
    Assert(conversion.casts.isNotEmpty());
    auto         node = value;
    auto isaConstExpr = isa.ConstExpr(node) != nullptr;
    for (auto i = 0; i < conversion.casts.length && node != nullptr; i++) {
        auto &c = conversion.casts.items[i];
        Assert(c.kind != Kind::NoCast);
        if (c.kind < Kind::_end_casts) {
            node = mem.New<TpCast>(pos, c.dst, node, c.kind, conversion.reason);
            if (isaConstExpr) {
                node = mk.ConstExpr(syntax, node);
            }
        } else switch (c.kind) {
            case Kind::AddressOfCast: {
                node = mk.AddressOf(syntax, node);
            } break;
            case Kind::ReferenceOfCast: {
                node = mk.ReferenceOf(syntax, node);
            } break;
            case Kind::DereferenceCast: {
                node = mk.Dereference(syntax, node);
            } break;
            case Kind::ToBoolCast: {
                node = mk.Condition(syntax, node, Tok::NotEqual, mk.ZeroOf(syntax, node->type));
            } break;
            default:
                Assert(0);
                break;
        }
        isaConstExpr = isa.ConstExpr(node) != nullptr;
    }
    conversion.dispose();
    return node;
}

tp_cast_list Typer::canCast(TpNode *value, const TpType &dst, tp_cast_reason reason) {
    const auto &src = value->type;
    tp_cast_list result{ src, dst, reason };
    Caster cast{ result };
    const auto isImplicit = result.isImplicit();
    if (src == dst) {
        return result;
    }
    //------------------------------------------------------
    if (src.isVoid() || dst.isVoid()) { // Bad. implicit/explicit Void → Any or Any → Void
        return cast.fail();
    }
    //------------------------------------------------------
    if (isa.Null(value)) {
        // Ok. implicit/explicit null → Any
        return cast.fromNull(dst).result;
    }
    //------------------------------------------------------
    if (src.isaModule() || dst.isaModule()) {
        // Bad. implicit/explicit Module → Any or Any → Module
        return cast.fail();
    }
    if (src.isOverloadSet() || dst.isOverloadSet()) {
        // Bad. implicit/explicit OverloadSet → Any or Any → OverloadSet
        return cast.fail();
    }
    if (src.isaTemplate() || dst.isaTemplate()) {
        // Bad. implicit/explicit Template → Any or Any → Template
        return cast.fail();
    }
    if (src.isaStruct()) {
        if (dst.isaStruct()) {
            Assert(0);
        } // Bad. implicit/explicit Struct → Any
        return cast.fail();
    }
    if (dst.isaStruct()) { // Bad. implicit/explicit Any → Struct
        return cast.fail();
    }
    if (src.isaFunction()) {
        if (dst.isaFunction() || dst.isVoidPointer()) { 
            // Ok. implicit/explicit Function1 → Function2 or Function → Void*
            return cast.bitCast(dst).result;
        } // Bad. implicit/explicit Function → Any
        return cast.fail();
    }
    if (dst.isaFunction()) {
        if (src.isVoidPointer()) { // Ok. implicit/explicit Void* → Function
            return cast.bitCast(dst).result;
        } // Bad. implicit/explicit Any → Function
        return cast.fail();
    }
    //------------------------------------------------------
    if (auto packedsrc = src.isPacked()) {
        if (auto   packedst = dst.isPacked()) {
            auto srcbuiltin = (TpBuiltin*)packedsrc->node;
            auto dstbuiltin = (TpBuiltin*)packedst->node;
            auto srcpacking = srcbuiltin->packing();
            auto dstpacking = dstbuiltin->packing();
            if (srcpacking.type.isaFloatingPoint()) {
                if (dstpacking.type.isaFloatingPoint()) {
                    return cast.PackedFloatingPointsToPackedFloatingPoints(packedsrc, packedst);
                }
                return cast.PackedFloatingPointsToPackedIntegers(packedsrc, packedst);
            }
            if (srcpacking.type.isIntegral()) {
                if (dstpacking.type.isaFloatingPoint()) {
                    return cast.PackedIntegersToPackedFloatingPoints(packedsrc, packedst);
                }
                return cast.PackedIntegersToPackedIntegers(packedsrc, packedst);
            }
            if (dstpacking.type.isaFloatingPoint()) {
                return cast.PackedIntegersToPackedFloatingPoints(packedsrc, packedst);
            }
            return cast.PackedIntegersToPackedIntegers(packedsrc, packedst);
        }
        return cast.fail();
    }
    if (auto packedst = dst.isPacked()) {
        if (auto numericsrc = src.isNumeric()) {
            auto srcbuiltin = (TpBuiltin*)numericsrc->node;
            auto dstbuiltin = (TpBuiltin*)packedst->node;
            auto dstpacking = dstbuiltin->packing();
            if (srcbuiltin->type.isaFloatingPoint()) {
                if (dstpacking.type.isaFloatingPoint()) {
                    return cast.FloatingPointToPackedFloatingPoints(numericsrc, packedst);
                }
                return cast.FloatingPointToPackedIntegers(numericsrc, packedst);
            }
            if (dstpacking.type.isaFloatingPoint()) {
                return cast.IntegerToPackedFloatingPoints(numericsrc, packedst);
            }
            return cast.IntegerToPackedIntegers(numericsrc, packedst);
        }
        return cast.fail();
    }
    //------------------------------------------------------
    if (dst.isBool()) {
        if (src.isaBuiltin()) { // Ok. implicit/explicit Builtin → Bool
            return cast.toBool().result;
        }
        if (isImplicit) { // Bad. implicit/explicit T → Bool
            return cast.fail();
        }
        if (src.isIndirect()) { // Ok. explicit T* → Bool or T& → Bool
            return cast.toBool().result;
        } // Bad. implicit/explicit Other → Bool
        return cast.fail();
    }
    //------------------------------------------------------
    //    Void* → Any 
    if (src.isVoidPointer()) { // {src} is Void* or Void&
        if (dst.isIndirect()) { // Ok. implicit/explicit Void*/& → T*/&
            return cast.bitCast(dst).result;
        }
        if (isImplicit) { // Bad. implicit Void* → Any
            return cast.fail();
        }
        if (const auto dstsym = dst.isIntegral()) { // Ok. explicit Void*/& → Integral
            return cast.PointerToIntegral(dstsym);
        }
        if (const auto dstsym = dst.isaFloatingPoint()) { // Ok. explicit Void*/& → FloatingPoint
            return cast.PointerToFloatingPoint(dstsym);
        }
        if (dst.isDirect()) { // Void*/& → T* → T
            return cast.bitCast(dst.mkPointer()).dereference(dst).result;
        }
        Assert(0);
    }
    // Any → Void*
    if (dst.isVoidPointer()) { // {dst} is Void* or Void&.
        if (src.isIndirect()) { // Ok. implicit/explicit T*/& → Void*/&
            return cast.bitCast(dst).result;
        }
        if (isImplicit) { // Bad. implicit Any → Void*
            return cast.fail();
        }
        if (const auto srcsym = src.isIntegral()) { // Ok. explicit Integral → Void*/&
            return cast.IntegralToPointer(srcsym, dst);
        }
        if (const auto srcsym = src.isaFloatingPoint()) { // Ok. explicit FloatingPoint → Void*/&
            return cast.FloatingPointToPointer(srcsym, dst);
        }
        if (src.isDirect()) { // Ok. explicit T → T& → Void*/&
            return cast.referenceOf(dst.mkReference()).bitCast(dst).result;
        }
        Assert(0);
    }
    //------------------------------------------------------
    if (auto srcptr = src.isaPointer()) {
        if (auto dstptr = dst.isaPointer()) { // Ok. explicit T* → U*
            return cast.PointerToPointer(srcptr, dstptr);
        }
        if (auto dstref = dst.isaReference()) { // Ok. explicit T* → U&
            return cast.PointerToReference(srcptr, dstref);
        }
        if (isImplicit) { // Bad. implicit T* → Any
            return cast.fail();
        }
        if (const auto dstsym = dst.isIntegral()) { // Ok. explicit T* → Integral
            return cast.PointerToIntegral(dstsym);
        }
        Assert(0);
    }
    if (auto dstptr = dst.isaPointer()) {
        if (auto srcref = src.isaReference()) { // Ok. explicit T& → U*
            return cast.ReferenceToPointer(srcref, dstptr);
        }
        if (isImplicit) { // Bad. implicit Any → T*
            return cast.fail();
        }
        if (auto srcsym = src.isIntegral()) { // Ok. explicit Integral → T*
            return cast.IntegralToPointer(srcsym, dst);
        }
        Assert(0);
    }
    //------------------------------------------------------
    if (auto srcref = src.isaReference()) {
        if (auto dstref = dst.isaReference()) { // Ok. explicit T& → U&
            return cast.ReferenceToReference(srcref, dstref);
        }
        if (isImplicit) { // Bad. implicit T& → Any
            return cast.fail();
        }
        Assert(0);
    }
    if (auto dstref = dst.isaReference()) {
        Assert(0);
    }
    //------------------------------------------------------
    if (auto srcsym = src.isSigned()) {
        if (auto dstsym = dst.isSigned()) {
            return cast.SameSign(srcsym, dstsym);
        }
        if (auto dstsym = dst.isUnsigned()) {
            return cast.SignedToUnsigned(srcsym, dstsym);
        }
        if (auto dstsym = dst.isaFloatingPoint()) {
            return cast.SignedToFloatingPoint(srcsym, dstsym);
        }
        Assert(0);
    }
    if (auto srcsym = src.isUnsigned()) {
        if (auto dstsym = dst.isSigned()) {
            return cast.UnsignedToSigned(srcsym, dstsym);
        }
        if (auto dstsym = dst.isUnsigned()) {
            return cast.SameSign(srcsym, dstsym);
        }
        if (auto dstsym = dst.isaFloatingPoint()) {
            return cast.UnsignedToFloatingPoint(srcsym, dstsym);
        }
        Assert(0);
    }
    if (auto srcsym = src.isaFloatingPoint()) {
        if (auto dstsym = dst.isSigned()) {
            return cast.FloatingPointToSigned(srcsym, dstsym);
        }
        if (auto dstsym = dst.isUnsigned()) {
            return cast.FloatingPointToUnsigned(srcsym, dstsym);
        }
        if (auto dstsym = dst.isaFloatingPoint()) {
            return cast.FloatingPointToFloatingPoint(srcsym, dstsym);
        }
        Assert(0);
    }
    //------------------------------------------------------
    Assert(0);
    return cast.fail();
}

tp_cast_list Typer::canBitCast(TpNode *value, const TpType &dst, tp_cast_reason reason) {
    const auto &src = value->type;
    tp_cast_list result{ src, dst, reason };
    Caster cast{ result };
    const auto isImplicit = result.isImplicit();
    if (src == dst) {
        return result;
    }
    //------------------------------------------------------
    if (src.isVoid() || dst.isVoid()) { // Bad. explicit Void → Any or Any → Void
        return cast.fail();
    }
    //------------------------------------------------------
    if (src.isIndirect()) {
        if (dst.isIndirect()) { // Ok. explicit T*/& → U*/&
            return cast.bitCast(dst).result;
        }
        if (auto dstsym = dst.isIntegral()) {
            auto dstint = (TpBuiltin*)dstsym->node;
            if (dstint->size == SIZEOF_POINTER) { // Ok. explicit T* → U/Int64
                return cast.bitCast(dst).result;
            }
            return cast.fail();
        }
        if (dst.isDouble()) { // Ok. explicit T*/& → Int64 → Double.
            return cast.bitCast(tree.tyInt64).bitCast(dst).result;
        }
        return cast.fail();
    }
    if (dst.isIndirect()) {
        if (auto srcsym = src.isIntegral()) {
            auto srcint = (TpBuiltin*)srcsym->node;
            if (srcint->size == SIZEOF_POINTER) { // Ok. explicit T*/& → U/Int64
                return cast.bitCast(dst).result;
            }
            return cast.fail();
        }
        if (src.isDouble()) { // Ok. explicit Double → Int64 → T*/&.
            return cast.bitCast(tree.tyInt64).bitCast(dst).result;
        }
        return cast.fail();
    }
    //------------------------------------------------------
    if (auto srcsym = src.isIntegral()) {
        auto srcint = (TpBuiltin*)srcsym->node;
        if (auto dstsym = dst.isIntegral()) {
            auto dstint = (TpBuiltin*)dstsym->node;
            if (dstint->size == srcint->size) {
                return cast.bitCast(dst).result;
            }
            return cast.fail();
        }
        if (auto dstsym = dst.isaFloatingPoint()) {
            auto dstflt = (TpBuiltin*)dstsym->node;
            if (srcint->size == dstflt->size) {
                return cast.bitCast(dst).result;
            }
            if (srcint->size < dstflt->size) {
                if (src.isSigned()) {
                    return cast.widen(signedOf(dstflt->size)).bitCast(dst).result;
                }
                return cast.widen(unsignedOf(dstflt->size)).bitCast(dst).result;
            }
            if (src.isSigned()) {
                return cast.narrow(signedOf(dstflt->size)).bitCast(dst).result;
            }
            return cast.narrow(unsignedOf(dstflt->size)).bitCast(dst).result;
        }
        return cast.fail();
    }
    if (auto dstsym = dst.isIntegral()) {
        auto dstint = (TpBuiltin*)dstsym->node;
        if (auto srcsym = src.isaFloatingPoint()) {
            auto srcflt = (TpBuiltin*)srcsym->node;
            if (dstint->size == srcflt->size) {
                return cast.bitCast(dst).result;
            }
        }
        return cast.fail();
    }
    Assert(0);
    return cast.fail();
}

//----------------------------------------------------------
Result Caster::PointerToPointer(Indirect src, Indirect dst) {
    Assert(src->pointee != dst->pointee);
    auto s = src;
    auto d = dst;
    TpSymbol *srcsym = nullptr;
    TpSymbol *dstsym = nullptr;
    while (true) {
        if (auto srcptr = s->pointee.isIndirect()) {
            if (auto dstptr = d->pointee.isIndirect()) {
                s = srcptr;
                d = dstptr;
            } else {
                break;
            }
        } else {
            srcsym = s->pointee.isDirect();
            dstsym = d->pointee.isDirect();
            break;
        }
    }
    if (srcsym && dstsym) {
        auto srcb = tp.isa.Builtin(srcsym);
        auto dstb = tp.isa.Builtin(dstsym);
        if (srcb != nullptr && dstb != nullptr) {
            if (srcb->type.isIntegral() && dstb->type.isIntegral()) {
                if (srcb->size == dstb->size) {
                    // Ok. implicit/explicit T* → U* where both T and U are Integrals of the same size
                    return bitCast(TpType(dst, TpType::Kind::Pointer)).result;
                }
            }
        }
    }
    if (result.isImplicit()) {
        return fail();
    }
    return bitCast(TpType(dst, TpType::Kind::Pointer)).result;
}

Result Caster::PointerToReference(Indirect src, Indirect dst) {
    Assert(0);
    return fail();
}

Result Caster::ReferenceToPointer(Indirect src, Indirect dst) {
    Assert(0);
    return fail();
}

Result Caster::ReferenceToReference(Indirect src, Indirect dst) {
    Assert(0);
    return fail();
}

Result Caster::PointerToIntegral(TpSymbol *dstsym) {
    const auto &dst = dstsym->node->type;
    const auto node = (TpBuiltin*)dstsym->node;
    const auto sign = dst.isSigned();
    if (node->size < SIZEOF_POINTER) {
        if (sign) { // Ok. explicit T* → Int64 → U
            return bitCast(tp.tree.tyInt64).narrow(dst).result;
        } // Ok. explicit T* → UInt64 → U
        return bitCast(tp.tree.tyUInt64).narrow(dst).result;
    } // Ok. explicit T* → U/Int64
    return bitCast(dst).result;
}

Result Caster::IntegralToPointer(TpSymbol *srcsym, Type dst) {
    const auto  src = (TpBuiltin*)srcsym->node;
    const auto sign = src->type.isSigned();
    if (src->size < SIZEOF_POINTER) {
        if (sign) { // Ok. explicit T → Int64 → U*
            return widen(tp.tree.tyInt64).bitCast(dst).result;
        } // Ok. explicit T → UInt64 → U*
        return widen(tp.tree.tyUInt64).bitCast(dst).result;
    } // Ok. explicit U/Int64 → U*
    return bitCast(dst).result;
}

Result Caster::PointerToFloatingPoint(TpSymbol *dstsym) {
    const auto &dst = dstsym->node->type;
    const auto node = (TpBuiltin*)dstsym->node;
    if (node->size == SIZEOF_FLOAT) {
        // Void* → UInt64 → UInt32 → Float
        return bitCast(tp.tree.tyUInt64).narrow(tp.tree.tyUInt32).bitCast(dst).result;
    } // Void* → UInt64 → Double
    return bitCast(tp.tree.tyUInt64).bitCast(dst).result;
}

Result Caster::FloatingPointToPointer(TpSymbol *srcsym, Type dst) {
    const auto node = (TpBuiltin*)srcsym->node;
    if (node->size < SIZEOF_POINTER) {
        // Float → UInt32 → UInt64 → Void*
        return bitCast(tp.tree.tyUInt32).widen(tp.tree.tyUInt64).bitCast(dst).result;
    } // Double → UInt64 → Void*
    return bitCast(tp.tree.tyUInt64).bitCast(dst).result;
}

Result Caster::SameSign(TpSymbol *srcsym, TpSymbol *dstsym) {
    auto src = (TpBuiltin*)srcsym->node;
    auto dst = (TpBuiltin*)dstsym->node;
    if (src->size < dst->size) {
        return widen(dst->type).result;
    }
    if (result.isImplicit()) {
        return fail();
    }
    return narrow(dst->type).result;
}

Result Caster::SignedToUnsigned(TpSymbol *srcsym, TpSymbol *dstsym) {
    if (result.isImplicit()) {
        return fail();
    }
    auto src = (TpBuiltin*)srcsym->node;
    auto dst = (TpBuiltin*)dstsym->node;
    if (src->size == dst->size) {
        return bitCast(dst->type).result;
    }
    if (src->size < dst->size) {
        const auto &tmp = signedOf(dst->size);
        return widen(tmp).bitCast(dst->type).result;
    }
    const auto &tmp = signedOf(dst->size);
    return narrow(tmp).bitCast(dst->type).result;
}

Result Caster::SignedToFloatingPoint(TpSymbol *srcsym, TpSymbol *dstsym) {
    auto src = (TpBuiltin*)srcsym->node;
    auto dst = (TpBuiltin*)dstsym->node;
    if (src->size == dst->size) { // Ok. implicit/explicit Signed → FloatingPoint
        return fp(dst->type).result;
    }
    if (src->size < dst->size) { // Ok. implicit/explicit small Signed → large FloatingPoint
        return widen(signedOf(dst->size)).fp(dst->type).result;
    }
    if (result.isImplicit()) { // Bad. implicit large Signed → small FloatingPoint
        return fail();
    } // Ok. explicit large Signed → small FloatingPoint
    return narrow(signedOf(dst->size)).fp(dst->type).result;
}

Result Caster::UnsignedToSigned(TpSymbol *srcsym, TpSymbol *dstsym) {
    if (result.isImplicit()) {
        return fail();
    }
    auto src = (TpBuiltin*)srcsym->node;
    auto dst = (TpBuiltin*)dstsym->node;
    if (src->size == dst->size) {
        return bitCast(dst->type).result;
    }
    if (src->size < dst->size) {
        const auto &tmp = unsignedOf(dst->size);
        return widen(tmp).bitCast(dst->type).result;
    }
    const auto &tmp = unsignedOf(dst->size);
    return narrow(tmp).bitCast(dst->type).result;
}

Result Caster::UnsignedToFloatingPoint(TpSymbol *srcsym, TpSymbol *dstsym) {
    auto src = (TpBuiltin*)srcsym->node;
    auto dst = (TpBuiltin*)dstsym->node;
    if (src->size == dst->size) { // Ok. implicit/explicit UInt32 → Float or UInt64 → Double
        return fp(dst->type).result;
    }
    if (src->size < dst->size) { // Ok. implicit/explicit small Unsigned → large FloatingPoint
        return widen(unsignedOf(dst->size)).fp(dst->type).result;
    }
    if (result.isImplicit()) { // Bad. implicit large Unsigned → small FloatingPoint
        return fail();
    } // Ok. explicit large Unsigned → small FloatingPoint
    return narrow(unsignedOf(dst->size)).fp(dst->type).result;
}

Result Caster::FloatingPointToSigned(TpSymbol *srcsym, TpSymbol *dstsym) {
    auto src = (TpBuiltin*)srcsym->node;
    auto dst = (TpBuiltin*)dstsym->node;
    if (src->size == dst->size) { // Ok. implicit/explicit Float → Int32 or Double → Int64
        return fp(dst->type).result;
    }
    if (src->size < dst->size) { // Ok. implicit/explicit Float → Int64
        return fp(dst->type).result;
    }
    if (result.isImplicit()) { // Bad. implicit Double → small Signed
        return fail();
    }
    if (dst->size == SIZEOF_INT) { // Ok. explicit Double → Int32
        return fp(dst->type).result;
    } // Ok. explicit Double → Int32 → small Signed
    return fp(tp.tree.tyInt32).narrow(dst->type).result;
}

Result Caster::FloatingPointToUnsigned(TpSymbol *srcsym, TpSymbol *dstsym) {
    auto src = (TpBuiltin*)srcsym->node;
    auto dst = (TpBuiltin*)dstsym->node;
    if (src->size == dst->size) { // Ok. implicit/explicit Float → UInt32 or Double → UInt64
        return fp(dst->type).result;
    }
    if (src->size < dst->size) { // Ok. implicit/explicit Float → UInt64
        return fp(dst->type).result;
    }
    if (result.isImplicit()) { // Bad. implicit Double → small Unsigned
        return fail();
    }
    if (dst->size == SIZEOF_INT) { // Ok. explicit Double → UInt
        return fp(dst->type).result;
    } // Ok. explicit Double → UInt32 → small Unsigned
    return fp(tp.tree.tyUInt32).narrow(dst->type).result;
}

Result Caster::FloatingPointToFloatingPoint(TpSymbol *srcsym, TpSymbol *dstsym) {
    auto src = (TpBuiltin*)srcsym->node;
    auto dst = (TpBuiltin*)dstsym->node;
    if (src->size < dst->size) { // Ok. implicit/explicit Float → Double
        return fp(dst->type).result;
    }
    if (result.isImplicit()) { // Bad. implicit Double → Float
        return fail();
    } // Ok. explicit Double → Float
    return fp(dst->type).result;
}

Result Caster::PackedFloatingPointsToPackedFloatingPoints(TpSymbol *srcsym, TpSymbol *dstsym) {
    auto  src = (TpBuiltin*)srcsym->node;
    auto  dst = (TpBuiltin*)dstsym->node;
    auto   srcpacking = src->packing();
    auto   dstpacking = dst->packing();
    auto &srcpacktype = srcpacking.type;
    auto &dstpacktype = dstpacking.type;
    auto   srcpacksym = srcpacktype.isaBuiltin();
    auto   dstpacksym = dstpacktype.isaBuiltin();
    auto srcpackcount = srcpacking.count;
    auto dstpackcount = dstpacking.count;
    auto  srcpacknode = (TpBuiltin*)srcpacksym->node;
    auto  dstpacknode = (TpBuiltin*)dstpacksym->node;
    auto  srcpacksize = srcpacknode->size;
    auto  dstpacksize = dstpacknode->size;
    fail_if(srcpackcount != dstpackcount); // Cannot change pack count.
    if (srcpacksize < dstpacksize) {
        // Ok. implicit/explicit Floatx4 → Doublex4
        //                    or Floatx8 → Doublex8
        return packed(dst->type, Kind::PackedFloatCast).result;
    }
    fail_if_implicit(); // Losing bits must be explicit.
    // Ok. explicit Doublex4 → Floatx4
    //           or Doublex8 → Floatx8
    return packed(dst->type, Kind::PackedFloatCast).result;
}

Result Caster::PackedFloatingPointsToPackedIntegers(TpSymbol *srcsym, TpSymbol *dstsym) {
    auto  src = (TpBuiltin*)srcsym->node;
    auto  dst = (TpBuiltin*)dstsym->node;
    auto   srcpacking = src->packing();
    auto   dstpacking = dst->packing();
    auto &srcpacktype = srcpacking.type;
    auto &dstpacktype = dstpacking.type;
    auto   srcpacksym = srcpacktype.isaBuiltin();
    auto   dstpacksym = dstpacktype.isaBuiltin();
    auto srcpackcount = srcpacking.count;
    auto dstpackcount = dstpacking.count;
    auto  srcpacknode = (TpBuiltin*)srcpacksym->node;
    auto  dstpacknode = (TpBuiltin*)dstpacksym->node;
    auto  srcpacksize = srcpacknode->size;
    auto  dstpacksize = dstpacknode->size;
    fail_if(srcpackcount != dstpackcount); // Cannot change pack count.
    fail_if(dstpacksize < SIZEOF_INT);     // Cannot reduce pack size to less than 32 bits.
    if (srcpacksize <= dstpacksize) {
        // Ok. explicit Floatx4  → U/Int32x4 | U/Int64x4
        //           or Floatx8  → U/Int32x8 | U/Int64x8
        //           or Doublex2 → U/Int64x2
        //           or Doublex4 → U/Int64x4
        //           or Doublex8 → U/Int64x8
        return packed(dst->type, Kind::PackedFloatCast).result;
    }
    fail_if_implicit(); // Losing bits must be explicit.
    // Ok. explicit Doublex2 → U/Int32x2
    //           or Doublex4 → U/Int32x4
    //           or Doublex8 → U/Int32x8
    return packed(dst->type, Kind::PackedFloatCast).result;
}

Result Caster::PackedIntegersToPackedFloatingPoints(TpSymbol *srcsym, TpSymbol *dstsym) {
    auto  src = (TpBuiltin*)srcsym->node;
    auto  dst = (TpBuiltin*)dstsym->node;
    auto   srcpacking = src->packing();
    auto   dstpacking = dst->packing();
    auto &srcpacktype = srcpacking.type;
    auto &dstpacktype = dstpacking.type;
    auto   srcpacksym = srcpacktype.isaBuiltin();
    auto   dstpacksym = dstpacktype.isaBuiltin();
    auto srcpackcount = srcpacking.count;
    auto dstpackcount = dstpacking.count;
    auto  srcpacknode = (TpBuiltin*)srcpacksym->node;
    auto  dstpacknode = (TpBuiltin*)dstpacksym->node;
    auto  srcpacksize = srcpacknode->size;
    auto  dstpacksize = dstpacknode->size;
    fail_if(srcpackcount != dstpackcount); // Cannot change pack count.
    fail_if(dstpacksize < SIZEOF_FLOAT);   // Cannot reduce pack size to less than 32 bits.
    if (srcpacksize <= dstpacksize) {
        // Ok. implicit/explicit U/Int32x4 | U/Int64x4  → Floatx4
        //                    or U/Int32x8 | U/Int64x8  → Floatx8
        //                    or U/Int32x16 → Floatx16
        //                    or U/Int32x2 | U/Int64x2 → Doublex2
        //                    or U/Int32x4 | U/Int64x4 → Doublex4
        //                    or U/Int32x8 | U/Int64x8 → Doublex8
        return packed(dst->type, Kind::PackedFloatCast).result;
    }
    fail_if_implicit(); // Losing bits must be explicit.
    // Ok. explicit U/Int64x4 → Floatx4
    return packed(dst->type, Kind::PackedFloatCast).result;
}

Result Caster::PackedIntegersToPackedIntegers(TpSymbol *srcsym, TpSymbol *dstsym) {
    auto  src = (TpBuiltin*)srcsym->node;
    auto  dst = (TpBuiltin*)dstsym->node;
    auto   srcpacking = src->packing();
    auto   dstpacking = dst->packing();
    auto &srcpacktype = srcpacking.type;
    auto &dstpacktype = dstpacking.type;
    auto   srcpacksym = srcpacktype.isaBuiltin();
    auto   dstpacksym = dstpacktype.isaBuiltin();
    auto srcpackcount = srcpacking.count;
    auto dstpackcount = dstpacking.count;
    auto  srcpacknode = (TpBuiltin*)srcpacksym->node;
    auto  dstpacknode = (TpBuiltin*)dstpacksym->node;
    auto  srcpacksize = srcpacknode->size;
    auto  dstpacksize = dstpacknode->size;
    fail_if(srcpackcount != dstpackcount); // Cannot change pack count.
    fail_if(srcpacksize != dstpacksize);   // Cannot reduce pack size.
    fail_if_implicit(); // Changing sign must be explicit.
    return packed(dst->type, Kind::PackedBitCast).result;
}

Result Caster::FloatingPointToPackedFloatingPoints(TpSymbol *srcsym, TpSymbol *dstsym) {
    auto  src = (TpBuiltin*)srcsym->node;
    auto  dst = (TpBuiltin*)dstsym->node;
    auto   dstpacking = dst->packing();
    auto &dstpacktype = dstpacking.type;
    auto   dstpacksym = dstpacktype.isaBuiltin();
    auto  dstpacknode = (TpBuiltin*)dstpacksym->node;
    auto      srcsize = src->size;
    auto  dstpacksize = dstpacknode->size;
    if (srcsize == dstpacksize) {
        // Ok. implicit/explicit Float  → PackedFloats
        //                    or Double → PackedDoubles
        return packed(dst->type, Kind::PackedBroadCast).result;
    }
    if (srcsize < dstpacksize) {
        // Ok. implicit/explicit Float → Double → PackedDoubles
        return fp(tp.tree.tyDouble).packed(dst->type, Kind::PackedBroadCast).result;
    }
    fail_if_implicit(); // Losing bits must be explicit.
    // Ok. explicit Double → Float → PackedFloats
    return fp(tp.tree.tyDouble).packed(dst->type, Kind::PackedBroadCast).result;
}

Result Caster::FloatingPointToPackedIntegers(TpSymbol *srcsym, TpSymbol *dstsym) {
    auto  src = (TpBuiltin*)srcsym->node;
    auto  dst = (TpBuiltin*)dstsym->node;
    auto   dstpacking = dst->packing();
    auto &dstpacktype = dstpacking.type;
    auto   dstpacksym = dstpacktype.isaBuiltin();
    auto  dstpacknode = (TpBuiltin*)dstpacksym->node;
    auto      srcsize = src->size;
    auto  dstpacksize = dstpacknode->size;
    if (srcsize <= dstpacksize) {
        // Ok. implicit/explicit Float  → U/Int32 → U/Int32x4 | U/Int32x8 | U/Int32x16
        //                    or Float  → U/Int64 → U/Int64x2 | U/Int64x4 | U/Int64x8
        //                    or Double → U/Int64 → U/Int64x2 | U/Int64x4 | U/Int64x8
        return fp(dstpacktype).packed(dst->type, Kind::PackedBroadCast).result;
    }
    fail_if_implicit(); // Losing bits must be explicit.
    if (srcsize == SIZEOF_FLOAT) {
        if (dstpacktype.isSigned()) {
            // Ok. explicit Float → Int32 → Integer → PackedIntegers
            return fp(tp.tree.tyInt32).narrow(dstpacktype).packed(dst->type, Kind::PackedBroadCast).result;
        } // Ok. explicit Float → UInt32 → Integer → PackedIntegers
        return fp(tp.tree.tyUInt32).narrow(dstpacktype).packed(dst->type, Kind::PackedBroadCast).result;
    }
    if (dstpacktype.isSigned()) {
        // Ok. explicit Double → Int64 → Integer → PackedIntegers
        return fp(tp.tree.tyInt64).narrow(dstpacktype).packed(dst->type, Kind::PackedBroadCast).result;
    } // Ok. explicit Double → UInt64 → Integer → PackedIntegers
    return fp(tp.tree.tyUInt64).narrow(dstpacktype).packed(dst->type, Kind::PackedBroadCast).result;
}

Result Caster::IntegerToPackedFloatingPoints(TpSymbol *srcsym, TpSymbol *dstsym) {
    auto  src = (TpBuiltin*)srcsym->node;
    auto  dst = (TpBuiltin*)dstsym->node;
    auto   dstpacking = dst->packing();
    auto &dstpacktype = dstpacking.type;
    auto   dstpacksym = dstpacktype.isaBuiltin();
    auto  dstpacknode = (TpBuiltin*)dstpacksym->node;
    auto      srcsize = src->size;
    auto  dstpacksize = dstpacknode->size;
    if (srcsize == dstpacksize) {
        // Ok. implicit/explicit U/Int32 → Float  → Floatx4  | Floatx8  | Floatx16
        //                    or U/Int64 → Double → Doublex2 | Doublex4 | Doublex8
        return fp(dstpacktype).packed(dst->type, Kind::PackedBroadCast).result;
    }
    if (srcsize < dstpacksize) { // Gaining bits.
        if (src->type.isUnsigned()) {
            // Ok. implicit/explicit UInt8 → UInt32 → Float  → Floatx4  | Floatx8  | Floatx16
            //                    or UInt8 → UInt64 → Double → Doublex2 | Doublex4 | Doublex8
            //                  ...and so on...
            return widen(unsignedOf(dstpacknode->size))
                .fp(floatOf(dstpacknode->size))
                .packed(dst->type, Kind::PackedBroadCast).result;
        }
        // Ok. implicit/explicit Int8 → Int32 → Float  → Floatx4  | Floatx8  | Floatx16
        //                    or Int8 → Int64 → Double → Doublex2 | Doublex4 | Doublex8
        //                  ...and so on...
        return widen(signedOf(dstpacknode->size))
            .fp(floatOf(dstpacknode->size))
            .packed(dst->type, Kind::PackedBroadCast).result;
    }
    fail_if_implicit(); // Losing bits must be explicit.
    // Ok. implicit/explicit UInt64 → Float → Floatx4 | Floatx8 | Floatx16
    return fp(tp.tree.tyFloat).packed(dst->type, Kind::PackedBroadCast).result;
}

Result Caster::IntegerToPackedIntegers(TpSymbol *srcsym, TpSymbol *dstsym) {
    auto  src = (TpBuiltin*)srcsym->node;
    auto  dst = (TpBuiltin*)dstsym->node;
    auto   dstpacking = dst->packing();
    auto &dstpacktype = dstpacking.type;
    auto   dstpacksym = dstpacktype.isaBuiltin();
    auto  dstpacknode = (TpBuiltin*)dstpacksym->node;
    auto      srcsize = src->size;
    auto  dstpacksize = dstpacknode->size;
    auto      srcsign = src->type.isSigned()   != nullptr;
    auto      dstsign = dstpacktype.isSigned() != nullptr;
    if (src->type == dstpacktype) { // Ok. implicit/explicit T → PackedT
        return packed(dst->type, Kind::PackedBroadCast).result;
    }
    if (srcsign != dstsign) {
        fail_if_implicit(); // Changing signs must be explicit.
    }
    if (srcsize > dstpacksize) {
        fail_if_implicit(); // Losing bits must be explicit.
    }
    if (srcsize == dstpacksize) {
        // Ok. implicit/explicit Signed   → Unsigned → PackedUnsigneds
        //                    or Unsigned → Signed   → PackedSigneds
        return bitCast(dstpacktype).packed(dst->type, Kind::PackedBroadCast).result;
    }
    if (srcsize < dstpacksize) {
        if (dstsign) {
            if (srcsign) {
                // Ok. implicit/explicit small Signed → large Signed → PackedSigneds
                return widen(signedOf(dstpacksize))
                    .packed(dst->type, Kind::PackedBroadCast).result;
            }
            // Ok. explicit small Unsigned → large Unsigned → Signed → PackedSigneds
            return widen(unsignedOf(dstpacksize))
                .bitCast(dstpacktype)
                .packed(dst->type, Kind::PackedBroadCast).result;
        }
        if (srcsign) {
            // Ok. explicit small Signed → large Signed → Unsigned → PackedUnsigneds
            return widen(signedOf(dstpacksize))
                .bitCast(dstpacktype)
                .packed(dst->type, Kind::PackedBroadCast).result;
        }
        // Ok. explicit small Unsigned → large Unsigned → PackedUnsigneds
        return widen(unsignedOf(dstpacksize))
            .packed(dst->type, Kind::PackedBroadCast).result;
    }
    if (dstsign) {
        if (srcsign) {
            // Ok. explicit large Signed → small Signed → PackedSigneds
            return narrow(signedOf(dstpacksize))
                .packed(dst->type, Kind::PackedBroadCast).result;
        }
        // Ok. explicit large Unsigned → small Unsigned → Signed → PackedSigneds
        return narrow(unsignedOf(dstpacksize))
            .bitCast(dstpacktype)
            .packed(dst->type, Kind::PackedBroadCast).result;
    }
    if (srcsign) {
        // Ok. explicit large Signed → small Signed → Unsigned → PackedSigneds
        return narrow(signedOf(dstpacksize))
            .bitCast(dstpacktype)
            .packed(dst->type, Kind::PackedBroadCast).result;
    }
    // Ok. explicit large Unsigned → small Unsigned → PackedUnsigneds
    return narrow(unsignedOf(dstpacksize))
        .packed(dst->type, Kind::PackedBroadCast).result;
}

} // namespace exy