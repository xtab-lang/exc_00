//////////////////////////////////////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2021-01-19
//////////////////////////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "register.h"

namespace exy {
bool Register::isValid() const {
    return (value & Mask_Kind) != 0ui64;
}

bool Register::isNotValid() const {
    return (value & Mask_Kind) == 0ui64;
}

Register Register::mkImm8(INT8 n) {
    Register reg{};
    reg.value = { UINT64(RegKind::Immediate) | (Size8 << Off_Size) | (1ui64 << Off_Sign) |
        (UINT64(UINT8(n)) << Off_Imm) };
    return reg;
}

Register Register::mkUImm8(UINT8 n) {
    Register reg{};
    reg.value = { UINT64(RegKind::Immediate) | (Size8 << Off_Size) | (UINT64(n) << Off_Imm) };
    return reg;
}

Register Register::mkImm16(INT16 n) {
    Register reg{};
    reg.value = { UINT64(RegKind::Immediate) | (Size16 << Off_Size) | (1ui64 << Off_Sign) |
        (UINT64(UINT16(n)) << Off_Imm) };
    return reg;
}

Register Register::mkUImm16(UINT16 n) {
    Register reg{};
    reg.value = { UINT64(RegKind::Immediate) | (Size16 << Off_Size) | (UINT64(n) << Off_Imm) };
    return reg;
}

Register Register::mkImm32(INT32 n) {
    Register reg{};
    reg.value = { UINT64(RegKind::Immediate) | (Size32 << Off_Size) | (1ui64 << Off_Sign) | 
        (UINT64(UINT32(n)) << Off_Imm) };
    return reg;
}

Register Register::mkUImm32(UINT32 n) {
    Register reg{};
    reg.value = { UINT64(RegKind::Immediate) | (Size32 << Off_Size) | (UINT64(n) << Off_Imm) };
    return reg;
}

Register Register::mkImm56(INT64 n) {
    Register reg{};
    auto high = UINT64(n) & Mask_Imm56_High;
    if (high == Mask_Imm56_High) {
        reg.value = { UINT64(RegKind::Immediate) | (Size64 << Off_Size) | (1ui64 << Off_Sign) |
            (UINT64(UINT64(n)) << Off_Imm) };
    } else if (high == 0ui64) {
        reg.value = { UINT64(RegKind::Immediate) | (Size64 << Off_Size) |
            (UINT64(UINT64(n)) << Off_Imm) };
    } else {
        Assert(0);
    }
    return reg;
}

Register Register::mkUImm56(UINT64 n) {
    Register reg{};
    auto high = n & Mask_Imm56_High;
    if (high == Mask_Imm56_High) {
        reg.value = { UINT64(RegKind::Immediate) | (Size64 << Off_Size) | (1ui64 << Off_Sign) |
            (UINT64(UINT64(n)) << Off_Imm) };
    } else if (high == 0ui64) {
        reg.value = { UINT64(RegKind::Immediate) | (Size64 << Off_Size) |
            (UINT64(UINT64(n)) << Off_Imm) };
    } else {
        Assert(0);
    }
    return reg;
}

Register Register::mkOff8(INT8 n) {
    Register reg{};
    reg.value = { UINT64(RegKind::Offset) | (Size8 << Off_Size) | (1ui64 << Off_Sign) |
        (UINT64(UINT8(n)) << Off_Imm) };
    return reg;
}

Register Register::mkUOff8(UINT8 n) {
    Register reg{};
    reg.value = { UINT64(RegKind::Offset) | (Size8 << Off_Size) | (UINT64(n) << Off_Imm) };
    return reg;
}

Register Register::mkOff16(INT16 n) {
    Register reg{};
    reg.value = { UINT64(RegKind::Offset) | (Size16 << Off_Size) | (1ui64 << Off_Sign) |
        (UINT64(UINT16(n)) << Off_Imm) };
    return reg;
}

Register Register::mkUOff16(UINT16 n) {
    Register reg{};
    reg.value = { UINT64(RegKind::Offset) | (Size16 << Off_Size) | (UINT64(n) << Off_Imm) };
    return reg;
}

Register Register::mkOff32(INT32 n) {
    Register reg{};
    reg.value = { UINT64(RegKind::Offset) | (Size32 << Off_Size) | (1ui64 << Off_Sign) |
        (UINT64(UINT32(n)) << Off_Imm) };
    return reg;
}

Register Register::mkUOff32(UINT32 n) {
    Register reg{};
    reg.value = { UINT64(RegKind::Offset) | (Size32 << Off_Size) | (UINT64(n) << Off_Imm) };
    return reg;
}

Register Register::mkOff56(INT64 n) {
    Register reg{};
    auto high = UINT64(n) & Mask_Imm56_High;
    if (high == Mask_Imm56_High) {
        reg.value = { UINT64(RegKind::Offset) | (Size64 << Off_Size) | (1ui64 << Off_Sign) |
            (UINT64(UINT64(n)) << Off_Imm) };
    } else if (high == 0ui64) {
        reg.value = { UINT64(RegKind::Offset) | (Size64 << Off_Size) |
            (UINT64(UINT64(n)) << Off_Imm) };
    } else {
        Assert(0);
    }
    return reg;
}

Register Register::mkUOff56(UINT64 n) {
    Register reg{};
    auto high = n & Mask_Imm56_High;
    if (high == Mask_Imm56_High) {
        reg.value = { UINT64(RegKind::Offset) | (Size64 << Off_Size) | (1ui64 << Off_Sign) |
            (UINT64(UINT64(n)) << Off_Imm) };
    } else if (high == 0ui64) {
        reg.value = { UINT64(RegKind::Offset) | (Size64 << Off_Size) |
            (UINT64(UINT64(n)) << Off_Imm) };
    } else {
        Assert(0);
    }
    return reg;
}

bool Register::isImmediate() const {
    return (value & Mask_Kind) == UINT64(RegKind::Immediate);
}

INT8 Register::imm8() const {
    return INT8((value & Mask_Imm8) >> Off_Imm);
}

INT16 Register::imm16() const {
    return INT16((value & Mask_Imm16) >> Off_Imm);
}

INT32 Register::imm32() const {
    return INT32((value & Mask_Imm32) >> Off_Imm);
}

INT64 Register::imm64() const {
    if (isSigned()) {
        return INT64(((value & Mask_Imm56) >> Off_Imm) | Mask_Imm56_High);
    }
    return INT64((value & Mask_Imm56) >> Off_Imm);
}

UINT8 Register::uimm8() const {
    return UINT8((value & Mask_Imm8) >> Off_Imm);
}

UINT16 Register::uimm16() const {
    return UINT16((value & Mask_Imm16) >> Off_Imm);
}

UINT32 Register::uimm32() const {
    return UINT32((value & Mask_Imm32) >> Off_Imm);
}

UINT64 Register::uimm64() const {
    return (value & Mask_Imm56) >> Off_Imm;
}

bool Register::isOffset() const {
    return (value & Mask_Kind) == UINT64(RegKind::Offset);
}

INT8 Register::off8() const {
    return INT8((value & Mask_Imm8) >> Off_Imm);
}

INT16 Register::off16() const {
    return INT16((value & Mask_Imm16) >> Off_Imm);
}

INT32 Register::off32() const {
    return INT32((value & Mask_Imm32) >> Off_Imm);
}

INT64 Register::off64() const {
    if (isSigned()) {
        return INT64(((value & Mask_Imm56) >> Off_Imm) | Mask_Imm56_High);
    }
    return INT64((value & Mask_Imm56) >> Off_Imm);
}

UINT8 Register::uoff8() const {
    return UINT8((value & Mask_Imm8) >> Off_Imm);
}

UINT16 Register::uoff16() const {
    return UINT16((value & Mask_Imm16) >> Off_Imm);
}

UINT32 Register::uoff32() const {
    return UINT32((value & Mask_Imm32) >> Off_Imm);
}

UINT64 Register::uoff64() const {
    return (value & Mask_Imm56) >> Off_Imm;
}

bool Register::isSigned() const {
    return (value & Mask_Sign) == Mask_Sign;
}

bool Register::isUnsigned() const {
    return (value & Mask_Sign) == zero;
}

int Register::size() const {
    switch ((value & Mask_Size) >> Off_Size) {
    case 0ui64:   return 0;
    case Size8:   return SizeOfByte;
    case Size16:  return SizeOfWord;
    case Size32:  return SizeOfInt;
    case Size64:  return SizeOfPointer;
    case Size128: return SizeOfXmm;
    case Size256: return SizeOfYmm;
    case Size512: return SizeOfZmm;
    default: Unreachable();
    }
}
} // namespace exy