//////////////////////////////////////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2021-02-01
//////////////////////////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "buffer.h"

#define err(msg, ...) print_error_noloc("Linker", msg, __VA_ARGS__)

namespace exy {
void Buffer::dispose() {
    data = MemFree(data);
    length = 0;
    capacity = 0;
}

View Buffer::align(int alignment, BYTE alignmentByte) {
    Assert(alignment > 0);
    auto offset = length;
    if (alignment > 1) {
        if (auto padding = ComputePadding(offset, alignment)) {
            if (alignmentByte) {
                reserve(padding);
                for (auto i = 0; i < padding; ++i) data[length++] = alignmentByte;
            } else {
                append(nullptr, padding);
            }
        }
    }
    return { offset, length - offset};
}

View Buffer::write(const void *v, int vlen, int alignment, BYTE alignmentByte) {
    align(alignment, alignmentByte);
    return append(v, vlen);
}

View Buffer::writeByte(BYTE byte) {
    return write(&byte, 1);
}

bool Buffer::saveToDisk(const String &fileName) {
    auto errors = comp.errors;
    auto handle = CreateFile(fileName.text, GENERIC_WRITE, 0, nullptr, OPEN_EXISTING,
                             FILE_ATTRIBUTE_NORMAL, nullptr);
    if (handle == INVALID_HANDLE_VALUE) {
        err("cannot open file %s#<yellow|underline>", &fileName);
        return false;
    } if (length) {
        DWORD bytesWritten{};
        if (WriteFile(handle, data, length, &bytesWritten, nullptr)) {
            Assert(bytesWritten == DWORD(length));
        } else {
            err("cannot write to file %s#<yellow|underline>", &fileName);
        }
    } if (!CloseHandle(handle)) {
        err("cannot close file %s#<yellow|underline>", &fileName);
    }
    return errors == comp.errors;
}

View Buffer::append(const void *v, int vlen) {
    auto offset = length;
    if (vlen > 0) {
        reserve(vlen);
        if (v) {
            MemCopy(data + length, (BYTE*)v, vlen);
        }
        length += vlen;
    } else {
        Assert(vlen == 0);
    }
    return { offset, length - offset };
}

void Buffer::reserve(int vlen) {
#define DEFAULT_BUFFER_SIZE 0x10
    auto cap = length + vlen;
    if (cap <= capacity) {
        return;
    } if (!capacity) {
        capacity = DEFAULT_BUFFER_SIZE;
    }
    while (capacity < cap) capacity *= 2;
    data = MemReAlloc<BYTE>(data, capacity);
}
} // namespace exy