#pragma once
#include "timeline.h"

namespace sim {

class TerminalUI {
public:
    // Start the Ncurses interactive visualizer
    static void run(const Timeline &timeline);
};

} // namespace sim
