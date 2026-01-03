#include "sim/clock.h"
#include <sstream>

namespace sim {

Clock::Clock() noexcept : now_(0) {}

Clock::Clock(uint64_t start_time) noexcept : now_(start_time) {}

uint64_t Clock::now() const noexcept {
    return now_;
}

void Clock::set(uint64_t t) noexcept {
    now_ = t;
}

void Clock::advance(uint64_t dt) noexcept {
    now_ += dt;
}

void Clock::reset() noexcept {
    now_ = 0;
}

std::string Clock::to_string() const {
    std::ostringstream ss;
    ss << now_ << " cycles";
    return ss.str();
}

} // namespace sim
