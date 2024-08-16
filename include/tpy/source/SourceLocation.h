/*
    This file defines a source location object that will contain the line,
   column, and path of a position within the source map.
*/
#ifndef TPY_SOURCE_SOURCELOCATION
#define TPY_SOURCE_SOURCELOCATION

#include <string>

namespace tpy::Source {
class SourceLocation {
  public:
    const std::string &path;
    size_t line, col;

    SourceLocation(const std::string &path, size_t line, size_t col)
        : path{path}, line{line}, col{col} {}
};
} // namespace tpy::Source

#endif