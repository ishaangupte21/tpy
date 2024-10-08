/*
    This file implements the ASTNode subclasses that will be used in the AST.
*/

#include "tpy/tree/ASTExpr.h"
#include "tpy/parse/Token.h"

namespace tpy::Tree {

/*
    The following pretty_print functions will dump the AST out in a
   human-readable format.
*/

auto ASTIntLiteralNode::pretty_print(FILE *result_file, int level) -> void {
    // Indentation space based on the level.
    // 4 spaces per level.
    for (int i = 0; i < level; i++) {
        putc(' ', result_file);
    }
    fputs("{\n", result_file);

    // Now, we need to indent to level + 1.
    for (int i = 0; i < level + 1; i++) {
        putc(' ', result_file);
    }

    // Node kind.
    fputs("kind: ASTIntLiteralNode\n", result_file);

    for (int i = 0; i < level + 1; i++) {
        putc(' ', result_file);
    }
    fprintf(result_file, "base: %d\n", base);

    // Now, we need to indent to level + 1.
    for (int i = 0; i < level + 1; i++) {
        putc(' ', result_file);
    }
    fprintf(result_file, "start: %llu\n", loc.local_pos);

    // Now, we need to indent to level + 1.
    for (int i = 0; i < level + 1; i++) {
        putc(' ', result_file);
    }
    fprintf(result_file, "end: %llu\n", loc.local_end());
    // Indentation space based on the level.
    // 4 spaces per level.
    for (int i = 0; i < level; i++) {
        putc(' ', result_file);
    }

    fputs("}\n", result_file);
}

auto ASTFloatLiteralNode::pretty_print(FILE *result_file, int level) -> void {
    // Indentation space based on the level.
    // 4 spaces per level.
    for (int i = 0; i < level; i++) {
        putc(' ', result_file);
    }
    fputs("{\n", result_file);

    // Now, we need to indent to level + 1.
    for (int i = 0; i < level + 1; i++) {
        putc(' ', result_file);
    }

    // Node kind.
    fputs("kind: ASTFloatLiteralNode\n", result_file);

    // Now, we need to indent to level + 1.
    for (int i = 0; i < level + 1; i++) {
        putc(' ', result_file);
    }
    fprintf(result_file, "start: %llu\n", loc.local_pos);

    // Now, we need to indent to level + 1.
    for (int i = 0; i < level + 1; i++) {
        putc(' ', result_file);
    }
    fprintf(result_file, "end: %llu\n", loc.local_end());
    // Indentation space based on the level.
    // 4 spaces per level.
    for (int i = 0; i < level; i++) {
        putc(' ', result_file);
    }

    fputs("}\n", result_file);
}

auto ASTStringLiteralNode::pretty_print(FILE *result_file, int level) -> void {
    // Indentation space based on the level.
    // 4 spaces per level.
    for (int i = 0; i < level; i++) {
        putc(' ', result_file);
    }
    fputs("{\n", result_file);

    // Now, we need to indent to level + 1.
    for (int i = 0; i < level + 1; i++) {
        putc(' ', result_file);
    }

    // Node kind.
    fputs("kind: ASTStringLiteralNode\n", result_file);

    // Now, we need to indent to level + 1.
    for (int i = 0; i < level + 1; i++) {
        putc(' ', result_file);
    }
    fprintf(result_file, "start: %llu\n", loc.local_pos);

    // Now, we need to indent to level + 1.
    for (int i = 0; i < level + 1; i++) {
        putc(' ', result_file);
    }
    fprintf(result_file, "end: %llu\n", loc.local_end());
    // Indentation space based on the level.
    // 4 spaces per level.
    for (int i = 0; i < level; i++) {
        putc(' ', result_file);
    }

    fputs("}\n", result_file);
}

auto ASTBoolLiteralNode::pretty_print(FILE *result_file, int level) -> void {
    // Indentation space based on the level.
    // 4 spaces per level.
    for (int i = 0; i < level; i++) {
        putc(' ', result_file);
    }
    fputs("{\n", result_file);

    // Now, we need to indent to level + 1.
    for (int i = 0; i < level + 1; i++) {
        putc(' ', result_file);
    }

    // Node kind.
    fputs("kind: ASTBoolLiteralNode\n", result_file);

    for (int i = 0; i < level + 1; i++) {
        putc(' ', result_file);
    }
    fprintf(result_file, "val: %s\n", val ? "True" : "False");

    // Now, we need to indent to level + 1.
    for (int i = 0; i < level + 1; i++) {
        putc(' ', result_file);
    }
    fprintf(result_file, "start: %llu\n", loc.local_pos);

    // Now, we need to indent to level + 1.
    for (int i = 0; i < level + 1; i++) {
        putc(' ', result_file);
    }
    fprintf(result_file, "end: %llu\n", loc.local_end());
    // Indentation space based on the level.
    // 4 spaces per level.
    for (int i = 0; i < level; i++) {
        putc(' ', result_file);
    }

    fputs("}\n", result_file);
}

auto ASTParenExprNode::pretty_print(FILE *result_file, int level) -> void {
    // Indentation space based on the level.
    // 4 spaces per level.
    for (int i = 0; i < level; i++) {
        putc(' ', result_file);
    }
    fputs("{\n", result_file);

    // Now, we need to indent to level + 1.
    for (int i = 0; i < level + 1; i++) {
        putc(' ', result_file);
    }

    // Node kind.
    fputs("kind: ASTParenExprNode\n", result_file);

    // Now, print the inner expression.

    // Now, we need to indent to level + 1.
    for (int i = 0; i < level + 1; i++) {
        putc(' ', result_file);
    }
    fputs("expr:", result_file);

    inner_expr->pretty_print(result_file, level + 1);

    // Indentation space based on the level.
    // 4 spaces per level.
    for (int i = 0; i < level; i++) {
        putc(' ', result_file);
    }

    fputs("}\n", result_file);
}

auto ASTListExprNode::pretty_print(FILE *result_file, int level) -> void {
    // Indentation space based on the level.
    // 4 spaces per level.
    for (int i = 0; i < level; i++) {
        putc(' ', result_file);
    }
    fputs("{\n", result_file);

    // Now, we need to indent to level + 1.
    for (int i = 0; i < level + 1; i++) {
        putc(' ', result_file);
    }

    // Node kind.
    fputs("kind: ASTListExprNode\n", result_file);

    // Now, we need to indent to level + 1.
    for (int i = 0; i < level + 1; i++) {
        putc(' ', result_file);
    }
    fputs("[\n", result_file);

    for (auto &x : list) {
        x->pretty_print(result_file, level + 2);
    }

    // Now, we need to indent to level + 1.
    for (int i = 0; i < level + 1; i++) {
        putc(' ', result_file);
    }
    fputs("]\n", result_file);

    for (int i = 0; i < level; i++) {
        putc(' ', result_file);
    }
    fputs("}\n", result_file);
}

auto ASTSetExprNode::pretty_print(FILE *result_file, int level) -> void {
    // Indentation space based on the level.
    // 4 spaces per level.
    for (int i = 0; i < level; i++) {
        putc(' ', result_file);
    }
    fputs("{\n", result_file);

    // Now, we need to indent to level + 1.
    for (int i = 0; i < level + 1; i++) {
        putc(' ', result_file);
    }

    // Node kind.
    fputs("kind: ASTSetExprNode\n", result_file);

    // Now, we need to indent to level + 1.
    for (int i = 0; i < level + 1; i++) {
        putc(' ', result_file);
    }
    fputs("[\n", result_file);

    for (auto x : contents) {
        x->pretty_print(result_file, level + 2);
    }

    // Now, we need to indent to level + 1.
    for (int i = 0; i < level + 1; i++) {
        putc(' ', result_file);
    }
    fputs("]\n", result_file);

    for (int i = 0; i < level; i++) {
        putc(' ', result_file);
    }
    fputs("}\n", result_file);
}

auto ASTDictExprNode::pretty_print(FILE *result_file, int level) -> void {
    // Indentation space based on the level.
    // 4 spaces per level.
    for (int i = 0; i < level; i++) {
        putc(' ', result_file);
    }
    fputs("{\n", result_file);

    // Now, we need to indent to level + 1.
    for (int i = 0; i < level + 1; i++) {
        putc(' ', result_file);
    }

    // Node kind.
    fputs("kind: ASTDictExprNode\n", result_file);

    // Now, we need to indent to level + 1.
    for (int i = 0; i < level + 1; i++) {
        putc(' ', result_file);
    }
    fputs("[\n", result_file);

    for (auto &x : contents) {
        x.first->pretty_print(result_file, level + 2);
        x.second->pretty_print(result_file, level + 2);
    }

    // Now, we need to indent to level + 1.
    for (int i = 0; i < level + 1; i++) {
        putc(' ', result_file);
    }
    fputs("]\n", result_file);

    for (int i = 0; i < level; i++) {
        putc(' ', result_file);
    }
    fputs("}\n", result_file);
}

auto ASTNameExprNode::pretty_print(FILE *result_file, int level) -> void {
    // Indentation space based on the level.
    // 4 spaces per level.
    for (int i = 0; i < level; i++) {
        putc(' ', result_file);
    }
    fputs("{\n", result_file);

    // Now, we need to indent to level + 1.
    for (int i = 0; i < level + 1; i++) {
        putc(' ', result_file);
    }

    // Node kind.
    fputs("kind: ASTNameExprNode\n", result_file);

    // Now, we need to indent to level + 1.
    for (int i = 0; i < level + 1; i++) {
        putc(' ', result_file);
    }
    fprintf(result_file, "start: %llu\n", loc.local_pos);

    // Now, we need to indent to level + 1.
    for (int i = 0; i < level + 1; i++) {
        putc(' ', result_file);
    }
    fprintf(result_file, "end: %llu\n", loc.local_end());
    // Indentation space based on the level.
    // 4 spaces per level.
    for (int i = 0; i < level; i++) {
        putc(' ', result_file);
    }

    fputs("}\n", result_file);
}

auto ASTAttrRefExprNode::pretty_print(FILE *result_file, int level) -> void {
    // Indentation space based on the level.
    // 4 spaces per level.
    for (int i = 0; i < level; i++) {
        putc(' ', result_file);
    }
    fputs("{\n", result_file);

    // Now, we need to indent to level + 1.
    for (int i = 0; i < level + 1; i++) {
        putc(' ', result_file);
    }

    // Node kind.
    fputs("kind: ASTAttrRefExprNode\n", result_file);
    lhs->pretty_print(result_file, level + 2);
    rhs->pretty_print(result_file, level + 2);

    // Indentation space based on the level.
    // 4 spaces per level.
    for (int i = 0; i < level; i++) {
        putc(' ', result_file);
    }

    fputs("}\n", result_file);
}

auto ASTCallExprNode::pretty_print(FILE *result_file, int level) -> void {
    // Indentation space based on the level.
    // 4 spaces per level.
    for (int i = 0; i < level; i++) {
        putc(' ', result_file);
    }
    fputs("{\n", result_file);

    // Now, we need to indent to level + 1.
    for (int i = 0; i < level + 1; i++) {
        putc(' ', result_file);
    }

    // Node kind.
    fputs("kind: ASTCallExprNode\n", result_file);

    callee->pretty_print(result_file, level + 2);

    // Now, we need to indent to level + 1.
    for (int i = 0; i < level + 1; i++) {
        putc(' ', result_file);
    }
    fputs("[\n", result_file);

    for (auto x : args) {
        x->pretty_print(result_file, level + 2);
    }

    // Now, we need to indent to level + 1.
    for (int i = 0; i < level + 1; i++) {
        putc(' ', result_file);
    }
    fputs("]\n", result_file);

    // Indentation space based on the level.
    // 4 spaces per level.
    for (int i = 0; i < level; i++) {
        putc(' ', result_file);
    }

    fputs("}\n", result_file);
}

auto ASTIndexSliceExprNode::pretty_print(FILE *result_file, int level) -> void {
    // Indentation space based on the level.
    // 4 spaces per level.
    for (int i = 0; i < level; i++) {
        putc(' ', result_file);
    }
    fputs("{\n", result_file);

    // Now, we need to indent to level + 1.
    for (int i = 0; i < level + 1; i++) {
        putc(' ', result_file);
    }

    // Node kind.
    fputs("kind: ASTIndexSliceExprNode\n", result_file);

    slicee->pretty_print(result_file, level + 2);
    index_expr->pretty_print(result_file, level + 2);

    // Indentation space based on the level.
    // 4 spaces per level.
    for (int i = 0; i < level; i++) {
        putc(' ', result_file);
    }

    fputs("}\n", result_file);
}

auto ASTProperSliceExprNode::pretty_print(FILE *result_file,
                                          int level) -> void {
    // Indentation space based on the level.
    // 4 spaces per level.
    for (int i = 0; i < level; i++) {
        putc(' ', result_file);
    }
    fputs("{\n", result_file);

    // Now, we need to indent to level + 1.
    for (int i = 0; i < level + 1; i++) {
        putc(' ', result_file);
    }

    // Node kind.
    fputs("kind: ASTProperSliceExprNode\n", result_file);

    slicee->pretty_print(result_file, level + 2);
    if (lower_bound) {
        lower_bound->pretty_print(result_file, level + 2);
    }
    if (upper_bound) {
        upper_bound->pretty_print(result_file, level + 2);
    }

    // Indentation space based on the level.
    // 4 spaces per level.
    for (int i = 0; i < level; i++) {
        putc(' ', result_file);
    }

    fputs("}\n", result_file);
}

auto ASTBinaryOpExprNode::pretty_print(FILE *result_file, int level) -> void {
    // Indentation space based on the level.
    // 4 spaces per level.
    for (int i = 0; i < level; i++) {
        putc(' ', result_file);
    }
    fputs("{\n", result_file);

    // Now, we need to indent to level + 1.
    for (int i = 0; i < level + 1; i++) {
        putc(' ', result_file);
    }

    // Node kind.
    fputs("kind: ASTBinaryOpExprNode\n", result_file);

    // Now, we need to indent to level + 2.
    for (int i = 0; i < level + 2; i++) {
        putc(' ', result_file);
    }

    fputs("op: ", result_file);
    fputs(Parse::token_names[static_cast<int>(op)], result_file);
    fputc('\n', result_file);

    lhs->pretty_print(result_file, level + 2);
    rhs->pretty_print(result_file, level + 2);

    // Indentation space based on the level.
    // 4 spaces per level.
    for (int i = 0; i < level; i++) {
        putc(' ', result_file);
    }

    fputs("}\n", result_file);
}

auto ASTUnaryOpExprNode::pretty_print(FILE *result_file, int level) -> void {
    // Indentation space based on the level.
    // 4 spaces per level.
    for (int i = 0; i < level; i++) {
        putc(' ', result_file);
    }
    fputs("{\n", result_file);

    // Now, we need to indent to level + 1.
    for (int i = 0; i < level + 1; i++) {
        putc(' ', result_file);
    }

    // Node kind.
    fputs("kind: ASTUnaryOpExprNode\n", result_file);

    // Now, we need to indent to level + 2.
    for (int i = 0; i < level + 2; i++) {
        putc(' ', result_file);
    }

    fputs("op: ", result_file);
    fputs(Parse::token_names[static_cast<int>(op)], result_file);
    fputc('\n', result_file);

    expr->pretty_print(result_file, level + 2);

    // Indentation space based on the level.
    // 4 spaces per level.
    for (int i = 0; i < level; i++) {
        putc(' ', result_file);
    }

    fputs("}\n", result_file);
}

auto ASTTernaryOpExprNode::pretty_print(FILE *result_file, int level) -> void {
    // Indentation space based on the level.
    // 4 spaces per level.
    for (int i = 0; i < level; i++) {
        putc(' ', result_file);
    }
    fputs("{\n", result_file);

    // Now, we need to indent to level + 1.
    for (int i = 0; i < level + 1; i++) {
        putc(' ', result_file);
    }

    // Node kind.
    fputs("kind: ASTTernaryOpExprNode\n", result_file);

    condition->pretty_print(result_file, level + 2);
    true_case->pretty_print(result_file, level + 2);
    false_case->pretty_print(result_file, level + 2);

    // Indentation space based on the level.
    // 4 spaces per level.
    for (int i = 0; i < level; i++) {
        putc(' ', result_file);
    }

    fputs("}\n", result_file);
}

} // namespace tpy::Tree