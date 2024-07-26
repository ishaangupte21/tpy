/*
    Lexical Tokens
*/

use std::fmt::Debug;

use src_manager::span::Span;

#[derive(Debug)]
pub struct Token {
    pub kind: TokenKind,
    pub span: Span,
}

impl Token {
    pub fn new(kind: TokenKind, span: Span) -> Self {
        Self { kind, span }
    }

    pub fn dummy() -> Self {
        Self {
            kind: TokenKind::Dummy,
            span: Span::new(usize::MAX, usize::MAX),
        }
    }
}

#[derive(Debug, PartialEq, Clone, Copy)]
pub enum TokenKind {
    Dummy,
    End,
    Indent,
    Dedent,
    Newline,
    Unknown,

    // Delimiters
    Semicolon,
    LeftParen,
    RightParen,
    LeftSquare,
    RightSquare,
    LeftCurly,
    RightCurly,
    Comma,
    Dot,
    Equals,
    PlusEquals,
    MinusEquals,
    AsteriskEquals,
    SlashEquals,
    SlashSlashEquals,
    PercentEquals,
    AmpersandEquals,
    BarEquals,
    CaretEquals,
    GreaterGreaterEquals,
    LessLessEquals,
    AsteriskAsteriskEquals,
    Colon,

    // Operators
    Plus,
    Minus,
    Asterisk,
    AsteriskAsterisk,
    Slash,
    SlashSlash,
    Percent,
    LessLess,
    GreaterGreater,
    Ampersand,
    Bar,
    Caret,
    Tilde,
    ColonEquals,
    Less,
    Greater,
    LessEquals,
    GreaterEquals,
    EqualsEquals,
    ExclamationEquals,

    // Special operators with two tokens.
    OperatorIsNot,
    OperatorNotIn,

    // Literals
    IntLiteral,
    FloatLiteral,
    OctalIntLiteral,
    HexIntLiteral,
    BinaryIntLiteral,
    SingleQuoteStringLiteral,
    DoubleQuoteStringLiteral,

    // Keywords & Identifiers
    Identifier,

    KeywordFalse,
    KeywordNone,
    KeywordTrue,
    KeywordAnd,
    KeywordAs,
    KeywordAssert,
    KeywordAsync,
    KeywordAwait,
    KeywordBreak,
    KeywordClass,
    KeywordContinue,
    KeywordDef,
    KeywordDel,
    KeywordElif,
    KeywordElse,
    KeywordExcept,
    KeywordFor,
    KeywordFrom,
    KeywordGlobal,
    KeywordIf,
    KeywordImport,
    KeywordIn,
    KeywordIs,
    KeywordLambda,
    KeywordNonlocal,
    KeywordNot,
    KeywordOr,
    KeywordPass,
    KeywordRaise,
    KeywordReturn,
    KeywordTry,
    KeywordWhile,
    KeywordWith,
    KeywordYield,
}
