#ifndef SIM_WORKLOAD_H
#define SIM_WORKLOAD_H

#include <string>
#include "sim/event.h"

namespace sim {

class Workload {
public:
    explicit Workload(const std::string &path);

    // Parse the trace file and push events into the provided queue.
    // Returns true on success, false on error.
    bool parse_into(EventQueue &q);

private:
    std::string path_;
};

} // namespace sim

#endif // SIM_WORKLOAD_H
