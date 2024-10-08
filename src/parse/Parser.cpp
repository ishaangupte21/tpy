/*
    This file implements the parser for Python source code.
*/

#include "tpy/parse/Parser.h"
#include "tpy/compiler/FrontendErrorHandler.h"
#include "tpy/parse/Token.h"
#include "tpy/source/Span.h"
#include "tpy/tree/ASTExpr.h"
#include "tpy/tree/ASTNode.h"

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

auto Parser::parse_py_expr() -> ReturnType {
    return parse_py_assignment_expr();
}

// This method will parse atoms and primary expressions as they are defined in
// the Python spec. We are combining these two categories as they almost the
// same.
auto Parser::parse_py_atom_and_primary_expr() -> ReturnType {
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
        auto inner_expr = parse_py_paren_expr();
        // Since the parse_py_paren_expr method will report all errors, we don't
        // have to check for that.
        if (!inner_expr.first) {
            return inner_expr;
        }

        result = inner_expr.first;
        break;
    }
    case TokenKind::LeftSquare: {
        auto list_expr = parse_py_list_expr();
        // Since the parse_py_list_expr method will report all errors, we don't
        // have to check for that.
        if (!list_expr.first) {
            return list_expr;
        }

        result = list_expr.first;
        break;
    }
    case TokenKind::LeftCurly: {
        auto dict_or_set_expr = parse_py_set_or_dict_expr();
        // Since the parse_py_set_or_dict_expr method will report all errors, we
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

    // Now that we have parsed the initial expression, we need to parse the left
    // recursive portions of the primary expressions. Attribute references,
    // slicing, and calls are part of this class of expressions.
    switch (tok.kind) {
    case TokenKind::Dot: {
        return parse_py_attr_ref_expr(result);
    }
    case TokenKind::LeftParen: {
        return parse_py_call_expr(result);
    }

    case TokenKind::LeftSquare: {
        return parse_py_slice_expr(result);
    }

    default: {
        return std::make_pair(result, false);
    }
    }
}

// This method will parse expressions enclosed within parentheses.
auto Parser::parse_py_paren_expr() -> ReturnType {
    // Store the position of the left parenthesis and consume the parenthesis.
    auto lparen_loc = tok.span;
    advance();

    // Now, we must have an expression inside.
    auto expr_start = tok.span;
    auto expr = parse_py_expr();
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
auto Parser::parse_py_list_expr() -> ReturnType {
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
    auto first_expr = parse_py_expr();

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
        auto expr = parse_py_expr();

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
auto Parser::parse_py_set_or_dict_expr() -> ReturnType {
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
    auto first_expr = parse_py_expr();

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
        return parse_py_dict_expr(first_expr.first, lcurly_loc);
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
        auto expr = parse_py_expr();

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

/*
    This method will continue parsing dict expressions after they have been
   distinguished from set expressions.
*/
auto Parser::parse_py_dict_expr(Tree::ASTNode *first_expr,
                                Source::Span start) -> ReturnType {
    // We will begin parsing here on the colon.
    advance();

    // Now, we need another expression for the value.
    auto first_val_expr_start = tok.span;
    auto first_val_expr = parse_py_expr();

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
        auto key_expr = parse_py_expr();

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
        auto val_expr = parse_py_expr();

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

/*
    This method will parse attribute reference expressions of the form
   'foo.bar'. These are somewhat like binary expressions, but are parsed
   slightly differently.
*/
auto Parser::parse_py_attr_ref_expr(Tree::ASTNode *expr) -> ReturnType {
    // Following the dot, we need a name, which can then be followed by
    // repeating combinations of dots and names, to create 'foo.bar.baz'.
    do {
        advance();
        if (!expect(TokenKind::Identifier)) {
            report_error(tok.span,
                         "expected identifier for attribute name after"
                         "'.' in attribute reference.");

            return std::make_pair(nullptr, true);
        }

        // Now that we have the identifier, we can create the node and replace
        // the existing node with the new one.
        auto *name_expr = arena.allocate<Tree::ASTNameExprNode>(tok.span);
        expr = arena.allocate<Tree::ASTAttrRefExprNode>(expr, name_expr,
                                                        expr->loc + tok.span);

        // Now, we can consume the identifier.
        advance();
    } while (expect(TokenKind::Dot));

    // Now that we have reached the end, we can return the node.
    return std::make_pair(expr, false);
}

/*
    THis method will parse call expressions. Since the Python spec treats
   functions like any other kind of object, the callee does not have to be a
   name expression.
 */
auto Parser::parse_py_call_expr(Tree::ASTNode *callee) -> ReturnType {
    // We can consume the left parenthesis.
    advance();

    // Now, there are two options. If we get a right parenthesis, it means that
    // we have no arguments.
    if (expect(TokenKind::RightParen)) {
        // We can create the node with no arguments.
        auto *node = arena.allocate<Tree::ASTCallExprNode>(
            callee, callee->loc + tok.span);

        // Now, we can consume the right parenthesis and return.
        advance();
        return std::make_pair(node, false);
    }

    // Now, we must have an expression as an argument.
    auto first_arg_start = tok.span;
    auto first_arg = parse_py_expr();

    if (!first_arg.first) {
        if (!first_arg.second) {
            report_error(
                first_arg_start,
                "expected expression as argument after '(' in function call.");

            return std::make_pair(nullptr, true);
        }

        return first_arg;
    }

    std::vector<Tree::ASTNode *> args;
    args.push_back(first_arg.first);

    // Now, while we still get commas, we need to check for arguments.
    while (expect(TokenKind::Comma)) {
        advance();

        auto arg_start = tok.span;
        auto arg = parse_py_expr();

        if (!arg.first) {
            if (!arg.second) {
                report_error(arg_start, "expected expression as argument after "
                                        "',' in function call.");
                return std::make_pair(nullptr, true);
            }

            return arg;
        }

        // If we get the argument as a valid expression, we can just add it to
        // the list.
        args.push_back(arg.first);
    }

    // Now, we need a closing ')' at the end of the call expression.
    if (!expect(TokenKind::RightParen)) {
        report_error(tok.span, "expected ')' at the end of a function call.");
        return std::make_pair(nullptr, true);
    }

    // Once we have matched the whole expression, we can make the node.
    auto *node = arena.allocate<Tree::ASTCallExprNode>(callee, std::move(args),
                                                       callee->loc + tok.span);
    // Consume the ')'
    advance();

    return std::make_pair(node, false);
}

/*
    This method will parse slice expressions. Python has two main types of slice
   expressions - indexing and proper list slicing. Indexing is where a single
   index is sliced, returning the element at that index. In proper list slicing,
   a range is passed to the slicing operator and a new list is returned in those
   bounds.
*/
auto Parser::parse_py_slice_expr(Tree::ASTNode *slicee) -> ReturnType {
    // First, we must consume the square bracket that opens the slice
    // expression.
    advance();

    // We have two possible cases here. We can have either an expression, or a
    // colon.
    if (expect(TokenKind::Colon)) {
        // If we get a colon, we know that it is a proper slice expression.
        return parse_py_proper_slice_expr(slicee, nullptr);
    }

    // Otherwise, we must have an expression here.
    auto index_expr_start = tok.span;
    auto index_expr = parse_py_expr();

    if (!index_expr.first) {
        if (!index_expr.second) {
            report_error(
                index_expr_start,
                "expected expression after '[' in slicing expression.");

            return std::make_pair(nullptr, true);
        }

        return index_expr;
    }

    // After the index expression, we can have a colon or square bracket.
    if (expect(TokenKind::Colon)) {
        return parse_py_proper_slice_expr(slicee, index_expr.first);
    }

    // If we don't have a colon, we must have a square bracket.
    if (!expect(TokenKind::RightSquare)) {
        report_error(tok.span, "expected closing ']' after index expression in "
                               "slicing expression.");

        return std::make_pair(nullptr, true);
    }

    // Otherwise, we can make the node.
    auto *node = arena.allocate<Tree::ASTIndexSliceExprNode>(
        slicee, index_expr.first, slicee->loc + tok.span);

    advance();

    return std::make_pair(node, false);
}

auto Parser::parse_py_proper_slice_expr(
    Tree::ASTNode *slicee, Tree::ASTNode *lower_bound) -> ReturnType {
    // The first thing we must do here is consume the colon.
    advance();

    // Now, we may or may not get an expression.
    // We will first check for a square bracket. If we get one, it means we
    // don't need an expression for the lower bound.
    if (expect(TokenKind::RightSquare)) {
        auto *node = arena.allocate<Tree::ASTProperSliceExprNode>(
            slicee, lower_bound, nullptr, slicee->loc + tok.span);

        // Consume the right square bracket.
        advance();

        return std::make_pair(node, false);
    }

    // Otherwise, we must have an expression here.
    auto upper_bound_start = tok.span;
    auto upper_bound = parse_py_expr();

    if (!upper_bound.first) {
        if (!upper_bound.second) {
            report_error(upper_bound_start,
                         "expected expression as upper bound after ':' in "
                         "proper slicing expression.");

            return std::make_pair(nullptr, true);
        }

        return upper_bound;
    }

    // Now, we must have a right square bracket.
    if (!expect(TokenKind::RightSquare)) {
        report_error(tok.span, "expected closing ']' in slicing expression.");
        return std::make_pair(nullptr, true);
    }

    // Now, we can make the node and consume the ']'.
    auto *node = arena.allocate<Tree::ASTProperSliceExprNode>(
        slicee, lower_bound, upper_bound.first, slicee->loc + tok.span);

    advance();

    return std::make_pair(node, false);
}

auto Parser::parse_py_exponentiation_expr() -> ReturnType {
    // Since this is a binary expression, we need an LHS and RHS.
    // For this case, the LHS will be a primary expression.
    auto lhs = parse_py_atom_and_primary_expr();
    if (!lhs.first) {
        // In this case, we want to propagate the error up to the caller.
        return lhs;
    }

    auto *lhs_node = lhs.first;

    // Now, we need the operator.
    while (expect(TokenKind::AsteriskAsterisk)) {
        // Consume the operator.
        advance();

        // After the operator, we must have an RHS.
        auto rhs_start = tok.span;
        auto rhs = parse_py_unary_op_expr();

        if (!rhs.first) {
            if (!rhs.second) {
                report_error(rhs_start, "expected expression on right hand "
                                        "size of the binary operator '**'.");
                return std::make_pair(nullptr, true);
            }

            return rhs;
        }

        // Now, that we have the RHS, we can create the node.
        lhs_node = arena.allocate<Tree::ASTBinaryOpExprNode>(
            lhs_node, rhs.first, TokenKind::AsteriskAsterisk,
            lhs_node->loc + rhs.first->loc);
    }

    // At the end, we can simply return the node.
    return std::make_pair(lhs_node, false);
}

auto Parser::parse_py_unary_op_expr() -> ReturnType {
    // First, we need to check for a unary operator.
    // If we don't have one, we can keep parsing expressions like normal.
    switch (tok.kind) {
    case TokenKind::Plus:
    case TokenKind::Minus:
    case TokenKind::Tilda: {
        // If we do get a unary operator, we need another unary expression after
        // it.
        auto op = tok;
        advance();

        auto expr_start = tok.span;
        auto expr = parse_py_unary_op_expr();

        if (!expr.first) {
            if (!expr.second) {
                report_error(expr_start,
                             "expected expression after unary operator.");
                return std::make_pair(nullptr, true);
            }

            return expr;
        }

        // Now, we can return the node.
        auto *node = arena.allocate<Tree::ASTUnaryOpExprNode>(
            expr.first, op.kind, op.span + expr.first->loc);

        return std::make_pair(node, false);
    }
    default: {
        // If we don't find a unary operator, we can just treat this like a
        // regular exponentiation expression.
        return parse_py_exponentiation_expr();
    }
    }
}

auto Parser::parse_py_multiplication_expr() -> ReturnType {
    // First, we need to obtain an LHS.
    auto lhs_start = tok.span;
    auto lhs = parse_py_unary_op_expr();

    if (!lhs.first) {
        // Here, we just want to propagate the error upto the caller.
        return lhs;
    }

    auto lhs_node = lhs.first;

    // Now, while we have operators of this precedence - '*', '/', and '%', we
    // want to consume the operator and get an RHS.
    while (true) {
        switch (tok.kind) {
        case TokenKind::Asterisk:
        case TokenKind::Slash:
        case TokenKind::Percent: {
            auto op = tok.kind;
            advance();

            auto rhs_start = tok.span;
            auto rhs = parse_py_unary_op_expr();

            if (!rhs.first) {
                if (!rhs.second) {
                    report_error(rhs_start, "expected expression on the right "
                                            "hand side of a binary operator.");

                    return std::make_pair(nullptr, true);
                }

                return rhs;
            }

            // Now, we can make the node.
            lhs_node = arena.allocate<Tree::ASTBinaryOpExprNode>(
                lhs_node, rhs.first, op, lhs_node->loc + rhs.first->loc);

            break;
        }
        default: {
            // If we don't find the operator, we want to return the LHS that we
            // have.
            return std::make_pair(lhs_node, false);
        }
        }
    }
}

auto Parser::parse_py_addition_expr() -> ReturnType {
    // First, we need to obtain an LHS.
    auto lhs_start = tok.span;
    auto lhs = parse_py_multiplication_expr();

    if (!lhs.first) {
        // Here, we just want to propagate the error upto the caller.
        return lhs;
    }

    auto lhs_node = lhs.first;

    // Now, while we have operators of this precedence - '+' and '-', we
    // want to consume the operator and get an RHS.
    while (true) {
        switch (tok.kind) {
        case TokenKind::Plus:
        case TokenKind::Minus: {
            auto op = tok.kind;
            advance();

            auto rhs_start = tok.span;
            auto rhs = parse_py_multiplication_expr();

            if (!rhs.first) {
                if (!rhs.second) {
                    report_error(rhs_start, "expected expression on the right "
                                            "hand side of a binary operator.");

                    return std::make_pair(nullptr, true);
                }

                return rhs;
            }

            // Now, we can make the node.
            lhs_node = arena.allocate<Tree::ASTBinaryOpExprNode>(
                lhs_node, rhs.first, op, lhs_node->loc + rhs.first->loc);
            break;
        }
        default: {
            // If we don't find the operator, we want to return the LHS that we
            // have.
            return std::make_pair(lhs_node, false);
        }
        }
    }
}

auto Parser::parse_py_bitshift_expr() -> ReturnType {
    // First, we need to obtain an LHS.
    auto lhs_start = tok.span;
    auto lhs = parse_py_addition_expr();

    if (!lhs.first) {
        // Here, we just want to propagate the error upto the caller.
        return lhs;
    }

    auto lhs_node = lhs.first;

    // Now, while we have operators of this precedence - '<<' and '>>', we
    // want to consume the operator and get an RHS.
    while (true) {
        switch (tok.kind) {
        case TokenKind::GreaterGreater:
        case TokenKind::LessLess: {
            auto op = tok.kind;
            advance();

            auto rhs_start = tok.span;
            auto rhs = parse_py_addition_expr();

            if (!rhs.first) {
                if (!rhs.second) {
                    report_error(rhs_start, "expected expression on the right "
                                            "hand side of a binary operator.");

                    return std::make_pair(nullptr, true);
                }

                return rhs;
            }

            // Now, we can make the node.
            lhs_node = arena.allocate<Tree::ASTBinaryOpExprNode>(
                lhs_node, rhs.first, op, lhs_node->loc + rhs.first->loc);
            break;
        }
        default: {
            // If we don't find the operator, we want to return the LHS that we
            // have.
            return std::make_pair(lhs_node, false);
        }
        }
    }
}

auto Parser::parse_py_bitwise_and_expr() -> ReturnType {
    // First, we need to obtain an LHS.
    auto lhs_start = tok.span;
    auto lhs = parse_py_bitshift_expr();

    if (!lhs.first) {
        // Here, we just want to propagate the error upto the caller.
        return lhs;
    }

    auto lhs_node = lhs.first;

    // Now, while we have the '&' operator, we must keep getting an RHS.
    while (expect(TokenKind::Ampersand)) {
        advance();

        auto rhs_start = tok.span;
        auto rhs = parse_py_bitshift_expr();

        if (!rhs.first) {
            if (!rhs.second) {
                report_error(rhs_start, "expected expression on the right hand "
                                        "side of the binary operator '&'.");

                return std::make_pair(nullptr, true);
            }

            return rhs;
        }

        // Now, we can make the node.
        lhs_node = arena.allocate<Tree::ASTBinaryOpExprNode>(
            lhs_node, rhs.first, TokenKind::Ampersand,
            lhs_node->loc + rhs.first->loc);
    }

    return std::make_pair(lhs_node, false);
}

auto Parser::parse_py_bitwise_xor_expr() -> ReturnType {
    // First, we need to obtain an LHS.
    auto lhs_start = tok.span;
    auto lhs = parse_py_bitwise_and_expr();

    if (!lhs.first) {
        // Here, we just want to propagate the error upto the caller.
        return lhs;
    }

    auto lhs_node = lhs.first;

    // Now, while we have the '^' operator, we must keep getting an RHS.
    while (expect(TokenKind::Caret)) {
        advance();

        auto rhs_start = tok.span;
        auto rhs = parse_py_bitwise_and_expr();

        if (!rhs.first) {
            if (!rhs.second) {
                report_error(rhs_start, "expected expression on the right hand "
                                        "side of the binary operator '^'.");

                return std::make_pair(nullptr, true);
            }

            return rhs;
        }

        // Now, we can make the node.
        lhs_node = arena.allocate<Tree::ASTBinaryOpExprNode>(
            lhs_node, rhs.first, TokenKind::Caret,
            lhs_node->loc + rhs.first->loc);
    }

    return std::make_pair(lhs_node, false);
}

auto Parser::parse_py_bitwise_or_expr() -> ReturnType {
    // First, we need to obtain an LHS.
    auto lhs_start = tok.span;
    auto lhs = parse_py_bitwise_xor_expr();

    if (!lhs.first) {
        // Here, we just want to propagate the error upto the caller.
        return lhs;
    }

    auto lhs_node = lhs.first;

    // Now, while we have the '|' operator, we must keep getting an RHS.
    while (expect(TokenKind::Bar)) {
        advance();

        auto rhs_start = tok.span;
        auto rhs = parse_py_bitwise_xor_expr();

        if (!rhs.first) {
            if (!rhs.second) {
                report_error(rhs_start, "expected expression on the right hand "
                                        "side of the binary operator '|'.");

                return std::make_pair(nullptr, true);
            }

            return rhs;
        }

        // Now, we can make the node.
        lhs_node = arena.allocate<Tree::ASTBinaryOpExprNode>(
            lhs_node, rhs.first, TokenKind::Bar,
            lhs_node->loc + rhs.first->loc);
    }

    return std::make_pair(lhs_node, false);
}

auto Parser::parse_py_comparison_expr() -> ReturnType {
    // First, we need to obtain an LHS.
    auto lhs_start = tok.span;
    auto lhs = parse_py_bitwise_or_expr();

    if (!lhs.first) {
        // Here, we just want to propagate the error upto the caller.
        return lhs;
    }

    auto lhs_node = lhs.first;

    // Now, while we have operators of this precedence - '>', '<', '>=', '<=',
    // '!=', '==', 'is', 'is not', 'in', 'not in', and '%', we want to consume
    // the operator and get an RHS.
    while (true) {
        switch (tok.kind) {
        case TokenKind::Less:
        case TokenKind::LessEquals:
        case TokenKind::Greater:
        case TokenKind::GreaterEquals:
        case TokenKind::EqualsEquals:
        case TokenKind::ExclamationEquals: {
            auto op = tok.kind;
            advance();

            auto rhs_start = tok.span;
            auto rhs = parse_py_bitwise_or_expr();

            if (!rhs.first) {
                if (!rhs.second) {
                    report_error(rhs_start, "expected expression on the right "
                                            "hand side of a binary operator.");

                    return std::make_pair(nullptr, true);
                }

                return rhs;
            }

            // Now, we can make the node.
            lhs_node = arena.allocate<Tree::ASTBinaryOpExprNode>(
                lhs_node, rhs.first, op, lhs_node->loc + rhs.first->loc);
            break;
        }

        case TokenKind::KeywordIs: {
            // This branch allows for two possible operators: 'is' and 'is not'.
            auto op = TokenKind::KeywordIs;
            advance();

            // Now, we must check for 'is not'.
            if (expect(TokenKind::KeywordNot)) {
                op = TokenKind::IsNotOp;
                advance();
            }

            // Now, we just need an RHS.
            auto rhs_start = tok.span;
            auto rhs = parse_py_bitwise_or_expr();

            if (!rhs.first) {
                if (!rhs.second) {
                    report_error(rhs_start, "expected expression on the right "
                                            "hand side of a binary operator.");

                    return std::make_pair(nullptr, true);
                }

                return rhs;
            }

            // Now, we can make the node.
            lhs_node = arena.allocate<Tree::ASTBinaryOpExprNode>(
                lhs_node, rhs.first, op, lhs_node->loc + rhs.first->loc);
            break;
        }

        case TokenKind::KeywordIn: {
            advance();

            // Now, we just need an RHS.
            auto rhs_start = tok.span;
            auto rhs = parse_py_bitwise_or_expr();

            if (!rhs.first) {
                if (!rhs.second) {
                    report_error(rhs_start,
                                 "expected expression on the right "
                                 "hand side of the binary operator 'in'.");

                    return std::make_pair(nullptr, true);
                }

                return rhs;
            }

            // Now, we can make the node.
            lhs_node = arena.allocate<Tree::ASTBinaryOpExprNode>(
                lhs_node, rhs.first, TokenKind::KeywordIn,
                lhs_node->loc + rhs.first->loc);
            break;
        }

        case TokenKind::KeywordNot: {
            // This is a tricky case. This 'not' must be followed by an 'in' to
            // form the complete operator 'not in'. However, for error recovery
            // purposes, we will treat just 'not' as 'not in' as well.
            auto not_loc = tok.span;
            advance();

            if (!expect(TokenKind::KeywordIn)) {
                report_error(not_loc, "'not' is not a valid operator. Did you "
                                      "mean 'not in' instead?");
            } else {
                // If we do get the operator, we must consume it.
                advance();
            }

            // Now, we just need an RHS.
            auto rhs_start = tok.span;
            auto rhs = parse_py_bitwise_or_expr();

            if (!rhs.first) {
                if (!rhs.second) {
                    report_error(rhs_start, "expected expression on the right "
                                            "hand side of a binary operator.");

                    return std::make_pair(nullptr, true);
                }

                return rhs;
            }

            // Now, we can make the node.
            lhs_node = arena.allocate<Tree::ASTBinaryOpExprNode>(
                lhs_node, rhs.first, TokenKind::NotInOp,
                lhs_node->loc + rhs.first->loc);
            break;
        }

        default: {
            // If we don't find the operator, we want to return the LHS that we
            // have.
            return std::make_pair(lhs_node, false);
        }
        }
    }
}

auto Parser::parse_py_unary_not_expr() -> ReturnType {
    // Here we have two choices - we can either get a 'not' operator, or
    // continue down to a comparison expression.
    if (!expect(TokenKind::KeywordNot)) {
        return parse_py_comparison_expr();
    }

    // Now that we know we have a 'not' operator, we must store its location and
    // advance.
    auto not_loc = tok.span;
    advance();

    // Following the 'not' keyword, we must have an expression.
    auto expr_start = tok.span;
    auto expr = parse_py_unary_not_expr();

    if (!expr.first) {
        if (!expr.second) {
            report_error(not_loc,
                         "expected expression after the unary operator '!'.");

            return std::make_pair(nullptr, true);
        }

        return expr;
    }

    // Otherwise, we have a valid expression and we can make the node.
    auto *node = arena.allocate<Tree::ASTUnaryOpExprNode>(
        expr.first, TokenKind::KeywordNot, not_loc + expr.first->loc);

    return std::make_pair(node, false);
}

auto Parser::parse_py_logical_and_expr() -> ReturnType {
    // First, we need to obtain an LHS.
    auto lhs_start = tok.span;
    auto lhs = parse_py_unary_not_expr();

    if (!lhs.first) {
        // Here, we just want to propagate the error upto the caller.
        return lhs;
    }

    auto lhs_node = lhs.first;

    // Now, while we have the 'and' operator, we must keep getting an RHS.
    while (expect(TokenKind::KeywordAnd)) {
        advance();

        auto rhs_start = tok.span;
        auto rhs = parse_py_unary_not_expr();

        if (!rhs.first) {
            if (!rhs.second) {
                report_error(rhs_start, "expected expression on the right hand "
                                        "side of the binary operator 'and'.");

                return std::make_pair(nullptr, true);
            }

            return rhs;
        }

        // Now, we can make the node.
        lhs_node = arena.allocate<Tree::ASTBinaryOpExprNode>(
            lhs_node, rhs.first, TokenKind::KeywordAnd,
            lhs_node->loc + rhs.first->loc);
    }

    return std::make_pair(lhs_node, false);
}

auto Parser::parse_py_logical_or_expr() -> ReturnType {
    // First, we need to obtain an LHS.
    auto lhs_start = tok.span;
    auto lhs = parse_py_logical_and_expr();

    if (!lhs.first) {
        // Here, we just want to propagate the error upto the caller.
        return lhs;
    }

    auto lhs_node = lhs.first;

    // Now, while we have the 'or' operator, we must keep getting an RHS.
    while (expect(TokenKind::KeywordOr)) {
        advance();

        auto rhs_start = tok.span;
        auto rhs = parse_py_logical_and_expr();

        if (!rhs.first) {
            if (!rhs.second) {
                report_error(rhs_start, "expected expression on the right hand "
                                        "side of the binary operator 'or'.");

                return std::make_pair(nullptr, true);
            }

            return rhs;
        }

        // Now, we can make the node.
        lhs_node = arena.allocate<Tree::ASTBinaryOpExprNode>(
            lhs_node, rhs.first, TokenKind::KeywordOr,
            lhs_node->loc + rhs.first->loc);
    }

    return std::make_pair(lhs_node, false);
}

auto Parser::parse_py_ternary_op_expr() -> ReturnType {
    // First, we must get an expression.
    auto condition = parse_py_logical_or_expr();
    if (!condition.first) {
        // Here, we will just propagate errors up to the caller.
        return condition;
    }

    // After this, we must check for a possible if-else clause.
    if (!expect(TokenKind::KeywordIf)) {
        return condition;
    }

    // Consume the if keyword.
    advance();

    // Now, we need another expression for the true case.
    auto true_expr_start = tok.span;
    auto true_expr = parse_py_logical_or_expr();

    if (!true_expr.first) {
        if (!true_expr.second) {
            report_error(
                true_expr_start,
                "expected expression after 'if' in conditional expression.");

            return std::make_pair(nullptr, true);
        }

        return true_expr;
    }

    // Now, we need an 'else' clause.
    if (!expect(TokenKind::KeywordElse)) {
        report_error(tok.span, "expected 'else' clause after expression within "
                               "conditional expression.");

        return std::make_pair(nullptr, true);
    }

    // Consume 'else'
    advance();

    // Finally, we need an expression for the false case.
    auto false_expr_start = tok.span;
    auto false_expr = parse_py_expr();

    if (!false_expr.first) {
        if (!false_expr.second) {
            report_error(
                false_expr_start,
                "expected expression after 'false' in conditional expression.");

            return std::make_pair(nullptr, true);
        }

        return false_expr;
    }

    // Now, we can make the node and return it.
    auto *node = arena.allocate<Tree::ASTTernaryOpExprNode>(
        condition.first, true_expr.first, false_expr.first,
        condition.first->loc + false_expr.first->loc);

    return std::make_pair(node, false);
}

auto Parser::parse_py_assignment_expr() -> ReturnType {
    // This part gets a little bit tricky. In order to determine if we have an
    // assignment expression, we need two lookahead tokens. The first case is
    // simple - if we don't have an identifier, we can directly proceed to
    // parsing further down.
    if (!expect(TokenKind::Identifier)) {
        return parse_py_ternary_op_expr();
    }

    // Now that we now we have an identifier, we need to get the 2nd lookahead;
    lexer.lex_next_tok(tok_2);

    // The 2nd lookahead will help us determine if this is an assignment
    // expression. If it is the ':=' operator, it means that we have an
    // assignment expression.
    if (tok_2.kind == TokenKind::ColonEquals) {
        // Here, we need to first construct the node for the identifier we
        // found.
        auto *id_node = arena.allocate<Tree::ASTNameExprNode>(tok.span);

        // Now, we can consume both the identifier and the ':=' operator.
        advance();
        advance();

        // Now, we must have the RHS of the assignment expression.
        auto rhs_start = tok.span;
        auto rhs = parse_py_expr();

        if (!rhs.first) {
            if (!rhs.second) {
                report_error(rhs_start, "expected expression on the right hand "
                                        "side of the binary operator ':='.");

                return std::make_pair(nullptr, true);
            }

            return rhs;
        }

        // Now, we can maek the node.
        auto *node = arena.allocate<Tree::ASTBinaryOpExprNode>(
            id_node, rhs.first, TokenKind::ColonEquals,
            id_node->loc + rhs.first->loc);

        return std::make_pair(node, false);
    }

    // Otherwise, we can simply continue parsing like normal and the parser will
    // know to use the 2nd lookahead next.
    return parse_py_ternary_op_expr();
}

} // namespace tpy::Parse