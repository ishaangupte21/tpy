/*
    Source file lexical analyzer
*/

use std::str::Chars;

use frontend_errors::report_frontend_error;
use keywords::KEYWORDS;
use src_manager::{span::Span, SrcFileMetadata};
use token::{Token, TokenKind};
use unicode_ident::{is_xid_continue, is_xid_start};

mod keywords;
pub mod token;

pub struct Lexer<'a> {
    metadata: &'a SrcFileMetadata<'a>,
    chars: Chars<'a>,
    remaining: usize,
    indentation_stack: Vec<i32>,
    accept_newlines: bool,
    was_last_tok_newline: bool,
    has_seen_lexical_err: bool,
}

impl Lexer<'_> {
    pub fn new<'a>(metadata: &'a SrcFileMetadata) -> Lexer<'a> {
        Lexer {
            remaining: metadata.len,
            chars: metadata.contents.chars(),
            metadata,
            indentation_stack: vec![0],
            accept_newlines: true,
            was_last_tok_newline: true,
            has_seen_lexical_err: false,
        }
    }

    // Gets the next character from the input and returns a null character if none exists.
    fn advance(&mut self) -> char {
        self.chars.next().unwrap_or('\0')
    }

    // Peeks ahead by one character in the input.
    fn peek(&self) -> char {
        self.chars.clone().next().unwrap_or('\0')
    }

    // Updates the remaining count.
    fn update_remaining(&mut self) {
        self.remaining = self.chars.as_str().len();
    }

    fn current_pos(&self) -> usize {
        self.metadata.len - self.remaining
    }

    pub fn accept_newlines(&mut self) {
        self.accept_newlines = true;
    }

    pub fn skip_newlines(&mut self) {
        self.accept_newlines = false;
    }

    fn report_lexical_error(&mut self, msg: &str, loc: Span) {
        self.has_seen_lexical_err = true;
        report_frontend_error(msg, loc, self.metadata);
    }

    // This method gets the next token from the lexical input.
    pub fn next(&mut self) -> Token {
        // We need to wrap the lexer routine in an infinite loop to allow restarting the lexer after comments.
        loop {
            // First check for the end of the file.
            // We cannot use the advance method here because if we have no whitespace, then we don't want the dedent token consuming the first character of the next proper token.
            // Therefore, we have to simply preview it here and consume it later once we are done with whitespace.
            if self.peek() == '\0' {
                let start = self.current_pos();
                return Token::new(TokenKind::End, Span::new(start, start + 1));
            };

            let mut tok_start = self.current_pos();

            // Now, if the previous token was a newline, we must check indentation.
            if self.was_last_tok_newline {
                let whitespace_count = self.consume_horizontal_whitespace();
                let current_indentation = *self.indentation_stack.last().unwrap_or(&0);

                self.update_remaining();
                let current_pos = self.current_pos();

                if whitespace_count > current_indentation {
                    self.indentation_stack.push(whitespace_count);
                    self.was_last_tok_newline = false;
                    return Token::new(TokenKind::Indent, Span::new(tok_start, current_pos));
                }
                if whitespace_count < current_indentation {
                    self.indentation_stack.pop();
                    self.was_last_tok_newline = false;
                    return Token::new(TokenKind::Dedent, Span::new(tok_start, current_pos));
                }

                // If there is no indentation, update the token start position.
                tok_start = current_pos;
            } else {
                // If we don't check indentation, then we must actually check for whitespace.
                self.consume_horizontal_whitespace();
                self.update_remaining();

                // Now, we can actually get the next character.
                tok_start = self.current_pos();
            }

            // Now that we know the character exists, we can actually consume it.
            // We need to check again for EOF here.
            let c = match self.advance() {
                '\0' => return Token::new(TokenKind::End, Span::new(tok_start, tok_start + 1)),
                c => c,
            };

            // Begin the actual Lexer DFA.
            let token_kind = match c {
                // First, we will handle whitespace.
                '\n' => {
                    if self.accept_newlines {
                        TokenKind::Newline
                    } else {
                        self.update_remaining();
                        continue;
                    }
                }
                '\r' => {
                    if self.peek() == '\n' {
                        self.advance();
                    }

                    if self.accept_newlines {
                        TokenKind::Newline
                    } else {
                        self.update_remaining();
                        continue;
                    }
                }

                // Here, we must handle delimiters.
                ';' => TokenKind::Semicolon,
                '(' => TokenKind::LeftParen,
                ')' => TokenKind::RightParen,
                '[' => TokenKind::LeftSquare,
                ']' => TokenKind::RightSquare,
                '{' => TokenKind::LeftCurly,
                '}' => TokenKind::RightCurly,
                ',' => TokenKind::Comma,
                '~' => TokenKind::Tilde,
                '+' => self.lex_plus_tokens(),
                '-' => self.lex_minus_tokens(),
                '*' => self.lex_asterisk_tokens(),
                '/' => self.lex_slash_tokens(),
                '%' => self.lex_percent_tokens(),
                '&' => self.lex_ampersand_tokens(),
                '|' => self.lex_bar_tokens(),
                '^' => self.lex_caret_tokens(),
                '=' => self.lex_equals_tokens(),
                ':' => self.lex_colon_tokens(),
                '!' => self.lex_exclamation_tokens(tok_start),
                '<' => self.lex_less_tokens(),
                '>' => self.lex_greater_tokens(),

                // This has an entrypoint for float literals.
                '.' => self.lex_dot_tokens(),

                // Now, we must handle literals.
                '1'..='9' => self.lex_numeric_literals(),
                '0' => self.lex_special_base_literals(tok_start),

                '\'' => self.lex_single_quote_string_literal(tok_start),
                '"' => self.lex_double_quote_string_literal(tok_start),

                // Identifiers and keywords.
                c if is_xid_start(c) => self.lex_identifier_or_keyword(tok_start),
                _ => todo!(),
            };

            self.update_remaining();

            if self.was_last_tok_newline && token_kind != TokenKind::Newline {
                self.was_last_tok_newline = false;
            } else if token_kind == TokenKind::Newline {
                self.was_last_tok_newline = true;
            }

            let tok_end = self.current_pos();
            return Token::new(token_kind, Span::new(tok_start, tok_end));
        }
    }

    // This method consumes all horizontal whitepsace ahead of a token.
    // It also counts it for intendation purposes.
    fn consume_horizontal_whitespace(&mut self) -> i32 {
        let mut count = 0;
        loop {
            match self.peek() {
                ' ' => {
                    count += 1;
                    self.advance();
                }
                '\t' => {
                    count += 4;
                    self.advance();
                }
                '\x0c' => {
                    self.advance();
                }
                _ => {
                    return count;
                }
            };
        }
    }

    // Scans tokens starting with '+'
    fn lex_plus_tokens(&mut self) -> TokenKind {
        match self.peek() {
            '=' => {
                self.advance();
                TokenKind::PlusEquals
            }
            _ => TokenKind::Plus,
        }
    }

    // Scans tokens starting with '-'
    fn lex_minus_tokens(&mut self) -> TokenKind {
        match self.peek() {
            '=' => {
                self.advance();
                TokenKind::MinusEquals
            }
            _ => TokenKind::Minus,
        }
    }

    // Scans tokens starting with '*'
    fn lex_asterisk_tokens(&mut self) -> TokenKind {
        match self.peek() {
            '=' => {
                self.advance();
                TokenKind::AsteriskEquals
            }
            '*' => {
                self.advance();
                match self.peek() {
                    '=' => {
                        self.advance();
                        TokenKind::AsteriskAsteriskEquals
                    }
                    _ => TokenKind::Asterisk,
                }
            }
            _ => TokenKind::Asterisk,
        }
    }

    // Scans tokens starting with '/'
    fn lex_slash_tokens(&mut self) -> TokenKind {
        match self.peek() {
            '=' => {
                self.advance();
                TokenKind::SlashEquals
            }
            '/' => {
                self.advance();
                match self.peek() {
                    '=' => {
                        self.advance();
                        TokenKind::SlashSlashEquals
                    }
                    _ => TokenKind::SlashSlash,
                }
            }
            _ => TokenKind::Slash,
        }
    }

    // Scans tokens starting with '%'
    fn lex_percent_tokens(&mut self) -> TokenKind {
        match self.peek() {
            '=' => {
                self.advance();
                TokenKind::PercentEquals
            }
            _ => TokenKind::Percent,
        }
    }

    // Scans tokens starting with '&'
    fn lex_ampersand_tokens(&mut self) -> TokenKind {
        match self.peek() {
            '=' => {
                self.advance();
                TokenKind::AmpersandEquals
            }
            _ => TokenKind::Ampersand,
        }
    }

    // Scans tokens starting with '|'
    fn lex_bar_tokens(&mut self) -> TokenKind {
        match self.peek() {
            '=' => {
                self.advance();
                TokenKind::BarEquals
            }
            _ => TokenKind::Bar,
        }
    }

    // Scans tokens starting with '^'
    fn lex_caret_tokens(&mut self) -> TokenKind {
        match self.peek() {
            '=' => {
                self.advance();
                TokenKind::CaretEquals
            }
            _ => TokenKind::Caret,
        }
    }

    // Scans tokens starting with '='
    fn lex_equals_tokens(&mut self) -> TokenKind {
        match self.peek() {
            '=' => {
                self.advance();
                TokenKind::EqualsEquals
            }
            _ => TokenKind::Equals,
        }
    }

    // Scans tokens starting with ':'
    fn lex_colon_tokens(&mut self) -> TokenKind {
        match self.peek() {
            '=' => {
                self.advance();
                TokenKind::ColonEquals
            }
            _ => TokenKind::Colon,
        }
    }

    // Scans tokens starting with '!'
    fn lex_exclamation_tokens(&mut self, tok_start: usize) -> TokenKind {
        match self.peek() {
            '=' => {
                self.advance();
                TokenKind::ExclamationEquals
            }
            _ => {
                self.report_lexical_error(
                    "unknown symbol '!'",
                    Span::new(tok_start, tok_start + 1),
                );
                TokenKind::ExclamationEquals
            }
        }
    }

    // Scans tokens starting with '<'
    fn lex_less_tokens(&mut self) -> TokenKind {
        match self.peek() {
            '=' => {
                self.advance();
                TokenKind::LessEquals
            }
            '<' => {
                self.advance();
                match self.peek() {
                    '=' => {
                        self.advance();
                        TokenKind::LessLessEquals
                    }
                    _ => TokenKind::LessLess,
                }
            }
            _ => TokenKind::Less,
        }
    }

    // Scans tokens starting with '>'
    fn lex_greater_tokens(&mut self) -> TokenKind {
        match self.peek() {
            '=' => {
                self.advance();
                TokenKind::GreaterEquals
            }
            '>' => {
                self.advance();
                match self.peek() {
                    '=' => {
                        self.advance();
                        TokenKind::GreaterGreaterEquals
                    }
                    _ => TokenKind::GreaterGreater,
                }
            }
            _ => TokenKind::Greater,
        }
    }

    // This method scans all decimal numeric literals.
    fn lex_numeric_literals(&mut self) -> TokenKind {
        // Keep consuming integer digits.
        loop {
            match self.peek() {
                '0'..='9' => {
                    self.advance();
                }
                '.' => break self.lex_float_literals(),
                _ => break TokenKind::IntLiteral,
            }
        }
    }

    // If a floating point is encountered, this method continues scanning them.
    fn lex_float_literals(&mut self) -> TokenKind {
        // Consume the floating point.
        self.advance();
        loop {
            match self.peek() {
                '0'..='9' => {
                    self.advance();
                }
                _ => break TokenKind::FloatLiteral,
            }
        }
    }

    // This method scans tokens starting with '.'
    fn lex_dot_tokens(&mut self) -> TokenKind {
        match self.peek() {
            '0'..='9' => {
                // This is a float literal - keep consuming the digits.
                self.advance();

                loop {
                    match self.peek() {
                        '0'..='9' => {
                            self.advance();
                        }
                        _ => break TokenKind::FloatLiteral,
                    }
                }
            }
            _ => TokenKind::Dot,
        }
    }

    fn lex_special_base_literals(&mut self, tok_start: usize) -> TokenKind {
        match self.peek() {
            'b' | 'B' => {
                self.advance();
                // The first digit must be a binary digit.
                if !matches!(self.peek(), '0' | '1') {
                    // If it isn't, report the error.
                    self.update_remaining();
                    let current_pos = self.current_pos();
                    self.report_lexical_error(
                        "expected binary digits after '0b'",
                        Span::new(tok_start, current_pos),
                    );
                    return TokenKind::Unknown;
                }

                self.advance();

                // Now, consume all following octal digits.
                loop {
                    match self.peek() {
                        '0' | '1' => {
                            self.advance();
                        }
                        _ => break TokenKind::BinaryIntLiteral,
                    }
                }
            }
            'o' | 'O' => {
                self.advance();
                // The first digit must be an octal digit.
                if !matches!(self.peek(), '0'..='7') {
                    // If it isn't, report the error.
                    self.update_remaining();
                    let current_pos = self.current_pos();
                    self.report_lexical_error(
                        "expected octal digits after '0o'",
                        Span::new(tok_start, current_pos),
                    );
                    return TokenKind::Unknown;
                }

                self.advance();

                // Now, consume all following octal digits.
                loop {
                    match self.peek() {
                        '0'..='7' => {
                            self.advance();
                        }
                        _ => break TokenKind::OctalIntLiteral,
                    }
                }
            }
            'x' | 'X' => {
                self.advance();
                // The first digit must be a hex digit.
                if !matches!(self.peek(), '0'..='9' | 'a'..='f' | 'A'..='F') {
                    // If it isn't, report the error.
                    self.update_remaining();
                    let current_pos = self.current_pos();
                    self.report_lexical_error(
                        "expected hex digits after '0x'",
                        Span::new(tok_start, current_pos),
                    );
                    return TokenKind::Unknown;
                }

                self.advance();

                // Now, consume all following octal digits.
                loop {
                    match self.peek() {
                        '0'..='9' | 'a'..='f' | 'A'..='F' => {
                            self.advance();
                        }
                        _ => break TokenKind::HexIntLiteral,
                    }
                }
            }
            _ => TokenKind::IntLiteral,
        }
    }

    // This method scans single quote string literals.
    fn lex_single_quote_string_literal(&mut self, tok_start: usize) -> TokenKind {
        // First, we must consume the starting single quote.
        self.advance();

        // Now, consume all characters until either the end of file or another single quote is encountered.
        loop {
            match self.peek() {
                '\'' => {
                    // Consume the single quote and end the token.
                    self.advance();
                    break TokenKind::SingleQuoteStringLiteral;
                }
                '\0' => {
                    // Here, we need to check if this is actually the end of the file.
                    // If it is, we need to report an error for an untermianted string literal.
                    self.update_remaining();
                    if self.remaining == 0 {
                        self.report_lexical_error(
                            "unterminated string literal",
                            Span::new(tok_start, self.current_pos()),
                        );
                        break TokenKind::SingleQuoteStringLiteral;
                    }

                    // Otherwise, just consume the character as these are allowed in strings.
                    self.advance();
                }
                '\n' => {
                    // On newline characters, we must report the error.
                    self.update_remaining();
                    self.report_lexical_error(
                        "encountered end of line in string literal",
                        Span::new(tok_start, self.current_pos()),
                    );

                    // However, we need to keep scanning to the end of the token, so we must keep going despite the error.
                    // The error will cause compilation to terminate after parsing anyways.
                    self.advance();
                }
                _ => {
                    self.advance();
                }
            }
        }
    }

    // This method scans double quote string literals.
    fn lex_double_quote_string_literal(&mut self, tok_start: usize) -> TokenKind {
        // First, we must consume the starting double quote.
        self.advance();

        // Now, consume all characters until either the end of file or another double quote is encountered.
        loop {
            match self.peek() {
                '"' => {
                    // Consume the double quote and end the token.
                    self.advance();
                    break TokenKind::DoubleQuoteStringLiteral;
                }
                '\0' => {
                    // Here, we need to check if this is actually the end of the file.
                    // If it is, we need to report an error for an untermianted string literal.
                    self.update_remaining();
                    if self.remaining == 0 {
                        self.report_lexical_error(
                            "unterminated string literal",
                            Span::new(tok_start, self.current_pos()),
                        );
                        break TokenKind::DoubleQuoteStringLiteral;
                    }

                    // Otherwise, just consume the character as these are allowed in strings.
                    self.advance();
                }
                '\n' => {
                    // On newline characters, we must report the error.
                    self.update_remaining();
                    self.report_lexical_error(
                        "encountered end of line in string literal",
                        Span::new(tok_start, self.current_pos()),
                    );

                    // However, we need to keep scanning to the end of the token, so we must keep going despite the error.
                    // The error will cause compilation to terminate after parsing anyways.
                    self.advance();
                }
                _ => {
                    self.advance();
                }
            }
        }
    }

    fn lex_identifier_or_keyword(&mut self, start: usize) -> TokenKind {
        // Consume the first character.
        self.advance();

        // Now, keep consuming while we have identifier keywords.
        while is_xid_continue(self.peek()) {
            self.advance();
        }

        // Now, we need to check for identifier or keyword using hte perfect hash table.
        self.update_remaining();
        let tok_contents = self
            .metadata
            .get_src_file_text(&Span::new(start, self.current_pos()));

        match KEYWORDS.get(tok_contents) {
            Some(keyword) => keyword.clone(),
            None => TokenKind::Identifier,
        }
    }
}
