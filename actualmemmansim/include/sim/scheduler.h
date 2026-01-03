#ifndef SIM_SCHEDULER_H
#define SIM_SCHEDULER_H

#include <cstdint>
#include <deque>
#include <unordered_map>
#include <optional>

#include "sim/process.h"

namespace sim {

class Scheduler {
public:
    Scheduler();

    // Register a new process with scheduler
    void add_process(uint32_t pid);

    // Mark process as terminated and remove from scheduling
    void terminate_process(uint32_t pid);

    // Block the currently running process
    void block_current();

    // Wake a blocked process (e.g., PAGEIN_COMPLETE)
    void wake_process(uint32_t pid);

    // Pick next process to run (if needed)
    std::optional<uint32_t> schedule_next();

    // Current running process (if any)
    std::optional<uint32_t> current() const noexcept;

    bool has_runnable() const noexcept;

private:
    std::deque<uint32_t> ready_queue_;
    std::unordered_map<uint32_t, ProcessState> states_;
    std::optional<uint32_t> current_;
};

} // namespace sim

#endif // SIM_SCHEDULER_H
