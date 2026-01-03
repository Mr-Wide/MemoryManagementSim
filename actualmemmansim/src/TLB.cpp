#include "sim/TLB.h"

#include <deque>
#include <algorithm>

namespace sim {

/*
 * FIFO TLB implementation
 *
 * Internal model:
 *  - deque<TLBEntry> entries_
 *  - front() = oldest
 *  - back()  = newest
 */

class TLBImpl {
public:
    

    explicit TLBImpl(size_t cap)
        : capacity(cap), hits(0), misses(0) {}

    std::optional<int> lookup(uint32_t pid, uint64_t vpn) {
        for (auto &e : entries) {
            if (e.pid == pid && e.vpn == vpn) {
                ++hits;
                return e.frame_id;
            }
        }
        ++misses;
        return std::nullopt;
    }

    void insert(uint32_t pid, uint64_t vpn, int frame_id) {
        // If entry already exists, update it (no FIFO reorder)
        for (auto &e : entries) {
            if (e.pid == pid && e.vpn == vpn) {
                e.frame_id = frame_id;
                return;
            }
        }

        // Evict oldest if full
        if (entries.size() >= capacity) {
            entries.pop_front();
        }

        entries.push_back(TLBEntry{pid, vpn, frame_id, false});
    }

    void flush_process(uint32_t pid) {
        entries.erase(
            std::remove_if(entries.begin(), entries.end(),
                [pid](const TLBEntry &e) {
                    return e.pid == pid;
                }),
            entries.end()
        );
    }

    void invalidate(uint32_t pid, uint64_t vpn) {
        entries.erase(
            std::remove_if(entries.begin(), entries.end(),
                [pid, vpn](const TLBEntry &e) {
                    return e.pid == pid && e.vpn == vpn;
                }),
            entries.end()
        );
    }

    void flush_all() {
        entries.clear();
    }

    size_t capacity;
    std::deque<TLBEntry> entries;
    uint64_t hits;
    uint64_t misses;
};
TLB::~TLB() = default;
// ---------------- TLB public wrapper ----------------

TLB::TLB(size_t capacity)
    : capacity_(capacity),
      hits_(0),
      misses_(0),
      impl_(std::make_unique<TLBImpl>(capacity)) {}

std::optional<int> TLB::lookup(uint32_t pid, uint64_t vpn) {
    auto res = impl_->lookup(pid, vpn);
    hits_   = impl_->hits;
    misses_ = impl_->misses;
    return res;
}

void TLB::insert(uint32_t pid, uint64_t vpn, int frame_id) {
    impl_->insert(pid, vpn, frame_id);
}

void TLB::flush_process(uint32_t pid) {
    impl_->flush_process(pid);
}

void TLB::invalidate(uint32_t pid, uint64_t vpn) {
    impl_->invalidate(pid, vpn);
}

void TLB::flush_all() {
    impl_->flush_all();
}

uint64_t TLB::hits() const noexcept {
    return hits_;
}

uint64_t TLB::misses() const noexcept {
    return misses_;
}

double TLB::hit_rate() const noexcept {
    uint64_t total = hits_ + misses_;
    if (total == 0) return 0.0;
    return static_cast<double>(hits_) / static_cast<double>(total);
}

} // namespace sim
