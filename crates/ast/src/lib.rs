/*
   This is the implementation of the AST for this python subset.
   The implementation is based on the one described in Appendix B of Engineering a Compiler.
   We will have a binary tree stored in a vector, where nodes are accessed by indeces.
*/

use nodes::ASTNodeType;
use src_manager::span::Span;
use std::fmt::Debug;

pub mod nodes;

/*
    Each AST node will contain two indeces: left and right.
    The meaning of these indeces will be dependent on the type of node.
*/
#[derive(Debug)]
pub struct ASTNode {
    pub node_type: ASTNodeType,
    pub span: Span,
    pub left: usize,
    pub right: usize,
}

impl ASTNode {
    pub fn new(node_type: ASTNodeType, span: Span, left: usize, right: usize) -> Self {
        Self {
            node_type,
            span,
            left,
            right,
        }
    }
}
