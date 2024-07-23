/*
    Implementation of the parser for the python subset.
*/

use ast::{nodes::ASTNodeType, ASTNode};
use frontend_errors::report_frontend_error;
use lexer::{
    token::{Token, TokenKind},
    Lexer,
};
use src_manager::span::Span;

pub struct Parser<'a> {
    lexer: Lexer<'a>,
    tok: Token,
    has_seen_parse_error: bool,
    abstract_syntax_tree: &'a mut Vec<ASTNode>,
    empty_node_list_index: usize,
}

// This is the return type for most parsing results.
// On success, we will return the index of the created node, and on failure, we will return whether the error has been reported or not.
type ReturnType = Result<usize, bool>;

impl Parser<'_> {
    pub fn new<'a>(lexer: Lexer<'a>, abstract_syntax_tree: &'a mut Vec<ASTNode>) -> Parser<'a> {
        // Here, we will create a single empty node list that can be used throughout as there is no difference.
        abstract_syntax_tree.push(ASTNode::new(
            ASTNodeType::EmptyNodeList,
            Span::new(0, 0),
            0,
            0,
        ));

        Parser {
            lexer,
            tok: Token::dummy(),
            has_seen_parse_error: false,
            abstract_syntax_tree,
            empty_node_list_index: 0,
        }
    }

    fn advance(&mut self) {
        self.tok = self.lexer.next();
    }

    fn expect(&self, kind: TokenKind) -> bool {
        self.tok.kind == kind
    }

    fn report_parse_error(&mut self, msg: &str, loc: Span) {
        self.has_seen_parse_error = true;
        report_frontend_error(msg, loc, &self.lexer.metadata)
    }

    pub fn parse_py_compilation_unit(&mut self) -> ReturnType {
        self.advance();
        self.parse_py_expr()
    }

    fn parse_py_expr_list(&mut self) -> ReturnType {
        // We need a single expression first.
        let first_expr = self.parse_py_expr()?;

        // Now, we need to check for following expressions.
        // Each time there is a comma in an expression list, we need an expression to follow.
        let mut expr_list = vec![first_expr];

        while self.expect(TokenKind::Comma) {
            // Consume the comma.
            self.advance();

            // Now, we need an expression.
            let next_expr_start = self.tok.span;
            let next_expr = match self.parse_py_expr() {
                Ok(expr) => expr,
                Err(has_error_been_handled) => {
                    if !has_error_been_handled {
                        self.report_parse_error("expected expression after ','", next_expr_start);
                    }

                    return Err(true);
                }
            };

            expr_list.push(next_expr);
        }

        // Now that there are no expressions left, we can create the node and return.
        self.abstract_syntax_tree.push(ASTNode::new(
            ASTNodeType::NodeList(expr_list),
            Span::new(0, 0),
            0,
            0,
        ));

        Ok(self.abstract_syntax_tree.len() - 1)
    }

    fn parse_py_expr(&mut self) -> ReturnType {
        self.parse_py_primary_expr()
    }

    /*
       This method parses python atoms and their postfix operators, essentially resulting in a primary expression.
    */
    fn parse_py_primary_expr(&mut self) -> ReturnType {
        let expr_start = self.tok.span;

        let expr = match self.tok.kind {
            TokenKind::IntLiteral => {
                self.parse_py_literal_expr_or_identifier(ASTNodeType::IntLiteral)
            }
            TokenKind::FloatLiteral => {
                self.parse_py_literal_expr_or_identifier(ASTNodeType::FloatLiteral)
            }
            TokenKind::HexIntLiteral => {
                self.parse_py_literal_expr_or_identifier(ASTNodeType::HexIntLiteral)
            }
            TokenKind::BinaryIntLiteral => {
                self.parse_py_literal_expr_or_identifier(ASTNodeType::BinaryIntLiteral)
            }
            TokenKind::OctalIntLiteral => {
                self.parse_py_literal_expr_or_identifier(ASTNodeType::OctalIntLiteral)
            }
            TokenKind::SingleQuoteStringLiteral => {
                self.parse_py_literal_expr_or_identifier(ASTNodeType::SingleQuoteStrLiteral)
            }
            TokenKind::DoubleQuoteStringLiteral => {
                self.parse_py_literal_expr_or_identifier(ASTNodeType::DoubleQuoteStrLiteral)
            }
            TokenKind::Identifier => {
                self.parse_py_literal_expr_or_identifier(ASTNodeType::Identifier)
            }

            TokenKind::LeftParen => match self.parse_py_paren_expr() {
                Ok(expr) => expr,
                Err(_) => return Err(true),
            },

            TokenKind::Unknown => {
                // This is a special case because the lexer was unable to figure out what literal this is.
                // This only occurs for malformed integer literals of non-decimal base.
                // For now, we can just keep it as an integer literal as it will not be parsed anyways.
                self.parse_py_literal_expr_or_identifier(ASTNodeType::IntLiteral)
            }

            _ => return Err(false),
        };

        // Now, we need to check for postfix-style operations: slicing, call expressions, and attribute references.
        let postfix_expr = match self.tok.kind {
            TokenKind::Dot => match self.parse_py_attr_ref_expr(expr, expr_start) {
                Ok(expr) => expr,
                Err(_) => return Err(false),
            },
            TokenKind::LeftParen => match self.parse_py_call_expr(expr, expr_start) {
                Ok(expr) => expr,
                Err(_) => return Err(false),
            },

            // For all other tokens, we do not need to parse anything further.
            _ => expr,
        };

        Ok(postfix_expr)
    }

    /*
       This method parses all python literals and identifiers
    */
    fn parse_py_literal_expr_or_identifier(&mut self, literal_type: ASTNodeType) -> usize {
        // Insert the AST node.
        self.abstract_syntax_tree
            .push(ASTNode::new(literal_type, self.tok.span, 0, 0));

        self.advance();

        self.abstract_syntax_tree.len() - 1
    }

    /*
       This method parses epxressions wrapped in parentheses.
       They are returned as primary expressions to allow precendence.
    */
    fn parse_py_paren_expr(&mut self) -> ReturnType {
        // Mark the start postion and consume the opening left parenthesis.
        let lparen_span = self.tok.span;
        self.advance();

        // Now, we need to have an expression inside.
        let inner_expr_start = self.tok.span;

        let inner_expr = match self.parse_py_expr() {
            Ok(expr) => expr,
            Err(has_been_reported) => {
                if !has_been_reported {
                    self.report_parse_error("expected expression after '('", inner_expr_start);
                }
                return Err(true);
            }
        };

        // Now, we need to have a right parenthesis to close.
        if !self.expect(TokenKind::RightParen) {
            self.report_parse_error("expected closing ')'", self.tok.span);
            return Err(true);
        }

        // Consume the right parenthesis.
        let rparen_span = self.tok.span;
        self.advance();

        // Create the new node and add it to the list.
        self.abstract_syntax_tree.push(ASTNode::new(
            ASTNodeType::ParenExpr,
            lparen_span + rparen_span,
            inner_expr,
            0,
        ));

        Ok(self.abstract_syntax_tree.len() - 1)
    }

    /*
       This method parses attribute reference expressions of the form `foo.bar`
    */
    fn parse_py_attr_ref_expr(&mut self, initial_lhs: usize, start: Span) -> ReturnType {
        // We need to consume the dot and then check for a following identifier.
        self.advance();

        // Now, we must have an identifier.
        if !self.expect(TokenKind::Identifier) {
            self.report_parse_error(
                "expected identifier after '.' in attribute reference expression",
                self.tok.span,
            );
            return Err(true);
        }

        let first_identifier_span = self.tok.span;
        let first_identifier_expr =
            self.parse_py_literal_expr_or_identifier(ASTNodeType::Identifier);

        // First, we must insert the new attribute ref expression.
        self.abstract_syntax_tree.push(ASTNode::new(
            ASTNodeType::AttrRefExpr,
            start + first_identifier_span,
            initial_lhs,
            first_identifier_expr,
        ));

        // Now, that newly created node is the LHS for the following expressions, if they exist.
        // However, we need to maintain the index to the first LHS expression as they are left associative.
        let initial_lhs = self.abstract_syntax_tree.len() - 1;
        let mut lhs = initial_lhs;

        // We must keep parsing attribute ref expressions while dots exist.
        while self.expect(TokenKind::Dot) {
            // Consume the dot.
            self.advance();

            // Now, we need an identifier.
            if !self.expect(TokenKind::Identifier) {
                self.report_parse_error(
                    "expected identifier after '.' in attribute reference expression",
                    self.tok.span,
                );
                return Err(true);
            }

            let identifier_span = self.tok.span;
            let identifier_expr = self.parse_py_literal_expr_or_identifier(ASTNodeType::Identifier);

            // Now, we can create the new node and add it.
            self.abstract_syntax_tree.push(ASTNode::new(
                ASTNodeType::AttrRefExpr,
                start + identifier_span,
                lhs,
                identifier_expr,
            ));

            // Set the new LHS to the newly created expression.
            lhs = self.abstract_syntax_tree.len() - 1;
        }

        Ok(initial_lhs)
    }

    fn parse_py_call_expr(&mut self, callee_expr: usize, start: Span) -> ReturnType {
        // Consume the left parenthesis.
        self.advance();

        // Check for a special case of no arguments.
        if self.expect(TokenKind::RightParen) {
            // We must create a node for the arguments.
            // Since we have no arguments, we can use the EmptyNodeList type.
            self.abstract_syntax_tree.push(ASTNode::new(
                ASTNodeType::CallExpr,
                start + self.tok.span,
                callee_expr,
                self.empty_node_list_index,
            ));

            return Ok(self.abstract_syntax_tree.len() - 1);
        }

        // Otherwise, we must have call arguments in the form of an expression list.
        let first_arg_start = self.tok.span;

        let call_expr_args = match self.parse_py_expr_list() {
            Ok(args) => args,
            Err(has_error_been_reporting) => {
                if !has_error_been_reporting {
                    self.report_parse_error(
                        "expected expression as argument after '(' in call expression",
                        first_arg_start,
                    );
                }

                return Err(true);
            }
        };

        // Now, we need to have a closing right parenthesis.
        if !self.expect(TokenKind::RightParen) {
            self.report_parse_error("expected closing ')' in calling expression", self.tok.span);
            return Err(true);
        }

        let rparen_span = self.tok.span;
        self.advance();

        // Add the node to the list.
        self.abstract_syntax_tree.push(ASTNode::new(
            ASTNodeType::CallExpr,
            start + rparen_span,
            call_expr_args,
            0,
        ));

        Ok(self.abstract_syntax_tree.len() - 1)
    }
}
