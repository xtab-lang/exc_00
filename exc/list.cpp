//////////////////////////////////////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2020-11-20
//////////////////////////////////////////////////////////////////////////////////////////////////

#include "pch.h"

namespace exy {

namespace list_internal_ {
void sort(void *items, int length, int size, int(*comparer)(void*, const void*, const void*),
          void *ctx) {
    qsort_s(items, length, size, (int(*)(void*, const void*, const void*))comparer, ctx);
}
} // namespace list_internal_
} // namespace exy
