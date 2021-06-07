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
};

} // namespace exy