/*
    This file implements a perfect hash table for identifying keywords.
*/

use phf::phf_map;

use crate::token::TokenKind;

// We will use the phf crate to generate a perfect hash function and map.
pub(crate) static KEYWORDS: phf::Map<&'static str, TokenKind> = phf_map! {
    "False" => TokenKind::KeywordFalse,
    "None" => TokenKind::KeywordNone,
    "True" => TokenKind::KeywordTrue,
    "and" => TokenKind::KeywordAnd,
    "as" => TokenKind::KeywordAs,
    "assert" => TokenKind::KeywordAssert,
    "async" => TokenKind::KeywordAsync,
    "await" => TokenKind::KeywordAwait,
    "break" => TokenKind::KeywordBreak,
    "class" => TokenKind::KeywordClass,
    "continue" => TokenKind::KeywordContinue,
    "def" => TokenKind::KeywordDef,
    "del" => TokenKind::KeywordDel,
    "elif" => TokenKind::KeywordElif,
    "else" => TokenKind::KeywordElse,
    "except" => TokenKind::KeywordExcept,
    "for" => TokenKind::KeywordFor,
    "from" => TokenKind::KeywordFrom,
    "global" => TokenKind::KeywordGlobal,
    "if" => TokenKind::KeywordIf,
    "import" => TokenKind::KeywordImport,
    "in" => TokenKind::KeywordIn,
    "is" => TokenKind::KeywordIs,
    "lambda" => TokenKind::KeywordLambda,
    "nonlocal" => TokenKind::KeywordNonlocal,
    "not" => TokenKind::KeywordNot,
    "or" => TokenKind::KeywordOr,
    "pass" => TokenKind::KeywordPass,
    "raise" => TokenKind::KeywordRaise,
    "return" => TokenKind::KeywordReturn,
    "try" => TokenKind::KeywordTry,
    "while" => TokenKind::KeywordWhile,
    "with" => TokenKind::KeywordWith,
    "yield" => TokenKind::KeywordYield,
};
