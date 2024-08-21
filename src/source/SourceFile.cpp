/*
    This file implements the source file object that will contain all relevant
   data regarding a Python source file.
*/

#include <optional>

#include "tpy/source/SourceFile.h"
#include "tpy/utility/Unicode.h"

namespace tpy::Source {
/*
    This method will take an integer position within the source file and convert
   it into a user-friendly location. This is primarily used for error reporting.
*/
auto SourceFile::get_loc_from_pos(size_t pos) -> SourceLocation {
    // First, we need to compute the line number.
    auto line_no = get_line_no_from_pos(pos);

    // Now, we can compute the column number.
    auto col_no = get_col_no_from_pos(pos, line_no);

    return SourceLocation{path, line_no, col_no};
}

/*
    This method will take a position and get the line number. We will use an
   upper bound binary search here in order to obtain the current line number.
*/
auto SourceFile::get_line_no_from_pos(size_t pos) -> size_t {
    // If there are no newline characters, we only have one line.
    if (line_map.empty()) {
        return 1;
    }

    // Otherwise, we need to find the first new line character ahead of the
    // desired position. We will use an lower bound form of the binary search
    // for this.
    int low = 0, high = line_map.size() - 1;
    int result = -1;

    while (low <= high) {
        auto mid = low + (high - low) / 2;

        if (line_map[mid].pos >= pos) {
            result = mid;
            high = mid - 1;
        } else {
            low = mid + 1;
        }
    }

    // If the result has a value, the line number is one more than the index of
    // the first greater newline. Otherwise, it is the last line, which means
    // one more than the size of the line map.
    return result > -1 ? result + 1 : line_map.size() + 1;
}

/*
    This method will get the column number from the position.
    Here, we will have to use check for unicode codepoints in order to return
   the right column number to the user.
*/
auto SourceFile::get_col_no_from_pos(size_t pos, size_t line_no) -> size_t {
    // First, we need to get the starting position of the line that we computed.
    // If we are on the first line, we know that the start is the start of the
    // buffer. Otherwise, the start of the line is after the preceding newline
    // character.
    uint8_t *line_start;
    if (line_no == 1) {
        line_start = reinterpret_cast<uint8_t *>(buffer->str());
    } else {
        auto &preceding_newline_char = line_map[line_no - 2];
        line_start = reinterpret_cast<uint8_t *>(buffer->data()) +
                     preceding_newline_char.pos + preceding_newline_char.len;
    }

    auto *pos_start = reinterpret_cast<uint8_t *>(buffer->data()) + pos;
    auto *hard_end = reinterpret_cast<uint8_t *>(buffer->abs_end());

    // Now, we need to iterate over the buffer, looking for any unicode
    // codepoints.
    auto *ptr = line_start;
    size_t col_no = 1;

    // We will iterate through the
    while (ptr < pos_start) {
        if (ptr[0] < 0x80) {
            ++ptr;
        } else {
            Utility::Unicode::decode_utf8_sequence(&ptr, hard_end);
        }

        ++col_no;
    }

    return col_no;
}
} // namespace tpy::Source