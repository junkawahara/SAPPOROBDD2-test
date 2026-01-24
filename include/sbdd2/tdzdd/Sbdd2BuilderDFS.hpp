/*
 * Sbdd2BuilderDFS.hpp - DFS-based ZDD/BDD Builder for SAPPOROBDD2
 *
 * Builds ZDD from TdZdd-compatible specs using depth-first search
 * with a custom cache for memoization. This approach uses less memory
 * than the BFS-based builder for many problems.
 */

#pragma once

#include <cassert>
#include <cstddef>
#include <cstring>
#include <vector>
#include <unordered_map>

#include "../dd_manager.hpp"
#include "../zdd.hpp"
#include "../bdd.hpp"

namespace sbdd2 {
namespace tdzdd {

/**
 * DFS cache for memoizing state -> Arc mappings.
 *
 * Uses (level, state_hash) as key, with collision resolution via
 * spec.equal_to() for state comparison.
 *
 * @tparam SPEC The spec type
 */
template<typename SPEC>
class DFSCache {
public:
    struct Entry {
        std::vector<char> state;
        Arc result;
        bool valid;

        Entry() : valid(false) {}
    };

private:
    int datasize_;
    std::unordered_map<std::size_t, std::vector<Entry>> table_;

    std::size_t make_key(int level, std::size_t state_hash) const {
        return static_cast<std::size_t>(level) ^ (state_hash * 314159257);
    }

public:
    explicit DFSCache(int datasize) : datasize_(datasize) {}

    /**
     * Look up a cached result for the given state.
     *
     * @param spec The spec (for equal_to comparison)
     * @param level Current level
     * @param state Current state
     * @param state_hash Hash of the state
     * @param[out] result The cached Arc if found
     * @return true if found, false otherwise
     */
    bool lookup(SPEC& spec, int level, const void* state,
                std::size_t state_hash, Arc& result) {
        std::size_t key = make_key(level, state_hash);
        auto it = table_.find(key);
        if (it == table_.end()) {
            return false;
        }

        for (const Entry& entry : it->second) {
            if (entry.valid && spec.equal_to(state, entry.state.data(), level)) {
                result = entry.result;
                return true;
            }
        }
        return false;
    }

    /**
     * Insert a result into the cache.
     *
     * @param spec The spec (for get_copy)
     * @param level Current level
     * @param state Current state
     * @param state_hash Hash of the state
     * @param result The Arc to cache
     */
    void insert(SPEC& spec, int level, const void* state,
                std::size_t state_hash, Arc result) {
        std::size_t key = make_key(level, state_hash);

        Entry entry;
        entry.state.resize(datasize_ > 0 ? datasize_ : 1);
        if (datasize_ > 0) {
            spec.get_copy(entry.state.data(), state);
        }
        entry.result = result;
        entry.valid = true;

        table_[key].push_back(entry);
    }

    /**
     * Clear the cache.
     */
    void clear() {
        table_.clear();
    }

    /**
     * Get the number of cached entries.
     */
    std::size_t size() const {
        std::size_t count = 0;
        for (const auto& kv : table_) {
            count += kv.second.size();
        }
        return count;
    }
};

namespace detail {

/**
 * Internal DFS build function.
 *
 * @param mgr DDManager
 * @param spec The spec
 * @param rootLevel The root level (used for variable mapping)
 * @param level Current level
 * @param state Current state (will be modified)
 * @param cache DFS cache for memoization
 * @param datasize Size of state data
 * @return Arc representing the ZDD for this state
 */
template<typename SPEC>
Arc dfs_build_zdd_impl(DDManager& mgr, SPEC& spec, int rootLevel, int level,
                       void* state, DFSCache<SPEC>& cache, int datasize) {
    // Terminal cases
    if (level == 0) {
        return ARC_TERMINAL_0;
    }
    if (level < 0) {
        return ARC_TERMINAL_1;
    }

    // Check cache
    std::size_t state_hash = spec.hash_code(state, level);
    Arc cached_result;
    if (cache.lookup(spec, level, state, state_hash, cached_result)) {
        return cached_result;
    }

    // Process 0-child
    std::vector<char> state0(datasize > 0 ? datasize : 1);
    if (datasize > 0) {
        spec.get_copy(state0.data(), state);
    }
    int level0 = spec.get_child(state0.data(), level, 0);
    Arc arc0 = dfs_build_zdd_impl(mgr, spec, rootLevel, level0, state0.data(), cache, datasize);
    if (datasize > 0) {
        spec.destruct(state0.data());
    }

    // Process 1-child
    std::vector<char> state1(datasize > 0 ? datasize : 1);
    if (datasize > 0) {
        spec.get_copy(state1.data(), state);
    }
    int level1 = spec.get_child(state1.data(), level, 1);
    Arc arc1 = dfs_build_zdd_impl(mgr, spec, rootLevel, level1, state1.data(), cache, datasize);
    if (datasize > 0) {
        spec.destruct(state1.data());
    }

    // ZDD reduction rule: if 1-arc is 0-terminal, return 0-arc
    Arc result;
    if (arc1 == ARC_TERMINAL_0) {
        result = arc0;
    } else {
        // Create node
        // TdZdd level L → SAPPOROBDD2 level L (same semantics: higher level = closer to root)
        bddvar var = mgr.var_of_lev(level);
        result = mgr.get_or_create_node_zdd(var, arc0, arc1, true);
    }

    // Cache the result
    cache.insert(spec, level, state, state_hash, result);

    return result;
}

/**
 * Internal DFS build function for BDD.
 */
template<typename SPEC>
Arc dfs_build_bdd_impl(DDManager& mgr, SPEC& spec, int rootLevel, int level,
                       void* state, DFSCache<SPEC>& cache, int datasize) {
    // Terminal cases
    if (level == 0) {
        return ARC_TERMINAL_0;
    }
    if (level < 0) {
        return ARC_TERMINAL_1;
    }

    // Check cache
    std::size_t state_hash = spec.hash_code(state, level);
    Arc cached_result;
    if (cache.lookup(spec, level, state, state_hash, cached_result)) {
        return cached_result;
    }

    // Process 0-child
    std::vector<char> state0(datasize > 0 ? datasize : 1);
    if (datasize > 0) {
        spec.get_copy(state0.data(), state);
    }
    int level0 = spec.get_child(state0.data(), level, 0);
    Arc arc0 = dfs_build_bdd_impl(mgr, spec, rootLevel, level0, state0.data(), cache, datasize);
    if (datasize > 0) {
        spec.destruct(state0.data());
    }

    // Process 1-child
    std::vector<char> state1(datasize > 0 ? datasize : 1);
    if (datasize > 0) {
        spec.get_copy(state1.data(), state);
    }
    int level1 = spec.get_child(state1.data(), level, 1);
    Arc arc1 = dfs_build_bdd_impl(mgr, spec, rootLevel, level1, state1.data(), cache, datasize);
    if (datasize > 0) {
        spec.destruct(state1.data());
    }

    // BDD reduction rule: if both children are the same, return the child
    Arc result;
    if (arc0 == arc1) {
        result = arc0;
    } else {
        // Create node
        // TdZdd level L → SAPPOROBDD2 level L (same semantics: higher level = closer to root)
        bddvar var = mgr.var_of_lev(level);
        result = mgr.get_or_create_node_bdd(var, arc0, arc1, true);
    }

    // Cache the result
    cache.insert(spec, level, state, state_hash, result);

    return result;
}

} // namespace detail

/**
 * Build a reduced ZDD from a TdZdd-compatible spec using DFS.
 *
 * This function uses depth-first search with memoization, which can be
 * more memory-efficient than BFS-based construction for many problems.
 *
 * @tparam SPEC The spec type (must satisfy TdZdd spec interface)
 * @param mgr The DDManager to use
 * @param spec The spec instance
 * @return ZDD (reduced)
 */
template<typename SPEC>
ZDD build_zdd_dfs(DDManager& mgr, SPEC& spec) {
    int const datasize = spec.datasize();

    // Get root state
    std::vector<char> rootState(datasize > 0 ? datasize : 1);
    int rootLevel = spec.get_root(rootState.data());

    // Handle trivial cases
    if (rootLevel == 0) {
        return ZDD::empty(mgr);
    }
    if (rootLevel < 0) {
        return ZDD::single(mgr);
    }

    // Ensure variables exist
    while (static_cast<int>(mgr.var_count()) < rootLevel) {
        mgr.new_var();
    }

    // Create cache and build
    DFSCache<SPEC> cache(datasize);
    Arc result = detail::dfs_build_zdd_impl(mgr, spec, rootLevel, rootLevel,
                                            rootState.data(), cache, datasize);

    // Clean up root state
    if (datasize > 0) {
        spec.destruct(rootState.data());
    }

    return ZDD(&mgr, result);
}

/**
 * Build a reduced BDD from a TdZdd-compatible spec using DFS.
 *
 * @tparam SPEC The spec type (must satisfy TdZdd spec interface)
 * @param mgr The DDManager to use
 * @param spec The spec instance
 * @return BDD (reduced)
 */
template<typename SPEC>
BDD build_bdd_dfs(DDManager& mgr, SPEC& spec) {
    int const datasize = spec.datasize();

    // Get root state
    std::vector<char> rootState(datasize > 0 ? datasize : 1);
    int rootLevel = spec.get_root(rootState.data());

    // Handle trivial cases
    if (rootLevel == 0) {
        return BDD::zero(mgr);
    }
    if (rootLevel < 0) {
        return BDD::one(mgr);
    }

    // Ensure variables exist
    while (static_cast<int>(mgr.var_count()) < rootLevel) {
        mgr.new_var();
    }

    // Create cache and build
    DFSCache<SPEC> cache(datasize);
    Arc result = detail::dfs_build_bdd_impl(mgr, spec, rootLevel, rootLevel,
                                            rootState.data(), cache, datasize);

    // Clean up root state
    if (datasize > 0) {
        spec.destruct(rootState.data());
    }

    return BDD(&mgr, result);
}

} // namespace tdzdd
} // namespace sbdd2
