#include "sim/allocator.h"

#include <algorithm>
#include <stdexcept>

namespace sim {

// ----------- Helpers -----------

uint64_t HeapAllocator::align_up(uint64_t n) noexcept {
    return (n + ALIGNMENT - 1) & ~(ALIGNMENT - 1);
}
// using first fit, can easily be changed into worst fit and best fit, but a lil harder to change into slab or buddy
// ----------- Constructor -----------

HeapAllocator::HeapAllocator(uint64_t heap_base, uint64_t heap_size)
    : heap_base_(heap_base),
      heap_size_(heap_size),
      allocated_bytes_(0),
      internal_frag_bytes_(0) {

    Block initial;
    initial.start = heap_base_;
    initial.size  = heap_size_;

    free_blocks_[heap_base_] = initial;
}

// ----------- Allocation (First-Fit) -----------

std::optional<uint64_t> HeapAllocator::alloc(uint64_t size) {
    if (size == 0)
        return std::nullopt;

    uint64_t aligned = align_up(size);
    uint64_t frag = aligned - size;

    for (auto it = free_blocks_.begin(); it != free_blocks_.end(); ++it) {
        Block free = it->second;

        if (free.size >= aligned) {
            uint64_t addr = free.start;

            // Remove old free block
            free_blocks_.erase(it);

            // Insert remaining free block (if any)
            if (free.size > aligned) {
                Block remainder;
                remainder.start = free.start + aligned;
                remainder.size  = free.size - aligned;
                free_blocks_[remainder.start] = remainder;
            }

            // Record allocated block
            Block alloc_block;
            alloc_block.start = addr;
            alloc_block.size  = aligned;
            allocated_blocks_[addr] = alloc_block;

            allocated_bytes_ += aligned;
            internal_frag_bytes_ += frag;

            return addr;
        }
    }

    return std::nullopt;
}

// ----------- Free -----------

void HeapAllocator::free(uint64_t addr) {
    auto it = allocated_blocks_.find(addr);
    if (it == allocated_blocks_.end())
        throw std::runtime_error("HeapAllocator::free invalid address");

    Block blk = it->second;

    allocated_bytes_ -= blk.size;

    // Internal fragmentation is reclaimed
    // (we conservatively subtract full block size alignment waste)
    // This matches how we accounted it during alloc.
    internal_frag_bytes_ -= (blk.size - align_up(blk.size));

    allocated_blocks_.erase(it);

    // Insert block into free list
    free_blocks_[blk.start] = blk;

    // ----------- Coalescing -----------

    auto curr = free_blocks_.find(blk.start);
    // Coalesce with previous block
    if (curr != free_blocks_.begin()) {
        auto prev = std::prev(curr);
        if (prev->second.start + prev->second.size == curr->second.start) {
            prev->second.size += curr->second.size;
            free_blocks_.erase(curr);
            curr = prev;
        }
    }

    // Coalesce with next block
    auto next = std::next(curr);
    if (next != free_blocks_.end()) {
        if (curr->second.start + curr->second.size == next->second.start) {
            curr->second.size += next->second.size;
            free_blocks_.erase(next);
        }
    }
}
// code is heavily unoptimized, there are instances where different features of c++ could be used, but this is a really basic C to C++ translation, cuz I got tired of writing C code
// Forgot to mention most of the code is AI generated, but hey It works well.

// ----------- Metrics -----------
// Internal Frag calculates frag inside VMA (like gaps inbetween heap and stack, such things) and is independent of page or other fragmentations.
uint64_t HeapAllocator::total_heap_size() const noexcept {
    return heap_size_;
}

uint64_t HeapAllocator::allocated_bytes() const noexcept {
    return allocated_bytes_;
}

uint64_t HeapAllocator::free_bytes() const noexcept {
    return heap_size_ - allocated_bytes_;
}

uint64_t HeapAllocator::largest_free_block() const noexcept {
    uint64_t max = 0;
    for (const auto &kv : free_blocks_) {
        max = std::max(max, kv.second.size);
    }
    return max;
}

uint64_t HeapAllocator::internal_fragmentation() const noexcept {
    return internal_frag_bytes_;
}

double HeapAllocator::external_fragmentation() const noexcept {
    uint64_t free = free_bytes();
    if (free == 0)
        return 0.0;

    double largest = static_cast<double>(largest_free_block());
    return 1.0 - (largest / static_cast<double>(free));
}

} // namespace sim
