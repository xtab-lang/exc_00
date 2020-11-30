////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2020-11-25
////////////////////////////////////////////////////////////////

#pragma once
#ifndef LIST_H_
#define LIST_H_

namespace exy {
template<typename T>
struct List {
    T  *items{};
    int length{};
    int capacity{};

    void dispose() {
        items = memfree(items);
        length = 0;
        capacity = 0;
    }

    template<typename Disposer>
    void dispose(Disposer disposer) {
        for (auto i = 0; i < length; ++i) {
            disposer(items[i]);
        }
        dispose();
    }

    T& append(T &item) {
        reserve(1);
        return items[length++] = item;
    }

    void reserve(int count) {
        if (!capacity) {
            capacity = max(count, 4);
            items = memalloc<T>(capacity);
            return;
        }
        auto cap = length + count;
        if (cap <= capacity) {
            return;
        }
        while (capacity < cap) capacity *= 2;
        items = memrealloc(items, capacity);
    }
};
} // namespace exy

#endif // LIST_H_