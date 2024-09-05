/*
    This file implements a generic arena allocator.
*/
#include "tpy/utility/ArenaAllocator.h"

namespace tpy::Utility {
/*
    This method will add a new slab of memory to the list held by the arena
   allocator.
*/
auto ArenaAllocator::create_new_slab() -> void {
    // First, we need to allocate a new slab.
    auto new_slab = std::make_unique<std::byte[]>(slab_size);

    // Now, set the start and end pointers and insert the slab.
    current_pos = new_slab.get();
    end_of_current_slab = current_pos + slab_size;

    slabs.push_front(std::move(new_slab));
}
} // namespace tpy::Utility