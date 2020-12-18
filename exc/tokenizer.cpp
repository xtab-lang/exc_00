#include "pch.h"
#include "tokenizer.h"

#include "src_char_stream.h"
#include "src_tok_stream.h"

#include "source.h"

namespace exy {
void Tokenizer::next(SourceFile &file) {
    CharStream       stream{ file };
    List<SourceChar> list{};
    while (true) {
        auto ch = stream.next();
        list.append(ch);
        if (stream.isEOF(ch)) {
            break;
        }
    }
    traceln("%s#<underline yellow> { size: %i#<bold> B, characters: %i#<bold>, thread: %i#<green> }",
            file.path, file.source.length, list.length, GetCurrentThreadId());
    tokenize(file, list);
    list.dispose();
}

void Tokenizer::tokenize(SourceFile &file, const List<SourceChar> &chars) {
    TokenStream stream{ file, chars };
    auto &tokens = file.tokens;
    while (true) {
        auto token = stream.next();
        tokens.append(token);
        /*if (token.kind >= Tok::Text) {
            traceln("%tn %t", token.kind, &token);
        }*/
        if (token.kind == Tok::EndOfFile) {
            break;
        }
    }
}
} // namespace exy