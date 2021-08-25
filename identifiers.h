#pragma once

namespace exy {
struct Identifiers {
    // Web protocols.
    Identifier kw_http;
    Identifier kw_https;
    Identifier kw_ws;
    Identifier kw_wss;
    // HTTP verbs.
    Identifier kw_GET;
    Identifier kw_POST;
    Identifier kw_DELETE;
    Identifier kw_PUT;
    Identifier kw_HEAD;
    Identifier kw_CONNECT;
    Identifier kw_OPTIONS;
    Identifier kw_TRACE;
    Identifier kw_PATCH;
    // Module systems.
    Identifier kw_console;
    Identifier kw_windows;
    Identifier kw_service;
    Identifier kw_driver;
    Identifier kw_dll;
    Identifier kw_lib;
    // Names.
    Identifier kw_star;  // '*'
    Identifier kw_blank; // '_'
    Identifier kw__args__;  // '__args__'
    Identifier kw_this;
    Identifier kw_super;
    // Known modules.
    Identifier kw_aio;
    Identifier kw_std;
    Identifier kw_collections;
    // Known structs.
    Identifier kw_OVERLAPPED;
    Identifier kw_SRWLOCK;
    Identifier kw_string;
    Identifier kw_String;
    Identifier kw_wstring;
    Identifier kw_WString;
    Identifier kw_Resumable;
    Identifier kw_List;
    // Known functions.
    Identifier kw_main;
    Identifier kw_print;
    Identifier kw_println;
    Identifier kw_next;
    Identifier kw_dispose;
    Identifier kw_malloc;
    Identifier kw_mfree;
    Identifier kw_open_close_parens;
    Identifier kw_open_close_brackets;
    Identifier kw_startup;
    Identifier kw_shutdown;
    Identifier kw_acquire;
    Identifier kw_release;
    // Known fields.
    Identifier kw_srw;        // 'srw'
    Identifier kw_overlapped; // 'overlapped'
    Identifier kw__awaiter__; // '__awaiter__'
    Identifier kw__resume__;  // '__resume__'
    Identifier kw__hResult__; // '__hResult__'
    Identifier kw__bytesTransferred__; // '__bytesTransferred__'
    Identifier kw__done__;    // '__done__'

    Identifier kw_source;     // 'source'
    Identifier kw_index;      // 'index'
    Identifier kw_text;       // 'text'
    Identifier kw_items;      // 'items'
    Identifier kw_length;     // 'length'
    // Blocks.
    Identifier kw_block;
    Identifier kw_if;
    Identifier kw_loop;

    void initialize();
    void dispose();

    Identifier get(const CHAR *v, INT vlen, UINT vhash);
    Identifier get(const CHAR *v, INT vlen);
    Identifier get(const CHAR *v, const CHAR *vend);
    Identifier get(const String&);
    Identifier get(Identifier);

    Identifier random(const CHAR *prefix, INT prefixlen);
    Identifier random(const String &prefix) { return random(prefix.text, prefix.length); }
    Identifier random(Identifier prefix) { return prefix == nullptr ? random(S("")) : random(prefix->text, prefix->length); }

private:
    Mem              mem{};
    Dict<Identifier> list{};
    SRWLOCK          srw{};
    INT              randomCounter = 1000;

    void lock() { AcquireSRWLockExclusive(&srw); }
    void unlock() { ReleaseSRWLockExclusive(&srw); }
    Identifier append(const CHAR *v, INT vlen, UINT vhash);
};

__declspec(selectany) Identifiers ids{};
} // namespace exy