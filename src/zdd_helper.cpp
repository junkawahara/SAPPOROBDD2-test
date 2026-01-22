// SAPPOROBDD 2.0 - ZDD Helper functions implementation
// MIT License

#include "sbdd2/zdd_helper.hpp"
#include <unordered_map>
#include <functional>
#include <limits>

namespace sbdd2 {

// ============== Weight Operations Implementation ==============

// Internal helper: filter by weight <= bound
static Arc weight_le_impl(DDManager* mgr, Arc f, long long bound,
                          const std::vector<long long>& weights,
                          std::unordered_map<std::uint64_t, Arc>& cache) {
    if (f == ARC_TERMINAL_0) return ARC_TERMINAL_0;
    if (bound < 0) return ARC_TERMINAL_0;
    if (f == ARC_TERMINAL_1) return ARC_TERMINAL_1;  // Empty set has weight 0

    // Cache key: combine arc and bound
    std::uint64_t key = f.data ^ (static_cast<std::uint64_t>(bound) << 32);
    auto it = cache.find(key);
    if (it != cache.end()) {
        return it->second;
    }

    const DDNode& node = mgr->node_at(f.index());
    bddvar var = node.var();

    // Get weight for this variable (0-indexed: var 1 -> weights[0])
    long long w = (var > 0 && var <= weights.size()) ? weights[var - 1] : 0;

    Arc r0 = weight_le_impl(mgr, node.arc0(), bound, weights, cache);
    Arc r1 = weight_le_impl(mgr, node.arc1(), bound - w, weights, cache);

    Arc result = mgr->get_or_create_node_zdd(var, r0, r1, true);
    cache[key] = result;
    return result;
}

ZDD weight_le(const ZDD& f, long long bound, const std::vector<long long>& weights) {
    if (!f.manager()) return f;
    if (f.is_zero()) return f;

    std::unordered_map<std::uint64_t, Arc> cache;
    Arc result = weight_le_impl(f.manager(), f.arc(), bound, weights, cache);
    return ZDD(f.manager(), result);
}

ZDD weight_lt(const ZDD& f, long long bound, const std::vector<long long>& weights) {
    return weight_le(f, bound - 1, weights);
}

// Internal helper: filter by weight >= bound
static Arc weight_ge_impl(DDManager* mgr, Arc f, long long bound,
                          const std::vector<long long>& weights,
                          long long current_max_remaining,
                          std::unordered_map<std::uint64_t, Arc>& cache) {
    if (f == ARC_TERMINAL_0) return ARC_TERMINAL_0;
    if (bound <= 0) return f;  // All sets have weight >= 0
    if (f == ARC_TERMINAL_1) {
        // Empty set has weight 0
        return (bound <= 0) ? ARC_TERMINAL_1 : ARC_TERMINAL_0;
    }

    // If we can't possibly reach the bound, return empty
    if (current_max_remaining < bound) {
        return ARC_TERMINAL_0;
    }

    // Cache key
    std::uint64_t key = f.data ^ (static_cast<std::uint64_t>(bound) << 32);
    auto it = cache.find(key);
    if (it != cache.end()) {
        return it->second;
    }

    const DDNode& node = mgr->node_at(f.index());
    bddvar var = node.var();

    long long w = (var > 0 && var <= weights.size()) ? weights[var - 1] : 0;

    // Calculate remaining max weight
    long long remaining_after = current_max_remaining - w;

    Arc r0 = weight_ge_impl(mgr, node.arc0(), bound, weights, remaining_after, cache);
    Arc r1 = weight_ge_impl(mgr, node.arc1(), bound - w, weights, remaining_after, cache);

    Arc result = mgr->get_or_create_node_zdd(var, r0, r1, true);
    cache[key] = result;
    return result;
}

ZDD weight_ge(const ZDD& f, long long bound, const std::vector<long long>& weights) {
    if (!f.manager()) return f;
    if (f.is_zero()) return f;
    if (bound <= 0) return f;

    // Calculate max possible weight
    long long max_weight = 0;
    for (long long w : weights) {
        if (w > 0) max_weight += w;
    }

    std::unordered_map<std::uint64_t, Arc> cache;
    Arc result = weight_ge_impl(f.manager(), f.arc(), bound, weights, max_weight, cache);
    return ZDD(f.manager(), result);
}

ZDD weight_gt(const ZDD& f, long long bound, const std::vector<long long>& weights) {
    return weight_ge(f, bound + 1, weights);
}

ZDD weight_range(const ZDD& f, long long lower_bound, long long upper_bound,
                 const std::vector<long long>& weights) {
    if (!f.manager()) return f;
    if (lower_bound > upper_bound) return ZDD::empty(*f.manager());

    // Apply both filters
    ZDD upper_filtered = weight_le(f, upper_bound, weights);
    return weight_ge(upper_filtered, lower_bound, weights);
}

ZDD weight_eq(const ZDD& f, long long bound, const std::vector<long long>& weights) {
    return weight_range(f, bound, bound, weights);
}

ZDD weight_ne(const ZDD& f, long long bound, const std::vector<long long>& weights) {
    if (!f.manager()) return f;
    return f - weight_eq(f, bound, weights);
}

} // namespace sbdd2
