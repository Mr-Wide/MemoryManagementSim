#include "sim/simrunner.h"

#include <iostream>
#include <string>

#include "sim/clock.h"
#include "sim/event.h"
#include "sim/workload.h"
#include "sim/MMU.h"
#include "sim/physicalmem.h"
#include "sim/scheduler.h"
#include "sim/metrics.h"

using namespace sim;

static uint64_t parse_u64(const std::string &s) {
    if (s.size() > 2 && s[0] == '0' &&
        (s[1] == 'x' || s[1] == 'X'))
        return std::stoull(s, nullptr, 16);
    return std::stoull(s);
}

bool sim::run_simulation(const SimConfig& cfg, Timeline& timeline) {

    Clock clock;
    EventQueue eq;
    Workload wl(cfg.trace_file);

    if (!wl.parse_into(eq)) {
        std::cerr << "Trace parse failed\n";
        return false;
    }

    constexpr uint64_t PAGE_SIZE = 4096;
    constexpr size_t NUM_FRAMES = 4;
    constexpr uint64_t PAGEIN_LATENCY = 10;

    PhysicalMemory pmem(NUM_FRAMES);
    constexpr size_t TLB_SIZE = 16;
    Metrics metrics;
    MMU mmu(pmem, PAGE_SIZE, TLB_SIZE, metrics);
    Scheduler sched;

    size_t page_faults = 0;

    while (!eq.empty()) {
        Event ev = eq.pop();
        if (ev.key.time > clock.now())
            clock.set(ev.key.time);

        uint32_t pid = ev.key.pid;

        if (ev.type == "PROC_START") {
            uint64_t base = parse_u64(ev.args[0]);
            uint64_t top  = parse_u64(ev.args[1]);

            mmu.register_process(pid, base, top - base, cfg.strategy);
            sched.add_process(pid);

            timeline.log(clock.now(), pid, "PROC_START");
        }
        else if (ev.type == "PROC_EXIT") {
            mmu.unregister_process(pid);
            sched.terminate_process(pid);

            timeline.log(clock.now(), pid, "PROC_EXIT");
        }
        else if (ev.type == "MALLOC") {
            auto &proc = mmu.process(pid);
            uint64_t size = parse_u64(ev.args[0]);

            auto addr = proc.heap_alloc(size);
            if (addr) {
                timeline.log(clock.now(), pid,
                    "MALLOC size=" + std::to_string(size));

                metrics.update_heap(
                    proc.heap().total_heap_size(),
                    proc.heap().allocated_bytes(),
                    proc.heap().free_bytes(),
                    proc.heap().largest_free_block(),
                    proc.heap().internal_fragmentation()
                );
            }
        }
        else if (ev.type == "FREE") {
            auto &proc = mmu.process(pid);
            uint64_t addr = parse_u64(ev.args[0]);
            proc.heap_free(addr);

            timeline.log(clock.now(), pid, "FREE");
        }
        else if (ev.type == "ACCESS") {
            auto running = sched.schedule_next();
            if (!running) continue;

            uint64_t vaddr = parse_u64(ev.args[0]);
            auto res = mmu.access(*running, vaddr);

            if (res != MMUAccessResult::HIT) {
                ++page_faults;
                sched.block_current();

                uint64_t vpn = mmu.vpn_from_vaddr(vaddr);
                eq.push(clock.now() + PAGEIN_LATENCY,
                        0, *running,
                        "PAGEIN_COMPLETE",
                        { std::to_string(vpn) });
            }
        }
        else if (ev.type == "PAGEIN_COMPLETE") {
            uint64_t vpn = std::stoull(ev.args[0]);
            mmu.complete_pagein(pid, vpn, clock.now());
            sched.wake_process(pid);
        }

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

    return true;
}
