/*
    This file defines a NewLineChar object for holding information about newline
   characters.
*/

#ifndef TPY_SOURCE_NEWLINECHAR
#define TPY_SOURCE_NEWLINECHAR

namespace tpy::Source {
class NewLineChar {
  public:
    size_t pos;
    int len;

    NewLineChar(size_t pos, int len) : pos{pos}, len{len} {}
};
} // namespace tpy::Source

#endif