// SAPPOROBDD 2.0 - UnreducedBDD implementation
// MIT License

#include "sbdd2/unreduced_bdd.hpp"
#include "sbdd2/bdd.hpp"
#include "sbdd2/qdd.hpp"
#include <stack>
#include <unordered_map>

namespace sbdd2 {

// Static factory methods
UnreducedBDD UnreducedBDD::zero(DDManager& mgr) {
    return UnreducedBDD(&mgr, ARC_TERMINAL_0);
}

UnreducedBDD UnreducedBDD::one(DDManager& mgr) {
    return UnreducedBDD(&mgr, ARC_TERMINAL_1);
}

// Create node without reduction
UnreducedBDD UnreducedBDD::node(DDManager& mgr, bddvar var,
                                 const UnreducedBDD& low, const UnreducedBDD& high) {
    // Create node without applying BDD reduction rule
    // This allows redundant nodes (low == high)
    Arc arc = mgr.get_or_create_node_bdd(var, low.arc_, high.arc_, false);
    return UnreducedBDD(&mgr, arc);
}

// Low/High children
UnreducedBDD UnreducedBDD::low() const {
    if (!manager_ || arc_.is_constant()) {
        return *this;
    }
    const DDNode& node = manager_->node_at(arc_.index());
    Arc child = node.arc0();
    if (arc_.is_negated()) {
        child = child.negated();
    }
    return UnreducedBDD(manager_, child);
}

UnreducedBDD UnreducedBDD::high() const {
    if (!manager_ || arc_.is_constant()) {
        return *this;
    }
    const DDNode& node = manager_->node_at(arc_.index());
    Arc child = node.arc1();
    if (arc_.is_negated()) {
        child = child.negated();
    }
    return UnreducedBDD(manager_, child);
}

// Negation
UnreducedBDD UnreducedBDD::operator~() const {
    if (!manager_) return UnreducedBDD();
    return UnreducedBDD(manager_, arc_.negated());
}

// Check if already reduced
bool UnreducedBDD::is_reduced() const {
    if (!manager_ || arc_.is_constant()) {
        return true;
    }
    return manager_->node_at(arc_.index()).is_reduced();
}

// Convert to reduced BDD
BDD UnreducedBDD::reduce() const {
    if (!manager_) return BDD();
    if (arc_.is_constant()) {
        return BDD(manager_, arc_);
    }

    // Reduction with memoization
    std::unordered_map<std::uint64_t, Arc> memo;

    std::function<Arc(Arc)> reduce_rec = [&](Arc a) -> Arc {
        if (a.is_constant()) {
            return a;
        }

        // Check memo
        auto it = memo.find(a.data);
        if (it != memo.end()) {
            return it->second;
        }

        const DDNode& node = manager_->node_at(a.index());
        bddvar v = node.var();

        Arc low = node.arc0();
        Arc high = node.arc1();
        if (a.is_negated()) {
            low = low.negated();
            high = high.negated();
        }

        // Recursively reduce children
        Arc r_low = reduce_rec(low);
        Arc r_high = reduce_rec(high);

        // Apply BDD reduction rule: if low == high, skip this node
        Arc result;
        if (r_low.data == r_high.data) {
            result = r_low;
        } else {
            result = manager_->get_or_create_node_bdd(v, r_low, r_high, true);
        }

        memo[a.data] = result;
        return result;
    };

    Arc result = reduce_rec(arc_);
    return BDD(manager_, result);
}

// Convert to QDD
QDD UnreducedBDD::to_qdd() const {
    if (!manager_) return QDD();
    if (arc_.is_constant()) {
        return QDD(manager_, arc_);
    }

    // QDD applies node sharing but not reduction rule
    std::unordered_map<std::uint64_t, Arc> memo;

    std::function<Arc(Arc)> convert_rec = [&](Arc a) -> Arc {
        if (a.is_constant()) {
            return a;
        }

        auto it = memo.find(a.data);
        if (it != memo.end()) {
            return it->second;
        }

        const DDNode& node = manager_->node_at(a.index());
        bddvar v = node.var();

        Arc low = node.arc0();
        Arc high = node.arc1();
        if (a.is_negated()) {
            low = low.negated();
            high = high.negated();
        }

        Arc r_low = convert_rec(low);
        Arc r_high = convert_rec(high);

        // Create node with sharing but without reduction
        Arc result = manager_->get_or_create_node_bdd(v, r_low, r_high, false);

        memo[a.data] = result;
        return result;
    };

    Arc result = convert_rec(arc_);
    return QDD(manager_, result);
}

} // namespace sbdd2
