#pragma once

namespace exy {
namespace heap {
void initialize();
void* alloc(INT n);
void* realloc(void *m, INT n);
void* free(void *m);
void dispose();
}

struct Mem {
    void dispose();

    template<typename T>
    T* alloc(INT count = 1) {
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

        auto unused() const { return length - used; }
        void* alloc(INT size);
    };
    SRWLOCK srw{};
    Slab  **slabs{};
    INT     length{};

    void* doAlloc(INT size);
};

template<typename T>
T* MemAlloc(INT count = 1) {
    return (T*)heap::alloc(sizeof(T) * count);
}

template<typename T>
T* MemReAlloc(T *m, INT count = 1) {
    return (T*)heap::realloc(m, sizeof(T) * count);
}

template<typename T>
T* MemFree(T *m) {
    return (T*)heap::free(m);
}

template<typename T>
constexpr void MemZero(T *mem, INT count = 1) {
    ZeroMemory(mem, sizeof(T) * count);
}

template<typename T, typename U>
constexpr void MemCopy(T *dst, U *src, INT count = 1) {
    CopyMemory((void*)dst, (void*)src, sizeof(T) * count);
}

template<typename T, typename U>
constexpr void MemMove(T *dst, U *src, INT count = 1) {
    MoveMemory(dst, src, sizeof(T) * count);
}

template<typename T, typename ...TArgs>
T* MemNew(TArgs&&...args) {
    auto m = MemAlloc<T>();
    return new(m) T{ meta::fwd<TArgs>(args)... };
}

template<typename T>
T* MemDispose(T *mem) {
    if (mem) {
        mem->dispose();
        MemFree(mem);
    }
    return (T*)nullptr;
}
} // namespace exy