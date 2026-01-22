// SAPPOROBDD 2.0 - ZDD implementation
// MIT License

#include "sbdd2/zdd.hpp"
#include "sbdd2/bdd.hpp"
#include <iostream>
#include <sstream>
#include <stack>
#include <unordered_set>
#include <unordered_map>
#include <functional>

namespace sbdd2 {

// Static factory methods
ZDD ZDD::empty(DDManager& mgr) {
    return mgr.zdd_empty();
}

ZDD ZDD::base(DDManager& mgr) {
    return mgr.zdd_base();
}

ZDD ZDD::single(DDManager& mgr, bddvar v) {
    // Single element set {v}
    Arc arc = mgr.get_or_create_node_zdd(v, ARC_TERMINAL_0, ARC_TERMINAL_1, true);
    return ZDD(&mgr, arc);
}

// Low/High children
ZDD ZDD::low() const {
    if (!manager_ || arc_.is_constant()) {
        return *this;
    }
    const DDNode& node = manager_->node_at(arc_.index());
    return ZDD(manager_, node.arc0());
}

ZDD ZDD::high() const {
    if (!manager_ || arc_.is_constant()) {
        return *this;
    }
    const DDNode& node = manager_->node_at(arc_.index());
    return ZDD(manager_, node.arc1());
}

// Family operations
ZDD ZDD::onset(bddvar v) const {
    if (!manager_ || arc_.is_constant()) {
        return ZDD::empty(*manager_);
    }

    bddvar top = manager_->node_at(arc_.index()).var();
    if (top > v) {
        return ZDD::empty(*manager_);
    }
    if (top == v) {
        return high();
    }

    // Recursive case
    const DDNode& node = manager_->node_at(arc_.index());
    ZDD lo(manager_, node.arc0());
    ZDD hi(manager_, node.arc1());

    ZDD lo_onset = lo.onset(v);
    ZDD hi_onset = hi.onset(v);

    // Reconstruct
    Arc result = manager_->get_or_create_node_zdd(top, lo_onset.arc_, hi_onset.arc_, true);
    return ZDD(manager_, result);
}

ZDD ZDD::offset(bddvar v) const {
    if (!manager_ || arc_.is_constant()) {
        return *this;
    }

    bddvar top = manager_->node_at(arc_.index()).var();
    if (top > v) {
        return *this;
    }
    if (top == v) {
        return low();
    }

    const DDNode& node = manager_->node_at(arc_.index());
    ZDD lo(manager_, node.arc0());
    ZDD hi(manager_, node.arc1());

    ZDD lo_offset = lo.offset(v);
    ZDD hi_offset = hi.offset(v);

    Arc result = manager_->get_or_create_node_zdd(top, lo_offset.arc_, hi_offset.arc_, true);
    return ZDD(manager_, result);
}

ZDD ZDD::onset0(bddvar v) const {
    // onset0: subsets containing v, keeping v
    ZDD on = onset(v);
    if (on.is_zero()) {
        return on;
    }

    // Add v back to all sets
    Arc result = manager_->get_or_create_node_zdd(v, ARC_TERMINAL_0, on.arc_, true);
    return ZDD(manager_, result);
}

ZDD ZDD::change(bddvar v) const {
    if (!manager_ || arc_.is_constant()) {
        if (arc_ == ARC_TERMINAL_0) {
            return *this;
        }
        // For base (terminal 1), toggle v means add v
        return ZDD::single(*manager_, v);
    }

    bddvar top = manager_->node_at(arc_.index()).var();
    const DDNode& node = manager_->node_at(arc_.index());

    if (top > v) {
        // v is not present, add v to all sets
        Arc result = manager_->get_or_create_node_zdd(v, ARC_TERMINAL_0, arc_, true);
        return ZDD(manager_, result);
    }

    if (top == v) {
        // Swap low and high
        Arc result = manager_->get_or_create_node_zdd(v, node.arc1(), node.arc0(), true);
        return ZDD(manager_, result);
    }

    // Recursive
    ZDD lo(manager_, node.arc0());
    ZDD hi(manager_, node.arc1());

    ZDD lo_change = lo.change(v);
    ZDD hi_change = hi.change(v);

    Arc result = manager_->get_or_create_node_zdd(top, lo_change.arc_, hi_change.arc_, true);
    return ZDD(manager_, result);
}

// Internal union function
static Arc zdd_union(DDManager* mgr, Arc f, Arc g) {
    // Terminal cases
    if (f == ARC_TERMINAL_0) return g;
    if (g == ARC_TERMINAL_0) return f;
    if (f == g) return f;

    // Order for cache
    if (f.data > g.data) std::swap(f, g);

    // Check cache
    Arc result;
    if (mgr->cache_lookup(CacheOp::UNION, f, g, result)) {
        return result;
    }

    bddvar f_var = f.is_constant() ? BDDVAR_MAX : mgr->node_at(f.index()).var();
    bddvar g_var = g.is_constant() ? BDDVAR_MAX : mgr->node_at(g.index()).var();
    bddvar top_var = std::min(f_var, g_var);

    Arc f0, f1, g0, g1;

    if (f.is_constant() || f_var > top_var) {
        f0 = f;
        f1 = ARC_TERMINAL_0;
    } else {
        const DDNode& node = mgr->node_at(f.index());
        f0 = node.arc0();
        f1 = node.arc1();
    }

    if (g.is_constant() || g_var > top_var) {
        g0 = g;
        g1 = ARC_TERMINAL_0;
    } else {
        const DDNode& node = mgr->node_at(g.index());
        g0 = node.arc0();
        g1 = node.arc1();
    }

    Arc r0 = zdd_union(mgr, f0, g0);
    Arc r1 = zdd_union(mgr, f1, g1);

    result = mgr->get_or_create_node_zdd(top_var, r0, r1, true);
    mgr->cache_insert(CacheOp::UNION, f, g, result);
    return result;
}

// Internal intersection function
static Arc zdd_intersect(DDManager* mgr, Arc f, Arc g) {
    if (f == ARC_TERMINAL_0) return ARC_TERMINAL_0;
    if (g == ARC_TERMINAL_0) return ARC_TERMINAL_0;
    if (f == g) return f;

    // Handle terminal 1 (base/empty set)
    if (f == ARC_TERMINAL_1) return g;
    if (g == ARC_TERMINAL_1) return f;

    if (f.data > g.data) std::swap(f, g);

    Arc result;
    if (mgr->cache_lookup(CacheOp::INTERSECT, f, g, result)) {
        return result;
    }

    bddvar f_var = mgr->node_at(f.index()).var();
    bddvar g_var = mgr->node_at(g.index()).var();

    if (f_var < g_var) {
        // f has smaller variable, g doesn't have it
        const DDNode& node = mgr->node_at(f.index());
        result = zdd_intersect(mgr, node.arc0(), g);
    } else if (f_var > g_var) {
        const DDNode& node = mgr->node_at(g.index());
        result = zdd_intersect(mgr, f, node.arc0());
    } else {
        const DDNode& f_node = mgr->node_at(f.index());
        const DDNode& g_node = mgr->node_at(g.index());

        Arc r0 = zdd_intersect(mgr, f_node.arc0(), g_node.arc0());
        Arc r1 = zdd_intersect(mgr, f_node.arc1(), g_node.arc1());

        result = mgr->get_or_create_node_zdd(f_var, r0, r1, true);
    }

    mgr->cache_insert(CacheOp::INTERSECT, f, g, result);
    return result;
}

// Internal difference function
static Arc zdd_diff(DDManager* mgr, Arc f, Arc g) {
    if (f == ARC_TERMINAL_0) return ARC_TERMINAL_0;
    if (g == ARC_TERMINAL_0) return f;
    if (f == g) return ARC_TERMINAL_0;

    Arc result;
    if (mgr->cache_lookup(CacheOp::DIFF, f, g, result)) {
        return result;
    }

    bddvar f_var = f.is_constant() ? BDDVAR_MAX : mgr->node_at(f.index()).var();
    bddvar g_var = g.is_constant() ? BDDVAR_MAX : mgr->node_at(g.index()).var();

    if (f.is_constant()) {
        // f is terminal 1 (base)
        if (g.is_constant()) {
            result = ARC_TERMINAL_0;  // base - base = empty
        } else {
            const DDNode& g_node = mgr->node_at(g.index());
            result = zdd_diff(mgr, f, g_node.arc0());
        }
    } else if (g.is_constant()) {
        // g is terminal, f is not
        if (g == ARC_TERMINAL_1) {
            const DDNode& f_node = mgr->node_at(f.index());
            Arc r0 = zdd_diff(mgr, f_node.arc0(), g);
            result = mgr->get_or_create_node_zdd(f_var, r0, f_node.arc1(), true);
        } else {
            result = f;  // f - empty = f
        }
    } else if (f_var < g_var) {
        const DDNode& f_node = mgr->node_at(f.index());
        Arc r0 = zdd_diff(mgr, f_node.arc0(), g);
        result = mgr->get_or_create_node_zdd(f_var, r0, f_node.arc1(), true);
    } else if (f_var > g_var) {
        const DDNode& g_node = mgr->node_at(g.index());
        result = zdd_diff(mgr, f, g_node.arc0());
    } else {
        const DDNode& f_node = mgr->node_at(f.index());
        const DDNode& g_node = mgr->node_at(g.index());

        Arc r0 = zdd_diff(mgr, f_node.arc0(), g_node.arc0());
        Arc r1 = zdd_diff(mgr, f_node.arc1(), g_node.arc1());

        result = mgr->get_or_create_node_zdd(f_var, r0, r1, true);
    }

    mgr->cache_insert(CacheOp::DIFF, f, g, result);
    return result;
}

// Set family operations
ZDD ZDD::operator+(const ZDD& other) const {
    if (!manager_ || !other.manager_ || manager_ != other.manager_) {
        throw DDIncompatibleException("ZDD managers do not match");
    }
    Arc result = zdd_union(manager_, arc_, other.arc_);
    return ZDD(manager_, result);
}

ZDD ZDD::operator-(const ZDD& other) const {
    if (!manager_ || !other.manager_ || manager_ != other.manager_) {
        throw DDIncompatibleException("ZDD managers do not match");
    }
    Arc result = zdd_diff(manager_, arc_, other.arc_);
    return ZDD(manager_, result);
}

ZDD ZDD::operator*(const ZDD& other) const {
    if (!manager_ || !other.manager_ || manager_ != other.manager_) {
        throw DDIncompatibleException("ZDD managers do not match");
    }
    Arc result = zdd_intersect(manager_, arc_, other.arc_);
    return ZDD(manager_, result);
}

// Quotient (division)
static Arc zdd_quotient(DDManager* mgr, Arc f, Arc g) {
    if (g == ARC_TERMINAL_0) {
        throw DDArgumentException("Division by empty set");
    }
    if (f == ARC_TERMINAL_0) return ARC_TERMINAL_0;
    if (g == ARC_TERMINAL_1) return f;  // f / base = f
    if (f == g) return ARC_TERMINAL_1;  // f / f = base

    Arc result;
    if (mgr->cache_lookup(CacheOp::QUOTIENT, f, g, result)) {
        return result;
    }

    bddvar g_var = mgr->node_at(g.index()).var();
    const DDNode& g_node = mgr->node_at(g.index());

    // Recursive quotient
    Arc f_offset = f;
    Arc f_onset = ARC_TERMINAL_0;

    if (!f.is_constant()) {
        bddvar f_var = mgr->node_at(f.index()).var();
        if (f_var == g_var) {
            const DDNode& f_node = mgr->node_at(f.index());
            f_offset = f_node.arc0();
            f_onset = f_node.arc1();
        } else if (f_var < g_var) {
            // f has variable that g doesn't have
            result = ARC_TERMINAL_0;
            mgr->cache_insert(CacheOp::QUOTIENT, f, g, result);
            return result;
        }
    }

    Arc q0 = zdd_quotient(mgr, f_offset, g_node.arc0());
    Arc q1 = zdd_quotient(mgr, f_onset, g_node.arc1());
    result = zdd_intersect(mgr, q0, q1);

    mgr->cache_insert(CacheOp::QUOTIENT, f, g, result);
    return result;
}

ZDD ZDD::operator/(const ZDD& other) const {
    if (!manager_ || !other.manager_ || manager_ != other.manager_) {
        throw DDIncompatibleException("ZDD managers do not match");
    }
    Arc result = zdd_quotient(manager_, arc_, other.arc_);
    return ZDD(manager_, result);
}

// Remainder
ZDD ZDD::operator%(const ZDD& other) const {
    if (!manager_ || !other.manager_ || manager_ != other.manager_) {
        throw DDIncompatibleException("ZDD managers do not match");
    }
    ZDD q = *this / other;
    ZDD qg = q.product(other);
    return *this - qg;
}

// Compound assignments
ZDD& ZDD::operator+=(const ZDD& other) {
    *this = *this + other;
    return *this;
}

ZDD& ZDD::operator-=(const ZDD& other) {
    *this = *this - other;
    return *this;
}

ZDD& ZDD::operator*=(const ZDD& other) {
    *this = *this * other;
    return *this;
}

ZDD& ZDD::operator/=(const ZDD& other) {
    *this = *this / other;
    return *this;
}

ZDD& ZDD::operator%=(const ZDD& other) {
    *this = *this % other;
    return *this;
}

// Product (cross product)
static Arc zdd_product(DDManager* mgr, Arc f, Arc g) {
    if (f == ARC_TERMINAL_0 || g == ARC_TERMINAL_0) return ARC_TERMINAL_0;
    if (f == ARC_TERMINAL_1) return g;
    if (g == ARC_TERMINAL_1) return f;

    Arc result;
    if (mgr->cache_lookup(CacheOp::PRODUCT, f, g, result)) {
        return result;
    }

    bddvar f_var = mgr->node_at(f.index()).var();
    bddvar g_var = mgr->node_at(g.index()).var();
    bddvar top_var = std::min(f_var, g_var);

    Arc f0, f1, g0, g1;

    if (f_var == top_var) {
        const DDNode& node = mgr->node_at(f.index());
        f0 = node.arc0();
        f1 = node.arc1();
    } else {
        f0 = f;
        f1 = ARC_TERMINAL_0;
    }

    if (g_var == top_var) {
        const DDNode& node = mgr->node_at(g.index());
        g0 = node.arc0();
        g1 = node.arc1();
    } else {
        g0 = g;
        g1 = ARC_TERMINAL_0;
    }

    // (f0 + v*f1) * (g0 + v*g1) = f0*g0 + v*(f0*g1 + f1*g0 + f1*g1)
    Arc r00 = zdd_product(mgr, f0, g0);
    Arc r01 = zdd_product(mgr, f0, g1);
    Arc r10 = zdd_product(mgr, f1, g0);
    Arc r11 = zdd_product(mgr, f1, g1);

    Arc r1 = zdd_union(mgr, zdd_union(mgr, r01, r10), r11);
    result = mgr->get_or_create_node_zdd(top_var, r00, r1, true);

    mgr->cache_insert(CacheOp::PRODUCT, f, g, result);
    return result;
}

ZDD ZDD::product(const ZDD& other) const {
    if (!manager_ || !other.manager_ || manager_ != other.manager_) {
        throw DDIncompatibleException("ZDD managers do not match");
    }
    Arc result = zdd_product(manager_, arc_, other.arc_);
    return ZDD(manager_, result);
}

// Counting
double ZDD::card() const {
    if (!manager_) return 0.0;
    if (arc_ == ARC_TERMINAL_0) return 0.0;
    if (arc_ == ARC_TERMINAL_1) return 1.0;

    std::unordered_map<bddindex, double> memo;

    std::function<double(Arc)> count_rec = [&](Arc a) -> double {
        if (a == ARC_TERMINAL_0) return 0.0;
        if (a == ARC_TERMINAL_1) return 1.0;

        bddindex idx = a.index();
        auto it = memo.find(idx);
        if (it != memo.end()) return it->second;

        const DDNode& node = manager_->node_at(idx);
        double result = count_rec(node.arc0()) + count_rec(node.arc1());
        memo[idx] = result;
        return result;
    };

    return count_rec(arc_);
}

double ZDD::count() const {
    return card();
}

// Enumeration
std::vector<std::vector<bddvar>> ZDD::enumerate() const {
    std::vector<std::vector<bddvar>> result;
    if (!manager_) return result;
    if (arc_ == ARC_TERMINAL_0) return result;

    std::vector<bddvar> current;

    std::function<void(Arc)> enum_rec = [&](Arc a) {
        if (a == ARC_TERMINAL_0) return;
        if (a == ARC_TERMINAL_1) {
            result.push_back(current);
            return;
        }

        const DDNode& node = manager_->node_at(a.index());
        bddvar v = node.var();

        // Low branch (v not in set)
        enum_rec(node.arc0());

        // High branch (v in set)
        current.push_back(v);
        enum_rec(node.arc1());
        current.pop_back();
    };

    enum_rec(arc_);
    return result;
}

// One arbitrary set
std::vector<bddvar> ZDD::one_set() const {
    std::vector<bddvar> result;
    if (!manager_) return result;
    if (arc_ == ARC_TERMINAL_0) return result;  // Empty, no set exists

    Arc current = arc_;
    while (current != ARC_TERMINAL_1 && current != ARC_TERMINAL_0) {
        const DDNode& node = manager_->node_at(current.index());
        bddvar v = node.var();

        // Prefer including the element if it leads to non-empty
        if (node.arc1() != ARC_TERMINAL_0) {
            result.push_back(v);
            current = node.arc1();
        } else {
            current = node.arc0();
        }
    }

    return result;
}

// Debug output
std::string ZDD::to_string() const {
    if (!manager_) return "invalid";
    if (arc_ == ARC_TERMINAL_0) return "empty";
    if (arc_ == ARC_TERMINAL_1) return "base";

    std::ostringstream oss;
    oss << "ZDD(id=" << arc_.data << ", size=" << size() << ", card=" << card() << ")";
    return oss.str();
}

void ZDD::print() const {
    std::cout << to_string() << std::endl;
}

} // namespace sbdd2
