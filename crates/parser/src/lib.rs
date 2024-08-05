/*
    Implementation of the parser for the python subset.
*/

use ast::{nodes::ASTNodeType, ASTNode};
use context::{ContextStack, ParserContext};
use frontend_errors::report_frontend_error;
use lexer::{
    token::{Token, TokenKind},
    Lexer,
};
use src_manager::span::Span;

mod context;
mod expr;
mod stmt;

pub struct Parser<'a> {
    lexer: Lexer<'a>,
    tok: Token,
    la_2: Token,
    has_seen_parse_error: bool,
    abstract_syntax_tree: &'a mut Vec<ASTNode>,
    empty_node_list_index: usize,
    context_stack: ContextStack,
}

// This is the return type for most parsing results.
// On success, we will return the index of the created node, and on failure, we will return whether the error has been reported or not.
pub(crate) type ReturnType = Result<usize, bool>;

impl Parser<'_> {
    pub fn new<'a>(lexer: Lexer<'a>, abstract_syntax_tree: &'a mut Vec<ASTNode>) -> Parser<'a> {
        // Here, we will create a single empty node list that can be used throughout as there is no difference.
        abstract_syntax_tree.push(ASTNode::new(
            ASTNodeType::EmptyNodeList,
            Span::empty(),
            0,
            0,
        ));

        Parser {
            lexer,
            tok: Token::dummy(),
            la_2: Token::dummy(),
            has_seen_parse_error: false,
            abstract_syntax_tree,
            empty_node_list_index: 0,
            context_stack: ContextStack::new(),
        }
    }

    fn push_context(&mut self, ctx: ParserContext) {
        self.context_stack.push(ctx);
    }

    fn pop_context(&mut self) {
        self.context_stack.pop();
    }

    fn get_span(&self, index: usize) -> Span {
        self.abstract_syntax_tree[index].span
    }

    fn last_ast_index(&self) -> usize {
        self.abstract_syntax_tree.len() - 1
    }

    fn advance(&mut self) {
        if self.la_2.kind == TokenKind::Dummy {
            self.tok = self.lexer.next()
        } else {
            self.tok = self.la_2.clone();
            self.la_2.kind = TokenKind::Dummy
        }
    }

    fn get_second_lookahead(&mut self) {
        self.la_2 = self.lexer.next()
    }

    fn expect(&self, kind: TokenKind) -> bool {
        self.tok.kind == kind
    }

    fn report_parse_error(&mut self, msg: &str, loc: Span) {
        self.has_seen_parse_error = true;
        report_frontend_error(msg, loc, self.lexer.metadata)
    }

    pub fn parse_py_compilation_unit(&mut self) -> ReturnType {
        self.advance();
        self.parse_py_stmt()
    }
}
