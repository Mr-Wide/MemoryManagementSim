#include "sim/allocator.h"

#include <algorithm>
#include <stdexcept>

namespace sim {

// ----------- Helpers -----------

uint64_t HeapAllocator::align_up(uint64_t n) noexcept {
    return (n + ALIGNMENT - 1) & ~(ALIGNMENT - 1);
}

// ----------- Constructor -----------

HeapAllocator::HeapAllocator(uint64_t heap_base, uint64_t heap_size)
    : heap_base_(heap_base),
      heap_size_(heap_size),
      allocated_bytes_(0),
      internal_frag_bytes_(0) {

    free_blocks_.emplace(heap_base_, Block{heap_base_, heap_size_});
}

// ----------- Allocation (First-Fit) -----------

std::optional<uint64_t> HeapAllocator::alloc(uint64_t size) {
    if (size == 0)
        return std::nullopt;

    const uint64_t aligned = align_up(size);
    const uint64_t frag    = aligned - size;

    for (auto it = free_blocks_.begin(); it != free_blocks_.end(); ++it) {
        Block &free = it->second;

        if (free.size < aligned)
            continue;

        const uint64_t addr = free.start;

        // Split block if needed
        if (free.size > aligned) {
            free_blocks_.emplace(
                free.start + aligned,
                Block{free.start + aligned, free.size - aligned}
            );
        }

        // Remove consumed block
        free_blocks_.erase(it);

        allocated_blocks_.emplace(
            addr, Block{addr, aligned}
        );

        allocated_bytes_      += aligned;
        internal_frag_bytes_  += frag;

        return addr;
    }

    return std::nullopt;
}

// ----------- Free -----------

void HeapAllocator::free(uint64_t addr) {
    auto it = allocated_blocks_.find(addr);
    if (it == allocated_blocks_.end())
        throw std::runtime_error("HeapAllocator::free invalid address");

    const Block blk = it->second;

    allocated_bytes_ -= blk.size;

    // reclaim internal fragmentation exactly as accounted
    internal_frag_bytes_ -= (blk.size - align_up(blk.size));

    allocated_blocks_.erase(it);

    // Insert into free list
    auto [curr, _] = free_blocks_.emplace(blk.start, blk);

    // ----------- Coalescing -----------

    // Coalesce with previous
    if (curr != free_blocks_.begin()) {
        auto prev = std::prev(curr);
        if (prev->second.start + prev->second.size == curr->second.start) {
            prev->second.size += curr->second.size;
            free_blocks_.erase(curr);
            curr = prev;
        }
    }

    // Coalesce with next
    auto next = std::next(curr);
    if (next != free_blocks_.end() &&
        curr->second.start + curr->second.size == next->second.start) {
        curr->second.size += next->second.size;
        free_blocks_.erase(next);
    }
}

// ----------- Metrics -----------

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
    uint64_t max_block = 0;
    for (const auto &[_, blk] : free_blocks_) {
        max_block = std::max(max_block, blk.size);
    }
    return max_block;
}

uint64_t HeapAllocator::internal_fragmentation() const noexcept {
    return internal_frag_bytes_;
}

double HeapAllocator::external_fragmentation() const noexcept {
    const uint64_t free = free_bytes();
    if (free == 0)
        return 0.0;

    return 1.0 - (static_cast<double>(largest_free_block()) /
                  static_cast<double>(free));
}

} // namespace sim
