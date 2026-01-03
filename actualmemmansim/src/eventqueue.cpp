#include "sim/event.h"
#include <stdexcept>

namespace sim {

bool EventCompare::operator()(const Event &a, const Event &b) const {
    if (a.key.time != b.key.time) return a.key.time > b.key.time;
    if (a.key.priority != b.key.priority) return a.key.priority > b.key.priority;
    if (a.key.pid != b.key.pid) return a.key.pid > b.key.pid;
    return a.key.seq > b.key.seq;
}

EventQueue::EventQueue() : seq_counter_(1) {}

void EventQueue::push(Event ev) {
    if (ev.key.seq == 0) ev.key.seq = seq_counter_++;
    pq_.push(std::move(ev));
}

void EventQueue::push(uint64_t time, int priority, uint32_t pid,
                      const std::string &type,
                      const std::vector<std::string> &args,
                      const std::string &raw) {
    Event ev;
    ev.key = {time, priority, pid, seq_counter_++};
    ev.type = type;
    ev.args = args;
    ev.raw_line = raw;
    pq_.push(std::move(ev));
}

Event EventQueue::pop() {
    if (pq_.empty())
        throw std::runtime_error("EventQueue::pop on empty queue");
    Event ev = pq_.top();
    pq_.pop();
    return ev;
}

const Event& EventQueue::top() const {
    if (pq_.empty())
        throw std::runtime_error("EventQueue::top on empty queue");
    return pq_.top();
}

bool EventQueue::empty() const noexcept {
    return pq_.empty();
}

size_t EventQueue::size() const noexcept {
    return pq_.size();
}

void EventQueue::clear() noexcept {
    pq_ = decltype(pq_)();
}

uint64_t EventQueue::next_seq() const noexcept {
    return seq_counter_;
}

} // namespace sim
