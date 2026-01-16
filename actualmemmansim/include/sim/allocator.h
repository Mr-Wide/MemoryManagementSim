#ifndef SIM_ALLOCATOR_H
#define SIM_ALLOCATOR_H

#include <cstdint>
#include <map>
#include <optional>

namespace sim {

enum class FitStrategy {
    FirstFit,
    BestFit,
    WorstFit
};

class HeapAllocator {
public:
    struct Block {
        uint64_t start;
        uint64_t size;
    };

    HeapAllocator(uint64_t heap_base, uint64_t heap_size,
                  FitStrategy strategy = FitStrategy::FirstFit);

    HeapAllocator(const HeapAllocator&) = delete;
    HeapAllocator& operator=(const HeapAllocator&) = delete;

    std::optional<uint64_t> alloc(uint64_t size);
    void free(uint64_t addr);

    uint64_t total_heap_size() const noexcept;
    uint64_t allocated_bytes() const noexcept;
    uint64_t free_bytes() const noexcept;
    uint64_t largest_free_block() const noexcept;
    uint64_t internal_fragmentation() const noexcept;
    double external_fragmentation() const noexcept;

private:
    uint64_t heap_base_;
    uint64_t heap_size_;
    std::map<uint64_t, Block> free_blocks_;
    std::map<uint64_t, Block> allocated_blocks_;
    uint64_t allocated_bytes_;
    uint64_t internal_frag_bytes_;
    FitStrategy strategy_;

    static constexpr uint64_t ALIGNMENT = 8;
    static uint64_t align_up(uint64_t n) noexcept;
};

} // namespace sim

#endif // SIM_ALLOCATOR_H
