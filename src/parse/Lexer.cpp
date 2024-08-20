/*
    This file implements the Lexical Analyzer that will scan tokens from the
   Python source files.
*/

#include "tpy/parse/Lexer.h"
#include "tpy/compiler/FrontendErrorHandler.h"

namespace tpy::Parse {
auto Lexer::report_error(char *start, size_t len, const char *msg) -> void {
    // We need to convert the start pointer into a local position and pass it on
    // to the Error Handler.
    Compiler::FrontendErrorHandler::report_error_with_local_pos(
        src_file, start - abs_buffer_start, len, msg);
}

/*
    This method provides the main lexer routine. This is where the scanning of
   source tokens originates.
*/
auto Lexer::lex_next_tok(Token &tok) -> void {
lexer_start:
    // First, we need to mark the start of a potential token.
    char *tok_start = ptr;

    // Next, we need to consume horizontal whitespace and then check for an
    // indent/dedent token. We will only do that if the previous token was a
    // newline token.
    int whitespace_count = consume_horizontal_whitespace();

    if (was_last_tok_newline) {
        // If the current whitespace count is greater than what is at the top of
        // the stack, we must add an indent token. If it is less, we must add a
        // dedent token.
        if (whitespace_count > whitespace_stack.top()) {
            was_last_tok_newline = false;
            whitespace_stack.push(whitespace_count);

            create_token(tok, TokenKind::Indent, tok_start, whitespace_count);
            return;
        }

        if (whitespace_count < whitespace_stack.top()) {
            was_last_tok_newline = false;
            whitespace_stack.pop();

            create_token(tok, TokenKind::Dedent, tok_start, whitespace_count);
            return;
        }

        // Otherwise, we can just fall through and keep scanning to generate the
        // next token.
    }

    // Since we know we have consumed whitespace, we must update the token start
    // position.
    tok_start = ptr;

    switch (*ptr) {
    case '\0': {
        // Here, we need to check if this is really the end of the file. If it
        // is, we can generate an end token. Otherwise, we must consume the null
        // character and restart the lexer.
        if (ptr == end_ptr) {
            create_token(tok, TokenKind::End, tok_start, 1);
            return;
        }

        ++ptr;
        goto lexer_start;
    }

    // Now, we must handle newline characters. If the parser is accepting
    // newline tokens, then we must return one. Otherwise, we just consume it
    // and keep going.
    case '\n': {
        ++ptr;
        if (accept_newlines) {
            create_token(tok, TokenKind::Newline, tok_start, 1, true);
            return;
        }

        goto lexer_start;
    }
    // Python allows for the CRLF return token, so we need to check for that.
    case '\r': {
        if (ptr[1] == '\n') {
            ptr += 2;
            if (accept_newlines) {
                create_token(tok, TokenKind::Newline, tok_start, 2, true);
                return;
            }

            goto lexer_start;
        }

        ++ptr;
        if (accept_newlines) {
            create_token(tok, TokenKind::Newline, tok_start, 1, true);
            return;
        }

        goto lexer_start;
    }

    // Now, we will begin with delimiters.
    case ';': {
        ++ptr;
        create_token(tok, TokenKind::Semicolon, tok_start, 1);
        return;
    }
    case '(': {
        ++ptr;
        create_token(tok, TokenKind::LeftParen, tok_start, 1);
        return;
    }
    case ')': {
        ++ptr;
        create_token(tok, TokenKind::RightParen, tok_start, 1);
        return;
    }
    case '[': {
        ++ptr;
        create_token(tok, TokenKind::LeftSquare, tok_start, 1);
        return;
    }
    case ']': {
        ++ptr;
        create_token(tok, TokenKind::RightSquare, tok_start, 1);
        return;
    }
    case '{': {
        ++ptr;
        create_token(tok, TokenKind::LeftCurly, tok_start, 1);
        return;
    }
    case '}': {
        ++ptr;
        create_token(tok, TokenKind::RightCurly, tok_start, 1);
        return;
    }
    case ',': {
        ++ptr;
        create_token(tok, TokenKind::Comma, tok_start, 1);
    }
    case '~': {
        ++ptr;
        create_token(tok, TokenKind::Tilda, tok_start, 1);
    }
    case '+': {
        if (ptr[1] == '=') {
            ptr += 2;
            create_token(tok, TokenKind::PlusEquals, tok_start, 2);
            return;
        }

        ++ptr;
        create_token(tok, TokenKind::Plus, tok_start, 1);
        return;
    }
    case '-': {
        if (ptr[1] == '=') {
            ptr += 2;
            create_token(tok, TokenKind::MinusEquals, tok_start, 2);
            return;
        }

        ++ptr;
        create_token(tok, TokenKind::Minus, tok_start, 1);
        return;
    }
    case '*': {
        if (ptr[1] == '=') {
            ptr += 2;
            create_token(tok, TokenKind::Asterisk, tok_start, 2);
            return;
        }
        if (ptr[1] == '*') {
            if (ptr[2] == '=') {
                ptr += 3;
                create_token(tok, TokenKind::AsteriskAsteriskEquals, tok_start,
                             3);
                return;
            }

            ptr += 2;
            create_token(tok, TokenKind::AsteriskAsterisk, tok_start, 2);
            return;
        }

        ++ptr;
        create_token(tok, TokenKind::Asterisk, tok_start, 1);
        return;
    }
    case '/': {
        if (ptr[1] == '=') {
            ptr += 2;
            create_token(tok, TokenKind::SlashEquals, tok_start, 2);
            return;
        }
        if (ptr[1] == '/') {
            if (ptr[2] == '=') {
                ptr += 3;
                create_token(tok, TokenKind::SlashSlashEquals, tok_start, 3);
                return;
            }

            ptr += 2;
            create_token(tok, TokenKind::SlashSlash, tok_start, 2);
            return;
        }

        ++ptr;
        create_token(tok, TokenKind::Slash, tok_start, 1);
        return;
    }
    case '%': {
        if (ptr[1] == '=') {
            ptr += 2;
            create_token(tok, TokenKind::PercentEquals, tok_start, 2);
            return;
        }

        ++ptr;
        create_token(tok, TokenKind::Percent, tok_start, 1);
        return;
    }
    case '&': {
        if (ptr[1] == '=') {
            ptr += 2;
            create_token(tok, TokenKind::AmpersandEquals, tok_start, 2);
            return;
        }

        ++ptr;
        create_token(tok, TokenKind::Ampersand, tok_start, 1);
        return;
    }
    case '^': {
        if (ptr[1] == '=') {
            ptr += 2;
            create_token(tok, TokenKind::CaretEquals, tok_start, 2);
            return;
        }

        ++ptr;
        create_token(tok, TokenKind::Caret, tok_start, 1);
        return;
    }
    case '|': {
        if (ptr[1] == '=') {
            ptr += 2;
            create_token(tok, TokenKind::BarEquals, tok_start, 2);
            return;
        }

        ++ptr;
        create_token(tok, TokenKind::Bar, tok_start, 1);
        return;
    }
    case '=': {
        if (ptr[1] == '=') {
            ptr += 2;
            create_token(tok, TokenKind::EqualsEquals, tok_start, 2);
            return;
        }

        ++ptr;
        create_token(tok, TokenKind::Equals, tok_start, 1);
        return;
    }
    case ':': {
        if (ptr[1] == '=') {
            ptr += 2;
            create_token(tok, TokenKind::ColonEquals, tok_start, 2);
            return;
        }

        ++ptr;
        create_token(tok, TokenKind::Colon, tok_start, 1);
        return;
    }
    case '!': {
        if (ptr[1] == '=') {
            ptr += 2;
            create_token(tok, TokenKind::ExclamationEquals, tok_start, 2);
            return;
        }

        // In most other languages, '!' is the logical not operator. Therefore,
        // it's highly likely that the user intended to use 'Not'.
        report_error(tok_start, 1,
                     "invalid operator '!'. Did you mean 'Not' instead?");

        ++ptr;
        create_token(tok, TokenKind::ErrorToken, tok_start, 1);
        return;
    }
    case '<': {
        if (ptr[1] == '=') {
            ptr += 2;
            create_token(tok, TokenKind::LessEquals, tok_start, 2);
            return;
        }
        if (ptr[1] == '<') {
            if (ptr[2] == '=') {
                ptr += 3;
                create_token(tok, TokenKind::LessLessEquals, tok_start, 3);
                return;
            }

            ptr += 2;
            create_token(tok, TokenKind::LessLess, tok_start, 2);
            return;
        }

        ++ptr;
        create_token(tok, TokenKind::Less, tok_start, 1);
        return;
    }
    case '>': {
        if (ptr[1] == '=') {
            ptr += 2;
            create_token(tok, TokenKind::GreaterEquals, tok_start, 2);
            return;
        }
        if (ptr[1] == '>') {
            if (ptr[2] == '=') {
                ptr += 3;
                create_token(tok, TokenKind::GreaterGreaterEquals, tok_start,
                             3);
                return;
            }

            ptr += 2;
            create_token(tok, TokenKind::GreaterGreater, tok_start, 2);
            return;
        }

        ++ptr;
        create_token(tok, TokenKind::Greater, tok_start, 1);
        return;
    }

    // Now, we must move on to scanning literals and identifiers. We will begin
    // with decimal integer literals as they are the starting point for floating
    // point literals as well. A decimal integer literal begins with the
    // digits 1-9.
    case '1':
    case '2':
    case '3':
    case '4':
    case '5':
    case '6':
    case '7':
    case '8':
    case '9': {
        lex_decimal_integer_literal(tok, tok_start);
        return;
    }
    }
}

/*
    This method consumes horizontal whitespace from the input and returns the
   count. This stage is critical for determining whether to insert an indent or
   dedent token into the stream. The idea is keep consuming whitespace while
   there is. Tabs are equal to one space and form feeds don't count as a space
   for indentation purposes.
*/
auto Lexer::consume_horizontal_whitespace() -> int {
    int whitespace_count = 0;

    while (true) {
        if (*ptr == ' ') {
            ++whitespace_count;
            ++ptr;
            continue;
        }
        if (*ptr == '\t') {
            whitespace_count += 4;
            ++ptr;
            continue;
        }
        if (*ptr == '\f') {
            ++ptr;
            continue;
        }

        return whitespace_count;
    }
}

auto Lexer::lex_decimal_integer_literal(Token &tok, char *start) -> void {
    // We know that the first character is a digit, so we can consume it.
    ++ptr;

    // Now, we need to consume numeric parts and separators.
    while (true) {
        switch (*ptr) {
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9': {
            ++ptr;
            continue;
        }

        // An underscore represents a numeric separator. All numeric separators
        // must be followed by a valid digit. If there isn't a valid digit, we
        // can return the token upto the point we have matched digits, and
        // consume the separator.
        case '_': {
            if (!isdigit(ptr[1])) {
                report_error(
                    ptr, 1,
                    "a numeric separator must be followed by a valid digit.");

                create_token(tok, TokenKind::IntLiteral, start, ptr - start);

                // Consume the separator.
                ++ptr;
                return;
            }

            // Otherwise, if we have a digit, we can consume both the separator
            // and the digit.
            ptr += 2;
            continue;
        }

        // If we encounter a '.', we know that this literal is a floating point
        // literal.
        case '.': {
            lex_floating_point_literal(tok, start);
            return;
        }

        // Also, we can go straight from the integers into the exponent part of
        // the floating point literal.
        case 'e':
        case 'E': {
            lex_floating_point_literal_exponent_part(tok, start);
            return;
        }

        // If we encounter any other characters, we know that we have
        // reached the end of the literal.
        default: {
            create_token(tok, TokenKind::IntLiteral, start, ptr - start);
            return;
        }
        }
    }
}

auto Lexer::lex_floating_point_literal(Token &tok, char *start) -> void {
    // First, we need to consume the floating point.
    ++ptr;

    // The first character after a floating point must always be a digit.
    if (!isdigit(*ptr)) {
        report_error(ptr, 1, "a floating point must be followed by a digit.");

        // We need to return the literal upto what we had before the invalid
        // character.
        create_token(tok, TokenKind::FloatLiteral, start, ptr - start);
        return;
    }

    // Consume the first digit.
    ++ptr;

    // Now, similar to the integer literals, we can expect digits and separators
    // to form the fraction part of the literal.
    while (true) {
        switch (*ptr) {
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9': {
            ++ptr;
            continue;
        }

        // An underscore represents a numeric separator. All numeric separators
        // must be followed by a valid digit. If there isn't a valid digit, we
        // can return the token upto the point we have matched digits, and
        // consume the separator.
        case '_': {
            if (!isdigit(ptr[1])) {
                report_error(
                    ptr, 1,
                    "a numeric separator must be followed by a valid digit.");

                create_token(tok, TokenKind::FloatLiteral, start, ptr - start);

                // Consume the separator.
                ++ptr;
                return;
            }

            // Otherwise, if we have a digit, we can consume both the separator
            // and the digit.
            ptr += 2;
            continue;
        }

        // Floating point literals can also have exponent parts which are
        // triggered by 'e' or 'E'.
        case 'e':
        case 'E': {
            lex_floating_point_literal_exponent_part(tok, start);
            return;
        }

        // For all other characters, return the floating point literal token.
        default: {
            create_token(tok, TokenKind::FloatLiteral, start, ptr - start);
            return;
        }
        }
    }
}

auto Lexer::lex_floating_point_literal_exponent_part(Token &tok,
                                                     char *start) -> void {
    // First, we must consume the exponent delimiter.
    ++ptr;

    // Now, the exponent delimiter can be followed by a sign.
    if (*ptr == '+' || *ptr == '-') {
        ++ptr;
    }

    // Now, we we must have a digit followed by digits and separators.
    if (!isdigit(*ptr)) {
        report_error(ptr, 1, "a floating point must be followed by a digit.");

        // We need to return the literal upto what we had before the invalid
        // character.
        create_token(tok, TokenKind::FloatLiteral, start, ptr - start);
        return;
    }

    // Consume the first digit.
    ++ptr;

    while (true) {
        switch (*ptr) {
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9': {
            ++ptr;
            continue;
        }

        // An underscore represents a numeric separator. All numeric separators
        // must be followed by a valid digit. If there isn't a valid digit, we
        // can return the token upto the point we have matched digits, and
        // consume the separator.
        case '_': {
            if (!isdigit(ptr[1])) {
                report_error(
                    ptr, 1,
                    "a numeric separator must be followed by a valid digit.");

                create_token(tok, TokenKind::FloatLiteral, start, ptr - start);

                // Consume the separator.
                ++ptr;
                return;
            }

            // Otherwise, if we have a digit, we can consume both the separator
            // and the digit.
            ptr += 2;
            continue;
        }

        // At the end of the exponent digits, we must finally return the
        // literal.
        default: {
            create_token(tok, TokenKind::FloatLiteral, start, ptr - start);
            return;
        }
        }
    }
}

} // namespace tpy::Parse