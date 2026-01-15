#include <iostream>
#include <string>
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

// ---------------- Helper ----------------
static uint64_t parse_u64(const std::string &s) {
    if (s.size() > 2 && s[0] == '0' &&
        (s[1] == 'x' || s[1] == 'X')) {
        return std::stoull(s, nullptr, 16);
    }
    return std::stoull(s);
}

// ---------------- Heap consistency check (public API only) ----------------
static void assert_heap_consistency(const HeapAllocator &heap) {
    // free_bytes is unsigned → cannot be negative, no warning
    if (heap.largest_free_block() > heap.free_bytes()) {
        std::cerr << "ERROR: largest_free_block > free_bytes\n";
        std::abort();
    }
}

// ---------------- Main ----------------
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

    // ================== SIMULATION ==================
    while (!eq.empty()) {
        Event ev = eq.pop();
        if (ev.key.time > clock.now())
            clock.set(ev.key.time);

        uint32_t pid = ev.key.pid;

        // ---------- PROCESS START ----------
        if (ev.type == "PROC_START") {
            uint64_t base = parse_u64(ev.args[0]);
            uint64_t top  = parse_u64(ev.args[1]);

            mmu.register_process(pid, base, top - base);
            sched.add_process(pid);

            timeline.log(clock.now(), pid, "PROC_START");
        }

        // ---------- PROCESS EXIT ----------
        else if (ev.type == "PROC_EXIT") {
            auto &proc = mmu.process(pid);

            // Ensure all allocated memory was freed (fragmentation allowed)
            if (proc.heap().allocated_bytes() != 0) {
                std::cerr << "ERROR: memory leak detected on PROC_EXIT pid=" << pid << "\n";
                std::abort();
            }

            mmu.unregister_process(pid);
            sched.terminate_process(pid);

            timeline.log(clock.now(), pid, "PROC_EXIT");
        }

        // ---------- MALLOC ----------
        else if (ev.type == "MALLOC") {
            auto &proc = mmu.process(pid);
            uint64_t size = parse_u64(ev.args[0]);

            auto addr = proc.heap_alloc(size);
            if (!addr) {
                std::cerr << "MALLOC failed pid=" << pid << "\n";
                continue;
            }

            // Update metrics & assert heap consistency
            metrics.update_heap(
                proc.heap().total_heap_size(),
                proc.heap().allocated_bytes(),
                proc.heap().free_bytes(),
                proc.heap().largest_free_block(),
                proc.heap().internal_fragmentation()
            );
            assert_heap_consistency(proc.heap());

            // Timeline log
            timeline.log(
                clock.now(),
                pid,
                "MALLOC size=" + std::to_string(size) +
                " addr=0x" + std::to_string(*addr)
            );
        }

        // ---------- FREE ----------
        else if (ev.type == "FREE") {
            auto &proc = mmu.process(pid);
            uint64_t addr = parse_u64(ev.args[0]);
            proc.heap_free(addr);

            // Update metrics & assert heap consistency AFTER free
            metrics.update_heap(
                proc.heap().total_heap_size(),
                proc.heap().allocated_bytes(),
                proc.heap().free_bytes(),
                proc.heap().largest_free_block(),
                proc.heap().internal_fragmentation()
            );
            assert_heap_consistency(proc.heap());

            timeline.log(
                clock.now(),
                pid,
                "FREE addr=0x" + std::to_string(addr)
            );
        }

        // ---------- ACCESS ----------
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
                timeline.log(clock.now(), *running,
                             "PAGE_FAULT vpn=" + std::to_string(vpn) + " → BLOCKED");

                eq.push(clock.now() + PAGEIN_LATENCY,
                        0,
                        *running,
                        "PAGEIN_COMPLETE",
                        { std::to_string(vpn) });
            }
        }

        // ---------- PAGEIN COMPLETE ----------
        else if (ev.type == "PAGEIN_COMPLETE") {
            uint64_t vpn = std::stoull(ev.args[0]);
            mmu.complete_pagein(pid, vpn, clock.now());
            sched.wake_process(pid);

            timeline.log(clock.now(), pid,
                         "PAGEIN_COMPLETE vpn=" + std::to_string(vpn) + " → READY");
        }

        // ---------- SNAPSHOT METRICS ----------
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
