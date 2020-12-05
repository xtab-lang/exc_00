//////////////////////////////////////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2020-12-05
//////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef QUEUE_H_
#define QUEUE_H_

namespace exy {
template<typename T>
struct Queue final {
    void dispose() {
        head = tail = nullptr;
    }

    template<typename TDisposer>
    void dispose(TDisposer disposer) {
        for (auto item = head; item; ) {
            auto next = (T*)item->qnext;
            disposer(item);
            item = next;
        }
        dispose();
    }

    T* prepend(T *item) {
        if (!item) {
            return nullptr;
        }
        Assert(!item->qprev && !item->qnext);
        if (head) {
            Assert(!head->qprev && !tail->qnext);
            item->qnext = head;
            head->qprev = (T*)item;
        } else {
            tail = item;
        }
        head = item;
        return item;
    }

    T* append(T *item) {
        if (!item) {
            return nullptr;
        }
        Assert(!item->qprev && !item->qnext);
        if (tail) {
            Assert(!head->qprev && !tail->qnext);
            item->qprev = tail;
            tail->qnext = (T*)item;
        } else {
            head = item;
        }
        tail = item;
        return item;
    }

    T* insertBefore(T *item, T *other) {
        if (!item) {
            return nullptr;
        } if (!other) {
            return prepend(item);
        }
        Assert(!item->qprev && !item->qnext);
        if (other == head) {
            Assert(!head->qprev && !tail->qnext);
        } else {
            if (other != tail) {
                Assert(other->qprev && other->qnext);
            }
            other->qprev->qnext = item;
        }
        item->qprev = other->qprev;
        other->qprev = item;
        item->qnext = other;
        if (other == head) {
            head = item;
        }
        return item;
    }

    T* insertAfter(T *item, T *other) {
        if (!item) {
            return nullptr;
        } if (!other) {
            return prepend(item);
        }
        Assert(!item->qprev && !item->qnext);
        if (other == tail) {
            Assert(!head->qprev && !tail->qnext);
        } else {
            other->qnext->qprev = item;
        }
        item->qnext = other->qnext;
        other->qnext = item;
        item->qprev = other;
        if (other == tail) {
            tail = item;
        }
        return item;
    }

    bool isEmpty() const {
        return head == nullptr;
    }

    bool isNotEmpty() const {
        return head != nullptr;
    }

    T* first() {
        return head;
    }

    const T* first() const {
        return head;
    }

    T* last() {
        return tail;
    }

    const T* last() const {
        return tail;
    }

    // Removes the first item in {this} queue and returns it. If
    // {this} queue is empty, returns nullptr.
    auto removeFirst() -> T* {
        if (auto found = head) {
            if (head == tail) {
                head = nullptr;
                tail = nullptr;
            } else {
                head = (T*)head->qnext;
                head->qprev = nullptr;
            }
            found->qnext = found->qprev = nullptr;
            return found;
        }
        return nullptr;
    }

    // Removes the last item in {this} queue and returns it. If
    // {this} queue is empty, returns nullptr.
    T* removeLast() {
        if (auto found = tail) {
            if (tail == head) {
                head = nullptr;
                tail = nullptr;
            } else {
                tail = (T*)tail->qprev;
                tail->qnext = nullptr;
            }
            found->qnext = found->qprev = nullptr;
            return found;
        }
        return nullptr;
    }

    // Removes an {item} from {this} queue.
    T* remove(T *item) {
        if (!item) {
            return nullptr;
        } if (item == tail) {
            return removeLast();
        } if (item == head) {
            return removeFirst();
        }
        auto prev = item->qprev, next = item->qnext;
        Assert(prev && next);
        prev->qnext = (T*)next;
        next->qprev = (T*)prev;
        item->qprev = item->qnext = nullptr;
        return item;
    }

    bool contains(T *item) {
        for (auto i = head; i; i = (T*)i->qnext) {
            if (i == item) {
                return true;
            }
        }
        return false;
    }

    // Applies a {visitor} to each item {this} queue.
    template<typename TVisitor>
    void forEach(TVisitor visitor) {
        for (auto item = head; item; item = (T*)item->qnext) {
            visitor(item);
        }
    }

    // Applies a {visitor} to each item {this} queue.
    template<typename TVisitor>
    void forEachInReverse(TVisitor visitor) {
        for (auto item = tail; item; item = (T*)item->qprev) {
            visitor(item);
        }
    }

    // Applies a {visitor} to each item {this} queue in a way that even if an item is added or
    // removed, the original order is preserved.
    template<typename TVisitor>
    void fixedForEach(TVisitor visitor) {
        for (auto item = head; item; ) {
            auto next = (T*)item->qnext;
            visitor(item);
            item = next;
        }
    }

    template<typename TReturn, typename TVisitor>
    TReturn first(TVisitor visitor) {
        for (auto item = head; item; item = (T*)item->qnext) {
            if (auto result = visitor(item)) {
                return result;
            }
        }
        return{};
    }

    template<typename TReturn, typename TVisitor>
    TReturn last(TVisitor visitor) {
        for (auto item = tail; item; item = (T*)item->qprev) {
            if (auto result = visitor(item)) {
                return result;
            }
        }
        return{};
    }
private:
    T *head{}, *tail{};
};
} // namespace exy

#endif // QUEUE_H_