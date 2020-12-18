//////////////////////////////////////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2020-12-09
//////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef SRC_CHAR_STREAM_H_
#define SRC_CHAR_STREAM_H_

namespace exy {
//--Begin forward declarations
struct SourceChar;
struct SourceFile;
//----End forward declarations
struct CharStream {
    const char *pos;
    const char *end;
    int         line = 1;
    int         col = 1;

    CharStream(SourceFile &file);

    SourceChar next();
    bool isEOF(const SourceChar&);
}; // struct CharStream
} // namespace exy

#endif // SRC_CHAR_STREAM_H_