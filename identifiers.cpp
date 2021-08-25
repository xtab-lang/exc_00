#include "pch.h"

namespace exy {
void Identifiers::initialize() {
    append(S(""), Dict<Identifier>::nullHash);
    // Web protocols.
    kw_http  = get(S("http"));
    kw_https = get(S("https"));
    kw_ws    = get(S("ws"));
    kw_wss   = get(S("wss"));
    // HTTP verbs.
    kw_GET      = get(S("GET"));
    kw_POST     = get(S("POST"));
    kw_DELETE   = get(S("DELETE"));
    kw_PUT      = get(S("PUT"));
    kw_HEAD     = get(S("HEAD"));
    kw_CONNECT  = get(S("CONNECT"));
    kw_OPTIONS  = get(S("OPTIONS"));
    kw_TRACE    = get(S("TRACE"));
    kw_PATCH    = get(S("PATCH"));
    // Module systems.
    kw_console = get(S("console"));
    kw_windows = get(S("windows"));
    kw_service = get(S("service"));
    kw_driver  = get(S("driver"));
    kw_dll     = get(S("dll"));
    kw_lib     = get(S("lib"));
    // Names.
    kw_star    = get(S("*"));
    kw_blank   = get(S("_"));
    kw__args__ = get(S("__args__"));
    kw_this    = get(S("this"));
    kw_super   = get(S("super"));
    // Known modules.
    kw_aio         = get(S("aio"));
    kw_std         = get(S("std"));
    kw_collections = get(S("collections"));
    // Known structs.
    kw_OVERLAPPED = get(S("OVERLAPPED"));
    kw_SRWLOCK    = get(S("SRWLOCK"));
    kw_string     = get(S("string"));
    kw_String     = get(S("String"));
    kw_wstring    = get(S("wstring"));
    kw_WString    = get(S("WString"));
    kw_Resumable  = get(S("Resumable"));
    kw_List       = get(S("List"));
    // Known functions.
    kw_main    = get(S("main"));
    kw_print   = get(S("print"));
    kw_println = get(S("println"));
    kw_next    = get(S("next"));
    kw_dispose = get(S("dispose"));
    kw_malloc  = get(S("malloc"));
    kw_mfree   = get(S("mfree"));
    kw_open_close_parens   = get(S("()"));
    kw_open_close_brackets = get(S("[]"));
    kw_startup  = get(S("startup"));
    kw_shutdown = get(S("shutdown"));
    kw_acquire  = get(S("acquire"));
    kw_release  = get(S("release"));
    // Known fields.
    kw_srw        = get(S("srw"));
    kw_overlapped = get(S("overlapped"));
    kw__awaiter__ = get(S("__awaiter__"));
    kw__resume__  = get(S("__resume__"));
    kw__hResult__ = get(S("__hResult__"));
    kw__bytesTransferred__ = get(S("kw__bytesTransferred__"));
    kw__done__   = get(S("__done__"));

    kw_source = get(S("source"));
    kw_index  = get(S("index"));
    kw_text   = get(S("text"));
    kw_items  = get(S("items"));
    kw_length = get(S("length"));
    // Blocks.
    kw_block = get(S("block"));
    kw_if    = get(S("if"));
    kw_loop  = get(S("loop"));
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
    s.append(S("`"));
    lock();
    auto suffix = randomCounter++;
    unlock();
    s.appendInt(suffix).append(S("`"));
    auto id = append(s.text, s.length, hash32(s.text, s.length));
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