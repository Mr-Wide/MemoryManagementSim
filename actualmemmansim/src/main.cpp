#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>

// Core Simulation Headers
#include "sim/clock.h"
#include "sim/event.h"
#include "sim/workload.h"
#include "sim/MMU.h"
#include "sim/physicalmem.h"
#include "sim/scheduler.h"
#include "sim/metrics.h"
#include "sim/timeline.h"
#include "sim/tui.h" 

using namespace sim;

// --- Helper: Parse Strings to Numbers ---
static uint64_t parse_u64(const std::string &s) {
    if (s.size() > 2 && s[0] == '0' && (s[1] == 'x' || s[1] == 'X'))
        return std::stoull(s, nullptr, 16);
    return std::stoull(s);
}

// --- Helper: Convert Number to Hex String (0x...) ---
static std::string to_hex(uint64_t val) {
    std::stringstream ss;
    ss << "0x" << std::uppercase << std::hex << val;
    return ss.str();
}

int main(int argc, char **argv) {
    if (argc < 2) {
        std::cerr << "usage: memsim <trace.csv>\n";
        return 1;
    }

    std::string trace_file = argv[1];

    // Simple Menu
    std::cout << "Select Allocation Strategy:\n";
    std::cout << "1. First Fit\n2. Best Fit\n3. Worst Fit\n";
    std::cout << "Choice: ";
    int c; std::cin >> c;

    FitStrategy strategy = FitStrategy::FirstFit;
    if (c == 2) strategy = FitStrategy::BestFit;
    if (c == 3) strategy = FitStrategy::WorstFit;

    std::cout << "\nRunning simulation on " << trace_file << "...\n";

    // Initialize Components
    Clock clock;
    EventQueue eq;
    Workload wl(trace_file);
    Timeline timeline; 

    if (!wl.parse_into(eq)) {
        std::cerr << "Error: Trace parse failed.\n";
        return 1;
    }

    constexpr uint64_t PAGE_SIZE = 4096;
    constexpr size_t NUM_FRAMES = 4;
    constexpr uint64_t PAGEIN_LATENCY = 10;
    constexpr size_t TLB_SIZE = 16;

    PhysicalMemory pmem(NUM_FRAMES);
    Metrics metrics;
    MMU mmu(pmem, PAGE_SIZE, TLB_SIZE, metrics);
    Scheduler sched;
    size_t page_faults = 0;

    // --- Main Event Loop ---
    while (!eq.empty()) {
        Event ev = eq.pop();
        
        if (ev.key.time > clock.now())
            clock.set(ev.key.time);

        uint32_t pid = ev.key.pid;

        // ---------- PROCESS START ----------
        if (ev.type == "PROC_START") {
            uint64_t base = parse_u64(ev.args[0]);
            uint64_t top  = parse_u64(ev.args[1]);

            mmu.register_process(pid, base, top - base, strategy);
            sched.add_process(pid);

            // Log: PROC_START
            timeline.log(clock.now(), pid, "PROC_START");
        }
        // ---------- PROCESS EXIT ----------
        else if (ev.type == "PROC_EXIT") {
            mmu.unregister_process(pid);
            sched.terminate_process(pid);

            // Log: PROC_EXIT
            timeline.log(clock.now(), pid, "PROC_EXIT");
        }
        // ---------- MALLOC ----------
        else if (ev.type == "MALLOC") {
            auto &proc = mmu.process(pid);
            uint64_t size = parse_u64(ev.args[0]);

            auto addr = proc.heap_alloc(size);
            
            // Update metrics regardless of success/fail to keep state current
            metrics.update_heap(
                proc.heap().total_heap_size(),
                proc.heap().allocated_bytes(),
                proc.heap().free_bytes(),
                proc.heap().largest_free_block(),
                proc.heap().internal_fragmentation()
            );

            if (addr) {
                // Log: MALLOC size=... -> addr=0x...
                std::string msg = "MALLOC size=" + std::to_string(size) + 
                                  " -> addr=" + to_hex(*addr);
                timeline.log(clock.now(), pid, msg);
            } else {
                timeline.log(clock.now(), pid, "MALLOC FAILED size=" + std::to_string(size));
            }
        }
        // ---------- FREE ----------
        else if (ev.type == "FREE") {
            auto &proc = mmu.process(pid);
            uint64_t addr = parse_u64(ev.args[0]);
            proc.heap_free(addr);
            
            // Update metrics
            metrics.update_heap(
                proc.heap().total_heap_size(),
                proc.heap().allocated_bytes(),
                proc.heap().free_bytes(),
                proc.heap().largest_free_block(),
                proc.heap().internal_fragmentation()
            );

            // Log: FREE addr=0x...
            timeline.log(clock.now(), pid, "FREE addr=" + to_hex(addr));
        }
        // ---------- ACCESS ----------
        else if (ev.type == "ACCESS") {
            auto running = sched.schedule_next();
            if (!running) continue;

            uint64_t vaddr = parse_u64(ev.args[0]);
            auto res = mmu.access(*running, vaddr);

            if (res == MMUAccessResult::HIT) {
                // Log: ACCESS vaddr=0x... (hit)
                std::string msg = "ACCESS vaddr=" + to_hex(vaddr) + " (hit)";
                timeline.log(clock.now(), *running, msg);
            } else {
                ++page_faults;
                sched.block_current();
                uint64_t vpn = mmu.vpn_from_vaddr(vaddr);

                // Log: PAGE_FAULT vpn=... -> BLOCKED
                std::string msg = "PAGE_FAULT vpn=" + std::to_string(vpn) + " -> BLOCKED";
                timeline.log(clock.now(), *running, msg);

                eq.push(clock.now() + PAGEIN_LATENCY,
                        0, *running,
                        "PAGEIN_COMPLETE",
                        { std::to_string(vpn) });
            }
        }
        // ---------- PAGEIN COMPLETE ----------
        else if (ev.type == "PAGEIN_COMPLETE") {
            uint64_t vpn = std::stoull(ev.args[0]);
            mmu.complete_pagein(pid, vpn, clock.now());
            sched.wake_process(pid);

            // Log: PAGEIN_COMPLETE vpn=... -> READY
            std::string msg = "PAGEIN_COMPLETE vpn=" + std::to_string(vpn) + " -> READY";
            timeline.log(clock.now(), pid, msg);
        }

        // Record Metrics Snapshot
        timeline.snapshot(
            clock.now(),
            metrics.allocated_bytes(),
            metrics.free_bytes(),
            metrics.largest_free_block(),
            metrics.internal_fragmentation(),
            metrics.external_fragmentation(),
            page_faults
        );
    }

    std::cout << "Simulation complete. Launching Visualizer...\n";
    TerminalUI::run(timeline);
    return 0;
}
