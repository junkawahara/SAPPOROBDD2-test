// SAPPOROBDD 2.0 - DD Node Reference (read-only, no ref counting)
// MIT License

#ifndef SBDD2_DD_NODE_REF_HPP
#define SBDD2_DD_NODE_REF_HPP

#include "types.hpp"
#include "dd_node.hpp"
#include "dd_manager.hpp"

namespace sbdd2 {

// Forward declarations
class BDD;
class ZDD;

// DDNodeRef - Read-only reference to a DD node
// Does NOT manage reference count (for performance during traversal)
// WARNING: Must not outlive the source BDD/ZDD
class DDNodeRef {
public:
    // Default constructor (creates invalid reference)
    DDNodeRef() : manager_(nullptr), arc_() {}

    // Constructor from arc and manager
    DDNodeRef(DDManager* manager, Arc arc)
        : manager_(manager), arc_(arc) {}

    // Copy constructor
    DDNodeRef(const DDNodeRef&) = default;
    DDNodeRef& operator=(const DDNodeRef&) = default;

    // Check if valid
    bool is_valid() const { return manager_ != nullptr; }
    explicit operator bool() const { return is_valid(); }

    // Check if terminal
    bool is_terminal() const { return arc_.is_constant(); }
    bool is_terminal_zero() const { return arc_ == ARC_TERMINAL_0; }
    bool is_terminal_one() const { return arc_ == ARC_TERMINAL_1; }

    // Get terminal value (only valid if is_terminal())
    bool terminal_value() const {
        return arc_.terminal_value() != arc_.is_negated();
    }

    // Get variable number (undefined for terminal)
    bddvar var() const;

    // Check if this node is reduced
    bool is_reduced() const;

    // Raw children (as stored in node, ignoring negation)
    DDNodeRef raw_child0() const;
    DDNodeRef raw_child1() const;

    // Logical children (considering negation edge)
    DDNodeRef child0() const;
    DDNodeRef child1() const;

    // Check if child edges are negated
    bool is_child0_negated() const;
    bool is_child1_negated() const;

    // Check if this reference is negated
    bool is_negated() const { return arc_.is_negated(); }

    // Get non-negated version
    DDNodeRef positive() const;

    // Get negated version
    DDNodeRef negated() const;

    // Convert to BDD/ZDD (increments reference count)
    BDD to_bdd() const;
    ZDD to_zdd() const;

    // Raw arc access
    Arc arc() const { return arc_; }

    // Manager access
    DDManager* manager() const { return manager_; }

    // Comparison
    bool operator==(const DDNodeRef& other) const {
        return manager_ == other.manager_ && arc_ == other.arc_;
    }
    bool operator!=(const DDNodeRef& other) const {
        return !(*this == other);
    }

    // Get index (for debugging/serialization)
    bddindex index() const { return arc_.index(); }

private:
    DDManager* manager_;
    Arc arc_;

    // Get the actual node (internal use)
    const DDNode* node_ptr() const;
};

} // namespace sbdd2

#endif // SBDD2_DD_NODE_REF_HPP
