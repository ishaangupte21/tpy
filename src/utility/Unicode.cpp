/*
    This file implements the interface for various unicode operations.
*/

#include <array>
#include <stdexcept>

#include "tpy/utility/Unicode.h"

namespace tpy::Utility {

/*
   This implementation of UTF-8 decoding is based off of
   https://github.com/google/cel-cpp/internal/utf8.cc, which itself is based off
   of
   https://go.googlesource.com/go/+/refs/heads/master/src/unicode/utf8/utf8.go.
*/

constexpr uint8_t SELF = 0x80;
constexpr uint8_t LOW = 0x80;
constexpr uint8_t HIGH = 0xbf;
constexpr uint8_t MASKX = 0x3f;
constexpr uint8_t MASK2 = 0x1f;
constexpr uint8_t MASK3 = 0xf;
constexpr uint8_t MASK4 = 0x7;
constexpr uint8_t TX = 0x80;
constexpr uint8_t T2 = 0xc0;
constexpr uint8_t T3 = 0xe0;
constexpr uint8_t T4 = 0xf0;
constexpr uint8_t XX = 0xf1;
constexpr uint8_t AS = 0xf0;
constexpr uint8_t S1 = 0x02;
constexpr uint8_t S2 = 0x13;
constexpr uint8_t S3 = 0x03;
constexpr uint8_t S4 = 0x23;
constexpr uint8_t S5 = 0x34;
constexpr uint8_t S6 = 0x04;
constexpr uint8_t S7 = 0x44;

constexpr std::array<uint8_t, 256> leading_lookup = {{
    //   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F
    AS, AS, AS, AS, AS, AS, AS, AS, AS, AS, AS, AS, AS, AS, AS, AS, // 0x00-0x0F
    AS, AS, AS, AS, AS, AS, AS, AS, AS, AS, AS, AS, AS, AS, AS, AS, // 0x10-0x1F
    AS, AS, AS, AS, AS, AS, AS, AS, AS, AS, AS, AS, AS, AS, AS, AS, // 0x20-0x2F
    AS, AS, AS, AS, AS, AS, AS, AS, AS, AS, AS, AS, AS, AS, AS, AS, // 0x30-0x3F
    AS, AS, AS, AS, AS, AS, AS, AS, AS, AS, AS, AS, AS, AS, AS, AS, // 0x40-0x4F
    AS, AS, AS, AS, AS, AS, AS, AS, AS, AS, AS, AS, AS, AS, AS, AS, // 0x50-0x5F
    AS, AS, AS, AS, AS, AS, AS, AS, AS, AS, AS, AS, AS, AS, AS, AS, // 0x60-0x6F
    AS, AS, AS, AS, AS, AS, AS, AS, AS, AS, AS, AS, AS, AS, AS, AS, // 0x70-0x7F
    //   1   2   3   4   5   6   7   8   9   A   B   C   D   E   F
    XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, // 0x80-0x8F
    XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, // 0x90-0x9F
    XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, // 0xA0-0xAF
    XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, // 0xB0-0xBF
    XX, XX, S1, S1, S1, S1, S1, S1, S1, S1, S1, S1, S1, S1, S1, S1, // 0xC0-0xCF
    S1, S1, S1, S1, S1, S1, S1, S1, S1, S1, S1, S1, S1, S1, S1, S1, // 0xD0-0xDF
    S2, S3, S3, S3, S3, S3, S3, S3, S3, S3, S3, S3, S3, S4, S3, S3, // 0xE0-0xEF
    S5, S6, S6, S6, S7, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, XX, // 0xF0-0xFF
}};

constexpr std::array<std::pair<uint8_t, uint8_t>, 16> accept_lookup = {{
    {LOW, HIGH},
    {0xa0, HIGH},
    {LOW, 0x9f},
    {0x90, HIGH},
    {LOW, 0x8f},
    {0x0, 0x0},
    {0x0, 0x0},
    {0x0, 0x0},
    {0x0, 0x0},
    {0x0, 0x0},
    {0x0, 0x0},
    {0x0, 0x0},
    {0x0, 0x0},
    {0x0, 0x0},
    {0x0, 0x0},
    {0x0, 0x0},
}};

/*
    This method will take a sequence of UTF-8 characters and return a decoded
   UTF-32 codepoint. In the case of invalid UTF-8 sequences, we will throw an
   exception that will be handled at the compiler driver level as we cannot
   recover from that error.
*/
auto Unicode::decode_utf8_sequence(uint8_t **start, uint8_t *end) -> uint32_t {
    auto b = **start;
    ++(*start);

    if (b < SELF) {
        return static_cast<uint32_t>(b);
    }

    auto leading = leading_lookup[b];
    if (leading == XX) {
        throw std::runtime_error{"malformed UTF-8 input."};
    }

    auto size = static_cast<size_t>(leading & 7) - 1;
    if (size > end - *start) {
        throw std::runtime_error{"malformed UTF-8 input."};
    }

    auto &accept = accept_lookup[leading >> 4];
    auto b1 = **start;
    ++(*start);

    if (b1 < accept.first || b1 > accept.second) {
        throw std::runtime_error{"malformed UTF-8 input."};
    }

    if (size <= 1) {
        return (static_cast<uint32_t>(b & MASK2) << 6) |
               static_cast<uint32_t>(b1 & MASKX);
    }

    auto b2 = **start;
    ++(*start);

    if (b2 < LOW || b2 > HIGH) {
        throw std::runtime_error{"malformed UTF-8 input."};
    }

    if (size <= 2) {
        return (static_cast<uint32_t>(b & MASK3) << 12) |
               (static_cast<uint32_t>(b1 & MASKX) << 6) |
               static_cast<uint32_t>(b2 & MASKX);
    }

    auto b3 = **start;
    ++(*start);

    if (b3 < LOW || b3 > HIGH) {
        throw std::runtime_error{"malformed UTF-8 input."};
    }

    return (static_cast<char32_t>(b & MASK4) << 18) |
           (static_cast<char32_t>(b1 & MASKX) << 12) |
           (static_cast<char32_t>(b2 & MASKX) << 6) |
           static_cast<char32_t>(b3 & MASKX);
}

} // namespace tpy::Utility