#ifndef SIM_CLOCK_H
#define SIM_CLOCK_H

#include <cstdint>
#include <string>

namespace sim {

class Clock {
public:
    Clock() noexcept;
    explicit Clock(uint64_t start_time) noexcept;

    // current simulated time (cycles)
    uint64_t now() const noexcept;

    // set current simulated time (absolute)
    void set(uint64_t t) noexcept;

    // advance simulated time by dt cycles (dt can be 0)
    void advance(uint64_t dt) noexcept;

    // convenience: set to zero
    void reset() noexcept;

    // human-friendly representation
    std::string to_string() const;

private:
    uint64_t now_;
};

} // namespace sim

#endif // SIM_CLOCK_H