#include "pch.h"
#include "token_processor.h"

#include "src.h"

#define err(pos, msg, ...) diagnostic("Tokenizer", pos, msg, __VA_ARGS__)

namespace exy {
TokenProcessor::TokenProcessor(SourceFile &file) : tokens(file.tokens) {}

void TokenProcessor::dispose() {
    opens.dispose();
    openAngles.dispose();
}

void TokenProcessor::run() {
    auto last = &tokens.last();
    Assert(last->kind == Tok::EndOfFile && *last->pos.range.start.text == '\0');
    auto j = -1;
    for (auto i = 0; i < tokens.length; i++) {
        auto &pos = tokens.items[i];
        if (pos.kind == Tok::EndOfFile) {
            break;
        }
        auto         open = opens.length ? tokens.items[opens.last()].kind : Tok::Unknown;
        auto        state = getState(open);
        auto         prev = j >= 0 ? tokens.items[j].kind : Tok::Unknown;
        auto    isEscaped = prev == Tok::BackSlash;
        auto isNotEscaped = !isEscaped;

        auto isInCode = state <= InCurlies;
        auto isInText = state >= InHashCurlies;

        switch (pos.kind) {
            case Tok::Space:
            case Tok::NewLine: {
                continue; // So that {j} does not change because of SP or NL.
            }

            case Tok::OpenSingleLineComment: if (isInCode) {
                auto commentPos = i; // Where the single-line comment will start.
                i = skipSingleLineComment(i);
                if (i < tokens.length) { // {i} is at NL.
                    auto &start = tokens.items[commentPos]; // Comment starts here.
                    auto   &end = tokens.items[i]; // Comment ends here. Excludes NL.
                    SourceToken comment{ start.pos.file, start.pos.range.start, end.pos.range.start, Tok::SingleLineComment };
                    tokens.erase(commentPos, i - commentPos); // Remove all tokens up to NL.
                    tokens.items[commentPos] = comment; // Replace the removed tokens with 1 single-line comment token.
                } else { // {i} is 1 past EOF.
                    --i; // Move to EOF.
                    auto &start = tokens.items[commentPos]; // Comment starts here.
                    auto   &end = tokens.items[i]; // Comment ends here. Excludes EOF.
                    SourceToken comment{ start.pos.file, start.pos.range.start, end.pos.range.start, Tok::SingleLineComment };
                    tokens.erase(commentPos, i - commentPos); // Remove all tokens up to 1 past NL.
                    tokens.items[commentPos] = comment; // Replace the removed tokens with 1 single-line comment token.
                }
                i = commentPos; // Because of {++i} above.
            } break;

            case Tok::OpenMultiLineComment: if (isInCode) {
                auto commentPos = i; // Where the multi-line comment will start.
                i = skipMultiLineComment(i);
                if (i == tokens.length) {
                    // {i} is 1 past EOF.
                    err(pos, "unmatched %tok", &pos);
                } else {
                    // {i} is at '*/'.
                    //++i; // Move 1 past '*/'.
                    auto &start = tokens.items[commentPos]; // Comment starts here.
                    auto   &end = tokens.items[i]; // Comment ends here. Includes '*/'.
                    SourceToken comment{ start.pos.file, start.pos.range.start, end.pos.range.start, Tok::MultiLineComment };
                    tokens.erase(commentPos, i - commentPos); // Remove all tokens up to 1 past '*/'.
                    tokens.items[commentPos] = comment; // Replace the removed tokens with 1 multi-line comment token.
                    i = commentPos; // Because of {++i} above.
                }
                continue; // So that {j} does not change because of SP or NL.
            } break;

            case Tok::SingleQuote:
            case Tok::WideSingleQuote:
            case Tok::RawSingleQuote: if (state == InDoubleQuoted) {
                break; // Do nothing because "'", "w'" or "r'" inside '"' is meaningless.
            } else if (state == InSingleQuoted && pos.kind == Tok::SingleQuote) {
                if (isNotEscaped) {
                    opens.pop(); // Remove "'", "w'" or "r'", to go back to code.
                }
            } else if (isInCode) {
                opens.push(i); // Begin text inside "'", "w'" or "r'".
            } break;

            case Tok::DoubleQuote:
            case Tok::WideDoubleQuote:
            case Tok::RawDoubleQuote: if (state == InSingleQuoted) {
                break; // Do nothing because '"', 'w"' or 'r"' inside '"' is meaningless.
            } else if (state == InDoubleQuoted && pos.kind == Tok::DoubleQuote) {
                if (isNotEscaped) {
                    opens.pop(); // Remove '"', 'w"' or 'r"', to go back to code.
                }
            } else if (isInCode) {
                opens.push(i); // Begin text inside '"', 'w"' or 'r"'.
            } break;

            case Tok::OpenParen:
            case Tok::OpenBracket:
            case Tok::OpenCurly:
            case Tok::HashOpenCurly: if (isInCode) {
                opens.push(i); // Begin code inside '(','[', '{' or text inside '#{'.
            } break;

            case Tok::HashOpenBracket:
            case Tok::HashOpenParen: if (isInText) {
                opens.push(i); // Begin code inside '#[' or '#(' in text without preceding '\'.
            } break;

            case Tok::CloseParen: if (state == State::InParens) {
                opens.pop(); // ')' in code with opening '(' or '#('. Pop state.
            } else if (isInCode) {
                err(pos, "unmatched %tok", &pos); // ')' in code without opening '(' or '#('.
            } break;

            case Tok::CloseBracket: if (state == State::InBrackets) {
                opens.pop(); // ']' in code with opening '[' or '#['. Pop state.
            } else if (isInCode) {
                err(pos, "unmatched %tok", &pos); // ']' in code without opening '[' or '#['.
            } break;

            case Tok::CloseCurly: if (state == State::InCurlies) {
                opens.pop(); // '}' in code with opening '{'. Pop state.
            } else if (isInCode) {
                err(pos, "unmatched %tok", &pos); // '}' in code without opening '{'.
            } break;

            case Tok::CloseCurlyHash: if (state == State::InHashCurlies) {
                opens.pop(); // '}#' in text with opening '#{'. Pop state.
            } else if (isInCode) {
                err(pos, "unmatched %tok", &pos); // '}#' in code.
            } break;

            case Tok::Less: if (isInCode) {
                if (prev == Tok::Text) {
                    // open-angle := identifier [SP | NL | CMT] '<'
                    openAngles.push(i);
                }
            } break;

            case Tok::Greater: if (isInCode && isaCloseAngle(i)) {
                if (openAngles.isNotEmpty()) {
                    // Mark '<'.
                    auto &less = tokens.items[openAngles.pop()];
                    less.kind = Tok::OpenAngle;
                    // Mark '>'.
                    pos.kind = Tok::CloseAngle;
                } else {
                    // Mark '>'.
                    auto &greater = pos;
                    greater.kind = Tok::CloseAngle;
                    err(pos, "unmatched %tok", &pos); // '>' in code.
                }
            } break;

            case Tok::RightShift: if (isInCode && isaCloseAngle(i)) {
                if (openAngles.isNotEmpty()) {
                    // Mark '<'.
                    auto &less = tokens.items[openAngles.pop()];
                    less.kind = Tok::OpenAngle;
                    // Mark '>'.
                    auto &greater = pos;
                    greater.kind = Tok::CloseAngle;
                    // Split the '>>' into 1 other '>' token.
                    SourceToken dup(pos);
                    tokens.insert(dup, /* at = */ i).kind = Tok::Greater;
                } else {
                    // Mark '>'.
                    auto &greater = pos;
                    greater.kind = Tok::CloseAngle;
                    err(pos, "unmatched %tok", &pos); // '>' in code.
                }
            } break;

            case Tok::UnsignedRightShift: if (isInCode && isaCloseAngle(i)) {
                if (openAngles.isNotEmpty()) {
                    // Mark '<'.
                    auto &less = tokens.items[openAngles.pop()];
                    less.kind = Tok::OpenAngle;
                    // Mark '>'.
                    auto &greater = pos;
                    greater.kind = Tok::CloseAngle;
                    // Split the '>>>' into 2 other '>' tokens.
                    SourceToken dup(pos);
                    tokens.insert(dup, /* at = */ i).kind = Tok::Greater;
                    tokens.insert(dup, /* at = */ i).kind = Tok::Greater;
                } else {
                    // Mark '>'.
                    auto &greater = pos;
                    greater.kind = Tok::CloseAngle;
                    err(pos, "unmatched %tok", &pos); // '>' in code.
                }
            } break;

            case Tok::Multiply: if (isInCode && isaPointerOrReference(i)) {
                pos.kind = Tok::Pointer;
            } break;

            case Tok::Exponentiation: if (isInCode && isaPointerOrReference(i)) {
                pos.kind = Tok::Pointer;
                // Split the '**' into 1 other '*' token.
                SourceToken dup(pos);
                tokens.insert(dup, /* at = */ i).kind = Tok::Pointer;
            } break;

            case Tok::And: if (isInCode && isaPointerOrReference(i)) {
                pos.kind = Tok::Reference;
            } break;

            case Tok::AndAnd: if (isInCode && isaPointerOrReference(i)) {
                pos.kind = Tok::Reference;
                // Split the '&&' into 1 other '&' token.
                SourceToken dup(pos);
                tokens.insert(dup, /* at = */ i).kind = Tok::Reference;
            } break;

            case Tok::Text: if (isInCode) {
                pos.keyword = kws.get(pos.sourceValue());
            } break;
        }
        j = i;
    }
    last = &tokens.last();
    Assert(last->kind == Tok::EndOfFile && *last->pos.range.start.text == '\0');
}

TokenProcessor::State TokenProcessor::getState(Tok tok) {
    switch (tok) {
        case Tok::Unknown: return InFile;

        case Tok::HashOpenParen:
        case Tok::OpenParen:   return State::InParens;

        case Tok::HashOpenBracket:
        case Tok::OpenBracket: return State::InBrackets;

        case Tok::OpenCurly:   return State::InCurlies;

        case Tok::SingleQuote:
        case Tok::WideSingleQuote:
        case Tok::RawSingleQuote: return State::InSingleQuoted;

        case Tok::DoubleQuote:
        case Tok::WideDoubleQuote:
        case Tok::RawDoubleQuote: return State::InDoubleQuoted;

        case Tok::HashOpenCurly:  return State::InHashCurlies;
    }
    Assert(0);
    return InFile;
}

INT TokenProcessor::skipSingleLineComment(INT i) {
    // {i} is at '//' hence ++{i} as initializer.
    for (++i; i < tokens.length; ++i) {
        auto &pos = tokens.items[i];
        if (pos.kind == Tok::NewLine) {
            return i;
        }
    }
    return tokens.length;
}

INT TokenProcessor::skipMultiLineComment(INT i) {
    // {i} is at '/*' hence ++{i} as initializer.
    for (++i; i < tokens.length; ++i) {
        auto &pos = tokens.items[i];
        if (pos.kind == Tok::CloseMultiLineComment) {
            return i;
        }
    }
    return tokens.length;
}

bool TokenProcessor::isaCloseAngle(INT i) {
    auto next = i + 1;
    for (; next < tokens.length; ++next) {
        auto &pos = tokens.items[next];
        if (pos.kind == Tok::OpenSingleLineComment) {
            next = skipSingleLineComment(next) - 1; // Because {next} is now at NL.
        } else if (pos.kind == Tok::OpenMultiLineComment) {
            next = skipMultiLineComment(next);
        } else if (pos.kind == Tok::Space || pos.kind == Tok::SingleLineComment || pos.kind == Tok::MultiLineComment) {
            // Do nothing.
        } else {
            break;
        }
    } 
    if (next >= tokens.length) {
        return true;
    }
    auto &pos = tokens.items[next];
    switch (pos.kind) {
        case Tok::NewLine:              // '>' then 'NL'
        case Tok::Less:                 // '>' then '>'
        case Tok::RightShift:           // '>' then '>>'
        case Tok::UnsignedRightShift:   // '>' then '>>>'
        case Tok::OpenParen:            // '>' then '('
        case Tok::CloseParen:           // '>' then ')'
        case Tok::OpenBracket:          // '>' then '['
        case Tok::CloseBracket:         // '>' then ']'
        case Tok::OpenCurly:            // '>' then '{'
        case Tok::CloseCurly:           // '>' then '}'
        case Tok::Colon:                // '>' then ':'
        case Tok::ColonColon:           // '>' then '::'
        case Tok::SemiColon:            // '>' then ';'
        case Tok::Comma:                // '>' then ','
        case Tok::Dot:                  // '>' then '.'
        case Tok::Multiply:             // '>' then '*'
        case Tok::Exponentiation:       // '>' then '**'
        case Tok::And:                  // '>' then '&'
        case Tok::AndAnd:               // '>' then '&&'
        case Tok::Assign:               // '>' then '='
        case Tok::EndOfFile:            // '>' then EOF
            return true;
        default:
            break;
    }
    return false;
}

bool TokenProcessor::isaPointerOrReference(INT i) {
    auto next = i + 1;
    for (; next < tokens.length; ++next) {
        auto &pos = tokens.items[next];
        if (pos.kind == Tok::OpenSingleLineComment) {
            next = skipSingleLineComment(next) - 1; // Because {next} is now at NL.
        } else if (pos.kind == Tok::OpenMultiLineComment) {
            next = skipMultiLineComment(next);
        } else if (pos.kind == Tok::Space || pos.kind == Tok::SingleLineComment || pos.kind == Tok::MultiLineComment) {
            // Do nothing.
        } else {
            break;
        }
    } 
    if (next >= tokens.length) {
        return true;
    }
    auto &pos = tokens.items[next];
    switch (pos.kind) {
        case Tok::NewLine:                  // '*' then 'NL'
        case Tok::Multiply:                 // '*' then '*'
        case Tok::Pointer:                  // '*' then '*'
        case Tok::Exponentiation:           // '*' then '**'
        case Tok::And:                      // '*' then '&'
        case Tok::Reference:                // '*' then '&'
        case Tok::AndAnd:                   // '*' then '&&'
        case Tok::Greater:                  // '*' then '>'
        case Tok::RightShift:               // '*' then '>>'
        case Tok::UnsignedRightShift:       // '*' then '>>>'
        case Tok::CloseParen:               // '*' then ')'
        case Tok::OpenBracket:              // '*' then '['
        case Tok::CloseBracket:             // '*' then ']'
        case Tok::OpenCurly:                // '*' then '{'
        case Tok::CloseCurly:               // '*' then '}'
        case Tok::SemiColon:                // '*' then ';'
        case Tok::Comma:                    // '*' then ','
        case Tok::Assign:                   // '*' then '='
        case Tok::EndOfFile:                // '*' then EOF
            return true;
    }
    return false;
}
} // namespace exy