#ifndef SIM_METRICS_H
#define SIM_METRICS_H

#include <cstdint>
#include <vector>

namespace sim {

/*
 * Metrics
 *
 * Collects simulation statistics across subsystems.
 * Does NOT know about MMU, TLB, or Scheduler internals.
 */
class Metrics {
public:
    Metrics();

    void reset();

    // ---------------- Heap metrics ----------------
    void update_heap(uint64_t total_heap,
                     uint64_t allocated,
                     uint64_t free,
                     uint64_t largest_free,
                     uint64_t internal_frag);

    uint64_t total_heap_size() const noexcept;
    uint64_t allocated_bytes() const noexcept;
    uint64_t free_bytes() const noexcept;
    uint64_t largest_free_block() const noexcept;
    uint64_t internal_fragmentation() const noexcept;
    double external_fragmentation() const noexcept;

    // ---------------- TLB metrics ----------------
    void record_tlb_hit();
    void record_tlb_miss();

    uint64_t tlb_hits() const noexcept;
    uint64_t tlb_misses() const noexcept;
    double tlb_hit_rate() const noexcept;

    // ---------------- Latency metrics ----------------
    void record_access_latency(uint64_t cycles);

    uint64_t latency_p50() const;
    uint64_t latency_p90() const;
    uint64_t latency_p99() const;

private:
    // Heap
    uint64_t total_heap_;
    uint64_t allocated_bytes_;
    uint64_t free_bytes_;
    uint64_t largest_free_block_;
    uint64_t internal_frag_bytes_;

    // TLB
    uint64_t tlb_hits_;
    uint64_t tlb_misses_;

    // Latency samples
    std::vector<uint64_t> latencies_;

    uint64_t percentile(double p) const;
};

} // namespace sim

#endif // SIM_METRICS_H
