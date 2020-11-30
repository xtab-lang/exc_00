//////////////////////////////////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2020-11-25
//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//

#pragma once
#ifndef MEM_H_
#define MEM_H_

namespace exy {
struct Heap {
    static UINT64  allocs;
    static UINT64  reallocs;
    static UINT64  frees;
    static UINT64  used;
    static SRWLOCK srw;

    static void initialize();
    static void dispose();

    static void* alloc(int size);
    static void* realloc(void *m, int size);
    static void* free(void *m);
};

struct Mem {
    void dispose();

    template<typename T>
    T* alloc(int count = 1) {
        return (T*)doAlloc(sizeof(T) * count);
    }

private:
    struct Slab {
        int length = 0;
        int   used = sizeof(int) * 2;

        int unused() const { return length - used; }
        void* alloc(int size);
    };
    SRWLOCK srw{};
    Slab  **slabs{};
    int     length{};

    void* doAlloc(int size);
};

template<typename T>
T* memalloc(int count = 1) {
    if (count > 0) {
        return (T*)Heap::alloc(sizeof(T) * count);
    }
    return (T*)nullptr;
}

template<typename T>
T* memrealloc(T *m, int count = 1) {
    if (m && count > 0) {
        return (T*)Heap::realloc(m, sizeof(T) * count);
    }
    return (T*)nullptr;
}

template<typename T>
T* memfree(T *m) {
    if (m) {
        return (T*)Heap::free(m);
    }
    return (T*)nullptr;
}

} // namespace exy

#endif // MEM_H_