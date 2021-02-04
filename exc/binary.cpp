//////////////////////////////////////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2021-02-01
//////////////////////////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "binary.h"

#include <time.h>

#define err(token, msg, ...) print_error("Binary", token, msg, __VA_ARGS__)

namespace exy {
namespace linker_pass {
using Reg = Register;

Binary::Binary(IrModule *mod) : mod(mod) {
    fileName.append(comp.options.path.outputFolder)
        .append(S("\\"))
        .append(mod->name)
        .append(S("."))
        .append(extension());
    timeStamp = (int)time(nullptr);
}

void Binary::dispose() {
    file.dispose();
    fileName.dispose();
}

void Binary::generate() {
    if (!fileName.ensureFileExistsAndTruncate()) {
        err(mod, "cannot truncate file %s#<yellow|underline>", &fileName);
        return;
    }
    /* Write headers. */
    writeDosHeaderAndStub();
    writeNtHeaders();
    writeSectionTable();
    /* Write sections. */
    writeCodeSection();
    writeDataSection();
    /* Flush. */
    optionalHeader()->SizeOfImage = file.length;
    file.saveToDisk(fileName);
}

void Binary::writeDosHeaderAndStub() {
    vwDosHeader.offset = file.length;
    auto hdr = file.alloc<IMAGE_DOS_HEADER>();
    hdr->e_magic = IMAGE_DOS_SIGNATURE;
    hdr->e_lfanew = 0x80;
    vwDosHeader.length = file.length - vwDosHeader.offset;

    vwDosStub.offset = file.length;
    file.write(S("This program cannot be run in DOS mode."));
    //file.write(S("This program was built by exy.lang"));
    file.write(nullptr, 0x80 - file.length);
    vwDosStub.length = file.length - vwDosStub.offset;
}

void Binary::writeNtHeaders() {
    vwNtHeaders.offset = file.length;
    file.writeObject(IMAGE_NT_SIGNATURE);
    writeFileHeader();
    writeOptionalHeader();
    vwNtHeaders.length = file.length - vwNtHeaders.offset;
}

void Binary::writeFileHeader() {
    vwFileHeader.offset = file.length;
    auto hdr = file.alloc<IMAGE_FILE_HEADER>();
    hdr->Machine = IMAGE_FILE_MACHINE_AMD64;
    hdr->NumberOfSections = 2;
    hdr->TimeDateStamp = timeStamp;
    hdr->SizeOfOptionalHeader = sizeof(IMAGE_OPTIONAL_HEADER);
    hdr->Characteristics = IMAGE_FILE_LARGE_ADDRESS_AWARE;
    if (mod->isaDll()) {
        hdr->Characteristics |= IMAGE_FILE_DLL;
    } else {
        hdr->Characteristics |= IMAGE_FILE_EXECUTABLE_IMAGE;
    }
    vwFileHeader.length = file.length - vwFileHeader.offset;
}

void Binary::writeOptionalHeader() {
    vwOptionalHeader.offset = file.length;
    auto hdr = file.alloc<IMAGE_OPTIONAL_HEADER>();
    hdr->Magic            = IMAGE_NT_OPTIONAL_HDR64_MAGIC;
    hdr->ImageBase        = IMAGE_BASE;
    hdr->SectionAlignment = IMAGE_SECTION_ALIGNMENT;
    hdr->FileAlignment    = IMAGE_FILE_ALIGNMENT;
    hdr->Subsystem        = subSystem();
    hdr->MajorImageVersion = 0;
    hdr->MinorImageVersion = 1;
    hdr->MajorSubsystemVersion = 4;
    hdr->DllCharacteristics = IMAGE_DLLCHARACTERISTICS_DYNAMIC_BASE | 
        IMAGE_DLLCHARACTERISTICS_NX_COMPAT | IMAGE_DLLCHARACTERISTICS_HIGH_ENTROPY_VA | 
        IMAGE_DLLCHARACTERISTICS_TERMINAL_SERVER_AWARE | IMAGE_DLLCHARACTERISTICS_NO_SEH;
    hdr->SizeOfStackReserve = IMAGE_SIZEOF_STACK_RESERVE;
    hdr->SizeOfStackCommit  = IMAGE_SIZEOF_STACK_COMMIT;
    hdr->SizeOfHeapReserve  = IMAGE_SIZEOF_HEAP_RESERVE;
    hdr->SizeOfHeapCommit   = IMAGE_SIZEOF_HEAP_COMMIT;
    hdr->NumberOfRvaAndSizes = IMAGE_NUMBEROF_DIRECTORY_ENTRIES;
    vwOptionalHeader.length = file.length - vwOptionalHeader.offset;
}

void Binary::writeSectionTable() {
    vwCodeSectionHeader = file.write(nullptr, sizeof(IMAGE_SECTION_HEADER));
    vwDataSectionHeader = file.write(nullptr, sizeof(IMAGE_SECTION_HEADER));
    file.align(IMAGE_FILE_ALIGNMENT);
    optionalHeader()->SizeOfHeaders = file.length;
}

void Binary::writeCodeSection() {
    // Write section to PE file. 
    vwCodeSection.offset = file.length;
    auto &buf = mod->code->peBuffer;
    file.write(buf.data, buf.length);
    file.align(IMAGE_FILE_ALIGNMENT, 0xCC);
    vwCodeSection.length = file.length - vwCodeSection.offset;

    updateCodeSectionSymbolOffsets();

    // Update PE headers.
    auto hdr = optionalHeader();
    hdr->BaseOfCode = vwCodeSection.offset;
    hdr->SizeOfCode = vwCodeSection.length;

    // Update section table.
    auto sec = codeSectionHeader();
    MemCopy(sec->Name, S(".text"));
    sec->Misc.VirtualSize = vwCodeSection.length;
    sec->VirtualAddress   = vwCodeSection.offset;
    sec->SizeOfRawData    = vwCodeSection.length;
    sec->PointerToRawData = vwCodeSection.offset;
    sec->Characteristics  = IMAGE_SCN_CNT_CODE | IMAGE_SCN_MEM_EXECUTE | IMAGE_SCN_MEM_READ;
}

void Binary::writeDataSection() {
    // Write section to PE file.
    vwDataSection.offset = file.length;
    auto &buf = mod->data->peBuffer;
    file.write(buf.data, buf.length);

    /*auto &exp = optionalHeader()->DataDirectory[IMAGE_DIRECTORY_ENTRY_EXPORT];
    exp.VirtualAddress = file.length;
    exp.Size = 0;

    auto &imp = optionalHeader()->DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT];
    imp.VirtualAddress = file.length;
    imp.Size = 0;

    auto &iat = optionalHeader()->DataDirectory[IMAGE_DIRECTORY_ENTRY_IAT];
    iat.VirtualAddress = file.length;
    iat.Size = 0;

    auto &tls = optionalHeader()->DataDirectory[IMAGE_DIRECTORY_ENTRY_TLS];
    tls.VirtualAddress = file.length;
    tls.Size = 0;

    auto &dbg = optionalHeader()->DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG];
    dbg.VirtualAddress = file.length;
    dbg.Size = 0;*/

    vwDataSection.length = file.length - vwDataSection.offset;

    // Update PE headers.
    auto hdr = optionalHeader();
    hdr->SizeOfInitializedData = vwDataSection.length;

    // Update section table.
    auto sec = dataSectionHeader();
    MemCopy(sec->Name, S(".data"));
    sec->Misc.VirtualSize = vwDataSection.length;
    sec->VirtualAddress   = vwDataSection.offset;
    sec->SizeOfRawData    = vwDataSection.length;
    sec->PointerToRawData = vwDataSection.offset;
    sec->Characteristics  = IMAGE_SCN_CNT_INITIALIZED_DATA | IMAGE_SCN_MEM_READ | IMAGE_SCN_MEM_WRITE;
}

void Binary::updateCodeSectionSymbolOffsets() {
    auto base = vwCodeSection.offset;
    auto  sec = mod->code;
    sec->reg = Reg::mkOff32(base);
    for (auto i = 0; i < sec->functions.length; ++i) {
        auto fn = sec->functions.items[i];
        fn->reg = Register::mkOff32(base + fn->reg.off32());
        for (auto block = fn->body.first(); block; block = block->qnext) {
            block->reg = Register::mkOff32(base + block->reg.off32());
        }
    }
    optionalHeader()->AddressOfEntryPoint = mod->entry->reg.uoff32();
}

IMAGE_DOS_HEADER* Binary::dosHeader() {
    return file.objectOf<IMAGE_DOS_HEADER>(vwDosHeader);
}

IMAGE_NT_HEADERS* Binary::NTHeaders() {
    return file.objectOf<IMAGE_NT_HEADERS>(vwNtHeaders);
}

IMAGE_FILE_HEADER* Binary::fileHeader() {
    return file.objectOf<IMAGE_FILE_HEADER>(vwFileHeader);
}

IMAGE_OPTIONAL_HEADER* Binary::optionalHeader() {
    return file.objectOf<IMAGE_OPTIONAL_HEADER>(vwOptionalHeader);
}

IMAGE_SECTION_HEADER* Binary::codeSectionHeader() {
    return file.objectOf<IMAGE_SECTION_HEADER>(vwCodeSectionHeader);
}

IMAGE_SECTION_HEADER* Binary::dataSectionHeader() {
    return file.objectOf<IMAGE_SECTION_HEADER>(vwDataSectionHeader);
}

String Binary::extension() {
    if (mod->isaDll()) {
        return { S("dll") };
    }
    return { S("exe") };
}

WORD Binary::subSystem() {
    switch (mod->binaryKind) {
    case BinaryKind::Console: return IMAGE_SUBSYSTEM_WINDOWS_CUI;
    case BinaryKind::Windows: return IMAGE_SUBSYSTEM_WINDOWS_GUI;
    case BinaryKind::Service: return IMAGE_SUBSYSTEM_WINDOWS_CUI;
    case BinaryKind::Dll:     return IMAGE_SUBSYSTEM_WINDOWS_CUI;
    default:
        err(mod, "unknown subsystem for file %s#<yellow|underline>", &fileName);
    }
    return IMAGE_SUBSYSTEM_UNKNOWN;
}
} // namespace linker_pass
} // namespace exy