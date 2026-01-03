// File: include/sim/Event.h
#ifndef SIM_EVENT_H
#define SIM_EVENT_H

#include <cstdint>
#include <string>
#include <vector>
#include <queue>

namespace sim {

// Deterministic ordering key used for events.
struct EventKey {
    uint64_t time{0};    // simulation time in cycles
    int priority{0};     // lower => higher priority
    uint32_t pid{0};     // pid tie-breaker
    uint64_t seq{0};     // insertion sequence (monotonic)
};

// Simple event record produced by parser or simulator.
struct Event {
    EventKey key;
    std::string type;               // e.g., "ACCESS", "MALLOC", "PAGEIN_COMPLETE"
    std::vector<std::string> args;  // raw args (strings)
    std::string raw_line;           // original CSV line (optional; for debugging)
};

// Comparator for the priority_queue: we want the smallest key first.
// Returns true when 'a' has lower priority than 'b' (so a comes after b).
struct EventCompare {
    bool operator()(Event const &a, Event const &b) const;
};

// EventQueue: deterministic priority queue wrapper.
// Full class definition (members visible) so all TUs compiling against this header
// see the same layout. This avoids ODR/compile problems and keeps the code simple.
class EventQueue {
public:
    EventQueue();

    // Push a pre-constructed Event. If ev.key.seq == 0, seq will be assigned.
    void push(Event ev);

    // Convenience: construct & push from components
    void push(uint64_t time, int priority, uint32_t pid,
              const std::string &type,
              const std::vector<std::string> &args = {},
              const std::string &raw = "");

    // Remove and return top event (caller must check empty())
    Event pop();

    // Peek at top event (caller must check empty())
    const Event & top() const;

    bool empty() const noexcept;
    size_t size() const noexcept;
    void clear() noexcept;

    // Expose next sequence for external use (read-only)
    uint64_t next_seq() const noexcept;

private:
    // underlying priority queue using EventCompare
    std::priority_queue<Event, std::vector<Event>, EventCompare> pq_;

    // insertion sequence counter to keep FIFO ordering for identical keys
    uint64_t seq_counter_{1};
};

} // namespace sim

#endif // SIM_EVENT_H
