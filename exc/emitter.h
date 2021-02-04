//////////////////////////////////////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2021-02-02
//////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef EMITTER_H
#define EMITTER_H

namespace exy {
namespace codegen_pass {
enum OpCode {
    /* add */
    Op_Add_EbGb    = 0x00,
    Op_Add_EvGv    = 0x01,
    Op_Add_GbEb    = 0x02,
    Op_Add_GvEv    = 0x03,
    Op_Add_AlIb    = 0x04,
    Op_Add_rAxIz   = 0x05,
    /* others */
    Op_Ret         = 0xC3
};

struct Emitter {
    Buffer &pe;

    Emitter(Buffer &pe) : pe(pe){}

    void ret();
};
} // namespace codegen_pass
} // namespace exy

#endif // EMITTER_H