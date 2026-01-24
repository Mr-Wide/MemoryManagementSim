#pragma once

#include <string>
#include "sim/timeline.h"
#include "sim/allocator.h"

namespace sim {

struct SimConfig {
    std::string trace_file;
    FitStrategy strategy;
};

bool run_simulation(const SimConfig& cfg, Timeline& timeline);

}
