/*
    Implementation of parsing for statements.
*/

use ast::{nodes::ASTNodeType, ASTNode};
use lexer::token::TokenKind;
use src_manager::span::Span;

use crate::{Parser, ReturnType};

impl Parser<'_> {
    /*
       Statement parsing
    */
    pub(crate) fn parse_py_stmt(&mut self) -> ReturnType {
        // First, we need to allow newlines between the statements.
        while self.expect(TokenKind::Newline) {
            self.advance();
        }

        // Now, we must begin parsing statements depending on the first token.
        let stmt = match self.tok.kind {
            TokenKind::KeywordPass => self.parse_py_pass_stmt(),
            TokenKind::KeywordReturn => self.parse_py_return_stmt(),
            TokenKind::KeywordDel => self.parse_py_del_stmt(),
            TokenKind::KeywordRaise => self.parse_py_raise_stmt(),
            TokenKind::KeywordBreak => self.parse_py_break_stmt(),
            TokenKind::KeywordContinue => self.parse_py_continue_stmt(),
            _ => todo!(),
        };

        stmt
    }

    fn parse_py_stmt_end(&mut self) -> bool {
        match self.tok.kind {
            TokenKind::Newline | TokenKind::Semicolon => {
                self.advance();
                true
            }
            TokenKind::End => true,
            _ => false,
        }
    }

    /*
       This method parses Python 'pass' statments.
    */
    fn parse_py_pass_stmt(&mut self) -> ReturnType {
        // There is nothing in this statement after the 'pass' keyword.
        let pass_kw_pos = self.tok.span;
        self.advance();

        // Now, we need to check for the end.
        if !self.parse_py_stmt_end() {
            self.report_parse_error("expected newline or ';' at statement end.", self.tok.span);
            return Err(true);
        }

        // Create the AST node now.
        self.abstract_syntax_tree
            .push(ASTNode::new(ASTNodeType::PassStmt, pass_kw_pos, 0, 0));

        Ok(self.last_ast_index())
    }

    /*
       This method parses Python 'return' statements.
    */
    fn parse_py_return_stmt(&mut self) -> ReturnType {
        let return_kw_span = self.tok.span;
        // Consume 'return'
        self.advance();

        // Now, we may have an expression.
        if self.is_py_expr_first() {
            // Now, we need an expression.
            let return_expr_start = self.tok.span;
            let return_expr = match self.parse_py_expr() {
                Ok(expr) => expr,
                Err(has_err_been_reported) => {
                    if !has_err_been_reported {
                        self.report_parse_error(
                            "expected expression after 'return'.",
                            return_expr_start,
                        );
                    }

                    return Err(true);
                }
            };

            // Now, we need the end of the statement.
            if !self.parse_py_stmt_end() {
                self.report_parse_error("expected newline or ';' at statement end.", self.tok.span);
                return Err(true);
            }

            self.abstract_syntax_tree.push(ASTNode::new(
                ASTNodeType::ReturnStmt,
                return_kw_span + self.get_span(return_expr),
                return_expr,
                0,
            ));

            Ok(self.last_ast_index())
        } else {
            // If we don't have an expression, we must return the return stmt node.
            if !self.parse_py_stmt_end() {
                self.report_parse_error("expected newline or ';' at statement end.", self.tok.span);
                return Err(true);
            }

            self.abstract_syntax_tree.push(ASTNode::new(
                ASTNodeType::ReturnStmt,
                return_kw_span,
                0,
                0,
            ));

            Ok(self.last_ast_index())
        }
    }

    fn parse_py_del_stmt_target(&mut self) -> ReturnType {
        // Since these can only be forms of a primary expression, we can skip the rest of expression parsing.
        let target_start = self.tok.span;
        let target = match self.parse_py_primary_expr() {
            Ok(expr) => expr,
            // This error will always have the value false.
            Err(has_err_been_reported) => {
                if !has_err_been_reported {
                    self.report_parse_error("expected name or slice to be deleted.", target_start);
                }
                return Err(true);
            }
        };

        // Now, we need to ensure that this target can actally be deleted.
        if !matches!(
            self.abstract_syntax_tree[target].node_type,
            ASTNodeType::AttrRefExpr | ASTNodeType::Identifier | ASTNodeType::SliceExpr
        ) {
            self.report_parse_error(
                "'del' operator can only be applied on names or slices.",
                self.get_span(target),
            );
            return Err(true);
        }

        Ok(target)
    }

    /*
       This method parses python 'del' statements.
    */
    fn parse_py_del_stmt(&mut self) -> ReturnType {
        // Mark the start of the statement.
        let del_kw_pos = self.tok.span;

        self.advance();

        // Now, we must have at least one 'target' to delete.
        // Since this method always returns true for errors, we can propagate the error up the call stack.
        let first_target = self.parse_py_del_stmt_target()?;
        let mut stmt_span = del_kw_pos + self.get_span(first_target);

        let mut targets = vec![first_target];

        // Now, while we have a comma, we need to get more targets.
        while self.expect(TokenKind::Comma) {
            // Consume the comma and expect another target.
            self.advance();

            let next_target = self.parse_py_del_stmt_target()?;
            stmt_span = stmt_span + self.get_span(next_target);

            targets.push(next_target);
        }

        // Now, we need the end.
        if !self.parse_py_stmt_end() {
            self.report_parse_error("expected newline or ';' at statement end.", self.tok.span);
            return Err(true);
        }

        // Create the target list node.
        self.abstract_syntax_tree.push(ASTNode::new(
            ASTNodeType::NodeList(targets),
            Span::empty(),
            0,
            0,
        ));

        // Create the statement node.
        self.abstract_syntax_tree.push(ASTNode::new(
            ASTNodeType::DelStmt,
            stmt_span,
            self.last_ast_index(),
            0,
        ));

        Ok(self.last_ast_index())
    }

    /*
       This method parses python 'raise' statments.
    */
    fn parse_py_raise_stmt(&mut self) -> ReturnType {
        let raise_stmt_start = self.tok.span;

        // Consume 'raise'
        self.advance();

        // Now, we need an expression for the exception.
        let exception_start = self.tok.span;
        let exception_expr = match self.parse_py_expr() {
            Ok(expr) => expr,
            Err(has_err_been_reported) => {
                if !has_err_been_reported {
                    self.report_parse_error(
                        "expected expression as exception after 'raise'.",
                        exception_start,
                    );
                }

                return Err(true);
            }
        };

        // Now, there may be a from component.
        if self.expect(TokenKind::KeywordFrom) {
            // Consume 'advance'.
            self.advance();

            // Now, we need another expression.
            let from_expr_start = self.tok.span;
            let from_expr = match self.parse_py_expr() {
                Ok(expr) => expr,
                Err(has_err_been_reported) => {
                    if !has_err_been_reported {
                        self.report_parse_error(
                            "expected expression after 'from'.",
                            from_expr_start,
                        );
                    }

                    return Err(true);
                }
            };

            // Check for the statement end and create the node.
            if !self.parse_py_stmt_end() {
                self.report_parse_error("expected newline or ';' at statement end.", self.tok.span);
                return Err(true);
            }

            self.abstract_syntax_tree.push(ASTNode::new(
                ASTNodeType::RaiseStmt,
                raise_stmt_start + self.get_span(from_expr),
                exception_expr,
                from_expr,
            ));

            Ok(self.last_ast_index())
        } else {
            // Otherwise, check for the statement and create the node.
            if !self.parse_py_stmt_end() {
                self.report_parse_error("expected newline or ';' at statement end.", self.tok.span);
                return Err(true);
            }

            self.abstract_syntax_tree.push(ASTNode::new(
                ASTNodeType::RaiseStmt,
                raise_stmt_start + self.get_span(exception_expr),
                exception_expr,
                0,
            ));

            Ok(self.last_ast_index())
        }
    }

    fn parse_py_break_stmt(&mut self) -> ReturnType {
        // Consume 'break'
        let break_kw_span = self.tok.span;
        self.advance();

        // Check for end and create the node.
        if !self.parse_py_stmt_end() {
            self.report_parse_error("expected newline or ';' at statement end.", self.tok.span);
            return Err(true);
        }

        self.abstract_syntax_tree
            .push(ASTNode::new(ASTNodeType::BreakStmt, break_kw_span, 0, 0));

        // Now, we need to check if this is semantically in the right place.
        self.check_break_stmt_in_valid_context(break_kw_span);

        Ok(self.last_ast_index())
    }

    fn parse_py_continue_stmt(&mut self) -> ReturnType {
        // Consume 'continue'
        let continue_kw_span = self.tok.span;
        self.advance();

        // Check for end and create the node.
        if !self.parse_py_stmt_end() {
            self.report_parse_error("expected newline or ';' at statement end.", self.tok.span);
            return Err(true);
        }

        self.abstract_syntax_tree.push(ASTNode::new(
            ASTNodeType::ContinueStmt,
            continue_kw_span,
            0,
            0,
        ));

        // Now, we need to check if this is semantically in the right place.
        self.check_continue_stmt_in_valid_context(continue_kw_span);

        Ok(self.last_ast_index())
    }
}
