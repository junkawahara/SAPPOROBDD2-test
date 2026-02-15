/**
 * @file mvdd_node_ref.cpp
 * @brief SAPPOROBDD 2.0 - MVDD Node Reference implementation
 * @author SAPPOROBDD Team
 * @copyright MIT License
 */

#include "sbdd2/mvdd_node_ref.hpp"
#include "sbdd2/mvbdd.hpp"
#include "sbdd2/mvzdd.hpp"

namespace sbdd2 {

// Get the actual node pointer
const DDNode* MVDDNodeRef::node_ptr() const {
    if (!manager_ || arc_.is_constant()) {
        return nullptr;
    }
    return &manager_->node_at(arc_.index());
}

// Get internal DD variable number
bddvar MVDDNodeRef::dd_var() const {
    if (arc_.is_constant()) {
        return 0;
    }
    const DDNode* node = node_ptr();
    return node ? node->var() : 0;
}

// Get MVDD variable number
bddvar MVDDNodeRef::mvdd_var() const {
    if (!var_table_ || arc_.is_constant()) {
        return 0;
    }
    bddvar dv = dd_var();
    return var_table_->mvdd_var_of(dv);
}

// Get child for specified value
MVDDNodeRef MVDDNodeRef::child(int value) const {
    if (!is_valid()) {
        return MVDDNodeRef();
    }
    if (value < 0 || value >= k()) {
        return MVDDNodeRef();
    }
    if (arc_.is_constant()) {
        return *this;
    }

    // Get top MVDD variable
    bddvar top_mv = mvdd_var();
    if (top_mv == 0) {
        // Internal DD variable is not mapped to MVDD variable
        return *this;
    }

    const auto& dd_vars = var_table_->dd_vars_of(top_mv);
    int num_dd_vars = static_cast<int>(dd_vars.size());

    // Traverse internal DD structure for the specified value
    // dd_vars[0] is lowest level (closest to terminal)
    // dd_vars[k-2] is highest level (closest to root)
    Arc current = arc_;

    for (int i = num_dd_vars - 1; i >= 0; --i) {
        if (current.is_constant()) {
            break;
        }

        const DDNode& node = manager_->node_at(current.index());
        bddvar node_var = node.var();

        // Check if this node's variable matches the expected DD variable
        if (node_var != dd_vars[i]) {
            // Variable is skipped in the DD structure
            // In ZDD: skipped means 0-arc (no element)
            // In BDD: skipped means both arcs lead to same child
            if (i == value - 1 && value > 0) {
                // The variable for this value is skipped
                // For ZDD: return terminal 0 (this value is not selectable)
                // For BDD: continue with current arc (don't care)
                // We treat it conservatively - return empty for ZDD case
                // Note: This is a simplification; actual behavior depends on DD type
                return MVDDNodeRef(manager_, ARC_TERMINAL_0, var_table_);
            }
            continue;
        }

        Arc arc0 = node.arc0();
        Arc arc1 = node.arc1();

        // Handle negation edge (for BDD)
        if (current.is_negated()) {
            arc0 = arc0.negated();
            arc1 = arc1.negated();
        }

        if (i == value - 1 && value > 0) {
            // Select 1-arc for this value
            return MVDDNodeRef(manager_, arc1, var_table_);
        } else {
            // Select 0-arc
            current = arc0;
        }
    }

    // value == 0 or finished traversing all DD variables for this MVDD variable
    return MVDDNodeRef(manager_, current, var_table_);
}

// Convert to MVBDD
MVBDD MVDDNodeRef::to_mvbdd() const {
    if (!is_valid()) {
        return MVBDD();
    }
    BDD bdd(manager_, arc_);
    // Need to create MVBDD with shared var_table
    // Since MVDDVarTable is const, we need to cast or copy
    auto table = std::make_shared<MVDDVarTable>(*var_table_);
    return MVBDD(manager_, table, bdd);
}

// Convert to MVZDD
MVZDD MVDDNodeRef::to_mvzdd() const {
    if (!is_valid()) {
        return MVZDD();
    }
    ZDD zdd(manager_, arc_);
    auto table = std::make_shared<MVDDVarTable>(*var_table_);
    return MVZDD(manager_, table, zdd);
}

} // namespace sbdd2
