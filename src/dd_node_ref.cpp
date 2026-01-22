// SAPPOROBDD 2.0 - DD Node Reference implementation
// MIT License

#include "sbdd2/dd_node_ref.hpp"
#include "sbdd2/bdd.hpp"
#include "sbdd2/zdd.hpp"

namespace sbdd2 {

// Get the actual node pointer
const DDNode* DDNodeRef::node_ptr() const {
    if (!manager_ || arc_.is_constant()) {
        return nullptr;
    }
    return &manager_->node_at(arc_.index());
}

// Get variable number
bddvar DDNodeRef::var() const {
    if (arc_.is_constant()) {
        return 0;  // Terminals don't have a variable
    }
    const DDNode* node = node_ptr();
    return node ? node->var() : 0;
}

// Check if reduced
bool DDNodeRef::is_reduced() const {
    if (arc_.is_constant()) {
        return true;  // Terminals are always reduced
    }
    const DDNode* node = node_ptr();
    return node ? node->is_reduced() : false;
}

// Raw child accessors (as stored, ignoring negation on this reference)
DDNodeRef DDNodeRef::raw_child0() const {
    if (arc_.is_constant()) {
        return DDNodeRef();  // Invalid
    }
    const DDNode* node = node_ptr();
    if (!node) return DDNodeRef();
    return DDNodeRef(manager_, node->arc0());
}

DDNodeRef DDNodeRef::raw_child1() const {
    if (arc_.is_constant()) {
        return DDNodeRef();
    }
    const DDNode* node = node_ptr();
    if (!node) return DDNodeRef();
    return DDNodeRef(manager_, node->arc1());
}

// Logical child accessors (considering negation edge on this reference)
DDNodeRef DDNodeRef::child0() const {
    if (arc_.is_constant()) {
        return DDNodeRef();
    }
    const DDNode* node = node_ptr();
    if (!node) return DDNodeRef();

    Arc child_arc = node->arc0();
    // If this reference is negated, negate the child
    if (arc_.is_negated()) {
        child_arc = child_arc.negated();
    }
    return DDNodeRef(manager_, child_arc);
}

DDNodeRef DDNodeRef::child1() const {
    if (arc_.is_constant()) {
        return DDNodeRef();
    }
    const DDNode* node = node_ptr();
    if (!node) return DDNodeRef();

    Arc child_arc = node->arc1();
    // If this reference is negated, negate the child
    if (arc_.is_negated()) {
        child_arc = child_arc.negated();
    }
    return DDNodeRef(manager_, child_arc);
}

// Check if child edges are negated (in the raw node)
bool DDNodeRef::is_child0_negated() const {
    if (arc_.is_constant()) return false;
    const DDNode* node = node_ptr();
    if (!node) return false;
    return node->arc0().is_negated();
}

bool DDNodeRef::is_child1_negated() const {
    if (arc_.is_constant()) return false;
    const DDNode* node = node_ptr();
    if (!node) return false;
    return node->arc1().is_negated();
}

// Get non-negated version
DDNodeRef DDNodeRef::positive() const {
    if (!arc_.is_negated()) return *this;
    return DDNodeRef(manager_, Arc(arc_.data & ~1ULL));
}

// Get negated version
DDNodeRef DDNodeRef::negated() const {
    return DDNodeRef(manager_, arc_.negated());
}

// Convert to BDD
BDD DDNodeRef::to_bdd() const {
    if (!manager_) {
        return BDD();
    }
    return BDD(manager_, arc_);
}

// Convert to ZDD
ZDD DDNodeRef::to_zdd() const {
    if (!manager_) {
        return ZDD();
    }
    return ZDD(manager_, arc_);
}

} // namespace sbdd2
