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

    // ---------------- Core ----------------
    Clock clock;
    EventQueue eq;
    Workload wl(argv[1]);

    if (!wl.parse_into(eq)) {
        std::cerr << "Trace parse failed\n";
        return 1;
    }

    constexpr uint64_t PAGE_SIZE = 4096;
    constexpr size_t   NUM_FRAMES = 4;
    constexpr uint64_t PAGEIN_LATENCY = 10;

    PhysicalMemory pmem(NUM_FRAMES);
    constexpr size_t TLB_SIZE = 16;
    Metrics metrics;
    MMU mmu(pmem, PAGE_SIZE, TLB_SIZE, metrics);
    Scheduler sched;

    size_t page_faults = 0;

    std::cout << "\nStarting simulation (Milestone D — allocator + fragmentation)\n\n";

    // ---------------- Event loop ----------------
    while (!eq.empty()) {
        Event ev = eq.pop();

        if (ev.key.time > clock.now())
            clock.set(ev.key.time);

        uint32_t pid = ev.key.pid;

        // ---------- PROCESS START ----------
        if (ev.type == "PROC_START") {
            uint64_t heap_base = parse_u64(ev.args[0]);
            uint64_t heap_top  = parse_u64(ev.args[1]);
            uint64_t heap_size = heap_top - heap_base;

            mmu.register_process(pid, heap_base, heap_size);
            sched.add_process(pid);

            std::cout << "[t=" << clock.now()
                      << "] PROC_START pid=" << pid << "\n";
        }

        // ---------- PROCESS EXIT ----------
        else if (ev.type == "PROC_EXIT") {
            mmu.unregister_process(pid);
            sched.terminate_process(pid);

            std::cout << "[t=" << clock.now()
                      << "] PROC_EXIT pid=" << pid << "\n";
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

            metrics.update_heap(
                proc.heap().total_heap_size(),
                proc.heap().allocated_bytes(),
                proc.heap().free_bytes(),
                proc.heap().largest_free_block(),
                proc.heap().internal_fragmentation()
            );

            std::cout << "[t=" << clock.now()
                      << "] MALLOC pid=" << pid
                      << " size=" << size
                      << " → addr=0x" << std::hex << *addr << std::dec
                      << "\n";
        }

        // ---------- FREE ----------
        else if (ev.type == "FREE") {
            auto &proc = mmu.process(pid);
            uint64_t addr = parse_u64(ev.args[0]);

            proc.heap_free(addr);

            metrics.update_heap(
                proc.heap().total_heap_size(),
                proc.heap().allocated_bytes(),
                proc.heap().free_bytes(),
                proc.heap().largest_free_block(),
                proc.heap().internal_fragmentation()
            );

            std::cout << "[t=" << clock.now()
                      << "] FREE pid=" << pid
                      << " addr=0x" << std::hex << addr << std::dec
                      << "\n";
        }

        // ---------- ACCESS ----------
        else if (ev.type == "ACCESS") {
            auto running = sched.schedule_next();
            if (!running)
                continue;

            uint64_t vaddr = parse_u64(ev.args[0]);
            auto res = mmu.access(*running, vaddr);

            if (res == MMUAccessResult::HIT) {
                std::cout << "[t=" << clock.now()
                          << "] ACCESS pid=" << *running
                          << " vaddr=0x" << std::hex << vaddr << std::dec
                          << " (hit)\n";
            } else {
                ++page_faults;
                sched.block_current();

                uint64_t vpn = mmu.vpn_from_vaddr(vaddr);
                eq.push(clock.now() + PAGEIN_LATENCY,
                        0,
                        *running,
                        "PAGEIN_COMPLETE",
                        { std::to_string(vpn) });

                std::cout << "[t=" << clock.now()
                          << "] PAGE_FAULT pid=" << *running
                          << " vpn=" << vpn
                          << " → BLOCKED\n";
            }
        }

        // ---------- PAGEIN COMPLETE ----------
        else if (ev.type == "PAGEIN_COMPLETE") {
            uint64_t vpn = std::stoull(ev.args[0]);

            mmu.complete_pagein(pid, vpn, clock.now());
            sched.wake_process(pid);

            std::cout << "[t=" << clock.now()
                      << "] PAGEIN_COMPLETE pid=" << pid
                      << " vpn=" << vpn
                      << " → READY\n";
        }
    }

    // ---------------- Summary ----------------
    std::cout << "\nSimulation complete\n";
    std::cout << "Total page faults: " << page_faults << "\n";

    std::cout << "\nFinal heap metrics:\n";
    std::cout << "  allocated_bytes = " << metrics.allocated_bytes() << "\n";
    std::cout << "  free_bytes      = " << metrics.free_bytes() << "\n";
    std::cout << "  largest_free    = " << metrics.largest_free_block() << "\n";
    std::cout << "  internal_frag   = " << metrics.internal_fragmentation() << "\n";
    std::cout << "  external_frag   = " << metrics.external_fragmentation() << "\n";

    return 0;
}
