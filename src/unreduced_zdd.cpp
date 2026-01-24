// SAPPOROBDD 2.0 - UnreducedZDD implementation
// MIT License

#include "sbdd2/unreduced_zdd.hpp"
#include "sbdd2/zdd.hpp"
#include "sbdd2/qdd.hpp"
#include <unordered_map>
#include <functional>

namespace sbdd2 {

// Static factory methods
UnreducedZDD UnreducedZDD::empty(DDManager& mgr) {
    return UnreducedZDD(&mgr, ARC_TERMINAL_0);
}

UnreducedZDD UnreducedZDD::single(DDManager& mgr) {
    return UnreducedZDD(&mgr, ARC_TERMINAL_1);
}

// Create node without reduction
UnreducedZDD UnreducedZDD::node(DDManager& mgr, bddvar var,
                                 const UnreducedZDD& low, const UnreducedZDD& high) {
    // Create node without applying ZDD reduction rule
    // This allows redundant nodes (high == terminal 0)
    Arc arc = mgr.get_or_create_node_zdd(var, low.arc_, high.arc_, false);
    return UnreducedZDD(&mgr, arc);
}

// Low/High children
UnreducedZDD UnreducedZDD::low() const {
    if (!manager_ || arc_.is_constant()) {
        return *this;
    }
    const DDNode& node = manager_->node_at(arc_.index());
    return UnreducedZDD(manager_, node.arc0());
}

UnreducedZDD UnreducedZDD::high() const {
    if (!manager_ || arc_.is_constant()) {
        return *this;
    }
    const DDNode& node = manager_->node_at(arc_.index());
    return UnreducedZDD(manager_, node.arc1());
}

// Check if already reduced
bool UnreducedZDD::is_reduced() const {
    if (!manager_ || arc_.is_constant()) {
        return true;
    }
    return manager_->node_at(arc_.index()).is_reduced();
}

// Convert to reduced ZDD
ZDD UnreducedZDD::reduce() const {
    if (!manager_) return ZDD();
    if (arc_.is_constant()) {
        return ZDD(manager_, arc_);
    }

    // Reduction with memoization
    std::unordered_map<std::uint64_t, Arc> memo;

    std::function<Arc(Arc)> reduce_rec = [&](Arc a) -> Arc {
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

        // Recursively reduce children
        Arc r_low = reduce_rec(low);
        Arc r_high = reduce_rec(high);

        // Apply ZDD reduction rule: if high == terminal 0, skip this node
        Arc result;
        if (r_high == ARC_TERMINAL_0) {
            result = r_low;
        } else {
            result = manager_->get_or_create_node_zdd(v, r_low, r_high, true);
        }

        memo[a.data] = result;
        return result;
    };

    Arc result = reduce_rec(arc_);
    return ZDD(manager_, result);
}

// Convert to QDD
QDD UnreducedZDD::to_qdd() const {
    if (!manager_) return QDD();
    if (arc_.is_constant()) {
        return QDD(manager_, arc_);
    }

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

        Arc r_low = convert_rec(low);
        Arc r_high = convert_rec(high);

        // Create node with sharing but without reduction
        Arc result = manager_->get_or_create_node_zdd(v, r_low, r_high, false);

        memo[a.data] = result;
        return result;
    };

    Arc result = convert_rec(arc_);
    return QDD(manager_, result);
}

} // namespace sbdd2
