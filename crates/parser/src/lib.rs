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
            Span::empty(),
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

    fn get_span(&self, index: usize) -> Span {
        self.abstract_syntax_tree[index].span
    }

    fn last_ast_index(&self) -> usize {
        self.abstract_syntax_tree.len() - 1
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

    fn parse_py_expr(&mut self) -> ReturnType {
        self.parse_py_conditional_expr()
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
                        self.report_parse_error("expected expression after ','.", next_expr_start);
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

        Ok(self.last_ast_index())
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

            TokenKind::LeftSquare => match self.parse_py_list_expr() {
                Ok(expr) => expr,
                Err(_) => return Err(true),
            },

            TokenKind::LeftCurly => match self.parse_py_dict_expr() {
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

        self.last_ast_index()
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
                    self.report_parse_error("expected expression after '('.", inner_expr_start);
                }
                return Err(true);
            }
        };

        // Now, we need to have a right parenthesis to close.
        if !self.expect(TokenKind::RightParen) {
            self.report_parse_error("expected closing ')'.", self.tok.span);
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

        Ok(self.last_ast_index())
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
                "expected identifier after '.' in attribute reference expression.",
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
        let initial_lhs = self.last_ast_index();
        let mut lhs = initial_lhs;

        // We must keep parsing attribute ref expressions while dots exist.
        while self.expect(TokenKind::Dot) {
            // Consume the dot.
            self.advance();

            // Now, we need an identifier.
            if !self.expect(TokenKind::Identifier) {
                self.report_parse_error(
                    "expected identifier after '.' in attribute reference expression.",
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
            lhs = self.last_ast_index();
        }

        Ok(initial_lhs)
    }

    /*
       This method parses python call expressions.
    */
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

            return Ok(self.last_ast_index());
        }

        // Otherwise, we must have call arguments in the form of an expression list.
        let first_arg_start = self.tok.span;

        let call_expr_args = match self.parse_py_expr_list() {
            Ok(args) => args,
            Err(has_error_been_reporting) => {
                if !has_error_been_reporting {
                    self.report_parse_error(
                        "expected expression as argument after '(' in call expression.",
                        first_arg_start,
                    );
                }

                return Err(true);
            }
        };

        // Now, we need to have a closing right parenthesis.
        if !self.expect(TokenKind::RightParen) {
            self.report_parse_error("expected closing ')' in calling expression.", self.tok.span);
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

        Ok(self.last_ast_index())
    }

    /*
       This method parses python list literals.
       It is a list of expressions enclosed by square brackets.
    */
    fn parse_py_list_expr(&mut self) -> ReturnType {
        // First, we must mark the start of the square bracket and consume it.
        let lsquare_start = self.tok.span;
        // Before we advance, we must tell the lexer to stop scanning newlines as the Python spec allows curly braces to be spread over lines.
        self.lexer.skip_newlines();

        self.advance();

        // First, we will check for the special case in which we have an empty list.
        if self.expect(TokenKind::RightSquare) {
            // Add the node to the AST.
            self.abstract_syntax_tree.push(ASTNode::new(
                ASTNodeType::ListLiteral,
                lsquare_start + self.tok.span,
                self.empty_node_list_index,
                0,
            ));

            // Consume the ']'
            self.advance();

            return Ok(self.last_ast_index());
        }

        // Otherwise, we need an expression list.
        let expr_list_start = self.tok.span;

        let expr_list = match self.parse_py_expr_list() {
            Ok(list) => list,
            Err(has_err_been_reported) => {
                if !has_err_been_reported {
                    self.report_parse_error(
                        "expected expression after '[' in list.",
                        expr_list_start,
                    );
                }

                return Err(true);
            }
        };

        // Now, we need a right square bracket.
        if !self.expect(TokenKind::RightSquare) {
            self.report_parse_error("expected ']' at the end of a list literal.", self.tok.span);
            return Err(true);
        }

        // Consume the ']'
        let rsquare_span = self.tok.span;

        // We also need to tell the lexer to resume scanning newlines.
        self.lexer.accept_newlines();
        self.advance();

        // Insert the node.
        self.abstract_syntax_tree.push(ASTNode::new(
            ASTNodeType::ListLiteral,
            lsquare_start + rsquare_span,
            expr_list,
            0,
        ));

        Ok(self.last_ast_index())
    }

    /*
       This method parses python dictionary literals.
       It is essentialy a list of key value pairs enclosed by two curly braces.
    */
    fn parse_py_dict_expr(&mut self) -> ReturnType {
        // First, we need to mark and consume the left curly brace.
        let lcurly_start = self.tok.span;

        // Before we advance, we must tell the lexer to stop scanning newlines as the Python spec allows curly braces to be spread over lines.
        self.lexer.skip_newlines();

        self.advance();

        // Special case for empty dictionary.
        if self.expect(TokenKind::RightCurly) {
            // Create the node.
            self.abstract_syntax_tree.push(ASTNode::new(
                ASTNodeType::DictExpr,
                lcurly_start + self.tok.span,
                self.empty_node_list_index,
                0,
            ));

            // Consume the right curly brace.
            self.advance();

            return Ok(self.last_ast_index());
        }

        // Now, we must have a key value pair.
        let first_key_value_pair = match self.parse_py_dict_key_val_pair() {
            Ok(pair) => pair,
            Err(_) => return Err(true),
        };

        let mut key_value_pairs = vec![first_key_value_pair];

        // Now, while we have comments, we need to keep parsing key/value pairs.
        while self.expect(TokenKind::Comma) {
            // Consume the comma.
            self.advance();

            // If the pair is successfully parsed, add it to the vector.
            match self.parse_py_dict_key_val_pair() {
                Ok(pair) => key_value_pairs.push(pair),
                Err(_) => return Err(true),
            };
        }

        // Now, we need a closing right parenthesis.
        if !self.expect(TokenKind::RightCurly) {
            self.report_parse_error("expected closing '}' in dictionary.", self.tok.span);
            return Err(true);
        }

        // Create the node for the key/value pair list.
        self.abstract_syntax_tree.push(ASTNode::new(
            ASTNodeType::NodeList(key_value_pairs),
            Span::empty(),
            0,
            0,
        ));

        // Now, we must create the node for the actual dictionary.
        self.abstract_syntax_tree.push(ASTNode::new(
            ASTNodeType::DictExpr,
            lcurly_start + self.tok.span,
            // Position of the list.
            self.last_ast_index(),
            0,
        ));

        // Consume the right curly and tell the lexer to resume scanning newlines.
        self.lexer.accept_newlines();
        self.advance();

        Ok(self.last_ast_index())
    }

    /*
       This function parses the key/value pairs inside a dict literal.
       <expr> : <expr>
    */
    fn parse_py_dict_key_val_pair(&mut self) -> ReturnType {
        let key_expr_start = self.tok.span;
        let key_expr = match self.parse_py_expr() {
            Ok(expr) => expr,
            Err(has_err_been_reported) => {
                if !has_err_been_reported {
                    self.report_parse_error(
                        "expected expression as key in dictionary.",
                        key_expr_start,
                    );
                }

                return Err(true);
            }
        };

        // Now, we need a colon.
        if !self.expect(TokenKind::Colon) {
            self.report_parse_error(
                "expected ':' between key and value expressions in dictionary.",
                self.tok.span,
            );
            return Err(true);
        }

        // Consume the colon.
        self.advance();

        // Now, we need the value expression.
        let val_expr_start = self.tok.span;
        let val_expr = match self.parse_py_expr() {
            Ok(expr) => expr,
            Err(has_err_been_reported) => {
                if !has_err_been_reported {
                    self.report_parse_error(
                        "expected expression as value after ':' in dictionary.",
                        val_expr_start,
                    );
                }

                return Err(true);
            }
        };

        // Now, we can create the node and add it to the tree.
        self.abstract_syntax_tree.push(ASTNode::new(
            ASTNodeType::DictKeyValuePair,
            Span::empty(),
            key_expr,
            val_expr,
        ));

        Ok(self.last_ast_index())
    }

    fn parse_py_exponentiation_expr(&mut self) -> ReturnType {
        // We don't need to report LHS errors at this level.
        let lhs = self.parse_py_primary_expr()?;

        // Because this is a right associative operator, it cannot be handled with a loop and must be handled recursively.
        if self.expect(TokenKind::AsteriskAsterisk) {
            // Consume the operator.
            self.advance();

            // We need the right hand side now.
            let rhs_start = self.tok.span;
            let rhs = match self.parse_py_exponentiation_expr() {
                Ok(expr) => expr,
                Err(has_err_been_reported) => {
                    if !has_err_been_reported {
                        self.report_parse_error(
                            "expected expression after '**', which is a binary operator.",
                            rhs_start,
                        );
                    }

                    return Err(true);
                }
            };

            // Create the node here.
            self.abstract_syntax_tree.push(ASTNode::new(
                ASTNodeType::BinaryExpr(TokenKind::AsteriskAsterisk),
                self.get_span(lhs) + self.get_span(rhs),
                lhs,
                rhs,
            ));

            Ok(self.last_ast_index())
        } else {
            Ok(lhs)
        }
    }

    fn parse_py_prefix_op_expr(&mut self) -> ReturnType {
        // First, we need to check for a prefix operator.
        // If there is one, we need to handle it and its following expression.
        if matches!(
            self.tok.kind,
            TokenKind::Plus | TokenKind::Minus | TokenKind::Tilde
        ) {
            let op = self.tok.kind;
            let op_pos = self.tok.span;

            // Consume the operator.
            self.advance();

            // Now, we need an expression.
            let expr_start = self.tok.span;
            let expr = match self.parse_py_exponentiation_expr() {
                Ok(expr) => expr,
                Err(has_err_been_reported) => {
                    if !has_err_been_reported {
                        self.report_parse_error(
                            "expected expression after prefix operator.",
                            expr_start,
                        );
                    }

                    return Err(true);
                }
            };

            // Now, create the node.
            self.abstract_syntax_tree.push(ASTNode::new(
                ASTNodeType::PrefixOpExpr(op),
                op_pos + self.get_span(expr),
                expr,
                0,
            ));

            Ok(self.last_ast_index())
        } else {
            // If not, we must parse the exponentiation expression and return its result.
            self.parse_py_exponentiation_expr()
        }
    }

    fn parse_py_multiplication_expr(&mut self) -> ReturnType {
        let mut lhs = self.parse_py_prefix_op_expr()?;

        // Now, while we have operators, we need to keep parsing RHS expressions.
        while matches!(
            self.tok.kind,
            TokenKind::Asterisk | TokenKind::Slash | TokenKind::SlashSlash | TokenKind::Percent
        ) {
            let op = self.tok.kind;
            // Consume the operator.
            self.advance();

            // Now, we need a RHS
            let rhs_start = self.tok.span;
            let rhs = match self.parse_py_prefix_op_expr() {
                Ok(expr) => expr,
                Err(has_err_been_reported) => {
                    if !has_err_been_reported {
                        self.report_parse_error(
                            "expected expression after binary operator.",
                            rhs_start,
                        );
                    }

                    return Err(true);
                }
            };

            // Now, construct an AST node.
            self.abstract_syntax_tree.push(ASTNode::new(
                ASTNodeType::BinaryExpr(op),
                self.get_span(lhs) + rhs_start,
                lhs,
                rhs,
            ));

            lhs = self.last_ast_index();
        }

        Ok(lhs)
    }

    fn parse_py_addition_expr(&mut self) -> ReturnType {
        let mut lhs = self.parse_py_multiplication_expr()?;

        // Parse the RHS of expressions while we have operators.
        while matches!(self.tok.kind, TokenKind::Plus | TokenKind::Minus) {
            let op = self.tok.kind;
            self.advance();

            let rhs_start = self.tok.span;
            let rhs = match self.parse_py_multiplication_expr() {
                Ok(expr) => expr,
                Err(has_err_been_reported) => {
                    if !has_err_been_reported {
                        self.report_parse_error(
                            "expected expression after binary operator.",
                            rhs_start,
                        );
                    }

                    return Err(true);
                }
            };

            // Now, construct an AST node.
            self.abstract_syntax_tree.push(ASTNode::new(
                ASTNodeType::BinaryExpr(op),
                self.get_span(lhs) + rhs_start,
                lhs,
                rhs,
            ));

            lhs = self.last_ast_index();
        }

        Ok(lhs)
    }

    fn parse_py_bitshift_expr(&mut self) -> ReturnType {
        let mut lhs = self.parse_py_addition_expr()?;

        // Parse the RHS of expressions while we have operators.
        while matches!(
            self.tok.kind,
            TokenKind::LessLess | TokenKind::GreaterGreater
        ) {
            let op = self.tok.kind;
            self.advance();

            let rhs_start = self.tok.span;
            let rhs = match self.parse_py_addition_expr() {
                Ok(expr) => expr,
                Err(has_err_been_reported) => {
                    if !has_err_been_reported {
                        self.report_parse_error(
                            "expected expression after binary operator.",
                            rhs_start,
                        );
                    }

                    return Err(true);
                }
            };

            // Now, construct an AST node.
            self.abstract_syntax_tree.push(ASTNode::new(
                ASTNodeType::BinaryExpr(op),
                self.get_span(lhs) + rhs_start,
                lhs,
                rhs,
            ));

            lhs = self.last_ast_index();
        }

        Ok(lhs)
    }

    fn parse_py_bitwise_and_expr(&mut self) -> ReturnType {
        let mut lhs = self.parse_py_bitshift_expr()?;

        // Parse the RHS of expressions while we have operators.
        while self.expect(TokenKind::Ampersand) {
            self.advance();

            let rhs_start = self.tok.span;
            let rhs = match self.parse_py_bitshift_expr() {
                Ok(expr) => expr,
                Err(has_err_been_reported) => {
                    if !has_err_been_reported {
                        self.report_parse_error(
                            "expected expression after binary operator.",
                            rhs_start,
                        );
                    }

                    return Err(true);
                }
            };

            // Now, construct an AST node.
            self.abstract_syntax_tree.push(ASTNode::new(
                ASTNodeType::BinaryExpr(TokenKind::Ampersand),
                self.get_span(lhs) + rhs_start,
                lhs,
                rhs,
            ));

            lhs = self.last_ast_index();
        }

        Ok(lhs)
    }

    fn parse_py_bitwise_xor_expr(&mut self) -> ReturnType {
        let mut lhs = self.parse_py_bitwise_and_expr()?;

        // Parse the RHS of expressions while we have operators.
        while self.expect(TokenKind::Caret) {
            self.advance();

            let rhs_start = self.tok.span;
            let rhs = match self.parse_py_bitwise_and_expr() {
                Ok(expr) => expr,
                Err(has_err_been_reported) => {
                    if !has_err_been_reported {
                        self.report_parse_error(
                            "expected expression after binary operator.",
                            rhs_start,
                        );
                    }

                    return Err(true);
                }
            };

            // Now, construct an AST node.
            self.abstract_syntax_tree.push(ASTNode::new(
                ASTNodeType::BinaryExpr(TokenKind::Caret),
                self.get_span(lhs) + rhs_start,
                lhs,
                rhs,
            ));

            lhs = self.last_ast_index();
        }

        Ok(lhs)
    }

    fn parse_py_bitwise_or_expr(&mut self) -> ReturnType {
        let mut lhs = self.parse_py_bitwise_xor_expr()?;

        // Parse the RHS of expressions while we have operators.
        while self.expect(TokenKind::Bar) {
            self.advance();

            let rhs_start = self.tok.span;
            let rhs = match self.parse_py_bitwise_xor_expr() {
                Ok(expr) => expr,
                Err(has_err_been_reported) => {
                    if !has_err_been_reported {
                        self.report_parse_error(
                            "expected expression after binary operator.",
                            rhs_start,
                        );
                    }

                    return Err(true);
                }
            };

            // Now, construct an AST node.
            self.abstract_syntax_tree.push(ASTNode::new(
                ASTNodeType::BinaryExpr(TokenKind::Bar),
                self.get_span(lhs) + rhs_start,
                lhs,
                rhs,
            ));

            lhs = self.last_ast_index();
        }

        Ok(lhs)
    }

    fn parse_py_comparison_expr(&mut self) -> ReturnType {
        // Get the LHS.
        let mut lhs = self.parse_py_bitwise_or_expr()?;

        // Keep Searching for the Operators.
        loop {
            let op = match self.tok.kind {
                TokenKind::Less => {
                    self.advance();
                    TokenKind::Less
                }
                TokenKind::LessEquals => {
                    self.advance();
                    TokenKind::LessEquals
                }
                TokenKind::Greater => {
                    self.advance();
                    TokenKind::Greater
                }
                TokenKind::ExclamationEquals => {
                    self.advance();
                    TokenKind::ExclamationEquals
                }
                TokenKind::EqualsEquals => {
                    self.advance();
                    TokenKind::EqualsEquals
                }
                TokenKind::KeywordIn => {
                    self.advance();
                    TokenKind::KeywordIn
                }
                TokenKind::KeywordIs => {
                    self.advance();
                    if self.expect(TokenKind::KeywordNot) {
                        self.advance();
                        TokenKind::OperatorIsNot
                    } else {
                        TokenKind::KeywordIs
                    }
                }
                TokenKind::KeywordNot => {
                    // This must be the 'not in' operator
                    let not_op_pos = self.tok.span;
                    self.advance();

                    if !self.expect(TokenKind::KeywordIn) {
                        self.report_parse_error("operator 'not' is a unary operator and cannot be used in a binary expression.", not_op_pos);
                    } else {
                        self.advance();
                    }

                    // For Parsing purposes, pretend as if the 'not in' operator was complete if it isn't.
                    TokenKind::OperatorNotIn
                }

                // For all other tokens, break out of the loop and return the expression.
                _ => break Ok(lhs),
            };

            let rhs_start = self.tok.span;
            let rhs = match self.parse_py_bitwise_or_expr() {
                Ok(expr) => expr,
                Err(has_err_been_reported) => {
                    if !has_err_been_reported {
                        self.report_parse_error(
                            "expected expression after binary operator.",
                            rhs_start,
                        );
                    }

                    return Err(true);
                }
            };

            // Now, construct an AST node.
            self.abstract_syntax_tree.push(ASTNode::new(
                ASTNodeType::BinaryExpr(op),
                self.get_span(lhs) + rhs_start,
                lhs,
                rhs,
            ));

            lhs = self.last_ast_index();
        }
    }

    fn parse_py_prefix_not_expr(&mut self) -> ReturnType {
        // First, we need to check for a prefix operator.
        // If there is one, we need to handle it and its following expression.
        if self.expect(TokenKind::KeywordNot) {
            let op_pos = self.tok.span;

            // Consume the operator.
            self.advance();

            // Now, we need an expression.
            let expr_start = self.tok.span;
            let expr = match self.parse_py_comparison_expr() {
                Ok(expr) => expr,
                Err(has_err_been_reported) => {
                    if !has_err_been_reported {
                        self.report_parse_error(
                            "expected expression after prefix operator.",
                            expr_start,
                        );
                    }

                    return Err(true);
                }
            };

            // Now, create the node.
            self.abstract_syntax_tree.push(ASTNode::new(
                ASTNodeType::PrefixOpExpr(TokenKind::KeywordNot),
                op_pos + self.get_span(expr),
                expr,
                0,
            ));

            Ok(self.last_ast_index())
        } else {
            // If not, we must parse the exponentiation expression and return its result.
            self.parse_py_comparison_expr()
        }
    }

    fn parse_py_logical_and_expr(&mut self) -> ReturnType {
        let mut lhs = self.parse_py_prefix_not_expr()?;

        // Parse the RHS of expressions while we have operators.
        while self.expect(TokenKind::KeywordAnd) {
            self.advance();

            let rhs_start = self.tok.span;
            let rhs = match self.parse_py_prefix_not_expr() {
                Ok(expr) => expr,
                Err(has_err_been_reported) => {
                    if !has_err_been_reported {
                        self.report_parse_error(
                            "expected expression after binary operator.",
                            rhs_start,
                        );
                    }

                    return Err(true);
                }
            };

            // Now, construct an AST node.
            self.abstract_syntax_tree.push(ASTNode::new(
                ASTNodeType::BinaryExpr(TokenKind::Bar),
                self.get_span(lhs) + rhs_start,
                lhs,
                rhs,
            ));

            lhs = self.last_ast_index();
        }

        Ok(lhs)
    }

    fn parse_py_logical_or_expr(&mut self) -> ReturnType {
        let mut lhs = self.parse_py_logical_and_expr()?;

        // Parse the RHS of expressions while we have operators.
        while self.expect(TokenKind::KeywordOr) {
            self.advance();

            let rhs_start = self.tok.span;
            let rhs = match self.parse_py_logical_or_expr() {
                Ok(expr) => expr,
                Err(has_err_been_reported) => {
                    if !has_err_been_reported {
                        self.report_parse_error(
                            "expected expression after binary operator.",
                            rhs_start,
                        );
                    }

                    return Err(true);
                }
            };

            // Now, construct an AST node.
            self.abstract_syntax_tree.push(ASTNode::new(
                ASTNodeType::BinaryExpr(TokenKind::Bar),
                self.get_span(lhs) + rhs_start,
                lhs,
                rhs,
            ));

            lhs = self.last_ast_index();
        }

        Ok(lhs)
    }

    fn parse_py_conditional_expr(&mut self) -> ReturnType {
        // We first need an expression.
        let result_expr = self.parse_py_logical_or_expr()?;

        // Now, if we have 'if' we can start parsing the conditional expression.
        if self.expect(TokenKind::KeywordIf) {
            // Consume 'if'
            let if_kw_pos = self.tok.span;
            self.advance();

            // Now, we need an expression for the condition.
            let condition_expr_start = self.tok.span;
            let condition_expr = match self.parse_py_logical_or_expr() {
                Ok(expr) => expr,
                Err(has_err_been_reported) => {
                    if !has_err_been_reported {
                        self.report_parse_error(
                            "expected expression after 'if' in conditional expression.",
                            condition_expr_start,
                        );
                    }

                    return Err(true);
                }
            };

            // Now, we need the else keyword.
            if !self.expect(TokenKind::KeywordElse) {
                self.report_parse_error(
                    "expected 'else' after expression in conditional expression.",
                    self.tok.span,
                );
                return Err(true);
            }

            // Consume 'else'
            self.advance();

            // Now, we need another expression for the else case.
            let else_expr_start = self.tok.span;
            let else_expr = match self.parse_py_expr() {
                Ok(expr) => expr,
                Err(has_err_been_reported) => {
                    if !has_err_been_reported {
                        self.report_parse_error(
                            "expected expression after 'else' in conditional expression.",
                            else_expr_start,
                        );
                    }

                    return Err(true);
                }
            };

            // Now, we need to construct the inside of the AST node.
            self.abstract_syntax_tree.push(ASTNode::new(
                ASTNodeType::IfElseOptions,
                Span::empty(),
                condition_expr,
                else_expr,
            ));

            // Now, we can construct the node for the full If-Else expression.
            self.abstract_syntax_tree.push(ASTNode::new(
                ASTNodeType::IfElseExpr,
                if_kw_pos + self.get_span(else_expr),
                result_expr,
                self.last_ast_index(),
            ));

            Ok(self.last_ast_index())
        } else {
            // Otherwise, just return what we previously had.
            Ok(result_expr)
        }
    }
}
