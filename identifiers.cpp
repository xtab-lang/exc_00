#include "pch.h"

namespace exy {
void Identifiers::initialize() {
    append(S(""), Dict<Identifier>::nullHash);
    kw_main     = get(S("main"));
    kw_GET      = get(S("GET"));
    kw_POST     = get(S("POST"));
    kw_DELETE   = get(S("DELETE"));
    kw_PUT      = get(S("PUT"));
    kw_HEAD     = get(S("HEAD"));
    kw_CONNECT  = get(S("CONNECT"));
    kw_OPTIONS  = get(S("OPTIONS"));
    kw_TRACE    = get(S("TRACE"));
    kw_PATCH    = get(S("PATCH"));
}

void Identifiers::dispose() {
    list.dispose();
    mem.dispose();
    randomCounter = 1000;
}

Identifier Identifiers::get(const CHAR *v, INT vlen, UINT vhash) {
    constexpr auto nullHash = Dict<Identifier>::nullHash;
    Assert(vlen >= 0);
    if (vlen > 0) {
        if (vhash != 0) {
            Assert(vhash != nullHash);
        } else {
            vhash = hash32(v, vlen);
        }
    } else if (vhash == nullHash) {
        // Do nothing because {vlen} is zero.
    } else {
        vhash = nullHash; // Because {vlen} is zero.
    }
    lock();
    if (vhash == nullHash) {
        auto id = list.items[0].value;
        unlock();
        return id;
    }
    const auto idx = list.indexOf(vhash);
    if (idx > 0) {
        auto id = list.items[idx].value;
        unlock();
        return id;
    }
    Assert(idx < 0);
    const auto id = append(v, vlen, vhash);
    unlock();
    return id;
}

Identifier Identifiers::get(const CHAR *v, INT vlen) {
    return get(v, vlen, 0u);
}

Identifier Identifiers::get(const CHAR *v, const CHAR *vend) {
    return get(v, (INT)(vend - v), 0u);
}

Identifier Identifiers::get(const String &v) {
    return get(v.text, v.length, v.hash);
}

Identifier Identifiers::get(Identifier v) {
    if (v != nullptr) {
        return get(v->text, v->length, v->hash);
    }
    return get(S(""));
}

Identifier Identifiers::random(const CHAR *prefix, INT prefixlen) {
    String s{};
    if (prefixlen > 0) {
        s.append(prefix, prefixlen);
    }
    s.append(S("__"));
    lock();
    s.appendInt(randomCounter++);
    auto id = append(s.text, s.length, hash32(s.text, s.length));
    unlock();
    s.dispose();
    return id;
}

Identifier Identifiers::append(const CHAR *v, INT vlen, UINT vhash) {
    auto text = mem.alloc<CHAR>(vlen + 1);
    if (vlen) MemCopy(text, v, vlen);
    auto str = mem.New<String>(text, vlen, vhash);
    list.append(vhash, str);
    return str;
}
} // namespace exy