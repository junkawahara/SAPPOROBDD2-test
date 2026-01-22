// SAPPOROBDD 2.0 - DD Node structure
// MIT License

#ifndef SBDD2_DD_NODE_HPP
#define SBDD2_DD_NODE_HPP

#include "types.hpp"
#include <atomic>
#include <cassert>

namespace sbdd2 {

// 128-bit node structure
// Layout (using two 64-bit words):
//
// Word 0 (low, 64 bits):
//   bits 0-43:  0-arc (44 bits)
//   bits 44-63: lower 20 bits of 1-arc
//
// Word 1 (high, 64 bits):
//   bits 0-23:  upper 24 bits of 1-arc
//   bit  24:    reduced flag
//   bits 25-40: reference count (16 bits)
//   bits 41-60: variable number (20 bits)
//   bits 61-63: reserved (3 bits)
//
class DDNode {
public:
    // Masks and shifts for word 0
    static constexpr std::uint64_t ARC0_MASK = (1ULL << 44) - 1;
    static constexpr int ARC1_LOW_SHIFT = 44;
    static constexpr std::uint64_t ARC1_LOW_MASK = (1ULL << 20) - 1;

    // Masks and shifts for word 1
    static constexpr int ARC1_HIGH_BITS = 24;
    static constexpr std::uint64_t ARC1_HIGH_MASK = (1ULL << 24) - 1;
    static constexpr int REDUCED_SHIFT = 24;
    static constexpr int REFCOUNT_SHIFT = 25;
    static constexpr std::uint64_t REFCOUNT_MASK = 0xFFFFULL;
    static constexpr int VAR_SHIFT = 41;
    static constexpr std::uint64_t VAR_MASK = (1ULL << 20) - 1;

private:
    std::uint64_t low_;   // word 0
    std::uint64_t high_;  // word 1

public:
    DDNode() : low_(0), high_(0) {}

    // Constructor with all fields
    DDNode(Arc arc0, Arc arc1, bddvar var, bool reduced = false, bddrefcount refcount = 0)
        : low_(0), high_(0)
    {
        set_arc0(arc0);
        set_arc1(arc1);
        set_var(var);
        set_reduced(reduced);
        set_refcount_raw(refcount);
    }

    // 0-arc accessors
    Arc arc0() const {
        return Arc(low_ & ARC0_MASK);
    }

    void set_arc0(Arc arc) {
        low_ = (low_ & ~ARC0_MASK) | (arc.data & ARC0_MASK);
    }

    // 1-arc accessors
    Arc arc1() const {
        std::uint64_t lo = (low_ >> ARC1_LOW_SHIFT) & ARC1_LOW_MASK;
        std::uint64_t hi = high_ & ARC1_HIGH_MASK;
        return Arc((hi << 20) | lo);
    }

    void set_arc1(Arc arc) {
        std::uint64_t lo = arc.data & ARC1_LOW_MASK;
        std::uint64_t hi = (arc.data >> 20) & ARC1_HIGH_MASK;
        low_ = (low_ & ARC0_MASK) | (lo << ARC1_LOW_SHIFT);
        high_ = (high_ & ~ARC1_HIGH_MASK) | hi;
    }

    // Variable number accessors
    bddvar var() const {
        return static_cast<bddvar>((high_ >> VAR_SHIFT) & VAR_MASK);
    }

    void set_var(bddvar v) {
        assert(v <= BDDVAR_MAX);
        high_ = (high_ & ~(VAR_MASK << VAR_SHIFT)) | (static_cast<std::uint64_t>(v) << VAR_SHIFT);
    }

    // Reduced flag accessors
    bool is_reduced() const {
        return (high_ & (1ULL << REDUCED_SHIFT)) != 0;
    }

    void set_reduced(bool r) {
        if (r) {
            high_ |= (1ULL << REDUCED_SHIFT);
        } else {
            high_ &= ~(1ULL << REDUCED_SHIFT);
        }
    }

    // Reference count accessors
    bddrefcount refcount() const {
        return static_cast<bddrefcount>((high_ >> REFCOUNT_SHIFT) & REFCOUNT_MASK);
    }

    // Raw set (non-atomic, for initialization)
    void set_refcount_raw(bddrefcount count) {
        high_ = (high_ & ~(REFCOUNT_MASK << REFCOUNT_SHIFT)) |
                (static_cast<std::uint64_t>(count) << REFCOUNT_SHIFT);
    }

    // Atomic increment (saturates at max)
    void inc_refcount() {
        bddrefcount current = refcount();
        if (current < BDDREFCOUNT_MAX) {
            set_refcount_raw(current + 1);
        }
    }

    // Atomic decrement (stops at 0, returns true if count became 0)
    bool dec_refcount() {
        bddrefcount current = refcount();
        if (current > 0 && current < BDDREFCOUNT_MAX) {
            set_refcount_raw(current - 1);
            return (current - 1) == 0;
        }
        return false;
    }

    // Check if node is in use (refcount > 0 or in hash table)
    bool is_alive() const {
        return refcount() > 0;
    }

    // Raw data access (for hashing, serialization)
    std::uint64_t raw_low() const { return low_; }
    std::uint64_t raw_high() const { return high_; }

    void set_raw(std::uint64_t lo, std::uint64_t hi) {
        low_ = lo;
        high_ = hi;
    }

    // Check if this slot is empty (all zeros)
    bool is_empty() const {
        return low_ == 0 && high_ == 0;
    }

    // Check if this slot is a tombstone (deleted)
    bool is_tombstone() const {
        // Use special marker: low = -1, high = 0
        return low_ == ~0ULL && high_ == 0;
    }

    // Mark as tombstone
    void mark_tombstone() {
        low_ = ~0ULL;
        high_ = 0;
    }

    // Clear the node
    void clear() {
        low_ = 0;
        high_ = 0;
    }

    // Equality (for hash table)
    bool equals(Arc arc0, Arc arc1, bddvar v) const {
        return this->arc0() == arc0 && this->arc1() == arc1 && var() == v;
    }
};

static_assert(sizeof(DDNode) == 16, "DDNode must be 128 bits (16 bytes)");

} // namespace sbdd2

#endif // SBDD2_DD_NODE_HPP
