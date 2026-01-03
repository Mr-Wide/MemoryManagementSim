#ifndef SIM_ALLOCATOR_H
#define SIM_ALLOCATOR_H

#include <cstdint>
#include <map>
#include <optional>

namespace sim {

/*
 * HeapAllocator
 *
 * Manages virtual heap address space for a single process.
 *
 * - Works purely in virtual address space
 * - Does NOT allocate physical memory
 * - Does NOT touch page tables
 * - Supports lazy allocation
 *
 * Address space model:
 *   [heap_base ........................................ heap_end)
 *
 * Internally maintains free and allocated blocks.
 */
class HeapAllocator {
public:
    struct Block {
        uint64_t start;   // virtual start address
        uint64_t size;    // size in bytes
    };

    // Create allocator with fixed heap base and max size
    HeapAllocator(uint64_t heap_base, uint64_t heap_size);

    // Disable copying (allocator state must be unique)
    HeapAllocator(const HeapAllocator&) = delete;
    HeapAllocator& operator=(const HeapAllocator&) = delete;

    // Allocate a block of at least `size` bytes.
    // Returns starting virtual address on success.
    // Returns nullopt if no suitable block exists.
    std::optional<uint64_t> alloc(uint64_t size);

    // Free a previously allocated block by its starting address.
    // Undefined behavior if addr was not returned by alloc().
    void free(uint64_t addr);

    // ---------------- Metrics ----------------

    uint64_t total_heap_size() const noexcept;
    uint64_t allocated_bytes() const noexcept;
    uint64_t free_bytes() const noexcept;
    uint64_t largest_free_block() const noexcept;

    // Internal fragmentation (bytes wasted inside allocated blocks)
    uint64_t internal_fragmentation() const noexcept;

    // External fragmentation ratio:
    // 1 - (largest_free_block / free_bytes)
    double external_fragmentation() const noexcept;

private:
    // Heap bounds
    uint64_t heap_base_;
    uint64_t heap_size_;

    // Free blocks, keyed by start address
    std::map<uint64_t, Block> free_blocks_;

    // Allocated blocks, keyed by start address
    std::map<uint64_t, Block> allocated_blocks_;

    // Fragmentation accounting
    uint64_t allocated_bytes_;
    uint64_t internal_frag_bytes_;

    // Alignment helper (kept simple for now)
    static constexpr uint64_t ALIGNMENT = 8;
    static uint64_t align_up(uint64_t n) noexcept;
};

} // namespace sim

#endif // SIM_ALLOCATOR_H
