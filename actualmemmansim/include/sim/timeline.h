#pragma once
#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace sim {

struct LoggedEvent {
    uint32_t pid;
    std::string message;
};

struct MetricsSnapshot {
    uint64_t allocated_bytes = 0;
    uint64_t free_bytes = 0;
    uint64_t largest_free = 0;
    double internal_frag = 0.0;
    double external_frag = 0.0;
    size_t page_faults = 0;
};

struct TimeSnapshot {
    uint64_t timestamp = 0;
    std::vector<LoggedEvent> events;
    MetricsSnapshot metrics;
};

class Timeline {
    std::map<uint64_t, TimeSnapshot> snapshots_;

public:
    void log(uint64_t time, uint32_t pid, const std::string &msg);

    void snapshot(uint64_t time,
                  uint64_t allocated,
                  uint64_t free,
                  uint64_t largest,
                  double internal_frag,
                  double external_frag,
                  size_t page_faults);

    const std::map<uint64_t, TimeSnapshot>& all() const;
};

} // namespace sim
