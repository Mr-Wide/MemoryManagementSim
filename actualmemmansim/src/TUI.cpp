#include "sim/TUI.h"
#include <ncurses.h> // The GUI library
#include <vector>
#include <string>
#include <algorithm>
#include <iomanip>
#include <sstream>

namespace sim {

// --- Helper: Draw a visual progress bar [|||||     ] ---
// win: Ncurses window, y/x: coordinates, width: size of bar
// percentage: 0.0 to 1.0, color_pair: color ID
void draw_bar(WINDOW* win, int y, int x, int width, double percentage, int color_pair) {
    mvwprintw(win, y, x, "[");
    int inner_width = width - 2;
    int filled = static_cast<int>(inner_width * percentage);
    
    // Safety bounds
    if (filled > inner_width) filled = inner_width;
    if (filled < 0) filled = 0;
    
    // Draw filled portion
    wattron(win, COLOR_PAIR(color_pair));
    for (int i = 0; i < inner_width; ++i) {
        if (i < filled) waddch(win, '|');
        else waddch(win, ' ');
    }
    wattroff(win, COLOR_PAIR(color_pair));
    waddch(win, ']');
}

void TerminalUI::run(const Timeline &timeline) {
    // --- 1. NCURSES SETUP ---
    initscr();            // Start Ncurses mode
    cbreak();             // Disable line buffering (instant input)
    noecho();             // Don't print user keypresses
    keypad(stdscr, TRUE); // Enable Arrow Keys
    curs_set(0);          // Hide the blinking cursor

    // Initialize Colors
    if (has_colors()) {
        start_color();
        init_pair(1, COLOR_GREEN, COLOR_BLACK);  // Pair 1: Green
        init_pair(2, COLOR_RED,   COLOR_BLACK);  // Pair 2: Red
        init_pair(3, COLOR_CYAN,  COLOR_BLACK);  // Pair 3: Cyan
        init_pair(4, COLOR_YELLOW, COLOR_BLACK); // Pair 4: Yellow
    }

    // --- 2. PREPARE DATA ---
    const auto &all_snaps = timeline.all();
    if (all_snaps.empty()) {
        endwin();
        printf("No simulation data to display.\n");
        return;
    }

    // Convert map keys to vector for easy index navigation (0, 1, 2...)
    std::vector<long long> timestamps;
    for (const auto& kv : all_snaps) {
        timestamps.push_back(kv.first);
    }

    size_t current_idx = 0; // Start at time 0
    bool running = true;

    // --- 3. MAIN GUI LOOP ---
    while (running) {
        long long current_time = timestamps[current_idx];
        const auto& snap = all_snaps.at(current_time);

        // Calculate Visualization Stats
        double total_mem = snap.metrics.allocated_bytes + snap.metrics.free_bytes;
        if (total_mem == 0) total_mem = 1; 

        double alloc_pct = (double)snap.metrics.allocated_bytes / total_mem;
        double ext_frag_pct = snap.metrics.external_frag; 

        clear(); // Clear screen buffer

        // === HEADER ===
        attron(A_BOLD);
        mvprintw(0, 2, " MEMORY SIMULATION VISUALIZER ");
        mvprintw(0, 40, " Time Tick: %lld ", current_time);
        attroff(A_BOLD);
        mvhline(1, 0, 0, COLS); // Horizontal separator line

        // === LEFT PANEL: METRICS ===
        // 1. Allocated Memory
        attron(COLOR_PAIR(3));
        mvprintw(3, 2, "MEMORY USAGE:");
        attroff(COLOR_PAIR(3));
        mvprintw(4, 2, "%lu / %lu bytes", snap.metrics.allocated_bytes, (uint64_t)total_mem);
        draw_bar(stdscr, 5, 2, 30, alloc_pct, 1); // Green Bar

        // 2. Fragmentation
        attron(COLOR_PAIR(3));
        mvprintw(7, 2, "EXTERNAL FRAGMENTATION:");
        attroff(COLOR_PAIR(3));
        mvprintw(8, 2, "%.2f %%", ext_frag_pct * 100.0);
        draw_bar(stdscr, 9, 2, 30, ext_frag_pct, 2); // Red Bar

        // 3. Text Stats
        mvprintw(12, 2, "Page Faults:       %lu", snap.metrics.page_faults);
        mvprintw(13, 2, "Largest Free Blk:  %lu", snap.metrics.largest_free);
        mvprintw(14, 2, "Internal Frag:     %.2f", snap.metrics.internal_frag);

        // Vertical Separator Line
        mvvline(2, 35, 0, LINES - 4);

        // === RIGHT PANEL: LOGS ===
        attron(A_BOLD | COLOR_PAIR(4));
        mvprintw(2, 38, " EVENTS AT THIS TICK ");
        attroff(A_BOLD | COLOR_PAIR(4));

        int row = 4;
        if (snap.events.empty()) {
            mvprintw(row, 38, "(No events)");
        } else {
            for (const auto& ev : snap.events) {
                if (row >= LINES - 3) break; // Stop if screen is full
                
                // Color code specific events for readability
                if (ev.message.find("BLOCKED") != std::string::npos || 
                    ev.message.find("FAILED") != std::string::npos) 
                {
                    attron(COLOR_PAIR(2)); // Red
                } 
                else if (ev.message.find("MALLOC") != std::string::npos || 
                         ev.message.find("READY") != std::string::npos)
                {
                    attron(COLOR_PAIR(1)); // Green
                }

                mvprintw(row, 38, "[PID %u] %s", ev.pid, ev.message.c_str());
                
                attroff(COLOR_PAIR(1));
                attroff(COLOR_PAIR(2));
                row++;
            }
        }

        // === FOOTER ===
        mvhline(LINES - 2, 0, 0, COLS);
        attron(A_REVERSE);
        mvprintw(LINES - 1, 0, " CONTROLS:  [<- Left] Previous Time    [Right ->] Next Time    [Q] Quit ");
        attroff(A_REVERSE);

        refresh(); // Render the screen

        // === INPUT HANDLING ===
        int ch = getch(); // Wait for user input
        switch (ch) {
            case KEY_LEFT:
                if (current_idx > 0) current_idx--;
                break;
            case KEY_RIGHT:
                if (current_idx < timestamps.size() - 1) current_idx++;
                break;
            case 'q':
            case 'Q':
                running = false;
                break;
        }
    }

    // Restore terminal settings before exiting
    endwin();
}

} // namespace sim
