//////////////////////////////////////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2021-01-19
//////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef REGISTER_H
#define REGISTER_H

namespace exy {
//--Begin forward declarations

//----End forward declarations

/*
                        1 1 1 1 1 1 1 1 1 1 2 2 2 2 2 2 2 2 2 2 3 3 3 3 3 3 3 3 3 3 4 4 4 4 4 4 4 4 4 4 5 5 5 5 5 5 5 5 5 5 6 6 6 6
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3
    | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | |
    | | | | | | 0 x x x x x x x x ― unsigned imm8 | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | |
    | | | | | | 1 x x x x x x x x ― signed imm8 | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | |
    | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | |
    | | | | | | 0 x x x x x x x x x x x x x x x x ― unsigned imm16| | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | |
    | | | | | | 1 x x x x x x x x x x x x x x x x ― signed imm16  | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | |
    | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | |
    | | | | | | 0 x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x ― unsigned imm32| | | | | | | | | | | | | | | | |
    | | | | | | 1 x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x ― signed imm32  | | | | | | | | | | | | | | | | |
    | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | |
    | | | | | | x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x ― unsigned offset (58 bits)
    | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | |
    | | | | | | 0 x x x x x x x x ― unsigned off8 | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | |
    | | | | | | 1 x x x x x x x x ― signed off8 | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | |
    | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | |
    | | | | | | 0 0 0 0 0 0 0 0 ― no base/register| | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | |
    | | | | | | 0 1 x x x x x x ― base/register | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | |
    | | | | | | 1 0 0 0 0 0 0 0 ― rip-relative  | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | |
    | | | | | | 1 1 0 0 0 0 0 0 ― rsp-relative  | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | |
    | | | | | |                 | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | |
    | | | | | |                 0 x x x x x x ― no index  | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | |
    | | | | | |                 1 x x x x x x ― index register| | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | |
    | | | | | |                               | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | |
    | | | | | |                               0 0 ― index × 1 | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | |
    | | | | | |                               0 1 ― index × 2 | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | |
    | | | | | |                               1 0 ― index × 4 | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | |
    | | | | | |                               1 1 ― index × 8 | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | |
    | | | | | |                                   | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | |
    | | | | | |                                   0 0 ― no disp | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | |
    | | | | | |                                   0 1 x x x x x x x x ― disp8 | | | | | | | | | | | | | | | | | | | | | | | | | | |
    | | | | | |                                   1 0 x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x x ― disp32| | |
    | | | | | |
    | | | | | | size
    | | | | | +⸻⸻
    | | | 0 0 0 ― invalid
    | | | 0 0 1 ―   8 bits
    | | | 0 1 0 ―  16 bits
    | | | 0 1 1 ―  32 bits
    | | | 1 0 0 ―  64 bits
    | | | 1 0 1 ― 128 bits
    | | | 1 1 0 ― 256 bits
    | | | 1 1 1 ― 512 bits
    | | |
    | | | kind
    | | +⸻⸻
    0 0 0 ― invalid
    0 0 1 ― imm
    0 1 0 ― gpr
    0 1 1 ― x/y/zmm
    1 0 0 ― [mem]
    1 0 1 ― offset
    1 1 0 ― invalid
    1 1 1 ― invalid
*/

/* name, sizeInBytes, isVolatile */
#define Declare64BitGprs(ZM)    \
    ZM(rax, 8, true)            \
    ZM(rcx, 8, true)            \
    ZM(rdx, 8, true)            \
    ZM(rbx, 8, false)           \
    ZM(rsp, 8, false)           \
    ZM(rbp, 8, false)           \
    ZM(rsi, 8, true)            \
    ZM(rdi, 8, true)            \
    ZM(r8,  8, true)            \
    ZM(r9,  8, true)            \
    ZM(r10, 8, true)            \
    ZM(r11, 8, true)            \
    ZM(r12, 8, false)           \
    ZM(r13, 8, false)           \
    ZM(r14, 8, false)           \
    ZM(r15, 8, false)

/* name, sizeInBytes, isVolatile */
#define Declare32BitGprs(ZM)    \
    ZM(eax,  4, true)           \
    ZM(ecx,  4, true)           \
    ZM(edx,  4, true)           \
    ZM(ebx,  4, false)          \
    ZM(esp,  4, false)          \
    ZM(ebp,  4, false)          \
    ZM(esi,  4, true)           \
    ZM(edi,  4, true)           \
    ZM(r8d,  4, true)           \
    ZM(r9d,  4, true)           \
    ZM(r10d, 4, true)           \
    ZM(r11d, 4, true)           \
    ZM(r12d, 4, false)          \
    ZM(r13d, 4, false)          \
    ZM(r14d, 4, false)          \
    ZM(r15d, 4, false)

/* name, sizeInBytes, isVolatile */
#define Declare16BitGprs(ZM)    \
    ZM(ax,   2, true)           \
    ZM(cx,   2, true)           \
    ZM(dx,   2, true)           \
    ZM(bx,   2, false)          \
    ZM(sp,   2, false)          \
    ZM(bp,   2, false)          \
    ZM(si,   2, true)           \
    ZM(di,   2, true)           \
    ZM(r8w,  2, true)           \
    ZM(r9w,  2, true)           \
    ZM(r10w, 2, true)           \
    ZM(r11w, 2, true)           \
    ZM(r12w, 2, false)          \
    ZM(r13w, 2, false)          \
    ZM(r14w, 2, false)          \
    ZM(r15w, 2, false)

/* name, sizeInBytes, isVolatile */
#define Declare8BitGprs(ZM)     \
    ZM(al,   1, true)           \
    ZM(cl,   1, true)           \
    ZM(dl,   1, true)           \
    ZM(bl,   1, false)          \
    ZM(spl,  1, false)          \
    ZM(bpl,  1, false)          \
    ZM(sil,  1, true)           \
    ZM(dil,  1, true)           \
    ZM(r8b,  1, true)           \
    ZM(r9b,  1, true)           \
    ZM(r10b, 1, true)           \
    ZM(r11b, 1, true)           \
    ZM(r12b, 1, false)          \
    ZM(r13b, 1, false)          \
    ZM(r14b, 1, false)          \
    ZM(r15b, 1, false)

/* name, sizeInBytes, isVolatile */
#define Declare32BitXmms(ZM)    \
    ZM(xmm0,  4, true)          \
    ZM(xmm1,  4, true)          \
    ZM(xmm2,  4, true)          \
    ZM(xmm3,  4, true)          \
    ZM(xmm4,  4, true)          \
    ZM(xmm5,  4, true)          \
    ZM(xmm6,  4, false)         \
    ZM(xmm7,  4, false)         \
    ZM(xmm8,  4, false)         \
    ZM(xmm9,  4, false)         \
    ZM(xmm10, 4, false)         \
    ZM(xmm11, 4, false)         \
    ZM(xmm12, 4, false)         \
    ZM(xmm13, 4, false)         \
    ZM(xmm14, 4, false)         \
    ZM(xmm15, 4, false)

/* name, sizeInBytes, isVolatile */
#define Declare64BitXmms(ZM)    \
    ZM(xmm0,  8, true)          \
    ZM(xmm1,  8, true)          \
    ZM(xmm2,  8, true)          \
    ZM(xmm3,  8, true)          \
    ZM(xmm4,  8, true)          \
    ZM(xmm5,  8, true)          \
    ZM(xmm6,  8, false)         \
    ZM(xmm7,  8, false)         \
    ZM(xmm8,  8, false)         \
    ZM(xmm9,  8, false)         \
    ZM(xmm10, 8, false)         \
    ZM(xmm11, 8, false)         \
    ZM(xmm12, 8, false)         \
    ZM(xmm13, 8, false)         \
    ZM(xmm14, 8, false)         \
    ZM(xmm15, 8, false)

/* name, sizeInBytes, isVolatile */
#define Declare128BitXmms(ZM)    \
    ZM(xmm0,  16, true)          \
    ZM(xmm1,  16, true)          \
    ZM(xmm2,  16, true)          \
    ZM(xmm3,  16, true)          \
    ZM(xmm4,  16, true)          \
    ZM(xmm5,  16, true)          \
    ZM(xmm6,  16, false)         \
    ZM(xmm7,  16, false)         \
    ZM(xmm8,  16, false)         \
    ZM(xmm9,  16, false)         \
    ZM(xmm10, 16, false)         \
    ZM(xmm11, 16, false)         \
    ZM(xmm12, 16, false)         \
    ZM(xmm13, 16, false)         \
    ZM(xmm14, 16, false)         \
    ZM(xmm15, 16, false)

/* name, sizeInBytes, isVolatile */
#define DeclareYmms(ZM)    \
    ZM(ymm0,  32, true)          \
    ZM(ymm1,  32, true)          \
    ZM(ymm2,  32, true)          \
    ZM(ymm3,  32, true)          \
    ZM(ymm4,  32, true)          \
    ZM(ymm5,  32, true)          \
    ZM(ymm6,  32, false)         \
    ZM(ymm7,  32, false)         \
    ZM(ymm8,  32, false)         \
    ZM(ymm9,  32, false)         \
    ZM(ymm10, 32, false)         \
    ZM(ymm11, 32, false)         \
    ZM(ymm12, 32, false)         \
    ZM(ymm13, 32, false)         \
    ZM(ymm14, 32, false)         \
    ZM(ymm15, 32, false)

/* name, sizeInBytes, isVolatile */
#define DeclareZmms(ZM)    \
    ZM(zmm0,  64, true)          \
    ZM(zmm1,  64, true)          \
    ZM(zmm2,  64, true)          \
    ZM(zmm3,  64, true)          \
    ZM(zmm4,  64, true)          \
    ZM(zmm5,  64, true)          \
    ZM(zmm6,  64, false)         \
    ZM(zmm7,  64, false)         \
    ZM(zmm8,  64, false)         \
    ZM(zmm9,  64, false)         \
    ZM(zmm10, 64, false)         \
    ZM(zmm11, 64, false)         \
    ZM(zmm12, 64, false)         \
    ZM(zmm13, 64, false)         \
    ZM(zmm14, 64, false)         \
    ZM(zmm15, 64, false)         \
    ZM(zmm16, 64, false)         \
    ZM(zmm17, 64, false)         \
    ZM(zmm18, 64, false)         \
    ZM(zmm19, 64, false)         \
    ZM(zmm20, 64, false)         \
    ZM(zmm21, 64, false)         \
    ZM(zmm22, 64, false)         \
    ZM(zmm23, 64, false)         \
    ZM(zmm24, 64, false)         \
    ZM(zmm25, 64, false)         \
    ZM(zmm26, 64, false)         \
    ZM(zmm27, 64, false)         \
    ZM(zmm28, 64, false)         \
    ZM(zmm29, 64, false)         \
    ZM(zmm30, 64, false)         \
    ZM(zmm31, 64, false)

enum class RegisterKind {
    Invalid, Immediate, Gpr, Xmm, Mem, Offset
};

enum class BaseRegisterKind {
    Invalid, Register, Rip, Rsp
};

enum class Gpr {
#define ZM(zName, zSize, zVolatile) zName,
    Declare64BitGprs(ZM)
#undef ZM
};

enum class Xmm {
#define ZM(zName, zSize, zVolatile) zName,
    Declare64BitXmms(ZM)
#undef ZM
};

struct Register {
    using Base  = Gpr;
    using Index = Gpr;

    using Imm8   = INT8;
    using UImm8  = UINT8;
    using Imm16  = INT16;
    using UImm16 = UINT16;
    using Imm32  = INT32;
    using UImm32 = UINT32;

    using Disp8  = __int8;
    using Disp32 = __int32;

    static Register mkImm8(Imm8);
    static Register mkUImm8(UImm8);
    static Register mkImm16(Imm16);
    static Register mkUImm16(UImm16);
    static Register mkImm32(Imm32);
    static Register mkUImm32(UImm32);

    static Register mkGpr(Gpr, int size);
    static Register mkXmm(Xmm, int size);

    static Register mkRip(Disp8);                           // [rip + disp8]
    static Register mkRsp(Disp8);                           // [rsp + disp8]

    static Register mkRipRelative(Disp32);                  // [rip + disp32]
    static Register mkRspRelative(Disp32);                  // [rsp + disp32]

    static Register mkMem(Base);                            // [base]
    static Register mkMem(Base, Index index);               // [base + index × scale]

    static Register mkMem(Base, Disp8);                     // [base + disp8]
    static Register mkMem(Base, Index index, Disp8);        // [base + index × scale + disp8]

    static Register mkMem(Base, Disp32);                    // [base + disp32]
    static Register mkMem(Base, Index index, Disp32);       // [base + index × scale + disp32]

    static Register mkMem(Disp32);                          // [disp32]
private:
    UINT8 _kind : 3 = 0u;
    UINT8 _size : 3 = 0u;
    union {
        struct {
            UINT8 _sign : 1;
            union {
                UINT8  _imm8  :  8;
                UINT16 _imm16 : 16;
                UINT32 _imm32 : 32;
            };
        } _imm;
        UINT64 _off58 : 58 = 0u;
        struct {
            struct {
                UINT8 _has  : 2;
                UINT8 _name : 6;
            } _base;
            struct {
                UINT8 _has   : 1;
                UINT8 _name  : 6;
            } _index;
            UINT8 _scale : 2;
            struct {
                UINT8 _has : 2;
                union {
                    UINT8  _disp8  : 8;
                    UINT32 _disp32 : 32;
                };
            } _disp;
        } _mem;
    };
};
} // namespace exy

#endif // REGISTER_H