/*
    This file defines the object that represents a single token within a Python
   source file, and the list of tokens.
*/

#ifndef TPY_PARSE_TOKEN_H
#define TPY_PARSE_TOKEN_H

#include "tpy/source/Span.h"

namespace tpy::Parse {

#define TOKEN_LIST(X)                                                          \
    X(Dummy)                                                                   \
    X(End)                                                                     \
    X(Newline)                                                                 \
    X(Indent)                                                                  \
    X(Dedent)                                                                  \
    X(Semicolon)                                                               \
    X(Colon)                                                                   \
    X(ColonEquals)                                                             \
    X(LeftParen)                                                               \
    X(RightParen)                                                              \
    X(LeftSquare)                                                              \
    X(RightSquare)                                                             \
    X(LeftCurly)                                                               \
    X(RightCurly)                                                              \
    X(Comma)                                                                   \
    X(Dot)                                                                     \
    X(At)                                                                      \
    X(Equals)                                                                  \
    X(Arrow)                                                                   \
    X(PlusEquals)                                                              \
    X(MinusEquals)                                                             \
    X(AsteriskEquals)                                                          \
    X(SlashEquals)                                                             \
    X(SlashSlashEquals)                                                        \
    X(PercentEquals)                                                           \
    X(AtEquals)                                                                \
    X(AmpersandEquals)                                                         \
    X(BarEquals)                                                               \
    X(CaretEquals)                                                             \
    X(GreaterGreaterEquals)                                                    \
    X(LessLessEquals)                                                          \
    X(AsteriskAsteriskEquals)                                                  \
    X(Plus)                                                                    \
    X(Minus)                                                                   \
    X(Asterisk)                                                                \
    X(AsteriskAsterisk)                                                        \
    X(Slash)                                                                   \
    X(SlashSlash)                                                              \
    X(Percent)                                                                 \
    X(LessLess)                                                                \
    X(GreaterGreater)                                                          \
    X(Ampersand)                                                               \
    X(Bar)                                                                     \
    X(Caret)                                                                   \
    X(Tilda)                                                                   \
    X(Less)                                                                    \
    X(Greater)                                                                 \
    X(LessEquals)                                                              \
    X(GreaterEquals)                                                           \
    X(EqualsEquals)                                                            \
    X(ExclamationEquals)                                                       \
    X(IntLiteral)                                                              \
    X(FloatLiteral)                                                            \
    X(StringLiteral)                                                           \
    X(HexIntLiteral)                                                           \
    X(BinaryIntLiteral)                                                        \
    X(OctalIntLiteral)                                                         \
    X(DoubleQuotedStringLiteral)                                               \
    X(SingleQuotedStringLiteral)                                               \
    X(KeywordFalse)                                                            \
    X(KeywordNone)                                                             \
    X(KeywordTrue)                                                             \
    X(KeywordAnd)                                                              \
    X(KeywordAs)                                                               \
    X(KeywordAssert)                                                           \
    X(KeywordAsync)                                                            \
    X(KeywordAwait)                                                            \
    X(KeywordBreak)                                                            \
    X(KeywordClass)                                                            \
    X(KeywordContinue)                                                         \
    X(KeywordDef)                                                              \
    X(KeywordDel)                                                              \
    X(KeywordElif)                                                             \
    X(KeywordElse)                                                             \
    X(KeywordExcept)                                                           \
    X(KeywordFinally)                                                          \
    X(KeywordFor)                                                              \
    X(KeywordFrom)                                                             \
    X(KeywordGlobal)                                                           \
    X(KeywordIf)                                                               \
    X(KeywordImport)                                                           \
    X(KeywordIn)                                                               \
    X(KeywordIs)                                                               \
    X(KeywordLambda)                                                           \
    X(KeywordNonlocal)                                                         \
    X(KeywordNot)                                                              \
    X(KeywordOr)                                                               \
    X(KeywordPass)                                                             \
    X(KeywordRaise)                                                            \
    X(KeywordReturn)                                                           \
    X(KeywordTry)                                                              \
    X(KeywordWhile)                                                            \
    X(KeywordWith)                                                             \
    X(KeywordYield)                                                            \
    X(Identifier)                                                              \
    /* We need to create some special token types here for two word            \
     * operators*/                                                             \
    X(NotInOp)                                                                 \
    X(IsNotOp)                                                                 \
    /* This error token will be used when the Lexer is unable to make a        \
     determination as to which token is intended.*/                            \
    X(ErrorToken)

#define F(x) x,
enum class TokenKind { TOKEN_LIST(F) };
#undef F

/*
    Ideally, we will only have one token instance that will be updated with new
   information. This will avoid constantly running a constructor and destructor.
*/
class Token {
    Token(TokenKind kind, Source::Span span) : kind{kind}, span{span} {}

  public:
    TokenKind kind;
    Source::Span span;

    auto update(TokenKind kind, Source::Span span) -> void {
        this->kind = kind;
        this->span = span;
    }

    static auto dummy() -> Token {
        return Token{TokenKind::Dummy, Source::Span::empty()};
    }
};

extern const char *token_names[];
} // namespace tpy::Parse

#endif