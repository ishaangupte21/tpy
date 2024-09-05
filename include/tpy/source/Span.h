/*
    This file defines the object that represents a single span within the source
   files.
*/
#ifndef TPY_SOURCE_SPAN
#define TPY_SOURCE_SPAN

#include <cstddef>

namespace tpy::Source {
/*
    This object will contain both the local and absolute positions.
    It will be used in tokens and IRs that need to report source code errors.
*/
class Span {
  public:
    size_t local_pos, absolute_pos, len;

    Span(size_t local_pos, size_t absolute_pos, size_t len)
        : local_pos{local_pos}, absolute_pos{absolute_pos}, len{len} {}

    auto local_end() -> size_t { return local_pos + len; }

    auto absolute_end() -> size_t { return absolute_pos + len; }

    static auto empty() -> Span { return Span{0, 0, 0}; }

    // For constructing the AST, we need to be able to combine spans that
    // represent regions.
    auto operator+(const Span &rhs) -> Source::Span &{
        len = (rhs.absolute_pos + len) - absolute_pos;
        return *this;
    }
};
} // namespace tpy::Source

#endif