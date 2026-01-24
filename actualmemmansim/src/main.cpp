#include <iostream>
#include "sim/TUI.h"
#include "sim/simrunner.h"

using namespace sim;

int main(int argc, char **argv) {
    if (argc < 2) {
        std::cerr << "usage: memsim <trace.csv>\n";
        return 1;
    }

    std::cout << "1. First Fit\n2. Best Fit\n3. Worst Fit\n";
    int c; std::cin >> c;

    FitStrategy strategy = FitStrategy::FirstFit;
    if (c == 2) strategy = FitStrategy::BestFit;
    if (c == 3) strategy = FitStrategy::WorstFit;

    Timeline timeline;
    run_simulation(argv[1], strategy, timeline);

    TerminalUI::run(timeline);
}
