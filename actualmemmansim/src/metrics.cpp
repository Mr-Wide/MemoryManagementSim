#include "sim/metrics.h"

#include <algorithm>
// calculates the metrics.
namespace sim {

Metrics::Metrics() {
    reset();
}

void Metrics::reset() {
    total_heap_ = 0;
    allocated_bytes_ = 0;
    free_bytes_ = 0;
    largest_free_block_ = 0;
    internal_frag_bytes_ = 0;

    tlb_hits_ = 0;
    tlb_misses_ = 0;

    latencies_.clear();
}

// ---------------- Heap ----------------

void Metrics::update_heap(uint64_t total_heap,
                          uint64_t allocated,
                          uint64_t free,
                          uint64_t largest_free,
                          uint64_t internal_frag) {
    total_heap_ = total_heap;
    allocated_bytes_ = allocated;
    free_bytes_ = free;
    largest_free_block_ = largest_free;
    internal_frag_bytes_ = internal_frag;
}

uint64_t Metrics::total_heap_size() const noexcept {
    return total_heap_;
}

uint64_t Metrics::allocated_bytes() const noexcept {
    return allocated_bytes_;
}

uint64_t Metrics::free_bytes() const noexcept {
    return free_bytes_;
}

uint64_t Metrics::largest_free_block() const noexcept {
    return largest_free_block_;
}

uint64_t Metrics::internal_fragmentation() const noexcept {
    return internal_frag_bytes_;
}

double Metrics::external_fragmentation() const noexcept {
    if (free_bytes_ == 0) return 0.0;
    return 1.0 - (double(largest_free_block_) / double(free_bytes_));
}

// ---------------- TLB ----------------

void Metrics::record_tlb_hit() {
    ++tlb_hits_;
}

void Metrics::record_tlb_miss() {
    ++tlb_misses_;
}

uint64_t Metrics::tlb_hits() const noexcept {
    return tlb_hits_;
}

uint64_t Metrics::tlb_misses() const noexcept {
    return tlb_misses_;
}

double Metrics::tlb_hit_rate() const noexcept {
    uint64_t total = tlb_hits_ + tlb_misses_;
    if (total == 0) return 0.0;
    return double(tlb_hits_) / double(total);
}

// ---------------- Latency ----------------

void Metrics::record_access_latency(uint64_t cycles) {
    latencies_.push_back(cycles);
}

uint64_t Metrics::percentile(double p) const {
    if (latencies_.empty())
        return 0;

    std::vector<uint64_t> sorted = latencies_;
    std::sort(sorted.begin(), sorted.end());

    size_t idx = static_cast<size_t>(p * (sorted.size() - 1));
    return sorted[idx];
}

uint64_t Metrics::latency_p50() const {
    return percentile(0.50);
}

uint64_t Metrics::latency_p90() const {
    return percentile(0.90);
}

uint64_t Metrics::latency_p99() const {
    return percentile(0.99);
}

} // namespace sim
