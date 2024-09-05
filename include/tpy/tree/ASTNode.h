/*
    This file defines the base AST Node that will be used to represent source
   within the input.
*/

#ifndef TPY_TREE_ASTNODE_H
#define TPY_TREE_ASTNODE_H

#include <cstdio>

#include "tpy/source/Span.h"

namespace tpy::Tree {
/*
    This object represents a node of the AST. It is the base class for all other
   AST nodes and contains only the basic information, which is a source
   location. The constructor is protected as this object should not be able to
   be instantiated by itself.
*/
class ASTNode {
  public:
    Source::Span loc;

    virtual ~ASTNode() {};

    // This method will "pretty-print" the AST in a human-readable format.
    virtual auto pretty_print(FILE *result_file, int level) -> void = 0;

  protected:
    explicit ASTNode(Source::Span loc) : loc{loc} {}
};
} // namespace tpy::Tree

#endif