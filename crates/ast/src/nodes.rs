/*
   Implementation of the individual AST Node types.
*/
use std::fmt::Debug;

use lexer::token::TokenKind;

#[derive(Debug)]
pub enum ASTNodeType {
    // This type is needed to store lists of nodes
    NodeList(Vec<usize>),
    // Empty List node in order to avoid creating a needless vector.
    EmptyNodeList,

    // These are the AST Node types.
    IntLiteral,
    FloatLiteral,
    HexIntLiteral,
    OctalIntLiteral,
    BinaryIntLiteral,
    SingleQuoteStrLiteral,
    DoubleQuoteStrLiteral,
    ListLiteral,
    DictLiteral,
    Identifier,
    ParenExpr,
    AttrRefExpr,
    CallExpr,
    DictExpr,
    DictKeyValuePair,

    // The binary expression needs an operator, which is a TokenKind taken directly from the token.
    BinaryExpr(TokenKind),

    // The prefix operator expression also needs an operator.
    PrefixOpExpr(TokenKind),

    // Assignment expressions are a special kind of binary expression.
    AssignmentExpr,

    // Inside part of the conditional expression, carrying the If and Else cases.
    IfElseOptions,

    IfElseExpr,

    // Slice Expressions - these can be complex due to ranges.
    SliceExpr,
    SliceExprRange,

    /*
       Statements
    */
    PassStmt,
    ReturnStmt,
    DelStmt,
    RaiseStmt,
}
