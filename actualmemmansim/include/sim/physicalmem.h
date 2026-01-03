#ifndef SIM_PHYSICAL_MEMORY_H
#define SIM_PHYSICAL_MEMORY_H

#include <cstdint>
#include <vector>
#include <optional>

namespace sim {

// Frame metadata
struct Frame {
    bool occupied = false;
    uint32_t pid = 0;
    uint64_t vpn = 0;
    uint64_t last_used = 0;
};

// Result of a frame allocation request
struct FrameAllocResult {
    int frame_id;
    bool evicted;
    uint32_t evicted_pid;
    uint64_t evicted_vpn;
};

class PhysicalMemory {
public:
    explicit PhysicalMemory(size_t num_frames);

    size_t num_frames() const noexcept;

    // Allocate a frame for (pid, vpn) at time 'now'.
    // May evict an existing frame using LRU.
    FrameAllocResult allocate(uint32_t pid, uint64_t vpn, uint64_t now);

    // Mark a frame as accessed (for LRU updates)
    void touch(int frame_id, uint64_t now);

    // Free a frame explicitly
    void free(int frame_id);

    const Frame& frame(int frame_id) const;

private:
    int find_free_frame() const;
    int find_lru_frame() const;

    std::vector<Frame> frames_;
};

} // namespace sim

#endif // SIM_PHYSICAL_MEMORY_H
