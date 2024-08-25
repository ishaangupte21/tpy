/*
    This file implements the Lexical Analyzer that will scan tokens from the
   Python source files.
*/

#include "tpy/parse/Lexer.h"
#include "tpy/compiler/FrontendErrorHandler.h"
#include "tpy/parse/Keywords.h"
#include "tpy/utility/Unicode.h"

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

    // A series of literals can begin with '0'. If the '0' digit is followed
    // by a 'b', we have a binary literal. If it is followed by an 'x', we
    // have a hex literal. If it is followed by a 'o', we have an octal
    // literal. If it is followed by a '.' or 'e', we have a floating point
    // literal. Otherwise, it is simply an integer literal of 0.
    case '0': {
        ++ptr;

        switch (*ptr) {
        case '.': {
            lex_floating_point_literal(tok, tok_start);
            return;
        }

        case 'e':
        case 'E': {
            lex_floating_point_literal_exponent_part(tok, tok_start);
            return;
        }

        case 'x':
        case 'X': {
            lex_hex_integer_literal(tok, tok_start);
            return;
        }

        case 'o':
        case 'O': {
            lex_octal_integer_literal(tok, tok_start);
            return;
        }

        case 'b':
        case 'B': {
            lex_binary_integer_literal(tok, tok_start);
            return;
        }

        default: {
            create_token(tok, TokenKind::IntLiteral, tok_start, 1);
            return;
        }
        }
    }

    // Python supports string literals that are enclosed in both a single and
    // double quote.
    case '\'': {
        lex_single_quote_string_literal(tok, tok_start);
        return;
    }

    case '"': {
        lex_double_quote_string_literal(tok, tok_start);
        return;
    }

    // Now, we can begin scanning identifiers.
    case 'a':
    case 'b':
    case 'c':
    case 'd':
    case 'e':
    case 'f':
    case 'g':
    case 'h':
    case 'i':
    case 'j':
    case 'k':
    case 'l':
    case 'm':
    case 'n':
    case 'o':
    case 'p':
    case 'q':
    case 'r':
    case 's':
    case 't':
    case 'u':
    case 'v':
    case 'w':
    case 'x':
    case 'y':
    case 'z':
    case 'A':
    case 'B':
    case 'C':
    case 'D':
    case 'E':
    case 'F':
    case 'G':
    case 'H':
    case 'I':
    case 'J':
    case 'K':
    case 'L':
    case 'M':
    case 'N':
    case 'O':
    case 'P':
    case 'Q':
    case 'R':
    case 'S':
    case 'T':
    case 'U':
    case 'V':
    case 'W':
    case 'X':
    case 'Y':
    case 'Z':
    case '_': {
        // We need to consume the first character here because if there is a
        // unicode codepoint, the pointer will already be advanced.
        ++ptr;
        lex_keyword_or_identifier(tok, tok_start);
        return;
    }

    // Comments in Python begin with '#'. The lex_comment method will return
    // false if an EOF was found and the token was created. It will return true
    // if a newline was found as parsing newline characters needs to be
    // deferred.
    case '#': {
        if (lex_comment(tok)) {
            goto lexer_start;
        }

        return;
    }

    // For all other characters, there are two options. If we have an ASCII
    // character, we definitely know that it is invalid. However, if we have
    // a unicode codepoint, we need to check if it is part of XID_START.
    default: {
        if (*ptr < 0) {
            auto cp = Utility::Unicode::decode_utf8_sequence(
                reinterpret_cast<uint8_t **>(&ptr),
                reinterpret_cast<uint8_t *>(end_ptr));

            if (Utility::Unicode::is_xid_start(cp)) {
                lex_keyword_or_identifier(tok, tok_start);
                return;
            }
            // Fallthrough here to report an error.
        } else {
            // If we have an ASCII character, we need to consume it as if it
            // never exists. For unicode codepoints, the pointer will already be
            // advanced.
            ++ptr;
        }

        report_error(tok_start, 1, "invalid character.");
        goto lexer_start;
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
                    ptr + 1, 1,
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
                    ptr + 1, 1,
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
                    ptr + 1, 1,
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

auto Lexer::lex_hex_integer_literal(Token &tok, char *start) -> void {
    // First, we need to consume the 'X' or 'x' delimiter that got here.
    ++ptr;

    // Now, we must have a hex digit or a separator.
    if (isxdigit(*ptr)) {
        ++ptr;
    } else if (*ptr == '_') {
        // If we have a separator, it must be followed by a valid hex digit.
        if (!isxdigit(ptr[1])) {
            report_error(
                ptr + 1, 1,
                "a numeric separator must be followed by a valid hex digit.");

            // Return the token.
            // We will use an error token because we are not sure what the user
            // has intended.
            create_token(tok, TokenKind::ErrorToken, start, ptr - start);

            // Consume the separator.
            ++ptr;

            return;
        }

        // Otherwise, we can consume both the separator and the digit.
        ptr += 2;
    } else {
        // If we do not have either a hex digit or a separator, we must report
        // an error.
        report_error(ptr, 1, "expected hex digit after '0x'.");

        create_token(tok, TokenKind::ErrorToken, start, ptr - start);
        return;
    }

    // Now, we can consume digits and separators while they are present.
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
        case '9':
        case 'a':
        case 'b':
        case 'c':
        case 'd':
        case 'e':
        case 'f':
        case 'A':
        case 'B':
        case 'C':
        case 'D':
        case 'E':
        case 'F': {
            ++ptr;
            continue;
        }

        // An underscore represents a numeric separator. All numeric separators
        // must be followed by a valid digit. If there isn't a valid digit, we
        // can return the token upto the point we have matched digits, and
        // consume the separator.
        case '_': {
            if (!isxdigit(ptr[1])) {
                report_error(ptr + 1, 1,
                             "a numeric separator must be followed by a valid "
                             "hex digit.");

                create_token(tok, TokenKind::HexIntLiteral, start, ptr - start);

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
            create_token(tok, TokenKind::HexIntLiteral, start, ptr - start);
            return;
        }
        }
    }
}

auto Lexer::lex_octal_integer_literal(Token &tok, char *start) -> void {
    // First, we need to consume the 'o' or 'O' delimiter that got here.
    ++ptr;

    // Now, we must have an octal digit or a separator.
    if (is_octal_digit(*ptr)) {
        ++ptr;
    } else if (*ptr == '_') {
        // If we have a separator, it must be followed by a valid octal digit.
        if (!is_octal_digit(ptr[1])) {
            report_error(
                ptr + 1, 1,
                "a numeric separator must be followed by a valid octal digit.");

            // Return the token.
            // We will use an error token because we are not sure what the user
            // has intended.
            create_token(tok, TokenKind::ErrorToken, start, ptr - start);

            // Consume the separator.
            ++ptr;

            return;
        }

        // Otherwise, we can consume both the separator and the digit.
        ptr += 2;
    } else {
        // If we do not have either an octal digit or a separator, we must
        // report an error.
        report_error(ptr, 1, "expected octal digit after '0o'.");

        create_token(tok, TokenKind::ErrorToken, start, ptr - start);
        return;
    }

    // Now, we can consume digits and separators while they are present.
    while (true) {
        switch (*ptr) {
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7': {
            ++ptr;
            continue;
        }

        // An underscore represents a numeric separator. All numeric separators
        // must be followed by a valid digit. If there isn't a valid digit, we
        // can return the token upto the point we have matched digits, and
        // consume the separator.
        case '_': {
            if (!is_octal_digit(ptr[1])) {
                report_error(ptr + 1, 1,
                             "a numeric separator must be followed by a valid "
                             "octal digit.");

                create_token(tok, TokenKind::OctalIntLiteral, start,
                             ptr - start);

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
            create_token(tok, TokenKind::OctalIntLiteral, start, ptr - start);
            return;
        }
        }
    }
}

auto Lexer::lex_binary_integer_literal(Token &tok, char *start) -> void {
    // First, we need to consume the 'b' or 'B' delimiter that got here.
    ++ptr;

    // Now, we must have a binary digit or a separator.
    if (is_binary_digit(*ptr)) {
        ++ptr;
    } else if (*ptr == '_') {
        // If we have a separator, it must be followed by a valid binary digit.
        if (!is_binary_digit(ptr[1])) {
            report_error(ptr + 1, 1,
                         "a numeric separator must be followed by a valid "
                         "binary digit.");

            // Return the token.
            // We will use an error token because we are not sure what the user
            // has intended.
            create_token(tok, TokenKind::ErrorToken, start, ptr - start);

            // Consume the separator.
            ++ptr;

            return;
        }

        // Otherwise, we can consume both the separator and the digit.
        ptr += 2;
    } else {
        // If we do not have either a binary digit or a separator, we must
        // report an error.
        report_error(ptr, 1, "expected binary digit after '0b'.");

        create_token(tok, TokenKind::ErrorToken, start, ptr - start);
        return;
    }

    // Now, we can consume digits and separators while they are present.
    while (true) {
        switch (*ptr) {
        case '0':
        case '1': {
            ++ptr;
            continue;
        }

        // An underscore represents a numeric separator. All numeric separators
        // must be followed by a valid digit. If there isn't a valid digit, we
        // can return the token upto the point we have matched digits, and
        // consume the separator.
        case '_': {
            if (!is_binary_digit(ptr[1])) {
                report_error(ptr + 1, 1,
                             "a numeric separator must be followed by a valid "
                             "binary digit.");

                create_token(tok, TokenKind::BinaryIntLiteral, start,
                             ptr - start);

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
            create_token(tok, TokenKind::BinaryIntLiteral, start, ptr - start);
            return;
        }
        }
    }
}

auto Lexer::lex_single_quote_string_literal(Token &tok, char *start) -> void {
    // Consume the starting single quote.
    ++ptr;

    // Now, we need to consume characters until we either hit the end of the
    // string or encounter an EOF. Because these strings support escape
    // characters, we need to check the character after a backslash. We will not
    // process escapes here as that is expensive and can be left for after
    // parsing.
    while (true) {
        switch (*ptr) {
        case '\'': {
            // This is the end of the string.
            // Consume the closing single quote.
            ++ptr;

            create_token(tok, TokenKind::StringLiteral, start, ptr - start);
            return;
        }
        case '\0': {
            // This is possibly the end of the file, and we need to check for
            // that. If it is the end of the file, we must report an error as
            // string literals must be terminated.
            if (ptr == end_ptr) {
                report_error(ptr, 1,
                             "expected closing '\'' in string literal, but "
                             "encountered file end instead.");

                create_token(tok, TokenKind::StringLiteral, start, ptr - start);
                return;
            }

            // Otherwise, null characters are accepted, so we must consume it
            // and keep going.
            ++ptr;
            continue;
        }
        case '\\': {
            // If we encounter a backslash, it must be followed by a valid
            // source character. Essentially, it cannot be followed by an end of
            // file.
            ++ptr;

            if (*ptr == 0) {
                report_error(
                    ptr, 1,
                    "expected character after '\\' in string literal, but "
                    "encountered file end instead.");

                create_token(tok, TokenKind::StringLiteral, start, ptr - start);
                return;
            }

            // Otherwise, we can accept either an ASCII character or a unicode
            // codepoint here.
            if (*ptr > 0) {
                ++ptr;
            } else {
                Utility::Unicode::decode_utf8_sequence(
                    reinterpret_cast<uint8_t **>(&ptr),
                    reinterpret_cast<uint8_t *>(end_ptr));
            }

            continue;
        }
        default: {
            // All other source characters are valid within a Python string.
            if (*ptr > 0) {
                ++ptr;
            } else {
                Utility::Unicode::decode_utf8_sequence(
                    reinterpret_cast<uint8_t **>(&ptr),
                    reinterpret_cast<uint8_t *>(end_ptr));
            }
            continue;
        }
        }
    }
}

auto Lexer::lex_double_quote_string_literal(Token &tok, char *start) -> void {
    // Consume the starting double quote.
    ++ptr;

    // Now, we need to consume characters until we either hit the end of the
    // string or encounter an EOF. Because these strings support escape
    // characters, we need to check the character after a backslash. We will not
    // process escapes here as that is expensive and can be left for after
    // parsing.
    while (true) {
        switch (*ptr) {
        case '"': {
            // This is the end of the string.
            // Consume the closing double quote.
            ++ptr;

            create_token(tok, TokenKind::StringLiteral, start, ptr - start);
            return;
        }
        case '\0': {
            // This is possibly the end of the file, and we need to check for
            // that. If it is the end of the file, we must report an error as
            // string literals must be terminated.
            if (ptr == end_ptr) {
                report_error(ptr, 1,
                             "expected closing '\"' in string literal, but "
                             "encountered file end instead.");

                create_token(tok, TokenKind::StringLiteral, start, ptr - start);
                return;
            }

            // Otherwise, null characters are accepted, so we must consume it
            // and keep going.
            ++ptr;
            continue;
        }
        case '\\': {
            // If we encounter a backslash, it must be followed by a valid
            // source character. Essentially, it cannot be followed by an end of
            // file.
            ++ptr;

            if (*ptr == 0) {
                report_error(
                    ptr, 1,
                    "expected character after '\\' in string literal, but "
                    "encountered file end instead.");

                create_token(tok, TokenKind::StringLiteral, start, ptr - start);
                return;
            }

            // Otherwise, we can accept either an ASCII character or a unicode
            // codepoint here.
            if (*ptr > 0) {
                ++ptr;
            } else {
                Utility::Unicode::decode_utf8_sequence(
                    reinterpret_cast<uint8_t **>(&ptr),
                    reinterpret_cast<uint8_t *>(end_ptr));
            }

            continue;
        }
        default: {
            // All other source characters are valid within a Python string.
            if (*ptr > 0) {
                ++ptr;
            } else {
                Utility::Unicode::decode_utf8_sequence(
                    reinterpret_cast<uint8_t **>(&ptr),
                    reinterpret_cast<uint8_t *>(end_ptr));
            }
            continue;
        }
        }
    }
}

auto Lexer::lex_keyword_or_identifier(Token &tok, char *start) -> void {
    // Python allows all unicode codepoints with the category xid_continue to
    // follow the start character in an identifier. In order to speed up all
    // keywords and common identifiers, we will check ASCII characters first.
    while (true) {
        switch (*ptr) {
        case 'a':
        case 'b':
        case 'c':
        case 'd':
        case 'e':
        case 'f':
        case 'g':
        case 'h':
        case 'i':
        case 'j':
        case 'k':
        case 'l':
        case 'm':
        case 'n':
        case 'o':
        case 'p':
        case 'q':
        case 'r':
        case 's':
        case 't':
        case 'u':
        case 'v':
        case 'w':
        case 'x':
        case 'y':
        case 'z':
        case 'A':
        case 'B':
        case 'C':
        case 'D':
        case 'E':
        case 'F':
        case 'G':
        case 'H':
        case 'I':
        case 'J':
        case 'K':
        case 'L':
        case 'M':
        case 'N':
        case 'O':
        case 'P':
        case 'Q':
        case 'R':
        case 'S':
        case 'T':
        case 'U':
        case 'V':
        case 'W':
        case 'X':
        case 'Y':
        case 'Z':
        case '0':
        case '1':
        case '2':
        case '3':
        case '4':
        case '5':
        case '6':
        case '7':
        case '8':
        case '9':
        case '_': {
            ++ptr;
            continue;
        }

        default: {
            // Here, we have two options. If we have an ascii character, we know
            // that it is not part of the keyword/identifier and can stop there.
            // Otherwise, we need to decode the utf-8 codepoint and check if it
            // is in the xid_continue set.
            if (*ptr < 0) {
                // We need to store the start of this codepoint incase it is not
                // part of the identifier.
                char *last_cp_start = ptr;

                auto cp = Utility::Unicode::decode_utf8_sequence(
                    reinterpret_cast<uint8_t **>(&ptr),
                    reinterpret_cast<uint8_t *>(end_ptr));

                if (Utility::Unicode::is_xid_continue(cp)) {
                    // If we have an XID_continue character, we must consume it
                    // and keep going.
                    continue;
                }

                // Otherwise, we need to backtrack on the input. We will then
                // fall through as that is where we will handle the end of the
                // identifier.
                ptr = last_cp_start;
            }

            // Before we create a token, we must check if this token is a
            // keyword.
            size_t tok_len = ptr - start;
            auto keyword_resp = KeywordLookup::is_keyword(start, tok_len);

            // If the response is null, it means we have an identifier.
            // Otherwise, we have a keyword.
            if (keyword_resp) {
                create_token(tok, keyword_resp->kind, start, tok_len);
            } else {
                create_token(tok, TokenKind::Identifier, start, tok_len);
            }

            return;
        }
        }
    }
}

auto Lexer::lex_comment(Token &tok) -> bool {
    // Consume the '#'.
    ++ptr;

    // Now, we need to keep consuming characters until we find a newline or EOF.
    while (true) {
        switch (*ptr) {
        // If we find a newline, we need to just return true and allow the
        // lexer to scan it.
        case '\r':
        case '\n': {
            return true;
        }

        // If we find a null character, we need to check if it is EOF. If it is
        // EOF, we can create the token and return false. Otherwise, we just
        // keep going.
        case '\0': {
            if (ptr == end_ptr) {
                create_token(tok, TokenKind::End, ptr, 1);
                return false;
            }

            ++ptr;
            continue;
        }

        // For all other characters, we just keep going.
        default: {
            ++ptr;
            continue;
        }
        }
    }
}

} // namespace tpy::Parse