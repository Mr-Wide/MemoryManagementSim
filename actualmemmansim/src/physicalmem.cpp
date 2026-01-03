#include "sim/physicalmem.h"

#include <stdexcept>

namespace sim {

PhysicalMemory::PhysicalMemory(size_t num_frames)
    : frames_(num_frames) {}

size_t PhysicalMemory::num_frames() const noexcept {
    return frames_.size();
}

FrameAllocResult PhysicalMemory::allocate(uint32_t pid, uint64_t vpn, uint64_t now) {
    // 1. Try free frame first
    int frame_id = find_free_frame();
    bool evicted = false;
    uint32_t old_pid = 0;
    uint64_t old_vpn = 0;

    // 2. If none free, evict LRU
    if (frame_id == -1) {
        frame_id = find_lru_frame();
        if (frame_id == -1) {
            throw std::runtime_error("PhysicalMemory: no frame available for eviction");
        }
        evicted = true;
        old_pid = frames_[frame_id].pid;
        old_vpn = frames_[frame_id].vpn;
    }

    // 3. Assign frame
    frames_[frame_id].occupied = true;
    frames_[frame_id].pid = pid;
    frames_[frame_id].vpn = vpn;
    frames_[frame_id].last_used = now;

    return FrameAllocResult{
        frame_id,
        evicted,
        old_pid,
        old_vpn
    };
}

void PhysicalMemory::touch(int frame_id, uint64_t now) {
    if (frame_id < 0 || static_cast<size_t>(frame_id) >= frames_.size()) {
        throw std::out_of_range("PhysicalMemory::touch invalid frame_id");
    }
    frames_[frame_id].last_used = now;
}

void PhysicalMemory::free(int frame_id) {
    if (frame_id < 0 || static_cast<size_t>(frame_id) >= frames_.size()) {
        throw std::out_of_range("PhysicalMemory::free invalid frame_id");
    }
    frames_[frame_id] = Frame{};
}

const Frame& PhysicalMemory::frame(int frame_id) const {
    if (frame_id < 0 || static_cast<size_t>(frame_id) >= frames_.size()) {
        throw std::out_of_range("PhysicalMemory::frame invalid frame_id");
    }
    return frames_[frame_id];
}

int PhysicalMemory::find_free_frame() const {
    for (size_t i = 0; i < frames_.size(); ++i) {
        if (!frames_[i].occupied) {
            return static_cast<int>(i);
        }
    }
    return -1;
}

int PhysicalMemory::find_lru_frame() const {
    int lru = -1;
    uint64_t oldest = UINT64_MAX;

    for (size_t i = 0; i < frames_.size(); ++i) {
        if (frames_[i].occupied && frames_[i].last_used < oldest) {
            oldest = frames_[i].last_used;
            lru = static_cast<int>(i);
        }
    }
    return lru;
}

} // namespace sim
