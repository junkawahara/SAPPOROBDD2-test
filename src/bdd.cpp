// SAPPOROBDD 2.0 - BDD implementation
// MIT License

#include "sbdd2/bdd.hpp"
#include "sbdd2/zdd.hpp"
#include <iostream>
#include <sstream>
#include <stack>
#include <unordered_set>
#include <unordered_map>
#include <cmath>
#include <functional>
#include <algorithm>

namespace sbdd2 {

// Static factory methods
BDD BDD::zero(DDManager& mgr) {
    return mgr.bdd_zero();
}

BDD BDD::one(DDManager& mgr) {
    return mgr.bdd_one();
}

BDD BDD::var(DDManager& mgr, bddvar v) {
    return mgr.var_bdd(v);
}

// Helper: get effective arc (considering negation)
static Arc effective_arc(Arc arc, const DDNode& node, bool is_one_child) {
    Arc child = is_one_child ? node.arc1() : node.arc0();
    if (arc.is_negated()) {
        child = child.negated();
    }
    return child;
}

// Low/High children
BDD BDD::low() const {
    if (!manager_ || arc_.is_constant()) {
        return *this;
    }
    const DDNode& node = manager_->node_at(arc_.index());
    Arc child = node.arc0();
    if (arc_.is_negated()) {
        child = child.negated();
    }
    return BDD(manager_, child);
}

BDD BDD::high() const {
    if (!manager_ || arc_.is_constant()) {
        return *this;
    }
    const DDNode& node = manager_->node_at(arc_.index());
    Arc child = node.arc1();
    if (arc_.is_negated()) {
        child = child.negated();
    }
    return BDD(manager_, child);
}

// Cofactors
BDD BDD::at0(bddvar v) const {
    if (!manager_ || arc_.is_constant()) {
        return *this;
    }
    return restrict(v, false);
}

BDD BDD::at1(bddvar v) const {
    if (!manager_ || arc_.is_constant()) {
        return *this;
    }
    return restrict(v, true);
}

// Negation
BDD BDD::operator~() const {
    if (!manager_) return BDD();
    return BDD(manager_, arc_.negated());
}

// Internal apply function
static Arc bdd_apply(DDManager* mgr, CacheOp op, Arc f, Arc g) {
    // Terminal cases
    bool f_term = f.is_constant();
    bool g_term = g.is_constant();
    bool f_val = f_term ? (f.terminal_value() != f.is_negated()) : false;
    bool g_val = g_term ? (g.terminal_value() != g.is_negated()) : false;

    switch (op) {
    case CacheOp::AND:
        if (f_term && !f_val) return ARC_TERMINAL_0;
        if (g_term && !g_val) return ARC_TERMINAL_0;
        if (f_term && f_val) return g;
        if (g_term && g_val) return f;
        if (f == g) return f;
        if (f.data == (g.data ^ 1)) return ARC_TERMINAL_0;  // f & ~f = 0
        break;

    case CacheOp::OR:
        if (f_term && f_val) return ARC_TERMINAL_1;
        if (g_term && g_val) return ARC_TERMINAL_1;
        if (f_term && !f_val) return g;
        if (g_term && !g_val) return f;
        if (f == g) return f;
        if (f.data == (g.data ^ 1)) return ARC_TERMINAL_1;  // f | ~f = 1
        break;

    case CacheOp::XOR:
        if (f_term && !f_val) return g;
        if (g_term && !g_val) return f;
        if (f_term && f_val) return Arc(g.data ^ 1);
        if (g_term && g_val) return Arc(f.data ^ 1);
        if (f == g) return ARC_TERMINAL_0;
        if (f.data == (g.data ^ 1)) return ARC_TERMINAL_1;
        break;

    case CacheOp::DIFF:  // f & ~g
        if (f_term && !f_val) return ARC_TERMINAL_0;
        if (g_term && g_val) return ARC_TERMINAL_0;
        if (g_term && !g_val) return f;
        if (f == g) return ARC_TERMINAL_0;
        break;

    default:
        break;
    }

    // Check cache
    Arc result;
    if (mgr->cache_lookup(op, f, g, result)) {
        return result;
    }

    // Recursive case
    bddvar f_var = f.is_constant() ? BDDVAR_MAX : mgr->node_at(f.index()).var();
    bddvar g_var = g.is_constant() ? BDDVAR_MAX : mgr->node_at(g.index()).var();
    bddvar top_var = std::min(f_var, g_var);

    Arc f0, f1, g0, g1;

    if (f.is_constant()) {
        f0 = f1 = f;
    } else if (f_var == top_var) {
        const DDNode& node = mgr->node_at(f.index());
        f0 = node.arc0();
        f1 = node.arc1();
        if (f.is_negated()) {
            f0 = f0.negated();
            f1 = f1.negated();
        }
    } else {
        f0 = f1 = f;
    }

    if (g.is_constant()) {
        g0 = g1 = g;
    } else if (g_var == top_var) {
        const DDNode& node = mgr->node_at(g.index());
        g0 = node.arc0();
        g1 = node.arc1();
        if (g.is_negated()) {
            g0 = g0.negated();
            g1 = g1.negated();
        }
    } else {
        g0 = g1 = g;
    }

    Arc r0 = bdd_apply(mgr, op, f0, g0);
    Arc r1 = bdd_apply(mgr, op, f1, g1);

    result = mgr->get_or_create_node_bdd(top_var, r0, r1, true);
    mgr->cache_insert(op, f, g, result);
    return result;
}

// Boolean operations
BDD BDD::operator&(const BDD& other) const {
    if (!manager_ || !other.manager_ || manager_ != other.manager_) {
        throw DDIncompatibleException("BDD managers do not match");
    }
    Arc result = bdd_apply(manager_, CacheOp::AND, arc_, other.arc_);
    return BDD(manager_, result);
}

BDD BDD::operator|(const BDD& other) const {
    if (!manager_ || !other.manager_ || manager_ != other.manager_) {
        throw DDIncompatibleException("BDD managers do not match");
    }
    Arc result = bdd_apply(manager_, CacheOp::OR, arc_, other.arc_);
    return BDD(manager_, result);
}

BDD BDD::operator^(const BDD& other) const {
    if (!manager_ || !other.manager_ || manager_ != other.manager_) {
        throw DDIncompatibleException("BDD managers do not match");
    }
    Arc result = bdd_apply(manager_, CacheOp::XOR, arc_, other.arc_);
    return BDD(manager_, result);
}

BDD BDD::operator-(const BDD& other) const {
    if (!manager_ || !other.manager_ || manager_ != other.manager_) {
        throw DDIncompatibleException("BDD managers do not match");
    }
    Arc result = bdd_apply(manager_, CacheOp::DIFF, arc_, other.arc_);
    return BDD(manager_, result);
}

// Compound assignments
BDD& BDD::operator&=(const BDD& other) {
    *this = *this & other;
    return *this;
}

BDD& BDD::operator|=(const BDD& other) {
    *this = *this | other;
    return *this;
}

BDD& BDD::operator^=(const BDD& other) {
    *this = *this ^ other;
    return *this;
}

BDD& BDD::operator-=(const BDD& other) {
    *this = *this - other;
    return *this;
}

// ITE operation
static Arc bdd_ite(DDManager* mgr, Arc f, Arc t, Arc e) {
    // Terminal cases
    if (f.is_constant()) {
        bool f_val = f.terminal_value() != f.is_negated();
        return f_val ? t : e;
    }
    if (t == e) return t;
    if (t.is_constant() && e.is_constant()) {
        bool t_val = t.terminal_value() != t.is_negated();
        bool e_val = e.terminal_value() != e.is_negated();
        if (t_val && !e_val) return f;
        if (!t_val && e_val) return f.negated();
    }

    // Check cache
    Arc result;
    if (mgr->cache_lookup3(CacheOp::ITE, f, t, e, result)) {
        return result;
    }

    // Get top variable
    bddvar f_var = mgr->node_at(f.index()).var();
    bddvar t_var = t.is_constant() ? BDDVAR_MAX : mgr->node_at(t.index()).var();
    bddvar e_var = e.is_constant() ? BDDVAR_MAX : mgr->node_at(e.index()).var();
    bddvar top_var = std::min(f_var, std::min(t_var, e_var));

    // Split
    Arc f0, f1, t0, t1, e0, e1;

    auto split = [mgr, top_var](Arc a, Arc& a0, Arc& a1) {
        if (a.is_constant()) {
            a0 = a1 = a;
        } else {
            bddvar v = mgr->node_at(a.index()).var();
            if (v == top_var) {
                const DDNode& node = mgr->node_at(a.index());
                a0 = node.arc0();
                a1 = node.arc1();
                if (a.is_negated()) {
                    a0 = a0.negated();
                    a1 = a1.negated();
                }
            } else {
                a0 = a1 = a;
            }
        }
    };

    split(f, f0, f1);
    split(t, t0, t1);
    split(e, e0, e1);

    Arc r0 = bdd_ite(mgr, f0, t0, e0);
    Arc r1 = bdd_ite(mgr, f1, t1, e1);

    result = mgr->get_or_create_node_bdd(top_var, r0, r1, true);
    mgr->cache_insert3(CacheOp::ITE, f, t, e, result);
    return result;
}

BDD BDD::ite(const BDD& t, const BDD& e) const {
    if (!manager_ || !t.manager_ || !e.manager_) {
        throw DDIncompatibleException("Invalid BDD");
    }
    if (manager_ != t.manager_ || manager_ != e.manager_) {
        throw DDIncompatibleException("BDD managers do not match");
    }
    Arc result = bdd_ite(manager_, arc_, t.arc_, e.arc_);
    return BDD(manager_, result);
}

// Restrict
static Arc bdd_restrict(DDManager* mgr, Arc f, bddvar v, bool value) {
    if (f.is_constant()) return f;

    bddvar f_var = mgr->node_at(f.index()).var();
    if (f_var > v) return f;

    const DDNode& node = mgr->node_at(f.index());
    Arc f0 = node.arc0();
    Arc f1 = node.arc1();
    if (f.is_negated()) {
        f0 = f0.negated();
        f1 = f1.negated();
    }

    if (f_var == v) {
        return value ? f1 : f0;
    }

    // f_var < v
    Arc r0 = bdd_restrict(mgr, f0, v, value);
    Arc r1 = bdd_restrict(mgr, f1, v, value);
    return mgr->get_or_create_node_bdd(f_var, r0, r1, true);
}

BDD BDD::restrict(bddvar v, bool value) const {
    if (!manager_) return BDD();
    Arc result = bdd_restrict(manager_, arc_, v, value);
    return BDD(manager_, result);
}

// Quantification
BDD BDD::exist(bddvar v) const {
    return at0(v) | at1(v);
}

BDD BDD::forall(bddvar v) const {
    return at0(v) & at1(v);
}

BDD BDD::exist(const std::vector<bddvar>& vars) const {
    BDD result = *this;
    for (bddvar v : vars) {
        result = result.exist(v);
    }
    return result;
}

BDD BDD::forall(const std::vector<bddvar>& vars) const {
    BDD result = *this;
    for (bddvar v : vars) {
        result = result.forall(v);
    }
    return result;
}

// Compose
static Arc bdd_compose(DDManager* mgr, Arc f, bddvar v, Arc g) {
    if (f.is_constant()) return f;

    Arc result;
    if (mgr->cache_lookup3(CacheOp::COMPOSE, f, Arc(v), g, result)) {
        return result;
    }

    bddvar f_var = mgr->node_at(f.index()).var();
    if (f_var > v) return f;

    const DDNode& node = mgr->node_at(f.index());
    Arc f0 = node.arc0();
    Arc f1 = node.arc1();
    if (f.is_negated()) {
        f0 = f0.negated();
        f1 = f1.negated();
    }

    if (f_var == v) {
        result = bdd_ite(mgr, g, f1, f0);
    } else {
        Arc r0 = bdd_compose(mgr, f0, v, g);
        Arc r1 = bdd_compose(mgr, f1, v, g);
        result = mgr->get_or_create_node_bdd(f_var, r0, r1, true);
    }

    mgr->cache_insert3(CacheOp::COMPOSE, f, Arc(v), g, result);
    return result;
}

BDD BDD::compose(bddvar v, const BDD& g) const {
    if (!manager_ || !g.manager_ || manager_ != g.manager_) {
        throw DDIncompatibleException("BDD managers do not match");
    }
    Arc result = bdd_compose(manager_, arc_, v, g.arc_);
    return BDD(manager_, result);
}

// Counting
double BDD::card() const {
    if (!manager_) return 0.0;
    if (arc_.is_constant()) {
        bool val = arc_.terminal_value() != arc_.is_negated();
        return val ? 1.0 : 0.0;
    }

    // Count with memoization
    std::unordered_map<std::uint64_t, double> memo;

    std::function<double(Arc, bddvar)> count_rec = [&](Arc a, bddvar level) -> double {
        if (a.is_constant()) {
            bool val = a.terminal_value() != a.is_negated();
            return val ? std::pow(2.0, manager_->var_count() - level + 1) : 0.0;
        }

        std::uint64_t key = a.data;
        auto it = memo.find(key);
        if (it != memo.end()) return it->second;

        const DDNode& node = manager_->node_at(a.index());
        bddvar v = node.var();

        // Account for skipped variables
        double skip_factor = std::pow(2.0, v - level);

        Arc a0 = node.arc0();
        Arc a1 = node.arc1();
        if (a.is_negated()) {
            a0 = a0.negated();
            a1 = a1.negated();
        }

        double c0 = count_rec(a0, v + 1);
        double c1 = count_rec(a1, v + 1);
        double result = skip_factor * (c0 + c1) / 2.0;

        memo[key] = result;
        return result;
    };

    return count_rec(arc_, 1);
}

double BDD::count(bddvar max_var) const {
    if (!manager_) return 0.0;
    if (arc_.is_constant()) {
        bool val = arc_.terminal_value() != arc_.is_negated();
        return val ? std::pow(2.0, max_var) : 0.0;
    }

    std::unordered_map<std::uint64_t, double> memo;

    std::function<double(Arc, bddvar)> count_rec = [&](Arc a, bddvar level) -> double {
        if (level > max_var) {
            bool val = a.is_constant() ?
                       (a.terminal_value() != a.is_negated()) : true;
            return val ? 1.0 : 0.0;
        }

        if (a.is_constant()) {
            bool val = a.terminal_value() != a.is_negated();
            return val ? std::pow(2.0, max_var - level + 1) : 0.0;
        }

        std::uint64_t key = (a.data << 20) | level;
        auto it = memo.find(key);
        if (it != memo.end()) return it->second;

        const DDNode& node = manager_->node_at(a.index());
        bddvar v = node.var();

        if (v > level) {
            // Variable not in this path, both branches are the same
            double c = count_rec(a, level + 1);
            double result = 2.0 * c;
            memo[key] = result;
            return result;
        }

        Arc a0 = node.arc0();
        Arc a1 = node.arc1();
        if (a.is_negated()) {
            a0 = a0.negated();
            a1 = a1.negated();
        }

        double c0 = count_rec(a0, v + 1);
        double c1 = count_rec(a1, v + 1);
        double result = c0 + c1;

        memo[key] = result;
        return result;
    };

    return count_rec(arc_, 1);
}

// Satisfying assignment
std::vector<int> BDD::one_sat() const {
    if (!manager_) return {};

    bddvar max_var = manager_->var_count();
    std::vector<int> result(max_var + 1, -1);

    if (arc_.is_constant()) {
        bool val = arc_.terminal_value() != arc_.is_negated();
        if (!val) return {};  // No satisfying assignment
        return result;  // All don't-care
    }

    Arc current = arc_;
    while (!current.is_constant()) {
        const DDNode& node = manager_->node_at(current.index());
        bddvar v = node.var();

        Arc a0 = node.arc0();
        Arc a1 = node.arc1();
        if (current.is_negated()) {
            a0 = a0.negated();
            a1 = a1.negated();
        }

        // Prefer 0-branch if it leads to 1
        bool take_low = true;
        if (a0.is_constant()) {
            bool val = a0.terminal_value() != a0.is_negated();
            if (!val) take_low = false;
        }

        if (take_low) {
            result[v] = 0;
            current = a0;
        } else {
            result[v] = 1;
            current = a1;
        }
    }

    // Check if we reached terminal 1
    bool final_val = current.terminal_value() != current.is_negated();
    if (!final_val) return {};

    return result;
}

// Debug output
std::string BDD::to_string() const {
    if (!manager_) return "invalid";
    if (arc_.is_constant()) {
        bool val = arc_.terminal_value() != arc_.is_negated();
        return val ? "1" : "0";
    }

    std::ostringstream oss;
    oss << "BDD(id=" << arc_.data << ", size=" << size() << ")";
    return oss.str();
}

void BDD::print() const {
    std::cout << to_string() << std::endl;
}

} // namespace sbdd2
