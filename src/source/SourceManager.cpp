/*
    This file implements the source manager that will handle the Python source
   files. This is designed to be easily scalable and extensible.
*/

#include "tpy/source/SourceManager.h"
#include "tpy/source/SourceFile.h"
#include "tpy/utility/MemoryBuffer.h"

namespace tpy::Source {
/*
    This method will open a python source file and load it into the cache held
   by the SourceManager. It will compute the starting offset for this source
   file and get the memory buffer as well as the line map.
*/
auto SourceManager::open_py_src_file(char *path)
    -> SourceFile * {
    // First, we need to get the source file as a Memory Buffer
    auto mem_buffer = Utility::MemoryBuffer::create_buffer_from_file(path);

    // Now, we need to analyze the source file for newline characters.
    auto line_map = analyze_py_src_file(mem_buffer);

    // The offset of the new source file can be computed by taking the offset of
    // the last and adding its size.
    size_t offset = 0;
    if (src_files.empty()) {
        offset = 0;
    } else {
        auto &last_src_file = src_files.back();
        offset = last_src_file->offset + last_src_file->size();
    }

    // Append the source file to the vector.
    src_files.emplace_back(new SourceFile(
        path, offset, std::move(mem_buffer), std::move(line_map)));

    // // Return the reference.
    return src_files.back();
}

/*
    This method will iterate over a source file and produce a line map. The idea
   is to know the location of each new-line character beforehand so that
   computing the source location of a position will be easier.
*/
auto SourceManager::analyze_py_src_file(
    const std::unique_ptr<Utility::MemoryBuffer> &mem_buffer)
    -> std::vector<NewLineChar> {
    // The idea here is to iterate over the entire source file, looking for
    // newline characters so that we can put them in the linemap.

    // We cannot use the str_start pointer here because that will consume the
    // UTF-8 BOM if there is one. Instead, we will use the start pointer and
    // cast it to type char.
    auto file_start = reinterpret_cast<char *>(mem_buffer->data());
    auto ptr = file_start;
    auto end = mem_buffer->char_end();

    std::vector<NewLineChar> newline_chars;

    while (ptr < end) {
        if (*ptr == '\n') {
            newline_chars.emplace_back(ptr - file_start, 1);
            ++ptr;
            continue;
        }

        // Windows style line terminators have a return carraige followed by a
        // newline. That must be handled as well.
        if (*ptr == '\r') {
            if (ptr[1] == '\n') {
                newline_chars.emplace_back(ptr - file_start, 2);
                ptr += 2;
            } else {
                newline_chars.emplace_back(ptr - file_start, 1);
                ++ptr;
            }

            continue;
        }

        ++ptr;
    }

    return newline_chars;
}

/*
    This method takes an arbitrary position and obtains the local source
   location of that position. We use an upper bound binary search to locate the
   desired source file and then pass on the local offset to the source file's
   location method.
*/
auto SourceManager::get_loc_from_pos(size_t pos) -> SourceLocation {
    // We need to use an upper bound binary search to find the first source file
    // with a position greater. The one before that is the source file with our
    // location. However, if we only have one source file, we can just use that
    // one.
    if (src_files.size() == 1) {
        // Once we have the source file, we need to compute the local offset and
        // pass that to the source file's local method.
        auto &src_file = src_files[0];
        return src_file->get_loc_from_pos(pos - src_file->offset);
    }

    int low = 0, high = src_files.size() - 1;
    int result = -1;

    while (low <= high) {
        auto mid = low + (high - low) / 2;

        if (src_files[mid]->offset <= pos) {
            low = mid + 1;
        } else {
            result = mid;
            high = mid - 1;
        }
    }

    // If the upper bound search did not yield a result, we must use the last
    // source file. Otherwise, we must use the source file of result - 1.
    auto &src_file = result > -1 ? src_files[result - 1] : src_files.back();

    return src_file->get_loc_from_pos(pos - src_file->offset);
}
} // namespace tpy::Source