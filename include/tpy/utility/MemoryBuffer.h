/*
    This file defines a generic memory buffer than can be used for a wide
   variety of things. This is inspired by the LLVM class of the same name.
*/
#ifndef TPY_UTILITY_MEMORYBUFFER
#define TPY_UTILITY_MEMORYBUFFER

#include <memory>

namespace tpy::Utility {
/*
    This is a generic memory buffer that is read/write.
*/
class MemoryBuffer {
    // This is the buffer that backs the object.
    std::byte *buffer;

    // This contains a string start point for the buffer.
    // This is processed with the UTF-8 BOM removed.
    char *str_start;

    // This is the size of the buffer.
    size_t size;

    // This is the length of the contents. Usually, it will be 1 less than the
    // buffer size.
    size_t contents_length;

    // We need to know if the buffer is mapped or allocated using malloc.
    bool is_mapped;

  public:
    MemoryBuffer(std::byte *buffer, size_t size, bool is_mapped)
        : buffer{buffer}, size{size}, contents_length{size - 1},
          is_mapped{is_mapped} {
        // When we construct this object, we must check for the UTF-8 BOM and
        // set the str_start pointer accordingly.
        auto unsigned_buffer = reinterpret_cast<uint8_t *>(buffer);
        if (unsigned_buffer[0] == 0xef && unsigned_buffer[1] == 0xbb &&
            unsigned_buffer[2] == 0xbf) {
            str_start = reinterpret_cast<char *>(buffer) + 3;
        } else {
            str_start = reinterpret_cast<char *>(buffer);
        }
    }

    MemoryBuffer(size_t size, bool is_mapped)
        : MemoryBuffer(new std::byte[size], size, is_mapped) {}

    static auto
    create_empty_buffer(size_t size) -> std::unique_ptr<MemoryBuffer> {
        return std::make_unique<MemoryBuffer>(size, false);
    }

    static auto
    create_buffer_from_file(char *) -> std::unique_ptr<MemoryBuffer>;

    auto data() const -> std::byte * { return buffer; }

    auto str() const -> char * { return str_start; }

    auto get_size() const -> size_t { return contents_length; }

    auto buffer_size() const -> size_t { return size; }

    auto end() const -> std::byte * { return data() + contents_length; }

    auto abs_end() const -> std::byte * { return data() + size; }

    auto char_end() const -> char * { return str() + size; }

    ~MemoryBuffer();
};

} // namespace tpy::Utility

#endif