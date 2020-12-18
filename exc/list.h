//////////////////////////////////////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2020-11-25
//////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef LIST_H_
#define LIST_H_

namespace exy {
namespace list_internal_ {
void sort(void *items, int length, int size, int(*comparer)(void*, const void*, const void*),
          void *ctx);
}

template<typename T>
struct List {
    T  *items{};
    int length{};
    int capacity{};

    List() = default;

    List(T *items, int length, int capacity) : items(items), length(length), capacity(capacity) {}

    void dispose() {
        items = MemFree(items);
        length = capacity = 0;
    }

    void clear() {
        length = 0;
    }

    template<typename TDisposer>
    void dispose(TDisposer disposer) {
        for (auto i = 0; i < length; ++i) {
            disposer(items[i]);
        }
        dispose();
    }

    template<typename TDisposer>
    void clear(TDisposer disposer) {
        for (auto i = 0; i < length; ++i) {
            disposer(items[i]);
        }
        clear();
    }

    auto reserve(int n) -> List& {
        const auto cap = length + n;
        if (cap > capacity) {
            if (capacity == 0) {
                capacity = max(cap, 4);
            } else {
                while (capacity < cap) {
                    capacity <<= 1;
                }
            }
            items = MemReAlloc(items, capacity);
        }
        return *this;
    }

    T& append() {
        reserve(1);
        return items[length++];
    }

    T& append(const T &item) {
        reserve(1);
        items[length] = item;
        return items[length++];
    }

    T& insert(int at, const T &item) {
        Assert(at >= 0 && at <= length);
        if (at == length) {
            return append(item);
        }
        reserve(1);
        MemMove(items + at + 1, items + at, length - at);
        items[at] = item;
        auto &result = items[at];
        ++length;
        return result;
    }

    T& prepend(const T &item) {
        return insert(0, item);
    }

    template<typename ...TArgs>
    T& place(TArgs&&...args) {
        reserve(1);
        new(&items[length]) T{ meta::fwd<TArgs>(args)... };
        return items[length++];
    }

    bool isEmpty() const {
        return length == 0;
    }

    bool isNotEmpty() const {
        return length > 0;
    }

    T& first() {
        Assert(length);
        return items[0];
    }

    const T& first() const {
        Assert(length);
        return items[0];
    }

    T& last() {
        Assert(length);
        return items[length - 1];
    }

    const T& last() const {
        Assert(length);
        return items[length - 1];
    }

    T& operator[](int index) {
        Assert(index >= 0 && index < length);
        return items[index];
    }

    const T& operator[](int index) const {
        Assert(index >= 0 && index < length);
        return items[index];
    }

    T removeAt(int index) {
        Assert(index >= 0 && index < length);
        const auto next = index + 1;
        if (next == length) {
            return items[--length];
        }
        auto item = items[index];
        OsCopy(items + index, items + next, length - next);
        --length;
        return item;
    }

    T removeFirst() {
        return removeAt(0);
    }

    T removeLast() {
        return items[--length];
    }

    T& push(const T &item) {
        return append(item);
    }

    T pop() {
        return removeLast();
    }

    template<typename TVisitor>
    void forEach(TVisitor visitor) {
        for (auto i = 0; i < length; ++i) {
            visitor(items[i]);
        }
    }

    template<typename TVisitor>
    void forEachInReverse(TVisitor visitor) {
        for (auto i = length; --i >= 0; ) {
            visitor(items[i]);
        }
    }

    // Applies a {visitor} to each item {this} upto {this} list's
    // length. Any item added during the iteration will not be
    // visited. FUCK YOU IF YOU REMOVE DURING ITERATION. FUCK YOU.
    template<typename TVisitor>
    void fixedForEach(TVisitor visitor) {
        for (auto i = 0, c = length; i < c; ++i) {
            visitor(items[i]);
        }
    }

    template<typename TComparer>
    void sort(TComparer comparer) {
        using TSorter = int(*)(void*, T*, T*);
        using TFunc = int(*)(T&, T&);
        auto sorter = [](void *ctx, T *a, T *b) {
            union {
                void *ctx;
                TComparer func;
            } cast{ ctx };
            return cast.func(*a, *b);
        };
        union {
            TComparer comparer;
            TFunc func;
        } cast1{ comparer };
        union {
            TSorter sorter;
            int(*func)(void*, const void*, const void*);
        } cast2{ sorter };
        list_internal_::sort(items, length, sizeof(T), cast2.func, cast1.func);
    }
};
} // namespace exy

#endif // LIST_H_