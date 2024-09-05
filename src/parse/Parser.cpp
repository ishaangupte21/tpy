/*
    This file implements the parser for Python source code.
*/

#include "tpy/parse/Parser.h"
#include "tpy/compiler/FrontendErrorHandler.h"
#include "tpy/parse/Token.h"
#include "tpy/source/Span.h"
#include "tpy/tree/ASTExpr.h"

// A shorthand to avoid having to write the whole thing out all the time.
namespace tpy::Parse {
/*
    This method will use the FrontendErrorHandler infrastructure to report
   syntax errors within the source code.
*/
auto Parser::report_error(Source::Span &loc, const char *msg) -> void {
    Compiler::FrontendErrorHandler::report_error_with_local_pos(
        lexer.src_file, loc.local_pos, loc.len, msg);
}

auto Parser::parse_expr() -> ReturnType {
    return parse_atom_and_primary_expr();
}

// This method will parse atoms and primary expressions as they are defined in
// the Python spec. We are combining these two categories as they almost the
// same.
auto Parser::parse_atom_and_primary_expr() -> ReturnType {
    Tree::ASTNode *result;

    switch (tok.kind) {
    case TokenKind::IntLiteral: {
        result = arena.allocate<Tree::ASTIntLiteralNode>(10, tok.span);
        advance();
        break;
    }
    case TokenKind::HexIntLiteral: {
        result = arena.allocate<Tree::ASTIntLiteralNode>(16, tok.span);
        advance();
        break;
    }
    case TokenKind::BinaryIntLiteral: {
        result = arena.allocate<Tree::ASTIntLiteralNode>(2, tok.span);
        advance();
        break;
    }
    case TokenKind::OctalIntLiteral: {
        result = arena.allocate<Tree::ASTIntLiteralNode>(8, tok.span);
        advance();
        break;
    }
    case TokenKind::FloatLiteral: {
        result = arena.allocate<Tree::ASTFloatLiteralNode>(tok.span);
        advance();
        break;
    }
    case TokenKind::StringLiteral: {
        result = arena.allocate<Tree::ASTStringLiteralNode>(tok.span);
        advance();
        break;
    }
    case TokenKind::KeywordTrue: {
        result = arena.allocate<Tree::ASTBoolLiteralNode>(true, tok.span);
        advance();
        break;
    }
    case TokenKind::KeywordFalse: {
        result = arena.allocate<Tree::ASTBoolLiteralNode>(false, tok.span);
        advance();
        break;
    }
    case TokenKind::Identifier: {
        result = arena.allocate<Tree::ASTNameExprNode>(tok.span);
        advance();
        break;
    }
    case TokenKind::LeftParen: {
        auto inner_expr = parse_paren_expr();
        // Since the parse_paren_expr method will report all errors, we don't
        // have to check for that.
        if (!inner_expr.first) {
            return inner_expr;
        }

        result = inner_expr.first;
        break;
    }
    case TokenKind::LeftSquare: {
        auto list_expr = parse_list_expr();
        // Since the parse_list_expr method will report all errors, we don't
        // have to check for that.
        if (!list_expr.first) {
            return list_expr;
        }

        result = list_expr.first;
        break;
    }
    case TokenKind::LeftCurly: {
        auto dict_or_set_expr = parse_set_or_dict_expr();
        // Since the parse_set_or_dict_expr method will report all errors, we
        // don't have to check for that.
        if (!dict_or_set_expr.first) {
            return dict_or_set_expr;
        }

        result = dict_or_set_expr.first;
        break;
    }
    case TokenKind::ErrorToken: {
        // This is a special case, because we know the user intended a literal
        // of some kind, but the exact kind is unclear.
        // Therefore, we will return an integer literal with the span of the
        // token.
        result = arena.allocate<Tree::ASTIntLiteralNode>(10, tok.span);
        advance();
        break;
    }
    // Finally, when we get a token that absolutely cannot be a literal, we
    // will return nullptr, but let the caller report the error to provide a
    // better message.
    default: {
        return std::make_pair(nullptr, false);
    }
    }

    // Once we have
    return std::make_pair(result, false);
}

// This method will parse expressions enclosed within parentheses.
auto Parser::parse_paren_expr() -> ReturnType {
    // Store the position of the left parenthesis and consume the parenthesis.
    auto lparen_loc = tok.span;
    advance();

    // Now, we must have an expression inside.
    auto expr_start = tok.span;
    auto expr = parse_expr();
    if (!expr.first) {
        if (!expr.second) {
            report_error(expr_start, "expected expression after '('.");
            return std::make_pair(nullptr, true);
        }

        return expr;
    }

    // Now, we need a closing ')'
    if (!expect(TokenKind::RightParen)) {
        report_error(tok.span, "expected closing ')' after expression.");
        return std::make_pair(nullptr, true);
    }

    // Create the node, and then consume the ')'.
    auto *node = arena.allocate<Tree::ASTParenExprNode>(expr.first,
                                                        lparen_loc + tok.span);
    advance();

    return std::make_pair(node, false);
}

// This method will parse list expressions. List expressions begin with a left
// square bracket and contain a list of expressions. They must end with a right
// square bracket.
auto Parser::parse_list_expr() -> ReturnType {
    // Mark the position of the left square bracket and consume it.
    // Additionally, newline characters are allowed within list literals, so we
    // must tell the lexer not to mark them.
    auto lsquare_loc = tok.span;
    lexer.skip_newlines();
    advance();

    // Now, we have a special case where there is an empty list literal.
    if (expect(TokenKind::RightSquare)) {
        auto *node =
            arena.allocate<Tree::ASTListExprNode>(lsquare_loc + tok.span);
        // Tell the lexer to start allowing newlines again.
        lexer.allow_newlines();

        // Consume the ']'
        advance();

        return std::make_pair(node, false);
    }

    // If we get here, it means the list is not empty. Therefore, we must have
    // an expression.
    auto first_expr_start = tok.span;
    auto first_expr = parse_expr();

    if (!first_expr.first) {
        if (!first_expr.second) {
            report_error(first_expr_start,
                         "expected expression after '[' in list literal.");
            return std::make_pair(nullptr, true);
        }

        return first_expr;
    }

    // Create the list of expressions and add the first expression.
    std::vector<Tree::ASTNode *> list;
    list.push_back(first_expr.first);

    // Now, while we have comments, we must have expressions.
    while (expect(TokenKind::Comma)) {
        // Consume the comma.
        advance();

        // The python spec allows trailing commas, so we must check for that.
        if (expect(TokenKind::RightSquare)) {
            // If we find the bracket, we can make the node and consume it.
            auto *node = arena.allocate<Tree::ASTListExprNode>(
                std::move(list), lsquare_loc + tok.span);
            // We need to tell the lexer to start marking newlines again.
            lexer.allow_newlines();
            advance();

            return std::make_pair(node, false);
        }

        // Now, we need an expression.
        auto expr_start = tok.span;
        auto expr = parse_expr();

        if (!expr.first) {
            if (!expr.second) {
                report_error(expr_start,
                             "expected expression after ',' in list literal.");
                return std::make_pair(nullptr, true);
            }

            return expr;
        }

        // Add the expression to the list.
        list.push_back(expr.first);
    }

    // Now, we need a right square bracket to end the list.
    if (!expect(TokenKind::RightSquare)) {
        report_error(tok.span, "expected closing ']' in list literal.");
        return std::make_pair(nullptr, true);
    }

    // If we find the bracket, we can make the node and consume it.
    auto *node = arena.allocate<Tree::ASTListExprNode>(std::move(list),
                                                       lsquare_loc + tok.span);
    // We need to tell the lexer to start marking newlines again.
    lexer.allow_newlines();
    advance();

    return std::make_pair(node, false);
}

// This method will parse set and dict expressions. We will begin by expecting a
// set, but if a colon is encountered after the first expression, we will
// transition to a dict.
auto Parser::parse_set_or_dict_expr() -> ReturnType {
    // Store the location of the opening curly brace and then consume it.
    // Additionally, newline characters are allowed within list literals, so we
    // must tell the lexer not to mark them.
    auto lcurly_loc = tok.span;
    lexer.skip_newlines();
    advance();

    // We need to handle the special case where we have an empty dict.
    if (expect(TokenKind::RightCurly)) {
        // Create the node and consume the right curly brace.
        auto *node =
            arena.allocate<Tree::ASTDictExprNode>(lcurly_loc + tok.span);

        // Tell the lexer to start allowing newlines again.
        lexer.allow_newlines();
        advance();

        return std::make_pair(node, false);
    }

    // Now, we must have an expression.
    auto first_expr_start = tok.span;
    auto first_expr = parse_expr();

    if (!first_expr.first) {
        if (!first_expr.second) {
            report_error(first_expr_start,
                         "expected expression after '{' in set literal.");
            return std::make_pair(nullptr, true);
        }

        return first_expr;
    }

    // Now, if we have a colon, it becomes a dict. Otherwise, it remains a set.
    if (expect(TokenKind::Colon)) {
        return parse_dict_expr(first_expr.first, lcurly_loc);
    }

    // Now that we know that we have a set, we can keep consuming expressions
    // while we have a comma.
    std::vector<Tree::ASTNode *> contents;
    contents.push_back(first_expr.first);

    while (expect(TokenKind::Comma)) {
        // Consume the comma and expect an expression.
        advance();

        // However, the Python spec allows trailing commas.
        if (expect(TokenKind::RightCurly)) {
            // Make the node and consume the right curly brace.
            auto *node = arena.allocate<Tree::ASTSetExprNode>(
                std::move(contents), lcurly_loc + tok.span);

            // Tell the lexer to start allowing newlines again.
            lexer.allow_newlines();
            advance();

            return std::make_pair(node, false);
        }

        auto expr_start = tok.span;
        auto expr = parse_expr();

        if (!expr.first) {
            if (!expr.second) {
                report_error(expr_start,
                             "expected expression after ',' in set literal.");
                return std::make_pair(nullptr, true);
            }

            return expr;
        }

        contents.push_back(expr.first);
    }

    // Now, we need a right curly brace to close the set literal.
    if (!expect(TokenKind::RightCurly)) {
        report_error(tok.span, "expected closing '}' in set literal.");
        return std::make_pair(nullptr, true);
    }

    // Make the node and consume the right curly brace.
    auto *node = arena.allocate<Tree::ASTSetExprNode>(std::move(contents),
                                                      lcurly_loc + tok.span);

    // Tell the lexer to start allowing newlines again.
    lexer.allow_newlines();
    advance();

    return std::make_pair(node, false);
}

auto Parser::parse_dict_expr(Tree::ASTNode *first_expr,
                             Source::Span start) -> ReturnType {
    // We will begin parsing here on the colon.
    advance();

    // Now, we need another expression for the value.
    auto first_val_expr_start = tok.span;
    auto first_val_expr = parse_expr();

    if (!first_val_expr.first) {
        if (!first_val_expr.second) {
            report_error(
                first_val_expr_start,
                "expected expression as value after ':' in dict literal");

            return std::make_pair(nullptr, true);
        }

        return first_val_expr;
    }

    // Create the list of key value pairs and add the first one.
    std::vector<std::pair<Tree::ASTNode *, Tree::ASTNode *>> contents;
    contents.emplace_back(first_expr, first_val_expr.first);

    // Now, while we have commas, we must keep having key value pairs.
    while (expect(TokenKind::Comma)) {
        // Consume the comma.
        advance();

        // The python spec allows trailing commas, so we must check for that.
        if (expect(TokenKind::RightCurly)) {
            // Create the node, then consume the curly brace.
            auto *node = arena.allocate<Tree::ASTDictExprNode>(
                std::move(contents), start + tok.span);
            // Tell the lexer to start allowing newlines again.
            lexer.allow_newlines();
            advance();

            return std::make_pair(node, false);
        }

        // Now, we need a key value pair.
        auto key_expr_start = tok.span;
        auto key_expr = parse_expr();

        if (!key_expr.first) {
            if (!key_expr.second) {
                report_error(key_expr_start,
                             "expected expression as key for key-value pair "
                             "after ',' in dict literal.");
                return std::make_pair(nullptr, true);
            }

            return key_expr;
        }

        // Now, we need a colon.
        if (!expect(TokenKind::Colon)) {
            report_error(tok.span, "expected ':' between key and value within "
                                   "key-value pair in dict literal.");
            return std::make_pair(nullptr, true);
        }

        // Consume the colon.
        advance();

        // Now, we must have an expression as the value.
        auto val_expr_start = tok.span;
        auto val_expr = parse_expr();

        if (!val_expr.first) {
            if (!val_expr.second) {
                report_error(val_expr_start,
                             "expected expression as value for key-value pair "
                             "after ':' in dict literal.");
                return std::make_pair(nullptr, true);
            }

            return val_expr;
        }

        // Now, we can add the key-value pair to the list.
        contents.emplace_back(key_expr.first, val_expr.first);
    }

    // At the end, we must have a closing curly brace.
    if (!expect(TokenKind::RightCurly)) {
        report_error(tok.span, "expected closing '}' in dict literal.");
    }

    // Create the node, then consume the curly brace.
    auto *node = arena.allocate<Tree::ASTDictExprNode>(std::move(contents),
                                                       start + tok.span);
    // Tell the lexer to start allowing newlines again.
    lexer.allow_newlines();
    advance();

    return std::make_pair(node, false);
}

} // namespace tpy::Parse