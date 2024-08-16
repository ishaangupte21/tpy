/*
    This file defines an interface for various unicode operations.
*/

#ifndef TPY_UTILITY_UNICODE
#define TPY_UTILITY_UNICODE

#include <cstdint>

namespace tpy::Utility {
class Unicode {

  public:
    // This method will take a sequence of UTF-8 characters and return a decoded
    // UTF-32 codepoint.
    static auto decode_utf8_sequence(uint8_t **start, uint8_t *end) -> uint32_t;
};
} // namespace tpy::Utility

#endif