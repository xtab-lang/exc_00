#include "pch.h"
#include "tokenizer.h"

#include "src2char_stream.h"
#include "src2tok_stream.h"

#include "source.h"

namespace exy {
namespace src2tok_pass {
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
    tokenize(file, list);
    traceln("%s#<underline yellow> { size: %i#<magenta> B, characters: %i#<magenta>, tokens: %i#<magenta>, thread: %i#<green> }",
            file.path, file.source.length, list.length, file.tokens.length, GetCurrentThreadId());
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
} // namespace src2tok_pass
} // namespace exy