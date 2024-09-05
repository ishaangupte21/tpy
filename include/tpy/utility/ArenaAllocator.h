/*
    This file defines a generic arena allocator.
    It will be used for things like the AST and source file mapping.
*/
#ifndef TPY_UTILITY_ARENAALLOCATOR
#define TPY_UTILITY_ARENAALLOCATOR

#include <forward_list>
#include <memory>
#include <new>

namespace tpy::Utility {
/*
    This is a simple arena allocator that uses the default C++ allocator under
   the hood.
*/
class ArenaAllocator {
    using ArenaSlab = std::unique_ptr<std::byte[]>;

    // In order to allow easily expanding the amount of slabs, we will use a
    // singly linked list.
    std::forward_list<ArenaSlab> slabs;

    // We also need 2 pointers: one to the current point in the current slab,
    // and one to the end of the current slab.
    std::byte *current_pos, *end_of_current_slab;

    // This is the size of each slab in the allocator.
    size_t slab_size;

    /*
        This method will allocate a new slab of memory and add it to the list of
       slabs.
    */
    auto create_new_slab() -> void;

    // This method provides the default size of a memory slab.
    static constexpr auto GET_DEFAULT_SLAB_SIZE() -> size_t { return 4096; }

  public:
    explicit ArenaAllocator(size_t slab_size) : slab_size{slab_size} {
        create_new_slab();
    }

    ArenaAllocator() : ArenaAllocator(GET_DEFAULT_SLAB_SIZE()) {}

    /*
        This method will instantiate the given object within the arena. This
       needs to have the function body in the header file to resolve linker
       issues that come with template instantiation. If it becomes an issue, we
       can resort to explicit instantiation and move it back to the
       'ArenaAllocator.cpp' file.
    */
    template <class T, typename... Args> auto allocate(Args &&...args) -> T * {
        // First, we need to obtain the memory.
        // If the current slab doesn't have enough, we need to allocate another
        // one.
        if (end_of_current_slab - current_pos < sizeof(T)) {
            create_new_slab();
        }
        auto mem = current_pos;
        current_pos += sizeof(T);

        // Now, we can instantiate the object at that memory location.
        return new (mem) T(std::forward<Args>(args)...);
    }
};
} // namespace tpy::Utility

#endif