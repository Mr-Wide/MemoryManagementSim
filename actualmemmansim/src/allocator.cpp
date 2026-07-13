#include "sim/allocator.h"
#include <algorithm>
#include <stdexcept>

namespace sim {

uint64_t HeapAllocator::align_up(uint64_t n) noexcept {
    return (n + ALIGNMENT - 1) & ~(ALIGNMENT - 1);
}

HeapAllocator::HeapAllocator(uint64_t heap_base, uint64_t heap_size,
                             FitStrategy strategy)
    : heap_base_(heap_base),
      heap_size_(heap_size),
      allocated_bytes_(0),
      internal_frag_bytes_(0),
      strategy_(strategy) {
    free_blocks_.emplace(heap_base_, Block{heap_base_, heap_size_});
}

std::optional<uint64_t> HeapAllocator::alloc(uint64_t size) {
    if (size == 0) return std::nullopt;
    const uint64_t aligned = align_up(size);
    const uint64_t frag    = aligned - size;

    auto chosen_it = free_blocks_.end();

    if (strategy_ == FitStrategy::FirstFit) {
        for (auto it = free_blocks_.begin(); it != free_blocks_.end(); ++it) {
            if (it->second.size >= aligned) {
                chosen_it = it;
                break;
            }
        }
    } 
    else if (strategy_ == FitStrategy::BestFit) {
        uint64_t min_size = UINT64_MAX;
        for (auto it = free_blocks_.begin(); it != free_blocks_.end(); ++it) {
            if (it->second.size >= aligned && it->second.size < min_size) {
                min_size = it->second.size;
                chosen_it = it;
            }
        }
    } 
    else if (strategy_ == FitStrategy::WorstFit) {
        uint64_t max_size = 0;
        for (auto it = free_blocks_.begin(); it != free_blocks_.end(); ++it) {
            if (it->second.size >= aligned && it->second.size > max_size) {
                max_size = it->second.size;
                chosen_it = it;
            }
        }
    }

    if (chosen_it == free_blocks_.end()) return std::nullopt;

    Block &free = chosen_it->second;
    const uint64_t addr = free.start;

    if (free.size > aligned) {
        free_blocks_.emplace(free.start + aligned,
                             Block{free.start + aligned, free.size - aligned});
    }

    free_blocks_.erase(chosen_it);
    allocated_blocks_.emplace(addr, Block{addr, aligned});

    allocated_bytes_ += aligned;
    internal_frag_bytes_ += frag;

    return addr;
}

void HeapAllocator::free(uint64_t addr) {
    auto it = allocated_blocks_.find(addr);
    if (it == allocated_blocks_.end())
        throw std::runtime_error("HeapAllocator::free invalid address");

    const Block blk = it->second;
    allocated_bytes_ -= blk.size;
    internal_frag_bytes_ -= (blk.size - align_up(blk.size));
    allocated_blocks_.erase(it);

    auto [curr, _] = free_blocks_.emplace(blk.start, blk);

    if (curr != free_blocks_.begin()) {
        auto prev = std::prev(curr);
        if (prev->second.start + prev->second.size == curr->second.start) {
            prev->second.size += curr->second.size;
            free_blocks_.erase(curr);
            curr = prev;
        }
    }

    auto next = std::next(curr);
    if (next != free_blocks_.end() &&
        curr->second.start + curr->second.size == next->second.start) {
        curr->second.size += next->second.size;
        free_blocks_.erase(next);
    }
}

uint64_t HeapAllocator::total_heap_size() const noexcept { return heap_size_; }
uint64_t HeapAllocator::allocated_bytes() const noexcept { return allocated_bytes_; }
uint64_t HeapAllocator::free_bytes() const noexcept { return heap_size_ - allocated_bytes_; }

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
    if (free == 0) return 0.0;
    return 1.0 - (static_cast<double>(largest_free_block()) / static_cast<double>(free));
}

} // namespace sim
