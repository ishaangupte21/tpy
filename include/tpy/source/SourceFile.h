/*
    This file defines the source file object that will contain all relevant data
   regarding a Python source file.
*/
#ifndef TPY_SOURCE_SOURCEFILE
#define TPY_SOURCE_SOURCEFILE

#include <string>
#include <tuple>
#include <vector>

#include "tpy/source/NewLineChar.h"
#include "tpy/source/SourceLocation.h"
#include "tpy/utility/MemoryBuffer.h"

namespace tpy::Source {
/*
    This object contains all metadata relating to a source file.
*/
class SourceFile {

    auto get_line_no_from_pos(size_t pos) -> size_t;

    auto get_col_no_from_pos(size_t pos, size_t line_no) -> size_t;

  public:
    std::string path;

    size_t offset;

    std::unique_ptr<Utility::MemoryBuffer> buffer;

    std::vector<NewLineChar> line_map;

    SourceFile(char *path, size_t offset,
               std::unique_ptr<Utility::MemoryBuffer> buffer,
               std::vector<NewLineChar> line_map)
        : path{path}, offset{offset}, buffer{std::move(buffer)},
          line_map{std::move(line_map)} {}

    auto size() -> size_t { return buffer->get_size(); }

    auto start() -> char * { return buffer->str(); }

    auto end() -> char * { return buffer->char_end(); }

    auto get_loc_from_pos(size_t pos) -> SourceLocation;
};
} // namespace tpy::Source

#endif