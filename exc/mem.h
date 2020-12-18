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

    template<typename T, typename ...TArgs>
    T* New(TArgs&&...args) {
        auto m = alloc<T>();
        return new(m) T{ meta::fwd<TArgs>(args)... };
    }

private:
    struct Slab {
        int length;
        int   used;

        int unused() const { return length - used; }
        void* alloc(int size);
    };
    SRWLOCK srw{};
    Slab  **slabs{};
    int     length{};

    void* doAlloc(int size);
};

template<typename T>
T* MemAlloc(int count = 1) {
    return (T*)Heap::alloc(sizeof(T) * count);
}

template<typename T>
T* MemReAlloc(T *m, int count = 1) {
    return (T*)Heap::realloc(m, sizeof(T) * count);
}

template<typename T>
T* MemFree(T *m) {
    return (T*)Heap::free(m);
}

template<typename T>
constexpr void MemZero(T *mem, int count = 1) { 
    ZeroMemory(mem, sizeof(T) * count); 
}

template<typename T, typename U>
constexpr void MemCopy(T *dst, U *src, int count = 1) { 
    CopyMemory((void*)dst, (void*)src, sizeof(T) * count); 
}

template<typename T, typename U>
constexpr void MemMove(T *dst, U *src, int count = 1) { 
    MoveMemory(dst, src, sizeof(T) * count); 
}

} // namespace exy

#endif // MEM_H_