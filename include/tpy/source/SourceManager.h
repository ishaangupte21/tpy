/*
    This file defines the source manager that will handle the Python source
   files. This is designed to be easily scalable and extensible.
*/
#ifndef TPY_SOURCE_SOURCEMANAGER
#define TPY_SOURCE_SOURCEMANAGER

#include <vector>

#include "SourceFile.h"
#include "SourceLocation.h"

namespace tpy::Source {
/*
    An instance of this class will contain all the necessary source file data.
*/
class SourceManager {
    // This is the list that contains the cache of all source files opened.
    std::vector<SourceFile *> src_files;

    auto analyze_py_src_file(const std::unique_ptr<Utility::MemoryBuffer>
                                 &mem_buffer) -> std::vector<NewLineChar>;

  public:
    auto open_py_src_file(char *path) -> SourceFile *;

    auto get_loc_from_pos(size_t pos) -> SourceLocation;

    ~SourceManager() {
        for (auto src_file : src_files) {
            delete src_file;
        }
    }
};

}; // namespace tpy::Source

#endif