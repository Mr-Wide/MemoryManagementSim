#include "TUI.h"
#include <iostream>

namespace sim {

void TerminalUI::run(const Timeline &timeline) {
    const auto &all = timeline.all();

    std::cout << "\nSimulation finished.\n";
    std::cout << "Available timestamps:\n";

    for (const auto &p : all)
        std::cout << "  " << p.first << "\n";

    while (true) {
        std::cout << "\nEnter timestamp (-1 to exit): ";
        long long t;
        std::cin >> t;

        if (!std::cin || t < 0)
            break;

        auto it = all.find(t);
        if (it == all.end()) {
            std::cout << "No data for this timestamp.\n";
            continue;
        }

        const auto &snap = it->second;

        std::cout << "\n=== Time " << t << " ===\n";

        if (snap.events.empty()) {
            std::cout << "(no events)\n";
        } else {
            for (const auto &e : snap.events) {
                std::cout << "pid=" << e.pid
                          << " : " << e.message << "\n";
            }
        }

        const auto &m = snap.metrics;
        std::cout << "\nMetrics:\n";
        std::cout << "  allocated_bytes = " << m.allocated_bytes << "\n";
        std::cout << "  free_bytes      = " << m.free_bytes << "\n";
        std::cout << "  largest_free    = " << m.largest_free << "\n";
        std::cout << "  internal_frag   = " << m.internal_frag << "\n";
        std::cout << "  external_frag   = " << m.external_frag << "\n";
        std::cout << "  page_faults     = " << m.page_faults << "\n";
    }
}

} // namespace sim
