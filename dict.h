//////////////////////////////////////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2020-12-02
//////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef DICT_H_
#define DICT_H_

namespace exy {

namespace internal_dict_ {
// Gets the 1st prime number that is larger than {value}.
INT nextPrime(INT current);
} // namespace internal_dict_

template<typename T, typename Hash = UINT>
struct Dict final {
    static const Hash nullHash = ~Hash(0);

    struct Item final {
        INT  bucket, next;
        Hash hash;
        T    value;
    };

    Item *items{};
    INT   length{}, capacity{};

    Dict() = default;

    // Disposes {this} dict without disposing each value.
    void dispose() {
        items = MemFree(items);
        length = capacity = 0;
    }

    // Clears {this} dict without clearing each value.
    void clear() {
        length = 0;
    }

    // Disposes {this} dict with a value-by-value {disposer}.
    template<typename TDisposer>
    void dispose(TDisposer disposer) {
        for (auto i = 0; i < length; ++i) {
            disposer(items[i].value);
        }
        dispose();
    }

    // Disposes {this} dict with a value-by-value {clearer}.
    template<typename TClearer>
    void clear(TClearer clearer) {
        for (auto i = 0; i < length; ++i) {
            clearer(items[i].value);
        }
        clear();
    }

    bool isEmpty() const {
        return length == 0;
    }

    T& first() {
        Assert(length);
        return items[0].value;
    }

    const T& first() const {
        Assert(length);
        return items[0].value;
    }

    T& last() {
        Assert(length);
        return items[length - 1];
    }

    const T& last() const {
        Assert(length);
        return items[length - 1].value;
    }

    T& operator[](INT index) {
        Assert(index >= 0 && index < length);
        return items[index].value;
    }

    const T& operator[](INT index) const {
        Assert(index >= 0 && index < length);
        return items[index].value;
    }

    // {append} by hash.
    T& append(Hash hash) {
        static const T empty{};
        return append(hash, empty);
    } T& append(Hash hash, const T &item) {
        Assert(hash);
        resize();
        const auto bucket = INT(hash % Hash(capacity));
        Assert(bucket < capacity);
        for (auto i = items[bucket].bucket; i >= 0; i = items[i].next) {
            if (items[i].hash == hash) {
                Assert(0);
            }
        }
        items[length].value = item;
        items[length].next = items[bucket].bucket;
        items[length].hash = hash;
        items[bucket].bucket = length;
        return items[length++].value;
    }

    // {append} by identifier.
    T& append(Identifier identifier) {
        return append(identifier->hash);
    }

    T& append(Identifier identifier, const T &item) {
        return append(identifier->hash, item);
    }

    // Get value by hash/name.
    T& get(Hash hash) {
        const auto idx = indexOf(hash);
        Assert(idx >= 0);
        return items[idx].value;
    }
    T& get(Identifier name) {
        return get(name->hash);
    }

    // {index} by hash.
    INT indexOf(Hash hash) const {
        Assert(hash);
        if (capacity == 0) {
            return -1; // Not initialized.
        }
        const auto bucket = INT(hash % Hash(capacity));
        if (!items) {
            // Do nothing.
        } else for (auto i = items[bucket].bucket; i >= 0; i = items[i].next) {
            if (items[i].hash == hash) {
                return i;
            }
        }
        return -1;
    }
    bool contains(Hash hash) const {
        return indexOf(hash) >= 0;
    }

    // {index} by identifier.
    INT indexOf(Identifier identifier) const {
        return indexOf(identifier->hash);
    }
    bool contains(Identifier identifier) const {
        return indexOf(identifier) >= 0;
    }

    // Applies a function {f} to each item of {this} dict.
    template<typename TVisitor>
    void forEach(TVisitor f) {
        for (auto i = 0; i < length; ++i) {
            f(items[i].value);
        }
    }

private:
    void resize() {
        Assert(length <= capacity);
        if (length == capacity) {
            const auto cap = internal_dict_::nextPrime(capacity);
            Assert(cap > capacity);
            items = MemReAlloc(items, cap);
            for (auto i = 0; i < cap; ++i) {
                items[i].next = items[i].bucket = -1;
            } for (capacity = 0; capacity < length; ++capacity) {
                const auto bucket = INT(items[capacity].hash % Hash(cap));
                items[capacity].next = items[bucket].bucket;
                items[bucket].bucket = capacity;
            }
            capacity = cap;
        }
    }
};

} // namespace exy

#endif // IDS_H_