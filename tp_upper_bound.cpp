#include "pch.h"
#include "typer.h"

namespace exy {
TpType Typer::upperBound(Type lhs, Type rhs, Tok op) {
    if (lhs == rhs) {
        return lhs;
    }
    //------------------------------------------------------
    if (lhs.isVoid() || rhs.isVoid()) {
        return tree.tyUnknown;
    }
    //------------------------------------------------------
    if (auto lptr = lhs.isVoidPointer()) {
        if (rhs.isIndirect()) {
            return rhs;
        }
        return tree.tyUnknown;
    }
    if (auto rptr = rhs.isVoidPointer()) {
        if (lhs.isIndirect()) {
            return lhs;
        }
        return tree.tyUnknown;
    }
    //------------------------------------------------------
    if (auto  lpacked = lhs.isPacked()) {
        auto lpacking = ((TpBuiltin*)lpacked->node)->packing();
        if (auto  rpacked = rhs.isPacked()) {
            auto rpacking = ((TpBuiltin*)rpacked->node)->packing();
            auto    upper = upperBound(lpacking.type, rpacking.type);
            if (upper.isKnown()) {
                TpPacking packing{ upper, max(lpacking.count, rpacking.count) };
                return packing.packedType();
            }
            return tree.tyUnknown;
        }
        if (rhs.isIntegral()) {
            auto upper = upperBound(lpacking.type, rhs);
            if (upper.isKnown()) {
                TpPacking packing{ upper, lpacking.count };
                return packing.packedType();
            }
            return tree.tyUnknown;
        }
        return tree.tyUnknown;
    }
    if (auto  rpacked = rhs.isPacked()) {
        auto rpacking = ((TpBuiltin*)rpacked->node)->packing();
        if (lhs.isIntegral()) {
            auto upper = upperBound(lhs, rpacking.type);
            if (upper.isKnown()) {
                TpPacking packing{ upper, rpacking.count };
                return packing.packedType();
            }
            return tree.tyUnknown;
        }
        return tree.tyUnknown;
    }
    //------------------------------------------------------
    if (auto lptr = lhs.isaPointer()) {
        if (auto rptr = rhs.isaPointer()) {
            auto upper = upperBound(lptr->pointee, rptr->pointee, op);
            if (upper.isUnknown()) {
                return upper;
            }
            return upper.mkPointer();
        }
        if (auto rptr = rhs.isaReference()) {
            auto upper = upperBound(lptr->pointee, rptr->pointee, op);
            if (upper.isUnknown()) {
                return upper;
            }
            return upper.mkReference();
        }
        return tree.tyUnknown;
    }
    if (auto rptr = rhs.isaPointer()) {
        if (auto lptr = lhs.isaReference()) {
            auto upper = upperBound(lptr->pointee, rptr->pointee, op);
            if (upper.isUnknown()) {
                return upper;
            }
            return upper.mkReference();
        }
        return tree.tyUnknown;
    }
    //------------------------------------------------------
    if (auto lptr = lhs.isaReference()) {
        if (auto  rptr = rhs.isaReference()) {
            auto upper = upperBound(lptr->pointee, rptr->pointee, op);
            if (upper.isUnknown()) {
                return upper;
            }
            return upper.mkReference();
        }
        if (auto direct = rhs.isDirect()) {
            auto  upper = upperBound(lptr->pointee, direct, op);
            if (upper.isUnknown()) {
                return upper;
            }
            return upper.mkReference();
        }
        return tree.tyUnknown;
    }
    if (auto rptr = rhs.isaReference()) {
        if (auto direct = lhs.isDirect()) {
            auto  upper = upperBound(direct, rptr->pointee, op);
            if (upper.isUnknown()) {
                return upper;
            }
            return upper.mkReference();
        }
        return tree.tyUnknown;
    }
    //------------------------------------------------------
    if (auto l = lhs.isIntegral()) {
        if (auto  r = rhs.isIntegral()) {
            auto lb = (TpBuiltin*)l->node;
            auto rb = (TpBuiltin*)r->node;
            if (lb->size > rb->size) {
                return lb->type;
            }
            return rb->type;
        }
        if (rhs.isaFloatingPoint()) {
            return rhs;
        }
        return tree.tyUnknown;
    }
    if (auto l = lhs.isaFloatingPoint()) {
        if (auto r = rhs.isaFloatingPoint()) {
            auto lb = (TpBuiltin*)l->node;
            auto rb = (TpBuiltin*)r->node;
            if (lb->size > rb->size) {
                return lb->type;
            }
            return rb->type;
        }
        if (rhs.isIntegral()) {
            return lhs;
        }
        return tree.tyUnknown;
    }
    return tree.tyUnknown;
}
} // namespace exy