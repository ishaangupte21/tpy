/*
    This file contains the definition of the tpy lexical analyzer for pyton
   source files.
*/

#include <stack>

#include "tpy/parse/Token.h"
#include "tpy/source/SourceFile.h"

namespace tpy::Parse {
/*
    This is the Lexical Analyzer that will be responsible for scanning tokens
   from the source input.
*/
class Lexer {
    // This is the source file object that contains all of the metadata for this
    // source file.
    Source::SourceFile &src_file;

    // This is the pointer to the start of the lexical buffer. In the case of a
    // UTF-8 BOM, this pointer will be configured to skip the BOM.
    char *ptr;

    // This is the pointer to the end of the lexical buffer. We will use this to
    // make sure null characters are indeed the end of the buffer.
    char *end_ptr;

    // This is the pointer to the absolute start of the buffer. It is usefull
    // when computing absolute offsets within the file.
    char *abs_buffer_start;

    // This is the stack that handles indentation levels. We will use this when
    // computing the insertion of Indent and Dedent tokens.
    std::stack<int> whitespace_stack;

    // In order to properly handle indentation, we must track if the last token
    // was a newline token. It will be initialized as true as we need to track
    // indentation for the first token.
    bool was_last_tok_newline = true;

    // There are certain places in Python source code where we want to ignore
    // newline characters. Therefore, we need to track if we are currently
    // accepting them.
    bool accept_newlines = true;

    /*
        The following methods are utility methods that are part of the lexer
       routine.
    */

    auto create_token(Token &tok, TokenKind kind, char *start, size_t len,
                      bool is_newline_tok = false) -> void {
        // First, we need to compute the local start position by subtracting the
        // start pointer from the buffer start. After that, the absolute start
        // position can be computed by simply adding the file's offset.
        auto local_pos = static_cast<size_t>(start - abs_buffer_start);

        tok.update(kind,
                   Source::Span{local_pos, local_pos + src_file.offset, len});

        was_last_tok_newline = is_newline_tok;
    }

    auto report_error(char *start, size_t len, const char *msg) -> void;

    auto consume_horizontal_whitespace() -> int;

    auto lex_decimal_integer_literal(Token &tok, char *start) -> void;

    auto lex_floating_point_literal(Token &tok, char *start) -> void;

    auto lex_floating_point_literal_exponent_part(Token &tok,
                                                  char *start) -> void;

  public:
    explicit Lexer(Source::SourceFile &src_file) : src_file{src_file} {
        ptr = src_file.start();
        end_ptr = reinterpret_cast<char *>(src_file.buffer->end());
        abs_buffer_start = reinterpret_cast<char *>(src_file.buffer->data());

        // All Python source files begin with a 0 on the indentation stack.
        whitespace_stack.push(0);
    }

    auto skip_newlines() -> void { accept_newlines = false; }

    auto allow_newlines() -> void { accept_newlines = true; }

    // This is the main lexer routine that will scan tokens from the Python
    // source.
    auto lex_next_tok(Token &tok) -> void;
};
} // namespace tpy::Parse