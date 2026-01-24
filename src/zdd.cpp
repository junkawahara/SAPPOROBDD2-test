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

#ifdef SBDD2_HAS_GMP
#include <gmpxx.h>
#endif

namespace sbdd2 {

// Forward declarations
static Arc zdd_meet_impl(DDManager* mgr, Arc f, Arc g);

// Static factory methods
ZDD ZDD::empty(DDManager& mgr) {
    return mgr.zdd_empty();
}

ZDD ZDD::single(DDManager& mgr) {
    return mgr.zdd_base();
}

ZDD ZDD::singleton(DDManager& mgr, bddvar v) {
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
    bddvar top_lev = manager_->lev_of_var(top);
    bddvar v_lev = manager_->lev_of_var(v);

    // SAPPOROBDD convention: larger level = closer to root
    // If top's level < v's level, v should have appeared earlier (closer to root)
    // but it didn't, so v is not in this subtree
    if (top_lev < v_lev) {
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
    bddvar top_lev = manager_->lev_of_var(top);
    bddvar v_lev = manager_->lev_of_var(v);

    // SAPPOROBDD convention: larger level = closer to root
    // If top's level < v's level, v should have appeared earlier but didn't
    // So v is not in this subtree, meaning all sets here don't contain v
    if (top_lev < v_lev) {
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

    // Add v back to all sets using change (handles ordering correctly)
    return on.change(v);
}

ZDD ZDD::change(bddvar v) const {
    if (!manager_ || arc_.is_constant()) {
        if (arc_ == ARC_TERMINAL_0) {
            return *this;
        }
        // For base (terminal 1), toggle v means add v
        return ZDD::singleton(*manager_, v);
    }

    bddvar top = manager_->node_at(arc_.index()).var();
    bddvar top_lev = manager_->lev_of_var(top);
    bddvar v_lev = manager_->lev_of_var(v);
    const DDNode& node = manager_->node_at(arc_.index());

    // SAPPOROBDD convention: larger level = closer to root
    // If v has larger level than top, v should be the new root
    // Since v was skipped, all sets don't contain v; toggling adds v to all sets
    if (top_lev < v_lev) {
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

    bddvar f_var = f.is_constant() ? 0 : mgr->node_at(f.index()).var();
    bddvar g_var = g.is_constant() ? 0 : mgr->node_at(g.index()).var();
    // Use level comparison to find top variable
    bddvar top_var = mgr->var_of_top_lev(f_var, g_var);

    Arc f0, f1, g0, g1;

    if (f.is_constant() || mgr->var_is_below(f_var, top_var)) {
        f0 = f;
        f1 = ARC_TERMINAL_0;
    } else {
        const DDNode& node = mgr->node_at(f.index());
        f0 = node.arc0();
        f1 = node.arc1();
    }

    if (g.is_constant() || mgr->var_is_below(g_var, top_var)) {
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

// Helper: check if ZDD contains the empty set by following 0-branches
static Arc zdd_contains_empty_set(DDManager* mgr, Arc f) {
    while (!f.is_constant()) {
        const DDNode& node = mgr->node_at(f.index());
        f = node.arc0();  // Follow 0-branch
    }
    return f;  // ARC_TERMINAL_1 if {} ∈ F, ARC_TERMINAL_0 otherwise
}

// Internal intersection function
static Arc zdd_intersect(DDManager* mgr, Arc f, Arc g) {
    if (f == ARC_TERMINAL_0) return ARC_TERMINAL_0;
    if (g == ARC_TERMINAL_0) return ARC_TERMINAL_0;
    if (f == g) return f;

    // Handle terminal 1 (base set {{}})
    // {{}} ∩ G = {{}} if {} ∈ G, else empty
    if (f == ARC_TERMINAL_1) return zdd_contains_empty_set(mgr, g);
    if (g == ARC_TERMINAL_1) return zdd_contains_empty_set(mgr, f);

    if (f.data > g.data) std::swap(f, g);

    Arc result;
    if (mgr->cache_lookup(CacheOp::INTERSECT, f, g, result)) {
        return result;
    }

    bddvar f_var = mgr->node_at(f.index()).var();
    bddvar g_var = mgr->node_at(g.index()).var();
    bddvar f_lev = mgr->lev_of_var(f_var);
    bddvar g_lev = mgr->lev_of_var(g_var);

    if (f_lev > g_lev) {
        // f has higher level (closer to root), g doesn't have it
        const DDNode& node = mgr->node_at(f.index());
        result = zdd_intersect(mgr, node.arc0(), g);
    } else if (f_lev < g_lev) {
        // g has higher level (closer to root), f doesn't have it
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

    bddvar f_var = f.is_constant() ? 0 : mgr->node_at(f.index()).var();
    bddvar g_var = g.is_constant() ? 0 : mgr->node_at(g.index()).var();
    bddvar f_lev = f.is_constant() ? BDDVAR_MAX : mgr->lev_of_var(f_var);
    bddvar g_lev = g.is_constant() ? BDDVAR_MAX : mgr->lev_of_var(g_var);

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
    } else if (f_lev > g_lev) {
        // f has higher level (closer to root)
        const DDNode& f_node = mgr->node_at(f.index());
        Arc r0 = zdd_diff(mgr, f_node.arc0(), g);
        result = mgr->get_or_create_node_zdd(f_var, r0, f_node.arc1(), true);
    } else if (f_lev < g_lev) {
        // g has higher level (closer to root)
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
// Algorithm based on SAPPOROBDD++: F / G = {S | S ∪ T ∈ F for all T ∈ G}
static Arc zdd_quotient(DDManager* mgr, Arc f, Arc g) {
    // g = 0 (empty family) is error
    if (g == ARC_TERMINAL_0) {
        throw DDArgumentException("Division by empty set");
    }
    // f = 0 => 0 / g = 0
    if (f == ARC_TERMINAL_0) return ARC_TERMINAL_0;
    // g = 1 (base, {{}}) => f / {{}} = f
    if (g == ARC_TERMINAL_1) return f;
    // f == g => f / f = {{}}
    if (f == g) return ARC_TERMINAL_1;

    Arc result;
    if (mgr->cache_lookup(CacheOp::QUOTIENT, f, g, result)) {
        return result;
    }

    // Get g's top variable (divisor's top)
    bddvar g_var = mgr->node_at(g.index()).var();
    bddvar g_lev = mgr->lev_of_var(g_var);

    // Check f's level
    if (f == ARC_TERMINAL_1) {
        // f = {{}} (base), g has non-empty sets
        // {{}} / G = 0 when G contains any non-empty set
        result = ARC_TERMINAL_0;
        mgr->cache_insert(CacheOp::QUOTIENT, f, g, result);
        return result;
    }

    bddvar f_var = mgr->node_at(f.index()).var();
    bddvar f_lev = mgr->lev_of_var(f_var);

    // SAPPOROBDD convention: larger level = closer to root
    // If f's level is smaller (farther from root) than g's level,
    // f doesn't contain sets with g's top variable at the right position
    if (f_lev < g_lev) {
        result = ARC_TERMINAL_0;
        mgr->cache_insert(CacheOp::QUOTIENT, f, g, result);
        return result;
    }

    // Use ZDD wrapper for onset/offset operations
    ZDD f_zdd(mgr, f);
    ZDD g_zdd(mgr, g);

    // q = f.onset(g_var) / g.onset(g_var)
    // onset: sets containing g_var, with g_var REMOVED
    // (SAPPOROBDD++ OnSet0 returns hi-branch directly, which removes the variable)
    ZDD f_onset = f_zdd.onset(g_var);
    ZDD g_onset = g_zdd.onset(g_var);

    result = zdd_quotient(mgr, f_onset.arc(), g_onset.arc());

    if (result != ARC_TERMINAL_0) {
        // g.offset(g_var): sets in g NOT containing g_var
        ZDD g_offset = g_zdd.offset(g_var);
        if (g_offset.arc() != ARC_TERMINAL_0) {
            ZDD f_offset = f_zdd.offset(g_var);
            Arc q2 = zdd_quotient(mgr, f_offset.arc(), g_offset.arc());
            result = zdd_intersect(mgr, result, q2);
        }
    }

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
    bddvar f_lev = mgr->lev_of_var(f_var);
    bddvar g_lev = mgr->lev_of_var(g_var);
    // Larger level = closer to root (SAPPOROBDD convention)
    bddvar top_var = (f_lev >= g_lev) ? f_var : g_var;

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

#ifdef SBDD2_HAS_GMP
std::string ZDD::exact_count() const {
    if (!manager_) return "0";
    if (arc_ == ARC_TERMINAL_0) return "0";
    if (arc_ == ARC_TERMINAL_1) return "1";

    std::unordered_map<bddindex, mpz_class> memo;

    std::function<mpz_class(Arc)> count_rec = [&](Arc a) -> mpz_class {
        if (a == ARC_TERMINAL_0) return 0;
        if (a == ARC_TERMINAL_1) return 1;

        bddindex idx = a.index();
        auto it = memo.find(idx);
        if (it != memo.end()) return it->second;

        const DDNode& node = manager_->node_at(idx);
        mpz_class result = count_rec(node.arc0()) + count_rec(node.arc1());
        memo[idx] = result;
        return result;
    };

    return count_rec(arc_).get_str();
}
#endif

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

// ============== Variable operations ==============

// Swap two variables
ZDD ZDD::swap(bddvar v1, bddvar v2) const {
    if (!manager_) return *this;
    if (v1 == v2) return *this;

    // Decompose based on presence of v1 and v2
    ZDD f00 = offset(v1).offset(v2);  // neither v1 nor v2
    ZDD f10 = onset(v1).offset(v2);   // v1 but not v2 (v1 removed)
    ZDD f01 = offset(v1).onset(v2);   // v2 but not v1 (v2 removed)
    ZDD f11 = onset(v1).onset(v2);    // both v1 and v2 (both removed)

    // After swapping:
    // - f00: unchanged (neither variable involved)
    // - f10: had v1, now should have v2 -> add v2
    // - f01: had v2, now should have v1 -> add v1
    // - f11: had both, still has both -> add both back
    return f00 + f10.change(v2) + f01.change(v1) + f11.change(v1).change(v2);
}

// Restrict by another ZDD
static Arc zdd_restrict_impl(DDManager* mgr, Arc f, Arc g) {
    // Terminal cases
    if (f == ARC_TERMINAL_0) return ARC_TERMINAL_0;
    if (g == ARC_TERMINAL_0) return ARC_TERMINAL_0;
    if (f == g) return g;
    if (g == ARC_TERMINAL_1) return f;  // g contains empty set

    // Cache lookup
    Arc result;
    if (mgr->cache_lookup(CacheOp::RESTRICT_ZDD, f, g, result)) {
        return result;
    }

    bddvar f_var = f.is_constant() ? BDDVAR_MAX : mgr->node_at(f.index()).var();
    bddvar g_var = mgr->node_at(g.index()).var();
    bddvar top_var = mgr->var_of_top_lev(f_var, g_var);

    Arc f0, f1, g0, g1;

    if (f.is_constant() || mgr->var_is_below(f_var, top_var)) {
        f0 = f;
        f1 = ARC_TERMINAL_0;
    } else {
        const DDNode& node = mgr->node_at(f.index());
        f0 = node.arc0();
        f1 = node.arc1();
    }

    if (mgr->var_is_below(g_var, top_var)) {
        g0 = g;
        g1 = ARC_TERMINAL_0;
    } else {
        const DDNode& node = mgr->node_at(g.index());
        g0 = node.arc0();
        g1 = node.arc1();
    }

    // Restrict: f1 must contain elements from g1 or g0, f0 must contain from g0
    Arc g01 = zdd_union(mgr, g0, g1);
    Arc r1 = zdd_restrict_impl(mgr, f1, g01);
    Arc r0 = zdd_restrict_impl(mgr, f0, g0);

    result = mgr->get_or_create_node_zdd(top_var, r0, r1, true);
    mgr->cache_insert(CacheOp::RESTRICT_ZDD, f, g, result);
    return result;
}

ZDD ZDD::restrict(const ZDD& g) const {
    if (!manager_ || !g.manager_ || manager_ != g.manager_) {
        throw DDIncompatibleException("ZDD managers do not match");
    }
    Arc result = zdd_restrict_impl(manager_, arc_, g.arc_);
    return ZDD(manager_, result);
}

// Permit by another ZDD
static Arc zdd_permit_impl(DDManager* mgr, Arc f, Arc g) {
    // Terminal cases
    if (f == ARC_TERMINAL_0) return ARC_TERMINAL_0;
    if (g == ARC_TERMINAL_0) return ARC_TERMINAL_0;
    if (f == g) return f;
    if (f == ARC_TERMINAL_1) return ARC_TERMINAL_1;  // Empty set is always permitted

    // Cache lookup
    Arc result;
    if (mgr->cache_lookup(CacheOp::PERMIT_ZDD, f, g, result)) {
        return result;
    }

    bddvar f_var = mgr->node_at(f.index()).var();
    bddvar g_var = g.is_constant() ? BDDVAR_MAX : mgr->node_at(g.index()).var();
    bddvar top_var = mgr->var_of_top_lev(f_var, g_var);

    Arc f0, f1, g0, g1;

    if (mgr->var_is_below(f_var, top_var)) {
        f0 = f;
        f1 = ARC_TERMINAL_0;
    } else {
        const DDNode& node = mgr->node_at(f.index());
        f0 = node.arc0();
        f1 = node.arc1();
    }

    if (g.is_constant() || mgr->var_is_below(g_var, top_var)) {
        g0 = g;
        g1 = ARC_TERMINAL_0;
    } else {
        const DDNode& node = mgr->node_at(g.index());
        g0 = node.arc0();
        g1 = node.arc1();
    }

    // Permit: f1 needs g1, f0 can use g0 or g1
    Arc r1 = zdd_permit_impl(mgr, f1, g1);
    Arc g01 = zdd_union(mgr, g0, g1);
    Arc r0 = zdd_permit_impl(mgr, f0, g01);

    result = mgr->get_or_create_node_zdd(top_var, r0, r1, true);
    mgr->cache_insert(CacheOp::PERMIT_ZDD, f, g, result);
    return result;
}

ZDD ZDD::permit(const ZDD& g) const {
    if (!manager_ || !g.manager_ || manager_ != g.manager_) {
        throw DDIncompatibleException("ZDD managers do not match");
    }
    Arc result = zdd_permit_impl(manager_, arc_, g.arc_);
    return ZDD(manager_, result);
}

// Permit symmetric (cardinality <= n)
static Arc zdd_permit_sym_impl(DDManager* mgr, Arc f, int n) {
    if (f == ARC_TERMINAL_0) return ARC_TERMINAL_0;
    if (n < 0) return ARC_TERMINAL_0;
    if (f == ARC_TERMINAL_1) return ARC_TERMINAL_1;
    if (n == 0) {
        // Only empty set can have cardinality 0
        // Check if f contains empty set
        Arc current = f;
        while (!current.is_constant()) {
            const DDNode& node = mgr->node_at(current.index());
            current = node.arc0();
        }
        return current;  // Terminal 1 if contains empty set, else 0
    }

    // Cache lookup using a combined key
    Arc key2 = Arc::terminal(false);
    key2.data = static_cast<std::uint64_t>(n);
    Arc result;
    if (mgr->cache_lookup(CacheOp::PERMIT_SYM, f, key2, result)) {
        return result;
    }

    const DDNode& node = mgr->node_at(f.index());
    bddvar top = node.var();

    Arc r0 = zdd_permit_sym_impl(mgr, node.arc0(), n);
    Arc r1 = zdd_permit_sym_impl(mgr, node.arc1(), n - 1);

    result = mgr->get_or_create_node_zdd(top, r0, r1, true);
    mgr->cache_insert(CacheOp::PERMIT_SYM, f, key2, result);
    return result;
}

ZDD ZDD::permit_sym(int n) const {
    if (!manager_) return *this;
    Arc result = zdd_permit_sym_impl(manager_, arc_, n);
    return ZDD(manager_, result);
}

// Always: items appearing in ALL sets
static Arc zdd_always_impl(DDManager* mgr, Arc f) {
    if (f == ARC_TERMINAL_0) return ARC_TERMINAL_0;
    if (f == ARC_TERMINAL_1) return ARC_TERMINAL_1;

    // Cache lookup
    Arc result;
    if (mgr->cache_lookup(CacheOp::ALWAYS, f, ARC_TERMINAL_0, result)) {
        return result;
    }

    const DDNode& node = mgr->node_at(f.index());
    bddvar top = node.var();
    Arc f0 = node.arc0();
    Arc f1 = node.arc1();

    Arc h = zdd_always_impl(mgr, f1);

    if (f0 == ARC_TERMINAL_0) {
        // All sets contain top variable
        result = mgr->get_or_create_node_zdd(top, ARC_TERMINAL_0, h, true);
    } else {
        // Need to compute meet (element-wise intersection) of always(f0) and always(f1)
        Arc h0 = zdd_always_impl(mgr, f0);
        // Use meet, not intersect: meet gives element-wise intersection of sets
        result = zdd_meet_impl(mgr, h, h0);
    }

    mgr->cache_insert(CacheOp::ALWAYS, f, ARC_TERMINAL_0, result);
    return result;
}

ZDD ZDD::always() const {
    if (!manager_) return *this;
    Arc result = zdd_always_impl(manager_, arc_);
    return ZDD(manager_, result);
}

// ============== Count/Size operations ==============

// Total literal count (sum of all set sizes)
std::uint64_t ZDD::lit() const {
    if (!manager_) return 0;
    if (arc_ == ARC_TERMINAL_0) return 0;
    if (arc_ == ARC_TERMINAL_1) return 0;  // Empty set has 0 elements

    std::unordered_map<bddindex, std::pair<double, double> > memo;  // (count, lit_sum)

    std::function<std::pair<double, double>(Arc)> count_rec = [&](Arc a) -> std::pair<double, double> {
        if (a == ARC_TERMINAL_0) return std::make_pair(0.0, 0.0);
        if (a == ARC_TERMINAL_1) return std::make_pair(1.0, 0.0);

        bddindex idx = a.index();
        std::unordered_map<bddindex, std::pair<double, double> >::iterator it = memo.find(idx);
        if (it != memo.end()) return it->second;

        const DDNode& node = manager_->node_at(idx);
        std::pair<double, double> res0 = count_rec(node.arc0());
        std::pair<double, double> res1 = count_rec(node.arc1());
        double cnt0 = res0.first, lit0 = res0.second;
        double cnt1 = res1.first, lit1 = res1.second;

        // High branch adds 1 element per set
        double total_cnt = cnt0 + cnt1;
        double total_lit = lit0 + lit1 + cnt1;

        memo[idx] = std::make_pair(total_cnt, total_lit);
        return std::make_pair(total_cnt, total_lit);
    };

    std::pair<double, double> result = count_rec(arc_);
    return static_cast<std::uint64_t>(result.second);
}

// Maximum set size (longest path)
std::uint64_t ZDD::len() const {
    if (!manager_) return 0;
    if (arc_ == ARC_TERMINAL_0) return 0;
    if (arc_ == ARC_TERMINAL_1) return 0;

    std::unordered_map<bddindex, std::uint64_t> memo;

    std::function<std::uint64_t(Arc)> len_rec = [&](Arc a) -> std::uint64_t {
        if (a == ARC_TERMINAL_0) return 0;
        if (a == ARC_TERMINAL_1) return 0;

        bddindex idx = a.index();
        auto it = memo.find(idx);
        if (it != memo.end()) return it->second;

        const DDNode& node = manager_->node_at(idx);
        std::uint64_t len0 = len_rec(node.arc0());
        std::uint64_t len1 = len_rec(node.arc1());

        // High branch adds 1 to length, but only if it leads to non-empty
        std::uint64_t max_len = std::max(len0, len1 + 1);
        memo[idx] = max_len;
        return max_len;
    };

    return len_rec(arc_);
}

// ============== Shift operations ==============

// Left shift: increase all variable numbers by s
static Arc zdd_lshift_impl(DDManager* mgr, Arc f, int s) {
    if (f.is_constant()) return f;
    if (s <= 0) return f;

    const DDNode& node = mgr->node_at(f.index());
    bddvar new_var = node.var() + static_cast<bddvar>(s);

    Arc r0 = zdd_lshift_impl(mgr, node.arc0(), s);
    Arc r1 = zdd_lshift_impl(mgr, node.arc1(), s);

    return mgr->get_or_create_node_zdd(new_var, r0, r1, true);
}

ZDD ZDD::operator<<(int s) const {
    if (!manager_ || s <= 0) return *this;
    Arc result = zdd_lshift_impl(manager_, arc_, s);
    return ZDD(manager_, result);
}

// Right shift: decrease all variable numbers by s
static Arc zdd_rshift_impl(DDManager* mgr, Arc f, int s) {
    if (f.is_constant()) return f;
    if (s <= 0) return f;

    const DDNode& node = mgr->node_at(f.index());
    bddvar var = node.var();

    if (var <= static_cast<bddvar>(s)) {
        // Variable would become 0 or negative, skip this level
        Arc r0 = zdd_rshift_impl(mgr, node.arc0(), s);
        Arc r1 = zdd_rshift_impl(mgr, node.arc1(), s);
        return zdd_union(mgr, r0, r1);
    }

    bddvar new_var = var - static_cast<bddvar>(s);
    Arc r0 = zdd_rshift_impl(mgr, node.arc0(), s);
    Arc r1 = zdd_rshift_impl(mgr, node.arc1(), s);

    return mgr->get_or_create_node_zdd(new_var, r0, r1, true);
}

ZDD ZDD::operator>>(int s) const {
    if (!manager_ || s <= 0) return *this;
    Arc result = zdd_rshift_impl(manager_, arc_, s);
    return ZDD(manager_, result);
}

ZDD& ZDD::operator<<=(int s) {
    *this = *this << s;
    return *this;
}

ZDD& ZDD::operator>>=(int s) {
    *this = *this >> s;
    return *this;
}

// ============== Symmetry and Implication ==============

// Check if v1 and v2 are symmetric
int ZDD::sym_chk(bddvar v1, bddvar v2) const {
    if (!manager_) return -1;
    if (v1 == v2) return 1;

    // Symmetric if the sets with v1 (v1 removed) that don't have v2
    // equal the sets with v2 (v2 removed) that don't have v1
    ZDD f10 = onset(v1).offset(v2);  // had v1 but not v2 (v1 removed)
    ZDD f01 = offset(v1).onset(v2);  // had v2 but not v1 (v2 removed)

    return (f10 == f01) ? 1 : 0;
}

// Get support (all variables appearing in ZDD)
static ZDD zdd_support(DDManager* mgr, Arc f) {
    if (f.is_constant()) {
        return ZDD::empty(*mgr);
    }

    std::unordered_set<bddvar> vars;
    std::function<void(Arc)> collect = [&](Arc a) {
        if (a.is_constant()) return;
        const DDNode& node = mgr->node_at(a.index());
        vars.insert(node.var());
        collect(node.arc0());
        collect(node.arc1());
    };
    collect(f);

    ZDD result = ZDD::empty(*mgr);
    for (bddvar v : vars) {
        result = result + ZDD::singleton(*mgr, v);
    }
    return result;
}

// Get symmetric groups
ZDD ZDD::sym_grp() const {
    if (!manager_) return *this;

    ZDD support = zdd_support(manager_, arc_);
    std::vector<bddvar> vars = support.one_set();

    ZDD result = ZDD::empty(*manager_);
    std::unordered_set<bddvar> processed;

    for (bddvar v1 : vars) {
        if (processed.count(v1)) continue;

        ZDD group = ZDD::singleton(*manager_, v1);
        for (bddvar v2 : vars) {
            if (v1 != v2 && !processed.count(v2)) {
                if (sym_chk(v1, v2) == 1) {
                    group = group + ZDD::singleton(*manager_, v2);
                    processed.insert(v2);
                }
            }
        }
        processed.insert(v1);

        // Only add non-singleton groups, or all if needed
        if (group.card() > 1) {
            // Convert group to a set representation
            std::vector<bddvar> group_vars = group.one_set();
            ZDD group_set = ZDD::single(*manager_);
            for (bddvar gv : group_vars) {
                group_set = group_set.change(gv);
            }
            result = result + group_set;
        }
    }

    return result;
}

// Get symmetric groups (naive version)
ZDD ZDD::sym_grp_naive() const {
    if (!manager_) return *this;

    ZDD support = zdd_support(manager_, arc_);
    std::vector<bddvar> vars;

    // Collect all variables
    ZDD temp = support;
    while (!temp.is_zero()) {
        std::vector<bddvar> one = temp.one_set();
        if (!one.empty()) {
            vars.push_back(one[0]);
            temp = temp.offset(one[0]);
        } else {
            break;
        }
    }

    ZDD result = ZDD::empty(*manager_);
    std::unordered_set<bddvar> processed;

    for (size_t i = 0; i < vars.size(); ++i) {
        bddvar v1 = vars[i];
        if (processed.count(v1)) continue;

        ZDD group = ZDD::singleton(*manager_, v1);
        ZDD f0 = offset(v1);
        ZDD f1 = onset0(v1);

        for (size_t j = i + 1; j < vars.size(); ++j) {
            bddvar v2 = vars[j];
            if (processed.count(v2)) continue;

            // Direct check: f0.onset0(v2) == f1.offset(v2)
            if (f0.onset0(v2) == f1.offset(v2)) {
                group = group + ZDD::singleton(*manager_, v2);
                processed.insert(v2);
            }
        }
        processed.insert(v1);

        // Add group as single set
        std::vector<bddvar> gvars;
        ZDD g = group;
        while (!g.is_zero()) {
            std::vector<bddvar> one = g.one_set();
            if (!one.empty()) {
                gvars.push_back(one[0]);
                g = g.offset(one[0]);
            } else {
                break;
            }
        }
        ZDD group_set = ZDD::single(*manager_);
        for (bddvar gv : gvars) {
            group_set = group_set.change(gv);
        }
        result = result + group_set;
    }

    return result;
}

// Get symmetric set for variable v
static Arc zdd_sym_set_impl(DDManager* mgr, Arc f0, Arc f1) {
    if (f0 == ARC_TERMINAL_0 && f1 == ARC_TERMINAL_0) return ARC_TERMINAL_0;
    if (f0 == ARC_TERMINAL_1 && f1 == ARC_TERMINAL_1) return ARC_TERMINAL_1;
    if (f0 == ARC_TERMINAL_0) {
        // Only f1 branch exists
        Arc result = zdd_sym_set_impl(mgr, ARC_TERMINAL_0, f1);
        if (!f1.is_constant()) {
            ZDD support = zdd_support(mgr, f1);
            result = zdd_diff(mgr, result, support.arc());
        }
        return result;
    }
    if (f1 == ARC_TERMINAL_0) {
        Arc result = zdd_sym_set_impl(mgr, f0, ARC_TERMINAL_0);
        if (!f0.is_constant()) {
            ZDD support = zdd_support(mgr, f0);
            result = zdd_diff(mgr, result, support.arc());
        }
        return result;
    }

    bddvar f0_var = f0.is_constant() ? BDDVAR_MAX : mgr->node_at(f0.index()).var();
    bddvar f1_var = f1.is_constant() ? BDDVAR_MAX : mgr->node_at(f1.index()).var();
    bddvar top = mgr->var_of_top_lev(f0_var, f1_var);

    Arc f00, f01, f10, f11;

    if (f0.is_constant() || mgr->var_is_below(f0_var, top)) {
        f00 = f0;
        f01 = ARC_TERMINAL_0;
    } else {
        const DDNode& node = mgr->node_at(f0.index());
        f00 = node.arc0();
        f01 = node.arc1();
    }

    if (f1.is_constant() || mgr->var_is_below(f1_var, top)) {
        f10 = f1;
        f11 = ARC_TERMINAL_0;
    } else {
        const DDNode& node = mgr->node_at(f1.index());
        f10 = node.arc0();
        f11 = node.arc1();
    }

    Arc h;
    if (f11 == ARC_TERMINAL_0) {
        h = zdd_sym_set_impl(mgr, f00, f10);
        if (f01 != ARC_TERMINAL_0) {
            ZDD support = zdd_support(mgr, f01);
            h = zdd_diff(mgr, h, support.arc());
        }
    } else if (f10 == ARC_TERMINAL_0) {
        h = zdd_sym_set_impl(mgr, f01, f11);
        if (f00 != ARC_TERMINAL_0) {
            ZDD support = zdd_support(mgr, f00);
            h = zdd_diff(mgr, h, support.arc());
        }
    } else {
        Arc h1 = zdd_sym_set_impl(mgr, f01, f11);
        Arc h0 = zdd_sym_set_impl(mgr, f00, f10);
        h = zdd_intersect(mgr, h0, h1);
    }

    // If f10 == f01, top is symmetric
    if (f10 == f01) {
        h = zdd_union(mgr, h, mgr->get_or_create_node_zdd(top, ARC_TERMINAL_0, ARC_TERMINAL_1, true));
    }

    return h;
}

ZDD ZDD::sym_set(bddvar v) const {
    if (!manager_) return *this;
    ZDD f0 = offset(v);
    ZDD f1 = onset0(v);
    Arc result = zdd_sym_set_impl(manager_, f0.arc_, f1.arc_);
    return ZDD(manager_, result);
}

// Implication check: does v1 imply v2?
int ZDD::imply_chk(bddvar v1, bddvar v2) const {
    if (!manager_) return -1;
    if (v1 == v2) return 1;

    // v1 implies v2 iff there's no set containing v1 but not v2
    ZDD f10 = onset0(v1).offset(v2);
    return f10.is_zero() ? 1 : 0;
}

// Co-implication check: do v1 and v2 mutually imply each other?
int ZDD::co_imply_chk(bddvar v1, bddvar v2) const {
    if (!manager_) return -1;
    if (v1 == v2) return 1;

    ZDD f10 = onset0(v1).offset(v2);  // v1 but not v2
    if (f10.is_zero()) return 1;  // v1 => v2, check v2 => v1

    ZDD f01 = offset(v1).onset0(v2);  // v2 but not v1
    ZDD diff = f10 - f01;
    return diff.is_zero() ? 1 : 0;
}

// Implication set: variables implied by v
ZDD ZDD::imply_set(bddvar v) const {
    if (!manager_) return *this;

    ZDD f1 = onset0(v);
    if (f1.is_zero()) {
        // v never appears, so v implies all variables trivially
        return zdd_support(manager_, arc_);
    }

    return f1.always();
}

// Co-implication set
static Arc zdd_co_imply_set_impl(DDManager* mgr, Arc f0, Arc f1) {
    if (f1 == ARC_TERMINAL_0) {
        return zdd_support(mgr, f0).arc();
    }
    if (f0 == ARC_TERMINAL_0 && f1 == ARC_TERMINAL_1) {
        return ARC_TERMINAL_1;
    }

    bddvar f0_var = f0.is_constant() ? BDDVAR_MAX : mgr->node_at(f0.index()).var();
    bddvar f1_var = f1.is_constant() ? BDDVAR_MAX : mgr->node_at(f1.index()).var();
    bddvar top = mgr->var_of_top_lev(f0_var, f1_var);

    Arc f00, f01, f10, f11;

    if (f0.is_constant() || mgr->var_is_below(f0_var, top)) {
        f00 = f0;
        f01 = ARC_TERMINAL_0;
    } else {
        const DDNode& node = mgr->node_at(f0.index());
        f00 = node.arc0();
        f01 = node.arc1();
    }

    if (f1.is_constant() || mgr->var_is_below(f1_var, top)) {
        f10 = f1;
        f11 = ARC_TERMINAL_0;
    } else {
        const DDNode& node = mgr->node_at(f1.index());
        f10 = node.arc0();
        f11 = node.arc1();
    }

    Arc h;
    if (f11 == ARC_TERMINAL_0) {
        h = zdd_co_imply_set_impl(mgr, f00, f10);
    } else if (f10 == ARC_TERMINAL_0) {
        h = zdd_co_imply_set_impl(mgr, f01, f11);
    } else {
        Arc h1 = zdd_co_imply_set_impl(mgr, f01, f11);
        Arc h0 = zdd_co_imply_set_impl(mgr, f00, f10);
        h = zdd_intersect(mgr, h0, h1);
    }

    // Check if top co-implies
    Arc diff = zdd_diff(mgr, f10, f01);
    if (diff == ARC_TERMINAL_0) {
        h = zdd_union(mgr, h, mgr->get_or_create_node_zdd(top, ARC_TERMINAL_0, ARC_TERMINAL_1, true));
    }

    return h;
}

ZDD ZDD::co_imply_set(bddvar v) const {
    if (!manager_) return *this;

    ZDD f0 = offset(v);
    ZDD f1 = onset0(v);
    if (f1.is_zero()) {
        return zdd_support(manager_, arc_);
    }

    Arc result = zdd_co_imply_set_impl(manager_, f0.arc_, f1.arc_);
    return ZDD(manager_, result);
}

// ============== Polynomial operations ==============

// Is this a polynomial (multiple elements)?
int ZDD::is_poly() const {
    if (!manager_) return 0;
    if (arc_.is_constant()) return 0;

    Arc current = arc_;
    while (!current.is_constant()) {
        const DDNode& node = manager_->node_at(current.index());
        if (node.arc0() != ARC_TERMINAL_0) {
            return 1;  // Has 0-branch that's not empty
        }
        current = node.arc1();
    }
    return 0;
}

// Get divisor (minimal element)
ZDD ZDD::divisor() const {
    if (!manager_) return *this;
    if (!is_poly()) return ZDD::single(*manager_);

    ZDD f = *this;
    ZDD support = zdd_support(manager_, arc_);

    while (!support.is_zero()) {
        std::vector<bddvar> vars = support.one_set();
        if (vars.empty()) break;

        bddvar t = vars[0];
        support = support.offset(t);

        ZDD f1 = f.onset0(t);
        if (f1.is_poly()) {
            f = f1;
        }
    }

    return f;
}

// ============== Meet operation ==============

static Arc zdd_meet_impl(DDManager* mgr, Arc f, Arc g) {
    // Terminal cases
    if (f == ARC_TERMINAL_0 || g == ARC_TERMINAL_0) return ARC_TERMINAL_0;
    if (f == ARC_TERMINAL_1) return ARC_TERMINAL_1;
    if (g == ARC_TERMINAL_1) return ARC_TERMINAL_1;
    if (f == g) return f;

    // Normalize for caching
    if (f.data > g.data) std::swap(f, g);

    // Cache lookup
    Arc result;
    if (mgr->cache_lookup(CacheOp::MEET, f, g, result)) {
        return result;
    }

    bddvar f_var = mgr->node_at(f.index()).var();
    bddvar g_var = mgr->node_at(g.index()).var();
    bddvar f_lev = mgr->lev_of_var(f_var);
    bddvar g_lev = mgr->lev_of_var(g_var);

    const DDNode& f_node = mgr->node_at(f.index());
    Arc f0 = f_node.arc0();
    Arc f1 = f_node.arc1();

    if (f_lev > g_lev) {
        // f has variable that g doesn't have (f has higher level, closer to root)
        Arc r0 = zdd_meet_impl(mgr, f0, g);
        Arc r1 = zdd_meet_impl(mgr, f1, g);
        result = zdd_union(mgr, r0, r1);
    } else if (f_lev < g_lev) {
        const DDNode& g_node = mgr->node_at(g.index());
        Arc g0 = g_node.arc0();
        Arc g1 = g_node.arc1();

        Arc r0 = zdd_meet_impl(mgr, f, g0);
        Arc r1 = zdd_meet_impl(mgr, f, g1);
        result = zdd_union(mgr, r0, r1);
    } else {
        // Same top variable
        const DDNode& g_node = mgr->node_at(g.index());
        Arc g0 = g_node.arc0();
        Arc g1 = g_node.arc1();

        // Meet(f0+v*f1, g0+v*g1) = Meet(f0,g0) + Meet(f0,g1) + Meet(f1,g0) + v*Meet(f1,g1)
        Arc r11 = zdd_meet_impl(mgr, f1, g1);
        Arc r00 = zdd_meet_impl(mgr, f0, g0);
        Arc r01 = zdd_meet_impl(mgr, f0, g1);
        Arc r10 = zdd_meet_impl(mgr, f1, g0);

        Arc r0 = zdd_union(mgr, zdd_union(mgr, r00, r01), r10);
        result = mgr->get_or_create_node_zdd(f_var, r0, r11, true);
    }

    mgr->cache_insert(CacheOp::MEET, f, g, result);
    return result;
}

ZDD zdd_meet(const ZDD& f, const ZDD& g) {
    if (!f.manager() || !g.manager() || f.manager() != g.manager()) {
        throw DDIncompatibleException("ZDD managers do not match");
    }
    Arc result = zdd_meet_impl(f.manager(), f.arc(), g.arc());
    return ZDD(f.manager(), result);
}

} // namespace sbdd2
