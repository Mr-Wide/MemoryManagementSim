#ifndef SIM_MMU_H
#define SIM_MMU_H

#include <cstdint>
#include <unordered_map>

#include "sim/process.h"
#include "sim/physicalmem.h"
#include "sim/clock.h"
#include "sim/TLB.h"
#include "sim/metrics.h"



namespace sim {

// Result of a memory access attempt
enum class MMUAccessResult {
    HIT,
    PAGE_FAULT
};

class MMU {
public:
    MMU(PhysicalMemory &pmem,
    uint64_t page_size,
    size_t tlb_size,
    Metrics &metrics);

    // Process lifecycle
    void register_process(uint32_t pid,
                      uint64_t heap_base,
                      uint64_t heap_size);

    void unregister_process(uint32_t pid);

    // Attempt an access (does NOT resolve faults)
    MMUAccessResult access(uint32_t pid, uint64_t vaddr);

    // Complete a previously faulted page-in
    void complete_pagein(uint32_t pid, uint64_t vpn, uint64_t now);

    uint64_t vpn_from_vaddr(uint64_t vaddr) const noexcept;

    Process& process(uint32_t pid);

private:
    PhysicalMemory &pmem_;
    uint64_t page_size_;
    TLB tlb_;
    Metrics &metrics_;

    std::unordered_map<uint32_t, Process> processes_;
};

} // namespace sim

#endif // SIM_MMU_H
