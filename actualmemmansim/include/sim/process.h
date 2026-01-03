#ifndef SIM_PROCESS_H
#define SIM_PROCESS_H

#include <cstdint>
#include <unordered_map>
#include <optional>
#include <memory>

#include "sim/allocator.h"

namespace sim {

// ---------------- Page Table ----------------

struct PageTableEntry {
    bool valid = false;
    int frame_id = -1;
};

// ---------------- Process State ----------------

enum class ProcessState {
    NEW,
    READY,
    RUNNING,
    BLOCKED,
    TERMINATED
};

// ---------------- Process ----------------

class Process {
public:
    Process(uint32_t pid, uint64_t heap_base, uint64_t heap_size);

    uint32_t pid() const noexcept;

    // -------- State management --------
    ProcessState state() const noexcept;
    void set_state(ProcessState s);

    // -------- Page table operations --------
    bool has_mapping(uint64_t vpn) const;
    PageTableEntry get_pte(uint64_t vpn) const;

    void map_page(uint64_t vpn, int frame_id);
    void unmap_page(uint64_t vpn);
    void clear_page_table();

    // -------- Blocking info --------
    void block_on_page(uint64_t vpn);
    void clear_block();

    bool is_blocked() const noexcept;
    std::optional<uint64_t> blocked_vpn() const noexcept;

    // -------- Heap interface --------
    std::optional<uint64_t> heap_alloc(uint64_t size);
    void heap_free(uint64_t addr);

    // -------- Heap metrics --------
    const HeapAllocator& heap() const noexcept;

private:
    uint32_t pid_;
    ProcessState state_;

    // Page table: VPN → PTE
    std::unordered_map<uint64_t, PageTableEntry> page_table_;

    // Blocking
    std::optional<uint64_t> blocked_vpn_;

    // Heap allocator (owned by process)
    std::unique_ptr<HeapAllocator> heap_;
};

} // namespace sim

#endif // SIM_PROCESS_H
