#pragma once

namespace exy {

template<typename T>
struct List {
    T  *items{};
    INT length{};
    INT capacity{};

    auto dispose() {
        items = MemFree(items);
        length = capacity = 0;
    }

    auto clear() {
        length = 0;
        return *this;
    }

    auto compact() {
        if (length < capacity) {
            capacity = length;
            items = MemReAlloc(items, capacity);
        }
        return *this;
    }

    template<typename TDisposer>
    auto dispose(TDisposer disposer) {
        for (auto i = 0; i < length; ++i) {
            disposer(items[i]);
        }
        dispose();
    }

    template<typename TDisposer>
    auto clear(TDisposer disposer) {
        for (auto i = 0; i < length; ++i) {
            disposer(items[i]);
        }
        clear();
    }

    auto reserve(INT n) {
    #define INITIAL_LIST_CAPACITY 4
        auto cap = length + n;
        if (cap > capacity) {
            if (capacity == 0) {
                capacity = max(cap, INITIAL_LIST_CAPACITY);
            } else while (capacity < cap) {
                capacity *= 2;
            }
            items = MemReAlloc(items, capacity);
        }
        return *this;
    }

    T& append(const T &item) {
        reserve(1);
        items[length] = item;
        return items[length++];
    }

    template<typename ...TArgs>
    T& place(TArgs&&...args) {
        reserve(1);
        new(&items[length]) T{ meta::fwd<TArgs>(args)... };
        return items[length++];
    }

    T& push(const T& item) {
        reserve(1);
        items[length] = item;
        return items[length++];
    }

    T& pop() {
        Assert(length > 0);
        return items[--length];
    }

    T& insert(const T &item, INT at) {
        Assert(at >= 0 && at <= length);
        reserve(1);
        MemMove(/* dst = */ items + at + 1, /* src = */ items + at, 1);
        items[at] = item;
        ++length;
        return items[at];
    }

    auto isEmpty() const {
        return length == 0;
    }

    auto isNotEmpty() const {
        return length > 0;
    }

    T* start() const {
        return items;
    }

    T* end() const {
        Assert(length > 0);
        return items + length - 1;
    }

    T& first() const {
        Assert(length > 0);
        return items[0];
    }

    T& last() const {
        Assert(length > 0);
        return items[length - 1];
    }
};

} // namespace exy