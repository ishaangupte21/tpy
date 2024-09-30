/*
    This file defines the interface that will be used to parse Python source
   files.
*/

#ifndef TPY_parse_py_PARSER_H
#define TPY_parse_py_PARSER_H

#include "Lexer.h"
#include "tpy/source/Span.h"
#include "tpy/tree/ASTNode.h"
#include "tpy/utility/ArenaAllocator.h"

namespace tpy::Parse {
/*
    The parser will be implemented using a standard Recursive descent parser and
   will built an AST of the source.
*/
class Parser {
    // This member is a reference to the lexer instance that will be used to
    // tokenize the source code.
    Lexer &lexer;

    // This member is the token instance that will be used for determining the
    // next step to take within the parser.
    Token tok = Token::dummy();

    // This member represents the arena allocator that will be used to quickly
    // allocate the AST nodes as they can all eventually be deallocated
    // together.
    Utility::ArenaAllocator &arena;

    // This is the return type of most Parser methods. It contains the Node
    // field and a boolean field representing whether any errors have been
    // previously reported. If an error has been reported, we will not report
    // the error that is caused by it to avoid duplicate errors on the same
    // issue.
    using ReturnType = std::pair<Tree::ASTNode *, bool>;

    // This method will advance in the input by getting the next token from
    // the lexer.
    auto advance() -> void { lexer.lex_next_tok(tok); }

    // This method will report errors.
    auto report_error(Source::Span &loc, const char *msg) -> void;

    // This method checks if the lookahead is the expected token.
    auto expect(TokenKind kind) -> bool { return kind == tok.kind; }

    /*
        These are the parser methods that will parse the structures of the
       Python syntax.
    */

    auto parse_py_expr() -> ReturnType;

    auto parse_py_atom_and_primary_expr() -> ReturnType;

    auto parse_py_paren_expr() -> ReturnType;

    auto parse_py_list_expr() -> ReturnType;

    auto parse_py_set_or_dict_expr() -> ReturnType;

    auto parse_py_dict_expr(Tree::ASTNode *first_expr,
                            Source::Span start) -> ReturnType;

    auto parse_py_attr_ref_expr(Tree::ASTNode *expr) -> ReturnType;

    auto parse_py_call_expr(Tree::ASTNode *callee) -> ReturnType;

    auto parse_py_slice_expr(Tree::ASTNode *slicee) -> ReturnType;

    auto parse_py_proper_slice_expr(Tree::ASTNode *slicee,
                                    Tree::ASTNode *lower_bound) -> ReturnType;

    auto parse_py_exponentiation_expr() -> ReturnType;

    auto parse_py_unary_op_expr() -> ReturnType;

    auto parse_py_multiplication_expr() -> ReturnType;

    auto parse_py_addition_expr() -> ReturnType;

    auto parse_py_bitshift_expr() -> ReturnType;

    auto parse_py_bitwise_and_expr() -> ReturnType;

    auto parse_py_bitwise_xor_expr() -> ReturnType;

    auto parse_py_bitwise_or_expr() -> ReturnType;

  public:
    Parser(Lexer &lexer, Utility::ArenaAllocator &arena)
        : lexer{lexer}, arena{arena} {}

    auto parse_py_compilation_unit() -> Tree::ASTNode * {
        advance();
        return parse_py_expr().first;
    }
};
} // namespace tpy::Parse

#endif