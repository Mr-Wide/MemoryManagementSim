#include "sim/scheduler.h"
#include <algorithm>
#include <stdexcept>

namespace sim {

Scheduler::Scheduler() = default;

void Scheduler::add_process(uint32_t pid) {
    if (states_.count(pid))
        throw std::runtime_error("Scheduler: process already exists");

    states_[pid] = ProcessState::READY;
    ready_queue_.push_back(pid);
}

void Scheduler::terminate_process(uint32_t pid) {
    states_[pid] = ProcessState::TERMINATED;

    // Remove from ready queue if present
    ready_queue_.erase(
        std::remove(ready_queue_.begin(), ready_queue_.end(), pid),
        ready_queue_.end()
    );

    if (current_ && *current_ == pid)
        current_.reset();
}

void Scheduler::block_current() {
    if (!current_)
        throw std::runtime_error("Scheduler: no running process to block");

    states_[*current_] = ProcessState::BLOCKED;
    current_.reset();
}

void Scheduler::wake_process(uint32_t pid) {
    auto it = states_.find(pid);
    if (it == states_.end())
        throw std::runtime_error("Scheduler: wake unknown process");

    if (it->second != ProcessState::BLOCKED)
        return; // ignore spurious wakeups

    it->second = ProcessState::READY;
    ready_queue_.push_back(pid);
}

std::optional<uint32_t> Scheduler::schedule_next() {
    if (current_)
        return current_;

    while (!ready_queue_.empty()) {
        uint32_t pid = ready_queue_.front();
        ready_queue_.pop_front();

        if (states_[pid] == ProcessState::READY) {
            states_[pid] = ProcessState::RUNNING;
            current_ = pid;
            return current_;
        }
    }

    return std::nullopt;
}

std::optional<uint32_t> Scheduler::current() const noexcept {
    return current_;
}

bool Scheduler::has_runnable() const noexcept {
    return current_.has_value() || !ready_queue_.empty();
}

} // namespace sim
