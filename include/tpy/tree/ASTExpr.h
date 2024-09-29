/*
    This file defines the AST nodes for Python expressions.
*/

#ifndef TPY_TREE_ASTEXPR_H
#define TPY_TREE_ASTEXPR_H

#include "ASTNode.h"
#include "tpy/source/Span.h"

#include <vector>

namespace tpy::Tree {
// This class defines the AST Node that will represent an integer literal in the
// input.
class ASTIntLiteralNode : public ASTNode {
  public:
    // This field represents the base of the integer that has been parsed.
    // This way, decimal, hex, binary, and octal integers will be held in the
    // same node.
    int base;

    ASTIntLiteralNode(int base, Source::Span loc) : ASTNode{loc}, base{base} {}

    virtual auto pretty_print(FILE *result_file, int level) -> void override;
};

// This class defines the AST Node that will represent a float literal in the
// input.
class ASTFloatLiteralNode : public ASTNode {
  public:
    explicit ASTFloatLiteralNode(Source::Span loc) : ASTNode{loc} {}

    virtual auto pretty_print(FILE *result_file, int level) -> void override;
};

// This class defines the AST Node that will represent a string literal in the
// input.
class ASTStringLiteralNode : public ASTNode {
  public:
    explicit ASTStringLiteralNode(Source::Span loc) : ASTNode{loc} {}

    virtual auto pretty_print(FILE *result_file, int level) -> void override;
};

// This class defines the AST Node that will represent a boolean literal in the
// input.
class ASTBoolLiteralNode : public ASTNode {
  public:
    // This field represents the value of the boolean literal as it will be
    // derived from the 'True' and 'False' keywords.
    bool val;

    ASTBoolLiteralNode(bool val, Source::Span loc) : ASTNode{loc}, val{val} {}

    virtual auto pretty_print(FILE *result_file, int level) -> void override;
};

// This class defines the AST Node that will an expression enclosed by
// parentheses in the input.
class ASTParenExprNode : public ASTNode {
  public:
    // This member is the expression node that is wrapped by the parentheses.
    ASTNode *inner_expr;

    ASTParenExprNode(ASTNode *inner_expr, Source::Span loc)
        : ASTNode{loc}, inner_expr{inner_expr} {}

    virtual auto pretty_print(FILE *result_file, int level) -> void override;
};

// This class defines the AST Node that will represent a list literal in the
// input.
class ASTListExprNode : public ASTNode {
  public:
    // This member is the list of expressions within the node.
    std::vector<ASTNode *> list;

    ASTListExprNode(std::vector<ASTNode *> list, Source::Span loc)
        : ASTNode{loc}, list{std::move(list)} {}

    ASTListExprNode(Source::Span loc) : ASTNode{loc} {}

    virtual auto pretty_print(FILE *result_file, int level) -> void override;
};

// This class defines the AST Node that will represent a set literal in the
// input.
class ASTSetExprNode : public ASTNode {
  public:
    // This member contains all of the expressions to be put into the set.
    std::vector<ASTNode *> contents;

    ASTSetExprNode(std::vector<ASTNode *> contents, Source::Span loc)
        : ASTNode{loc}, contents{std::move(contents)} {}

    virtual auto pretty_print(FILE *result_file, int level) -> void override;
};

// This class defines the AST Node that will represent a dict literal in the
// input.
class ASTDictExprNode : public ASTNode {
  public:
    // This member contains the key value pairs to be put into the dict.
    std::vector<std::pair<ASTNode *, ASTNode *>> contents;

    ASTDictExprNode(std::vector<std::pair<ASTNode *, ASTNode *>> contents,
                    Source::Span loc)
        : ASTNode{loc}, contents{std::move(contents)} {}

    ASTDictExprNode(Source::Span loc) : ASTNode{loc} {}

    virtual auto pretty_print(FILE *result_file, int level) -> void override;
};

// This class defines the ASTNode that will hold expressions defined by
// identifiers that act as names.
class ASTNameExprNode : public ASTNode {
  public:
    explicit ASTNameExprNode(Source::Span loc) : ASTNode{loc} {}

    virtual auto pretty_print(FILE *result_file, int level) -> void override;
};

// This class defines the ASTNode that will hold attribute reference expressions
// of the form 'foo.bar'.
class ASTAttrRefExprNode : public ASTNode {
  public:
    // These two members represent the two sides of the expression with the dot
    // in between. With the form 'foo.bar', 'foo' is the lhs and 'bar' is the
    // rhs.
    ASTNode *lhs;
    ASTNameExprNode *rhs;

    ASTAttrRefExprNode(ASTNode *lhs, ASTNameExprNode *rhs, Source::Span loc)
        : ASTNode{loc}, lhs{lhs}, rhs{rhs} {}

    virtual auto pretty_print(FILE *result_file, int level) -> void override;
};

// This class defines the ASTNode that will hold call expressions.
class ASTCallExprNode : public ASTNode {
  public:
    // This member represents the expression that will be called.
    ASTNode *callee;

    // This member is a list of all of the arguments in the call expression.
    std::vector<ASTNode *> args;

    ASTCallExprNode(ASTNode *callee, std::vector<ASTNode *> args,
                    Source::Span loc)
        : ASTNode{loc}, callee{callee}, args{std::move(args)} {}

    ASTCallExprNode(ASTNode *callee, Source::Span loc)
        : ASTNode{loc}, callee{callee} {}

    virtual auto pretty_print(FILE *result_file, int level) -> void override;
};

class ASTIndexSliceExprNode : public ASTNode {
  public:
    // This member represents the expression that will be sliced.
    ASTNode *slicee;

    // This member represents the expression that will be evaluated to determine
    // the index of slicing.
    ASTNode *index_expr;

    ASTIndexSliceExprNode(ASTNode *slicee, ASTNode *index_expr,
                          Source::Span loc)
        : ASTNode{loc}, slicee{slicee}, index_expr{index_expr} {}

    virtual auto pretty_print(FILE *result_file, int level) -> void override;
};

class ASTProperSliceExprNode : public ASTNode {
  public:
    ASTNode *slicee;

    ASTNode *lower_bound, *upper_bound;

    ASTProperSliceExprNode(ASTNode *slicee, ASTNode *lower_bound,
                           ASTNode *upper_bound, Source::Span loc)
        : ASTNode{loc}, slicee{slicee}, lower_bound{lower_bound},
          upper_bound{upper_bound} {}
    
    virtual auto pretty_print(FILE *result_file, int level) -> void override;
};

} // namespace tpy::Tree

#endif