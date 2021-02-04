//////////////////////////////////////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2021-02-01
//////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef BINARY_H
#define BINARY_H

#include "ir.h"

namespace exy {
//--Begin forward declarations
//----End forward declarations

namespace linker_pass {
struct Binary {
    IrModule  *mod;
    String     fileName{};
    Buffer     file;
    int        timeStamp;

    Binary(IrModule *mod);
    virtual void dispose();

    void generate();
private:
    void writeDosHeaderAndStub();
    void writeNtHeaders();
    void writeFileHeader();
    void writeOptionalHeader();

    void writeSectionTable();

    void writeCodeSection();
    void writeDataSection();

    void updateCodeSectionSymbolOffsets();

    View vwDosHeader;
    View vwDosStub;
    View vwNtHeaders;
    View vwFileHeader;
    View vwOptionalHeader;
    View vwCodeSectionHeader;
    View vwDataSectionHeader;
    View vwCodeSection;
    View vwDataSection;

    IMAGE_DOS_HEADER*  dosHeader();
    IMAGE_NT_HEADERS*  NTHeaders();
    IMAGE_FILE_HEADER* fileHeader();
    IMAGE_OPTIONAL_HEADER* optionalHeader();
    IMAGE_SECTION_HEADER*  codeSectionHeader();
    IMAGE_SECTION_HEADER*  dataSectionHeader();

    String extension();
    WORD subSystem();
};

/*
    offset  size    content
    ――――――  ――――    ―――――――
    000h    040h    IMAGE_DOS_HEADER
    040h    040h    DOS_STUB
    080h    004h    IMAGE_NT_SIGNATURE -----+
    084h    014h    IMAGE_FILE_HEADER       |-- IMAGE_NT_HEADERS
    098h    0F0h    IMAGE_OPTIONAL_HEADER --+
    188h    028h    IMAGE_SECTION_HEADER (.text)
    1B0h    028h    IMAGE_SECTION_HEADER (.data)
    1D8h    028h    padding
    200h    ???     .text
    ???     ???     .data
*/
#define IMAGE_SECTION_ALIGNMENT 0x200 // 0x1000
#define IMAGE_FILE_ALIGNMENT    0x200
#define IMAGE_BASE              0x0000000140000000ui64

#define IMAGE_SIZEOF_STACK_RESERVE 0x00100000
#define IMAGE_SIZEOF_STACK_COMMIT  0x00001000
#define IMAGE_SIZEOF_HEAP_RESERVE  0x00100000
#define IMAGE_SIZEOF_HEAP_COMMIT   0x00001000
} // namespace linker_pass
} // namespace exy

#endif // BINARY_H