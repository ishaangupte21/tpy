/*
   Implementation of the individual AST Node types.
*/
use crate::ASTNode;
use std::fmt::Debug;

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
    Identifier,
    ParenExpr,
    AttrRefExpr,
    CallExpr,
}
