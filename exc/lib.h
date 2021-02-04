//////////////////////////////////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2020-11-21
//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//\\//

#pragma once
#ifndef LIB_H_
#define LIB_H_

namespace lib {
void start();
void stop();
} // namespace lib


namespace exy {
namespace meta {
template<typename T> struct RemoveReference;
template<typename T> struct RemoveReference { using Type = T; };
template<typename T> struct RemoveReference<T&> { using Type = T; };
template<typename T> struct RemoveReference<T&&> { using Type = T; };
template<typename T> T&& move(T&& t) {
    using RValueReference = typename RemoveReference<T>::Type&&;
    return static_cast<RValueReference>(t);
}
template<typename T> T&& fwd(typename RemoveReference<T>::Type &t) {
    return static_cast<T&&>(t);
}
template<typename T>
void swap(T &a, T &b) {
    T temp = fwd<T&>(a); // or T temp(std::move(t1));
    a = fwd<T&>(b);
    b = fwd<T&>(temp);
}
template<typename U, typename T>
U reinterpret(T t) {
    union {
        T t;
        U u;
    } un = {};
    un.t = t;
    return un.u;
}
} // namespace meta
} // namespace exy

#define ComputePadding(zOffset, zAlignment)   (-(zOffset) & ((zAlignment) - 1))
#define ComputeAlignment(zOffset, zAlignment) (((zOffset) + ((zAlignment) - 1)) & -(zAlignment))

#define IsAPowerOf2(zValue) (((zValue) & 3) == 2)

#include "hash.h"
#include "format.h"
#include "err.h"
#include "mem.h"
#include "string.h"
#include "date.h"
#include "console.h"
#include "list.h"
#include "queue.h"
#include "dict.h"
#include "aio.h"

#endif // LIB_H_