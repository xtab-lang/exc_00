//////////////////////////////////////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2020-12-09
//////////////////////////////////////////////////////////////////////////////////////////////////

#pragma once
#ifndef SRC2CHAR_STREAM_H_
#define SRC2CHAR_STREAM_H_

namespace exy {
namespace src2tok_pass {
struct CharStream {
    const char *pos;
    const char *end;
    int         line = 1;
    int         col = 1;

    CharStream(SourceFile &file);

    SourceChar next();
    bool isEOF(const SourceChar&);
};
} // namespace src2tok_pass
} // namespace exy

#endif // SRC2CHAR_STREAM_H_