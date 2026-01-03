#include "sim/process.h"

#include <stdexcept>

namespace sim {

Process::Process(uint32_t pid, uint64_t heap_base, uint64_t heap_size)
    : pid_(pid),
      state_(ProcessState::NEW),
      heap_(std::make_unique<HeapAllocator>(heap_base, heap_size)) {}

uint32_t Process::pid() const noexcept {
    return pid_;
}

// ---------------- State ----------------

ProcessState Process::state() const noexcept {
    return state_;
}

void Process::set_state(ProcessState s) {
    state_ = s;
}

// ---------------- Page table ----------------

bool Process::has_mapping(uint64_t vpn) const {
    auto it = page_table_.find(vpn);
    return it != page_table_.end() && it->second.valid;
}

PageTableEntry Process::get_pte(uint64_t vpn) const {
    auto it = page_table_.find(vpn);
    if (it != page_table_.end())
        return it->second;
    return PageTableEntry{};
}

void Process::map_page(uint64_t vpn, int frame_id) {
    page_table_[vpn] = PageTableEntry{true, frame_id};
}

void Process::unmap_page(uint64_t vpn) {
    auto it = page_table_.find(vpn);
    if (it != page_table_.end()) {
        it->second.valid = false;
        it->second.frame_id = -1;
    }
}

void Process::clear_page_table() {
    page_table_.clear();
}

// ---------------- Blocking ----------------

void Process::block_on_page(uint64_t vpn) {
    blocked_vpn_ = vpn;
    state_ = ProcessState::BLOCKED;
}

void Process::clear_block() {
    blocked_vpn_.reset();
}

bool Process::is_blocked() const noexcept {
    return state_ == ProcessState::BLOCKED;
}

std::optional<uint64_t> Process::blocked_vpn() const noexcept {
    return blocked_vpn_;
}

// ---------------- Heap ----------------

std::optional<uint64_t> Process::heap_alloc(uint64_t size) {
    return heap_->alloc(size);
}

void Process::heap_free(uint64_t addr) {
    heap_->free(addr);
}

const HeapAllocator& Process::heap() const noexcept {
    return *heap_;
}

} // namespace sim
