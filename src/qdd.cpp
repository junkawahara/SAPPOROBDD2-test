// SAPPOROBDD 2.0 - QDD implementation
// MIT License

#include "sbdd2/qdd.hpp"
#include "sbdd2/bdd.hpp"
#include "sbdd2/zdd.hpp"
#include "sbdd2/unreduced_bdd.hpp"
#include "sbdd2/unreduced_zdd.hpp"
#include <unordered_map>
#include <functional>

namespace sbdd2 {

// Constructors from Unreduced types
QDD::QDD(const UnreducedBDD& ubdd)
    : DDBase(ubdd.manager(), ubdd.arc())
{
    // QDD from UnreducedBDD: apply node sharing but not reduction
}

QDD::QDD(const UnreducedZDD& uzdd)
    : DDBase(uzdd.manager(), uzdd.arc())
{
    // QDD from UnreducedZDD: apply node sharing but not reduction
}

// Static factory methods
QDD QDD::zero(DDManager& mgr) {
    return QDD(&mgr, ARC_TERMINAL_0);
}

QDD QDD::one(DDManager& mgr) {
    return QDD(&mgr, ARC_TERMINAL_1);
}

// Create node with sharing but no skip rule
QDD QDD::node(DDManager& mgr, bddvar var, const QDD& low, const QDD& high) {
    // QDD does not apply skip rules, but does share nodes
    Arc arc = mgr.get_or_create_node_bdd(var, low.arc_, high.arc_, false);
    return QDD(&mgr, arc);
}

// Low/High children
QDD QDD::low() const {
    if (!manager_ || arc_.is_constant()) {
        return *this;
    }
    const DDNode& node = manager_->node_at(arc_.index());
    Arc child = node.arc0();
    if (arc_.is_negated()) {
        child = child.negated();
    }
    return QDD(manager_, child);
}

QDD QDD::high() const {
    if (!manager_ || arc_.is_constant()) {
        return *this;
    }
    const DDNode& node = manager_->node_at(arc_.index());
    Arc child = node.arc1();
    if (arc_.is_negated()) {
        child = child.negated();
    }
    return QDD(manager_, child);
}

// Negation
QDD QDD::operator~() const {
    if (!manager_) return QDD();
    return QDD(manager_, arc_.negated());
}

// Convert to BDD (applies BDD skip rule)
BDD QDD::to_bdd() const {
    if (!manager_) return BDD();
    if (arc_.is_constant()) {
        return BDD(manager_, arc_);
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
        if (a.is_negated()) {
            low = low.negated();
            high = high.negated();
        }

        Arc r_low = convert_rec(low);
        Arc r_high = convert_rec(high);

        // Apply BDD skip rule
        Arc result;
        if (r_low.data == r_high.data) {
            result = r_low;
        } else {
            result = manager_->get_or_create_node_bdd(v, r_low, r_high, true);
        }

        memo[a.data] = result;
        return result;
    };

    Arc result = convert_rec(arc_);
    return BDD(manager_, result);
}

// Convert to ZDD (applies ZDD skip rule)
ZDD QDD::to_zdd() const {
    if (!manager_) return ZDD();
    if (arc_.is_constant()) {
        return ZDD(manager_, arc_);
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

        // Apply ZDD skip rule
        Arc result;
        if (r_high == ARC_TERMINAL_0) {
            result = r_low;
        } else {
            result = manager_->get_or_create_node_zdd(v, r_low, r_high, true);
        }

        memo[a.data] = result;
        return result;
    };

    Arc result = convert_rec(arc_);
    return ZDD(manager_, result);
}

} // namespace sbdd2
