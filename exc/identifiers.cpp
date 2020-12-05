////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2020-12-02
////////////////////////////////////////////////////////////////

#include "pch.h"
#include "identifiers.h"

namespace exy {
void Identifiers::initialize() {
    append(S(""), Dict<Identifier>::nullHash);
}

void Identifiers::dispose() {
    list.dispose();
    mem.dispose();
}

Identifier Identifiers::get(const char *v, int vlen, UINT vhash) {
    auto nullHash = Dict<Identifier>::nullHash;
    Assert(vlen >= 0);
    if (vlen) {
        if (vhash) {
            Assert(vhash != nullHash);
        } else {
            vhash = hash32(v, vlen);
        }
    } else if (vhash == nullHash) {
        // Do nothing.
    } else {
        vhash = nullHash;
    }
    lock();
    if (vhash == nullHash) {
        auto id = list.items[0].value;
        unlock();
        return id;
    }
    const auto idx = list.indexOf(vhash);
    Assert(idx);
    if (idx > 0) {
        auto id = list.items[idx].value;
        unlock();
        return id;
    }
    const auto id = append(v, vlen, vhash);
    unlock();
    return id;
}

Identifier Identifiers::get(const char *v, int vlen) {
    return get(v, vlen, 0u);
}

Identifier Identifiers::get(const char *v, const char *vend) {
    return get(v, (int)(vend - v), 0u);
}

Identifier Identifiers::get(const String &v) {
    return get(v.text, v.length, v.hash);
}

Identifier Identifiers::get(Identifier v) {
    if (v) {
        return get(v->text, v->length, v->hash);
    }
    return get(S(""));
}

Identifier Identifiers::append(const char *v, int vlen, UINT vhash) {
    auto text = mem.alloc<char>(vlen + 1);
    if (vlen) MemCopy(text, v, vlen);
    auto  str = mem.New<String>(text, vlen, vhash);
    list.append(vhash, str);
    return str;
}
} // namespace exy