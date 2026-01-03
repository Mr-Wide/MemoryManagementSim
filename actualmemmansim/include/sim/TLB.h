#ifndef SIM_TLB_H
#define SIM_TLB_H

#include <cstdint>
#include <optional>
#include <memory>
namespace sim {

/*
 * TLBEntry
 *
 * Represents a cached virtual → physical translation.
 * Tagged by (pid, vpn) to avoid ASID complexity.
 */
struct TLBEntry {
    uint32_t pid;
    uint64_t vpn;
    int frame_id;

    // Used by CLOCK / LRU policies
    bool referenced = false;
};

/*
 * TLB
 *
 * Translation Lookaside Buffer.
 *
 * - Purely a cache over page table translations
 * - Does NOT allocate frames
 * - Does NOT trigger page faults
 *
 * Lookup semantics:
 *   - hit  → returns frame_id
 *   - miss → caller must consult page table
 */
class TLB {
public:
    explicit TLB(size_t capacity);

    // Disable copying (stateful cache)
    TLB(const TLB&) = delete;
    TLB& operator=(const TLB&) = delete;

    // Lookup translation
    // Returns frame_id on hit
    std::optional<int> lookup(uint32_t pid, uint64_t vpn);

    // Insert or update an entry
    void insert(uint32_t pid, uint64_t vpn, int frame_id);

    // Invalidate all entries for a process
    void flush_process(uint32_t pid);

    // Invalidate single translation (used on eviction)
    void invalidate(uint32_t pid, uint64_t vpn);

    // Flush entire TLB
    void flush_all();

    // -------- Metrics --------
    uint64_t hits() const noexcept;
    uint64_t misses() const noexcept;
    double hit_rate() const noexcept;
    ~TLB();

private:
    size_t capacity_;

    // Replacement policy state is opaque here
    // Implementation decides data structure
    uint64_t hits_;
    uint64_t misses_;
    std::unique_ptr<class TLBImpl> impl_;

};

} // namespace sim

#endif // SIM_TLB_H
