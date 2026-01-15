#pragma once
#include "timeline.h"

namespace sim {

class TerminalUI {
public:
    // Start interactive inspection of the timeline
    static void run(const Timeline &timeline);
};

} // namespace sim
