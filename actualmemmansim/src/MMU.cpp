#include "sim/MMU.h"
#include "sim/physicalmem.h"

#include <stdexcept>

namespace sim {

MMU::MMU(PhysicalMemory &pmem,
         uint64_t page_size,
         size_t tlb_size,
         Metrics &metrics)
    : pmem_(pmem),
      page_size_(page_size),
      tlb_(tlb_size),
      metrics_(metrics) {}

// ---------------- Process management ----------------

void MMU::register_process(uint32_t pid,
                           uint64_t heap_base,
                           uint64_t heap_size) {
    if (processes_.count(pid))
        throw std::runtime_error("MMU: process already registered");

    processes_.emplace(pid, Process(pid, heap_base, heap_size));
}

void MMU::unregister_process(uint32_t pid) {
    auto it = processes_.find(pid);
    if (it == processes_.end())
        return;

    tlb_.flush_process(pid);
    it->second.clear_page_table();
    processes_.erase(it);
}

Process &MMU::process(uint32_t pid) {
    auto it = processes_.find(pid);
    if (it == processes_.end())
        throw std::runtime_error("MMU: access from unknown process");
    return it->second;
}

// ---------------- Address helpers ----------------

uint64_t MMU::vpn_from_vaddr(uint64_t vaddr) const noexcept {
    return vaddr / page_size_;
}

// ---------------- Memory access ----------------

MMUAccessResult MMU::access(uint32_t pid, uint64_t vaddr) {
    auto &proc = process(pid);
    uint64_t vpn = vpn_from_vaddr(vaddr);

    // ---------- 1. TLB lookup ----------
    if (auto frame = tlb_.lookup(pid, vpn)) {
        metrics_.record_tlb_hit();
        metrics_.record_access_latency(1);   // fast path
        return MMUAccessResult::HIT;
    }

    metrics_.record_tlb_miss();

    // ---------- 2. Page table ----------
    if (proc.has_mapping(vpn)) {
        auto pte = proc.get_pte(vpn);
        tlb_.insert(pid, vpn, pte.frame_id);
        metrics_.record_access_latency(5);   // page-table hit
        return MMUAccessResult::HIT;
    }

    // ---------- 3. Page fault ----------
    metrics_.record_access_latency(100);     // page fault path
    return MMUAccessResult::PAGE_FAULT;
}

// ---------------- Page-in completion ----------------

void MMU::complete_pagein(uint32_t pid,
                          uint64_t vpn,
                          uint64_t now) {
    auto &proc = process(pid);

    // Allocate frame (may evict)
    FrameAllocResult res = pmem_.allocate(pid, vpn, now);

    // If eviction happened, clean up old mapping
    if (res.evicted) {
        auto &old_proc = process(res.evicted_pid);
        old_proc.unmap_page(res.evicted_vpn);
        tlb_.invalidate(res.evicted_pid, res.evicted_vpn);
    }

    // Map new page
    proc.map_page(vpn, res.frame_id);

    // Fill TLB
    tlb_.insert(pid, vpn, res.frame_id);
}


} // namespace sim
