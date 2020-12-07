//////////////////////////////////////////////////////////////////////////////////////////////////
// author: exy.lang
//   date: 2020-12-04
//////////////////////////////////////////////////////////////////////////////////////////////////

#include "pch.h"
#include "preparser.h"

#include "source.h"
#include "compiler.h"

namespace exy {
//------------------------------------------------------------------------------------------------
static Dict<Keyword> keywords{};

void initializeKeywords() {
    #define ZM(zName, zText) keywords.append(hash32(S(zText)), Keyword::zName);
        DeclareKeywords(ZM)
    #undef ZM
    #define ZM(zName, zText) keywords.append(hash32(S(zText)), Keyword::zName);
        DeclareModifiers(ZM)
    #undef ZM
    #define ZM(zName, zText) keywords.append(hash32(S(zText)), Keyword::zName);
        DeclareUserDefinedTypeKeywords(ZM)
    #undef ZM
    #define ZM(zName, zSize) keywords.append(hash32(S(#zName)), Keyword::zName);
        DeclareBuiltinTypeKeywords(ZM)
    #undef ZM
}

void disposeKeywords() {
    keywords.dispose();
}

Keyword getKeyword(const String &value) {
    auto hash = hash32(value.text, value.length);
    auto  idx = keywords.indexOf(hash);
    if (idx >= 0) {
        return keywords.items[idx].value;
    }
    return Keyword::None;
}
//------------------------------------------------------------------------------------------------
enum class State {
    InCode,
    InParens,
    InBrackets,
    InCurlies,

    InHashCurlies,
    InSingleQuoted,
    InDoubleQuoted,
};

auto getState(Tok kind) {
    switch (kind) {
        case Tok::Unknown:     return State::InCode;

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

        default: {
            Assert(0);
        } break;
    }
    return State::InCode;
}
//------------------------------------------------------------------------------------------------
struct TokenStream {
    List<SourceToken> &list;
    List<int>          opens{};
    List<int>          closes{};

    TokenStream(SourceFile &file) : list(file.tokens) {}

    void dispose() {
        opens.dispose();
        closes.dispose();
    }

    void process() {
        int j = -1;

        for (auto i = 0; i < list.length; ++i) {
            auto  &pos = list.items[i];
            if (pos.kind == Tok::EndOfFile) {
                break;
            }

            auto         last = opens.length ? list.items[opens.last()].kind : Tok::Unknown;
            auto        state = getState(last);
            auto         prev = j >= 0 ? list.items[j].kind : Tok::Unknown;
            auto    isEscaped = prev == Tok::BackSlash;
            auto isNotEscaped = !isEscaped;

            auto    isInCode = state == State::InCode || state == State::InParens || 
                state == State::InBrackets || state == State::InCurlies;
            auto isNotInCode = !isInCode;

            switch (pos.kind) {
                case Tok::Space:
                case Tok::NewLine: {
                    continue; // So that {j} does not change because of SP or NL.
                }

                case Tok::OpenSingleLineComment: if (isInCode) {
                    i = skipSingleLineComment(i);
                    continue; // So that {j} does not chage because of '//'.
                } break;

                case Tok::OpenMultiLineComment: if (isInCode) {
                    i = skipMultiLineComment(i);
                    continue; // So that {j} does not chage because of '//'.
                } break;

                case Tok::SingleQuote:
                case Tok::WideSingleQuote:
                case Tok::RawSingleQuote: if (state == State::InDoubleQuoted) {
                    break; // "'", "w'" or "r'" inside '"'. Do nothing.
                } else if (state == State::InSingleQuoted) {
                    if (isNotEscaped) { // "'" without a preceding '\'.
                        opens.pop(); // Pop state.
                        closes.push(i); // Mark close.
                    }
                } else if (isInCode) {
                    opens.push(i); // Begin 'InSingleQuoted'.
                } break;

                case Tok::DoubleQuote:
                case Tok::WideDoubleQuote:
                case Tok::RawDoubleQuote: if (state == State::InSingleQuoted) {
                    break; // '"' inside "'". Do nothing.
                } else if (state == State::InDoubleQuoted) {
                    if (isNotEscaped) { // '"' without a preceding '\'.
                        opens.pop(); // Pop state.
                        closes.push(i); // Mark close.
                    }
                } else if (isInCode) {
                    opens.push(i); // Begin 'InDoubleQuoted'.
                } break;

                case Tok::OpenParen:
                case Tok::OpenBracket:
                case Tok::OpenCurly:
                case Tok::HashOpenCurly: if (isInCode) {
                    opens.push(i); // '(','[', '{' or '#{' in code. Open.
                } break;

                case Tok::HashOpenBracket:
                case Tok::HashOpenParen: if (isNotInCode && isNotEscaped) {
                    opens.push(i); // '#[' or '#(' in text without preceding '\'.
                } break;

                case Tok::CloseParen: if (state == State::InParens) {
                    opens.pop(); // ')' in code with opening '(' or '#('. Pop state.
                    closes.push(i); // Mark close.
                } else if (isInCode) {
                    err(pos, "unmatched %t", &pos); // ')' in code without opening '(' or '#('.
                } break;

                case Tok::CloseBracket: if (state == State::InBrackets) {
                    opens.pop(); // ']' in code with opening '[' or '#['. Pop state.
                    closes.push(i); // Mark close.
                } else if (isInCode) {
                    err(pos, "unmatched %t", &pos); // ']' in code without opening '[' or '#['.
                } break;

                case Tok::CloseCurly: if (state == State::InCurlies) {
                    opens.pop(); // '}' in code with opening '{'. Pop state.
                    closes.push(i); // Mark close.
                } else if (isInCode) {
                    err(pos, "unmatched %t", &pos); // '}' in code without opening '{'.
                } break;

                case Tok::CloseCurlyHash: if (state == State::InHashCurlies) {
                    opens.pop(); // '}#' in text with opening '#{'. Pop state.
                    closes.push(i); // Mark close.
                } else if (isInCode) {
                    err(pos, "unmatched %t", &pos); // '}#' in code.
                } break;

                case Tok::Greater:
                case Tok::RightShift:
                case Tok::UnsignedRightShift: if (isInCode && isCloseAngle(i)) {
                    auto found = findOpenOfCloseAngle(i);
                    if (found > 0) {
                        markOpenAndCloseAngles(found, i);
                    }
                } break;

                case Tok::Multiply:
                case Tok::Exponentiation: if (isInCode && isPointerOrReference(i)) {
                    markPointer(i);
                } break;

                case Tok::And:
                case Tok::AndAnd: if (isInCode && isPointerOrReference(i)) {
                    markReference(i);
                } break;

                case Tok::Text: if (isInCode) {
                    pos.keyword = getKeyword(pos.value());
                } break;

                default: {
                } break;
            }
            j = i;
        }
        for (auto i = opens.length - 1; i >= 0; i--) {
            auto &pos = list.items[i];
            err(pos, "unmatched %t", &pos); // '}#' in code.
        }
    }

    bool isCloseAngle(int i) {
        auto next = i + 1; 
        for (; next < list.length; ++next) {
            auto &pos = list.items[next];
            if (pos.kind == Tok::OpenSingleLineComment) {
                next = skipSingleLineComment(i);
            } else if (pos.kind == Tok::OpenMultiLineComment) {
                next = skipMultiLineComment(i);
            } else if (pos.kind == Tok::Space || pos.kind == Tok::NewLine) {
                continue;
            }
        } if (next >= list.length) {
            return true;
        }
        auto &pos = list.items[next];
        switch (pos.kind) {
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

    int findOpenOfCloseAngle(int i) {
        int start = closes.length ? closes.last() : 0;
        for (auto j = i - 1; j > start; --j) {
            auto &pos = list.items[j];
            if (pos.kind == Tok::Less) {
                if (isOpenAngle(j)) {
                    return j;
                }
            }
        }
        return -1;
    }

    bool isOpenAngle(int i) {
        for (auto j = i - 1; j > 0; --j) {
            auto &pos = list.items[j];
            if (pos.kind == Tok::Space || pos.kind == Tok::NewLine) {
                continue;
            } if (pos.kind == Tok::Text) {
                return j;
            }
        }
        return false;
    }

    void markOpenAndCloseAngles(int open, int close) {
        list.items[open].kind = Tok::OpenAngle;
        switch (list.items[close].kind) {
            case Tok::Greater: {
            } break; // Do nothing.
            case Tok::RightShift: {
                list.insert(close, SourceToken(list.items[close])); // Copy '>>' to next index.
                list.items[close + 1].kind = Tok::Greater; // Rename it.
            } break;
            case Tok::UnsignedRightShift: {
                list.insert(close, SourceToken(list.items[close])); // Copy '>>>' to next two indices.
                list.insert(close, SourceToken(list.items[close]));
                list.items[close + 1].kind = Tok::Greater; // Rename them.
                list.items[close + 2].kind = Tok::Greater; // Rename them.
            } break;
            default:
                Assert(0);
                break;
        }
        list.items[close].kind = Tok::CloseAngle;
    }

    bool isPointerOrReference(int i) {
        auto next = i + 1;
        for (; next < list.length; ++next) {
            auto &pos = list.items[next];
            if (pos.kind == Tok::OpenSingleLineComment) {
                next = skipSingleLineComment(i);
            } else if (pos.kind == Tok::OpenMultiLineComment) {
                next = skipMultiLineComment(i);
            } else if (pos.kind == Tok::Space || pos.kind == Tok::NewLine) {
                continue;
            }
            return isTokenAfterPointerOrReference(next);
        } if (next >= list.length) {
            return true;
        }
        return false;
    }

    bool isTokenAfterPointerOrReference(int i) {
        auto &pos = list.items[i];
        switch (pos.kind) {
            case Tok::Multiply:                 // '*' then '*'
            case Tok::Exponentiation:           // '*' then '**'
            case Tok::And:                      // '*' then '&'
            case Tok::AndAnd:                   // '*' then '&&'
            case Tok::CloseAngle:               // '*' then '>'
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
            default:
                return false;
        }
    }

    void markPointer(int i) {
        if (list.items[i].kind == Tok::Multiply) {
            list.items[i].kind = Tok::Pointer;
        } else {
            list.insert(i, SourceToken(list.items[i]));
            list.items[i].kind = Tok::Pointer;
            list.items[i + 1].kind = Tok::Pointer;
        }
    }

    void markReference(int i) {
        if (list.items[i].kind == Tok::And) {
            list.items[i].kind = Tok::Reference;
        } else {
            list.insert(i, SourceToken(list.items[i]));
            list.items[i].kind = Tok::Reference;
            list.items[i + 1].kind = Tok::Reference;
        }
    }

    int skipSingleLineComment(int i) {
        // {i} is at '//'
        for (++i; i < list.length; ++i) {
            auto &pos = list.items[i];
            if (pos.kind == Tok::NewLine) {
                return i;
            }
        }
        return list.length;
    }

    int skipMultiLineComment(int i) {
        // {i} is at '/*'
        for (++i; i < list.length; ++i) {
            auto &pos = list.items[i];
            if (pos.kind == Tok::CloseMultiLineComment) {
                return i;
            }
        }
        return list.length;
    }
};
//------------------------------------------------------------------------------------------------
struct PreParser {
    auto next(SourceFile &file) {
        Assert(file.tokens.length > 0);
        TokenStream stream{ file };
        stream.process();
        stream.dispose();
        /*for (auto i = 0; i < file.tokens.length; ++i) {
            auto   &pos = file.tokens.items[i];
            auto &start = pos.range.start;
            auto   &end = pos.range.end;
            if (start.line == end.line) {
                trace("%s#<underline cyan>(%i#<cyan>:%i#<darkcyan>-%i#<darkcyan>) → ", 
                      file.dotName, start.line, start.col, end.col);
            } else {
                trace("%s#<underline cyan>(%i#<cyan>:%i#<darkcyan>-%i#<cyan>:%i#<darkcyan>) → ", 
                      file.dotName, start.line, start.col, end.line, end.col);
            }
            auto name = pos.name();
            trace("%s#<green>: ", &name);
            auto svalue = pos.sourceValue();
            auto value = pos.value();
            traceln("%s#<yellow> (%i#<darkyellow>)", &value, svalue.length);
        }*/
    }
};
namespace tok_pass {
void preparse() {
    initializeKeywords();
    SourceFileProvider provider{};
    PreParser          preparser{};
    aio::run(preparser, provider);
    provider.dispose();
    disposeKeywords();
}
} // namespace tok_pass
} // namespace exy