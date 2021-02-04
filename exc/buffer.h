//////////////////////////////////////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2021-02-01
//////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef BUFFER_H
#define BUFFER_H

namespace exy {
struct View {
    int offset{};
    int length{};
};

struct Buffer {
    BYTE *data{};
    int   length{};
    int   capacity{};

    void dispose();

    View align(int alignment, BYTE alignmentByte = 0);
    View write(const void *v, int vlen, int alignment = 1, BYTE alignmentByte = 0);
    View writeByte(BYTE);

    bool saveToDisk(const String &fileName);
    
    template<typename T>
    View writeObject(const T &item, int alignment = 1) {
        return write(&item, sizeof(T), alignment);
    }

    template<typename T>
    T* alloc(int alignment = 1) {
        auto  vw = write(nullptr, sizeof(T), alignment);
        auto mem = data + vw.offset;
        return (T*)mem;
    }

    template<typename T>
    T* objectAt(int offset) {
        Assert(offset >= 0 && offset < length);
        return (T*)(data + offset);
    }
    template<typename T>
    T* objectOf(View &view) {
        Assert(view.offset >= 0 && view.offset < length && (view.offset + view.length) <= length);
        return (T*)(data + view.offset);
    }
private:
    View append(const void *v, int vlen);
    void reserve(int vlen);
};
} // namespace exy

#endif // BUFFER_H