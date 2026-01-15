#include "sim/timeline.h"

namespace sim {

void Timeline::log(uint64_t time, uint32_t pid, const std::string &msg) {
    auto &snap = snapshots_[time];
    snap.timestamp = time;
    snap.events.push_back({pid, msg});
}

void Timeline::snapshot(uint64_t time,
                        uint64_t allocated,
                        uint64_t free,
                        uint64_t largest,
                        double internal_frag,
                        double external_frag,
                        size_t page_faults) {
    auto &snap = snapshots_[time];
    snap.timestamp = time;
    snap.metrics.allocated_bytes = allocated;
    snap.metrics.free_bytes = free;
    snap.metrics.largest_free = largest;
    snap.metrics.internal_frag = internal_frag;
    snap.metrics.external_frag = external_frag;
    snap.metrics.page_faults = page_faults;
}

const std::map<uint64_t, TimeSnapshot>& Timeline::all() const {
    return snapshots_;
}

} // namespace sim
