#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <algorithm>

#include "sim/clock.h"
#include "sim/event.h"
#include "sim/workload.h"
#include "sim/MMU.h"
#include "sim/physicalmem.h"
#include "sim/scheduler.h"
#include "sim/metrics.h"
#include "sim/timeline.h"
#include "sim/TUI.h"

using namespace sim;

// Parse a string as decimal or hex
static uint64_t parse_u64(const std::string &s) {
    if (s.size() > 2 && s[0] == '0' &&
        (s[1] == 'x' || s[1] == 'X')) {
        return std::stoull(s, nullptr, 16);
    }
    return std::stoull(s);
}

int main(int argc, char **argv) {
    if (argc < 2) {
        std::cerr << "usage: memsim <trace.csv>\n";
        return 1;
    }

    Clock clock;
    EventQueue eq;
    Workload wl(argv[1]);

    if (!wl.parse_into(eq)) {
        std::cerr << "Trace parse failed\n";
        return 1;
    }

    constexpr uint64_t PAGE_SIZE = 4096;
    constexpr size_t NUM_FRAMES = 4;
    constexpr uint64_t PAGEIN_LATENCY = 10;

    PhysicalMemory pmem(NUM_FRAMES);
    constexpr size_t TLB_SIZE = 16;
    Metrics metrics;
    MMU mmu(pmem, PAGE_SIZE, TLB_SIZE, metrics);
    Scheduler sched;
    Timeline timeline;

    size_t page_faults = 0;

    // Track allocated addresses per process
    std::map<uint32_t, std::vector<uint64_t>> allocated_map;

    // ================== SIMULATION LOOP ==================
    while (!eq.empty()) {
        Event ev = eq.pop();
        if (ev.key.time > clock.now())
            clock.set(ev.key.time);

        uint32_t pid = ev.key.pid;

        if (ev.type == "PROC_START") {
            uint64_t base = parse_u64(ev.args[0]);
            uint64_t top  = parse_u64(ev.args[1]);
            mmu.register_process(pid, base, top - base);
            sched.add_process(pid);
            timeline.log(clock.now(), pid, "PROC_START");
        }

        else if (ev.type == "PROC_EXIT") {
            auto &proc = mmu.process(pid);

            // Warning for memory still allocated, do NOT abort
            if (proc.heap().allocated_bytes() != 0) {
                std::cerr << "WARNING: memory not freed on PROC_EXIT pid=" 
                          << pid
                          << ", allocated_bytes=" << proc.heap().allocated_bytes()
                          << "\n";
            }

            mmu.unregister_process(pid);
            sched.terminate_process(pid);
            timeline.log(clock.now(), pid, "PROC_EXIT");

            // Clear allocated addresses map
            allocated_map.erase(pid);
        }

        else if (ev.type == "MALLOC") {
            auto &proc = mmu.process(pid);
            uint64_t size = parse_u64(ev.args[0]);

            auto addr = proc.heap_alloc(size);
            if (addr) {
                allocated_map[pid].push_back(*addr);

                metrics.update_heap(
                    proc.heap().total_heap_size(),
                    proc.heap().allocated_bytes(),
                    proc.heap().free_bytes(),
                    proc.heap().largest_free_block(),
                    proc.heap().internal_fragmentation()
                );

                timeline.log(
                    clock.now(),
                    pid,
                    "MALLOC size=" + std::to_string(size) +
                    " addr=0x" + std::to_string(*addr)
                );
            } else {
                timeline.log(
                    clock.now(),
                    pid,
                    "MALLOC FAILED size=" + std::to_string(size)
                );
            }
        }

        else if (ev.type == "FREE") {
            auto &proc = mmu.process(pid);

            if (!allocated_map[pid].empty()) {
                uint64_t addr_to_free = allocated_map[pid].back();
                allocated_map[pid].pop_back();

                proc.heap_free(addr_to_free);

                metrics.update_heap(
                    proc.heap().total_heap_size(),
                    proc.heap().allocated_bytes(),
                    proc.heap().free_bytes(),
                    proc.heap().largest_free_block(),
                    proc.heap().internal_fragmentation()
                );

                timeline.log(
                    clock.now(),
                    pid,
                    "FREE addr=0x" + std::to_string(addr_to_free)
                );
            } else {
                std::cerr << "WARNING: No allocated blocks to free for pid=" 
                          << pid << "\n";
            }
        }

        else if (ev.type == "ACCESS") {
            auto running = sched.schedule_next();
            if (!running) continue;

            uint64_t vaddr = parse_u64(ev.args[0]);
            auto res = mmu.access(*running, vaddr);

            if (res == MMUAccessResult::HIT) {
                timeline.log(clock.now(), *running, "ACCESS HIT");
            } else {
                ++page_faults;
                sched.block_current();

                uint64_t vpn = mmu.vpn_from_vaddr(vaddr);
                timeline.log(
                    clock.now(),
                    *running,
                    "PAGE_FAULT vpn=" + std::to_string(vpn) + " → BLOCKED"
                );

                eq.push(clock.now() + PAGEIN_LATENCY,
                        0,
                        *running,
                        "PAGEIN_COMPLETE",
                        { std::to_string(vpn) });
            }
        }

        else if (ev.type == "PAGEIN_COMPLETE") {
            uint64_t vpn = std::stoull(ev.args[0]);
            mmu.complete_pagein(pid, vpn, clock.now());
            sched.wake_process(pid);

            timeline.log(
                clock.now(),
                pid,
                "PAGEIN_COMPLETE vpn=" + std::to_string(vpn) + " → READY"
            );
        }

        // ---------------- Update metrics snapshot ----------------
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

    // ================== TERMINAL UI ==================
    TerminalUI::run(timeline);

    return 0;
}
