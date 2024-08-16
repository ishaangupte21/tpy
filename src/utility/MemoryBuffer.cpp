/*
    This file implements a generic memory buffer than can be used for a wide
   variety of things. This is inspired by the LLVM class of the same name.
*/

#include "tpy/utility/MemoryBuffer.h"

#include <fcntl.h>
#include <stdexcept>
#include <sys/stat.h>

// OS Specific headers
#ifdef _WIN32
#include <io.h>

#define READ_TYPE_FLAG (_O_BINARY)

#else
#include <sys/mmap.h>
#include <unistd.h>

#define READ_TYPE_FLAG (O_RDONLY)

#endif

namespace tpy::Utility {
/*
    This method will open the file into a buffer.
    If the file is larger than 16384 bytes, we will map the file into a buffer.
    For now, however, memory mapping will only be supposed on POSIX based
   systems.
*/
auto MemoryBuffer::create_buffer_from_file(char *file_path)
    -> std::unique_ptr<MemoryBuffer> {
    // First, we must open the file.
    errno = 0;
    int fd = open(file_path, READ_TYPE_FLAG);
    if (fd == -1) {
        throw std::runtime_error{strerror(errno)};
    }

    // Now, we need to get the file information from the OS.
    struct stat file_stat {};

    errno = 0;
    if (fstat(fd, &file_stat) == -1) {
        throw std::runtime_error{strerror(errno)};
    }

    size_t file_size = file_stat.st_size;

#ifndef _WIN32
    // If the file is larger than 16384 bytes, we must map it.
    if (file_size > 16384) {
        errno = 0;
        std::byte *buffer = reinterpret_cast<std::byte *>(
            mmap(nullptr, file_size + 1, PROT_READ, MAP_SHARED, fd, 0));

        if (buffer == MAP_FAILED) {
            throw std::runtime_exception{strerror(errno)};
        }

        buffer[file_size] = std::byte{0};

        return std::make_unique<MemoryBuffer>(buffer, file_size + 1, true);
    }
#endif

    // Otherwise, read it normally using chunks.
    constexpr int CHUNK_SIZE = 1024;
    auto buffer = new std::byte[file_size + 1];

    size_t offset = 0;
    while (true) {
        size_t bytes_read = read(fd, buffer + offset, CHUNK_SIZE);
        if (!bytes_read) {
            break;
        }

        offset += bytes_read;
    }

    buffer[file_size] = std::byte{0};

    // Once we're done reading, we can close the file.
    close(fd);

    return std::make_unique<MemoryBuffer>(buffer, file_size + 1, false);
}
} // namespace tpy::Utility