/*
 * Sbdd2BuilderMP.hpp - OpenMP Parallel TdZdd-style ZDD/BDD Builder for SAPPOROBDD2
 *
 * Builds UnreducedZDD/ZDD from TdZdd-compatible specs using
 * parallel breadth-first top-down construction.
 */

#pragma once

#include <cassert>
#include <cstddef>
#include <cstring>
#include <vector>
#include <unordered_map>
#include <array>
#include <mutex>

#ifdef _OPENMP
#include <omp.h>
#endif

#include "../dd_manager.hpp"
#include "../zdd.hpp"
#include "../bdd.hpp"
#include "../unreduced_zdd.hpp"
#include "../unreduced_bdd.hpp"
#include "../mvzdd.hpp"
#include "../mvbdd.hpp"

namespace sbdd2 {
namespace tdzdd {

/**
 * Get number of threads for parallel construction.
 */
inline int get_num_threads() {
#ifdef _OPENMP
    return omp_get_max_threads();
#else
    return 1;
#endif
}

/**
 * Build an UnreducedZDD from a TdZdd-compatible spec using OpenMP parallelism.
 *
 * @tparam SPEC The spec type (must satisfy TdZdd spec interface)
 * @param mgr The DDManager to use
 * @param spec The spec instance
 * @return UnreducedZDD
 */
template<typename SPEC>
UnreducedZDD build_unreduced_zdd_mp(DDManager& mgr, SPEC& spec) {
    int const datasize = spec.datasize();
    int const ARITY = SPEC::ARITY;

    // Get root
    std::vector<char> rootState(datasize > 0 ? datasize : 1);
    int rootLevel = spec.get_root(rootState.data());

    if (rootLevel == 0) {
        return UnreducedZDD::empty(mgr);
    }
    if (rootLevel < 0) {
        return UnreducedZDD::single(mgr);
    }

    // Ensure variables exist
    while (static_cast<int>(mgr.var_count()) < rootLevel) {
        mgr.new_var();
    }

    // Data structures for each level
    std::vector<std::vector<std::vector<char>>> nodeStates(rootLevel + 1);
    std::vector<std::vector<std::array<Arc, 2>>> nodeChildren(rootLevel + 1);
    std::vector<std::vector<Arc>> nodeArcs(rootLevel + 1);

    // Phase 1: Top-down state exploration
    nodeStates[rootLevel].push_back(rootState);

    for (int level = rootLevel; level >= 1; --level) {
        std::size_t numNodes = nodeStates[level].size();
        if (numNodes == 0) continue;

        nodeChildren[level].resize(numNodes);
        nodeArcs[level].resize(numNodes);

        // Create placeholder nodes for this level
        // TdZdd level L → SAPPOROBDD2 level L (same semantics: higher level = closer to root)
        bddvar var = mgr.var_of_lev(level);
        for (std::size_t col = 0; col < numNodes; ++col) {
            bddindex placeholder_idx = mgr.create_placeholder_zdd(var);
            nodeArcs[level][col] = Arc::node(placeholder_idx, false);
        }

        // Thread-local storage for next level nodes
        int numThreads = get_num_threads();
        std::vector<std::vector<std::vector<char>>> threadLocalStates(numThreads);
        std::vector<std::unordered_map<std::size_t, std::vector<std::size_t>>> threadLocalHash(numThreads);

        // Global hash for final merging
        std::unordered_map<std::size_t, std::vector<std::size_t>> globalHash;
        std::mutex globalMutex;

        // Process children in parallel
        #ifdef _OPENMP
        #pragma omp parallel
        #endif
        {
            #ifdef _OPENMP
            int tid = omp_get_thread_num();
            #else
            int tid = 0;
            #endif

            #ifdef _OPENMP
            #pragma omp for schedule(dynamic)
            #endif
            for (std::size_t col = 0; col < numNodes; ++col) {
                for (int b = 0; b < ARITY; ++b) {
                    // Copy state for child computation
                    std::vector<char> childState(datasize > 0 ? datasize : 1);
                    if (datasize > 0) {
                        spec.get_copy(childState.data(), nodeStates[level][col].data());
                    }

                    int childLevel = spec.get_child(childState.data(), level, b);

                    if (childLevel == 0) {
                        nodeChildren[level][col][b] = ARC_TERMINAL_0;
                    } else if (childLevel < 0) {
                        nodeChildren[level][col][b] = ARC_TERMINAL_1;
                    } else {
                        assert(childLevel < level);

                        std::size_t hashCode = spec.hash_code(childState.data(), childLevel);

                        // Look for existing node in thread-local storage first
                        bool found = false;
                        std::size_t childCol = 0;

                        // Check thread-local hash
                        auto& localHash = threadLocalHash[tid];
                        auto it = localHash.find(hashCode);
                        if (it != localHash.end()) {
                            for (std::size_t existingIdx : it->second) {
                                if (spec.equal_to(childState.data(),
                                                  threadLocalStates[tid][existingIdx].data(),
                                                  childLevel)) {
                                    // Found in thread-local
                                    childCol = existingIdx;
                                    found = true;
                                    break;
                                }
                            }
                        }

                        if (!found) {
                            // Add to thread-local storage
                            childCol = threadLocalStates[tid].size();
                            threadLocalStates[tid].push_back(childState);
                            localHash[hashCode].push_back(childCol);
                        }

                        // Store placeholder reference (will be resolved later)
                        // Use a special encoding: (tid << 48) | childCol
                        std::uint64_t encodedRef = (static_cast<std::uint64_t>(tid) << 48) | childCol;
                        nodeChildren[level][col][b] = Arc::placeholder(childLevel, encodedRef);
                    }
                }

                // Destruct the state for this node
                if (datasize > 0) {
                    spec.destruct(nodeStates[level][col].data());
                }
            }
        }

        // Merge thread-local results into global state for next level
        if (level > 1) {
            int nextLevel = level - 1;
            std::vector<std::pair<int, std::size_t>> threadColMapping;  // (tid, localCol) -> globalCol

            // First pass: merge all unique states
            for (int tid = 0; tid < numThreads; ++tid) {
                for (std::size_t localCol = 0; localCol < threadLocalStates[tid].size(); ++localCol) {
                    auto& state = threadLocalStates[tid][localCol];
                    std::size_t hashCode = spec.hash_code(state.data(), nextLevel);

                    bool found = false;
                    std::size_t globalCol = 0;

                    auto it = globalHash.find(hashCode);
                    if (it != globalHash.end()) {
                        for (std::size_t existingCol : it->second) {
                            if (spec.equal_to(state.data(),
                                              nodeStates[nextLevel][existingCol].data(),
                                              nextLevel)) {
                                globalCol = existingCol;
                                found = true;
                                break;
                            }
                        }
                    }

                    if (!found) {
                        globalCol = nodeStates[nextLevel].size();
                        nodeStates[nextLevel].push_back(state);
                        globalHash[hashCode].push_back(globalCol);
                    }

                    threadColMapping.push_back(std::make_pair(tid, globalCol));
                }
            }

            // Build mapping table: (tid, localCol) -> globalCol
            std::vector<std::vector<std::size_t>> colMapping(numThreads);
            std::size_t mappingIdx = 0;
            for (int tid = 0; tid < numThreads; ++tid) {
                colMapping[tid].resize(threadLocalStates[tid].size());
                for (std::size_t localCol = 0; localCol < threadLocalStates[tid].size(); ++localCol) {
                    colMapping[tid][localCol] = threadColMapping[mappingIdx++].second;
                }
            }

            // Update placeholder references with global column indices
            for (std::size_t col = 0; col < numNodes; ++col) {
                for (int b = 0; b < ARITY; ++b) {
                    Arc& arc = nodeChildren[level][col][b];
                    if (arc.is_placeholder()) {
                        int childLevel = arc.placeholder_level();
                        std::uint64_t encodedRef = arc.placeholder_col();
                        int tid = static_cast<int>(encodedRef >> 48);
                        std::size_t localCol = encodedRef & 0xFFFFFFFFFFFFULL;
                        std::size_t globalCol = colMapping[tid][localCol];
                        arc = Arc::placeholder(childLevel, globalCol);
                    }
                }
            }
        }

        // Clear state storage for processed level
        nodeStates[level].clear();
        nodeStates[level].shrink_to_fit();
        spec.destructLevel(level);
    }

    // Phase 2: Bottom-up node finalization (sequential for correctness)
    for (int level = 1; level <= rootLevel; ++level) {
        std::size_t numNodes = nodeArcs[level].size();

        for (std::size_t col = 0; col < numNodes; ++col) {
            Arc arc0 = nodeChildren[level][col][0];
            Arc arc1 = nodeChildren[level][col][1];

            // Resolve placeholder arcs to actual arcs
            if (arc0.is_placeholder()) {
                int childLevel = arc0.placeholder_level();
                std::size_t childCol = arc0.placeholder_col();
                arc0 = nodeArcs[childLevel][childCol];
            }
            if (arc1.is_placeholder()) {
                int childLevel = arc1.placeholder_level();
                std::size_t childCol = arc1.placeholder_col();
                arc1 = nodeArcs[childLevel][childCol];
            }

            // Finalize the node
            bddindex placeholder_idx = nodeArcs[level][col].index();
            Arc finalArc = mgr.finalize_node_zdd(placeholder_idx, arc0, arc1, false);
            nodeArcs[level][col] = finalArc;
        }

        nodeChildren[level].clear();
        nodeChildren[level].shrink_to_fit();
    }

    mgr.clear_unlinked_nodes();

    return UnreducedZDD(&mgr, nodeArcs[rootLevel][0]);
}

/**
 * Build a reduced ZDD from a TdZdd-compatible spec using OpenMP parallelism.
 *
 * @tparam SPEC The spec type (must satisfy TdZdd spec interface)
 * @param mgr The DDManager to use
 * @param spec The spec instance
 * @return ZDD (reduced)
 */
template<typename SPEC>
ZDD build_zdd_mp(DDManager& mgr, SPEC& spec) {
    UnreducedZDD unreduced = build_unreduced_zdd_mp(mgr, spec);
    return unreduced.reduce();
}

/**
 * Build an UnreducedBDD from a TdZdd-compatible spec using OpenMP parallelism.
 */
template<typename SPEC>
UnreducedBDD build_unreduced_bdd_mp(DDManager& mgr, SPEC& spec) {
    int const datasize = spec.datasize();
    int const ARITY = SPEC::ARITY;

    std::vector<char> rootState(datasize > 0 ? datasize : 1);
    int rootLevel = spec.get_root(rootState.data());

    if (rootLevel == 0) {
        return UnreducedBDD::zero(mgr);
    }
    if (rootLevel < 0) {
        return UnreducedBDD::one(mgr);
    }

    while (static_cast<int>(mgr.var_count()) < rootLevel) {
        mgr.new_var();
    }

    std::vector<std::vector<std::vector<char>>> nodeStates(rootLevel + 1);
    std::vector<std::vector<std::array<Arc, 2>>> nodeChildren(rootLevel + 1);
    std::vector<std::vector<Arc>> nodeArcs(rootLevel + 1);

    nodeStates[rootLevel].push_back(rootState);

    for (int level = rootLevel; level >= 1; --level) {
        std::size_t numNodes = nodeStates[level].size();
        if (numNodes == 0) continue;

        nodeChildren[level].resize(numNodes);
        nodeArcs[level].resize(numNodes);

        // Map: TdZdd level L → variable (rootLevel - L + 1)
        bddvar var = static_cast<bddvar>(rootLevel - level + 1);
        for (std::size_t col = 0; col < numNodes; ++col) {
            bddindex placeholder_idx = mgr.create_placeholder_bdd(var);
            nodeArcs[level][col] = Arc::node(placeholder_idx, false);
        }

        int numThreads = get_num_threads();
        std::vector<std::vector<std::vector<char>>> threadLocalStates(numThreads);
        std::vector<std::unordered_map<std::size_t, std::vector<std::size_t>>> threadLocalHash(numThreads);

        std::unordered_map<std::size_t, std::vector<std::size_t>> globalHash;

        #ifdef _OPENMP
        #pragma omp parallel
        #endif
        {
            #ifdef _OPENMP
            int tid = omp_get_thread_num();
            #else
            int tid = 0;
            #endif

            #ifdef _OPENMP
            #pragma omp for schedule(dynamic)
            #endif
            for (std::size_t col = 0; col < numNodes; ++col) {
                for (int b = 0; b < ARITY; ++b) {
                    std::vector<char> childState(datasize > 0 ? datasize : 1);
                    if (datasize > 0) {
                        spec.get_copy(childState.data(), nodeStates[level][col].data());
                    }

                    int childLevel = spec.get_child(childState.data(), level, b);

                    if (childLevel == 0) {
                        nodeChildren[level][col][b] = ARC_TERMINAL_0;
                    } else if (childLevel < 0) {
                        nodeChildren[level][col][b] = ARC_TERMINAL_1;
                    } else {
                        assert(childLevel < level);

                        std::size_t hashCode = spec.hash_code(childState.data(), childLevel);

                        bool found = false;
                        std::size_t childCol = 0;

                        auto& localHash = threadLocalHash[tid];
                        auto it = localHash.find(hashCode);
                        if (it != localHash.end()) {
                            for (std::size_t existingIdx : it->second) {
                                if (spec.equal_to(childState.data(),
                                                  threadLocalStates[tid][existingIdx].data(),
                                                  childLevel)) {
                                    childCol = existingIdx;
                                    found = true;
                                    break;
                                }
                            }
                        }

                        if (!found) {
                            childCol = threadLocalStates[tid].size();
                            threadLocalStates[tid].push_back(childState);
                            localHash[hashCode].push_back(childCol);
                        }

                        std::uint64_t encodedRef = (static_cast<std::uint64_t>(tid) << 48) | childCol;
                        nodeChildren[level][col][b] = Arc::placeholder(childLevel, encodedRef);
                    }
                }

                if (datasize > 0) {
                    spec.destruct(nodeStates[level][col].data());
                }
            }
        }

        if (level > 1) {
            int nextLevel = level - 1;
            std::vector<std::pair<int, std::size_t>> threadColMapping;

            for (int tid = 0; tid < numThreads; ++tid) {
                for (std::size_t localCol = 0; localCol < threadLocalStates[tid].size(); ++localCol) {
                    auto& state = threadLocalStates[tid][localCol];
                    std::size_t hashCode = spec.hash_code(state.data(), nextLevel);

                    bool found = false;
                    std::size_t globalCol = 0;

                    auto it = globalHash.find(hashCode);
                    if (it != globalHash.end()) {
                        for (std::size_t existingCol : it->second) {
                            if (spec.equal_to(state.data(),
                                              nodeStates[nextLevel][existingCol].data(),
                                              nextLevel)) {
                                globalCol = existingCol;
                                found = true;
                                break;
                            }
                        }
                    }

                    if (!found) {
                        globalCol = nodeStates[nextLevel].size();
                        nodeStates[nextLevel].push_back(state);
                        globalHash[hashCode].push_back(globalCol);
                    }

                    threadColMapping.push_back(std::make_pair(tid, globalCol));
                }
            }

            std::vector<std::vector<std::size_t>> colMapping(numThreads);
            std::size_t mappingIdx = 0;
            for (int tid = 0; tid < numThreads; ++tid) {
                colMapping[tid].resize(threadLocalStates[tid].size());
                for (std::size_t localCol = 0; localCol < threadLocalStates[tid].size(); ++localCol) {
                    colMapping[tid][localCol] = threadColMapping[mappingIdx++].second;
                }
            }

            for (std::size_t col = 0; col < numNodes; ++col) {
                for (int b = 0; b < ARITY; ++b) {
                    Arc& arc = nodeChildren[level][col][b];
                    if (arc.is_placeholder()) {
                        int childLevel = arc.placeholder_level();
                        std::uint64_t encodedRef = arc.placeholder_col();
                        int tid = static_cast<int>(encodedRef >> 48);
                        std::size_t localCol = encodedRef & 0xFFFFFFFFFFFFULL;
                        std::size_t globalCol = colMapping[tid][localCol];
                        arc = Arc::placeholder(childLevel, globalCol);
                    }
                }
            }
        }

        nodeStates[level].clear();
        nodeStates[level].shrink_to_fit();
        spec.destructLevel(level);
    }

    for (int level = 1; level <= rootLevel; ++level) {
        std::size_t numNodes = nodeArcs[level].size();

        for (std::size_t col = 0; col < numNodes; ++col) {
            Arc arc0 = nodeChildren[level][col][0];
            Arc arc1 = nodeChildren[level][col][1];

            if (arc0.is_placeholder()) {
                int childLevel = arc0.placeholder_level();
                std::size_t childCol = arc0.placeholder_col();
                arc0 = nodeArcs[childLevel][childCol];
            }
            if (arc1.is_placeholder()) {
                int childLevel = arc1.placeholder_level();
                std::size_t childCol = arc1.placeholder_col();
                arc1 = nodeArcs[childLevel][childCol];
            }

            bddindex placeholder_idx = nodeArcs[level][col].index();
            Arc finalArc = mgr.finalize_node_bdd(placeholder_idx, arc0, arc1, false);
            nodeArcs[level][col] = finalArc;
        }

        nodeChildren[level].clear();
        nodeChildren[level].shrink_to_fit();
    }

    mgr.clear_unlinked_nodes();

    return UnreducedBDD(&mgr, nodeArcs[rootLevel][0]);
}

/**
 * Build a reduced BDD from a TdZdd-compatible spec using OpenMP parallelism.
 */
template<typename SPEC>
BDD build_bdd_mp(DDManager& mgr, SPEC& spec) {
    UnreducedBDD unreduced = build_unreduced_bdd_mp(mgr, spec);
    return unreduced.reduce();
}

/**
 * Build an MVZDD from a VarArity spec using OpenMP parallelism.
 *
 * @tparam SPEC The spec type (must have getArity() method)
 * @param mgr The DDManager to use
 * @param spec The spec instance
 * @return MVZDD
 */
template<typename SPEC>
MVZDD build_mvzdd_va_mp(DDManager& mgr, SPEC& spec) {
    int const datasize = spec.datasize();
    int const ARITY = spec.getArity();

    // Get root
    std::vector<char> rootState(datasize > 0 ? datasize : 1);
    int rootLevel = spec.get_root(rootState.data());

    // Create MVZDD with k = ARITY
    MVZDD base_mvzdd = MVZDD::empty(mgr, ARITY);

    if (rootLevel == 0) {
        return base_mvzdd;
    }
    if (rootLevel < 0) {
        return MVZDD::single(mgr, ARITY);
    }

    // Pre-create MVDD variables for all levels
    while (static_cast<int>(base_mvzdd.mvdd_var_count()) < rootLevel) {
        base_mvzdd.new_var();
    }

    // Terminal MVZDDs
    MVZDD empty_mvzdd = MVZDD(base_mvzdd.manager(), base_mvzdd.var_table(),
                               ZDD::empty(mgr));
    MVZDD base_term = MVZDD(base_mvzdd.manager(), base_mvzdd.var_table(),
                             ZDD::single(mgr));

    // Data structures
    std::vector<std::vector<std::vector<char>>> nodeStates(rootLevel + 1);
    std::vector<std::vector<std::vector<std::pair<int, std::size_t>>>> childRefs(rootLevel + 1);
    std::vector<std::vector<MVZDD>> nodeMVZDDs(rootLevel + 1);

    // Phase 1: Top-down state exploration
    nodeStates[rootLevel].push_back(rootState);

    for (int level = rootLevel; level >= 1; --level) {
        std::size_t numNodes = nodeStates[level].size();
        if (numNodes == 0) continue;

        childRefs[level].resize(numNodes);

        // Thread-local storage for next level nodes
        int numThreads = get_num_threads();
        std::vector<std::vector<std::vector<char>>> threadLocalStates(numThreads);
        std::vector<std::unordered_map<std::size_t, std::vector<std::size_t>>> threadLocalHash(numThreads);

        // Process children in parallel
        #ifdef _OPENMP
        #pragma omp parallel
        #endif
        {
            #ifdef _OPENMP
            int tid = omp_get_thread_num();
            #else
            int tid = 0;
            #endif

            #ifdef _OPENMP
            #pragma omp for schedule(dynamic)
            #endif
            for (std::size_t col = 0; col < numNodes; ++col) {
                childRefs[level][col].resize(ARITY);

                for (int b = 0; b < ARITY; ++b) {
                    std::vector<char> childState(datasize > 0 ? datasize : 1);
                    if (datasize > 0) {
                        spec.get_copy(childState.data(), nodeStates[level][col].data());
                    }

                    int childLevel = spec.get_child(childState.data(), level, b);

                    if (childLevel <= 0) {
                        childRefs[level][col][b] = {childLevel, 0};
                    } else {
                        assert(childLevel < level);

                        std::size_t hashCode = spec.hash_code(childState.data(), childLevel);

                        bool found = false;
                        std::size_t childCol = 0;

                        auto& localHash = threadLocalHash[tid];
                        auto it = localHash.find(hashCode);
                        if (it != localHash.end()) {
                            for (std::size_t existingIdx : it->second) {
                                if (spec.equal_to(childState.data(),
                                                  threadLocalStates[tid][existingIdx].data(),
                                                  childLevel)) {
                                    childCol = existingIdx;
                                    found = true;
                                    break;
                                }
                            }
                        }

                        if (!found) {
                            childCol = threadLocalStates[tid].size();
                            threadLocalStates[tid].push_back(childState);
                            localHash[hashCode].push_back(childCol);
                        }

                        // Encode (tid, localCol) as a placeholder reference
                        std::uint64_t encodedRef = (static_cast<std::uint64_t>(tid) << 48) | childCol;
                        childRefs[level][col][b] = {childLevel, encodedRef};
                    }
                }

                if (datasize > 0) {
                    spec.destruct(nodeStates[level][col].data());
                }
            }
        }

        // Merge thread-local results into global state for next level
        if (level > 1) {
            int nextLevel = level - 1;
            std::unordered_map<std::size_t, std::vector<std::size_t>> globalHash;
            std::vector<std::pair<int, std::size_t>> threadColMapping;

            for (int tid = 0; tid < numThreads; ++tid) {
                for (std::size_t localCol = 0; localCol < threadLocalStates[tid].size(); ++localCol) {
                    auto& state = threadLocalStates[tid][localCol];
                    std::size_t hashCode = spec.hash_code(state.data(), nextLevel);

                    bool found = false;
                    std::size_t globalCol = 0;

                    auto it = globalHash.find(hashCode);
                    if (it != globalHash.end()) {
                        for (std::size_t existingCol : it->second) {
                            if (spec.equal_to(state.data(),
                                              nodeStates[nextLevel][existingCol].data(),
                                              nextLevel)) {
                                globalCol = existingCol;
                                found = true;
                                break;
                            }
                        }
                    }

                    if (!found) {
                        globalCol = nodeStates[nextLevel].size();
                        nodeStates[nextLevel].push_back(state);
                        globalHash[hashCode].push_back(globalCol);
                    }

                    threadColMapping.push_back(std::make_pair(tid, globalCol));
                }
            }

            // Build mapping table
            std::vector<std::vector<std::size_t>> colMapping(numThreads);
            std::size_t mappingIdx = 0;
            for (int tid = 0; tid < numThreads; ++tid) {
                colMapping[tid].resize(threadLocalStates[tid].size());
                for (std::size_t localCol = 0; localCol < threadLocalStates[tid].size(); ++localCol) {
                    colMapping[tid][localCol] = threadColMapping[mappingIdx++].second;
                }
            }

            // Update placeholder references with global column indices
            for (std::size_t col = 0; col < numNodes; ++col) {
                for (int b = 0; b < ARITY; ++b) {
                    auto& ref = childRefs[level][col][b];
                    if (ref.first > 0) {
                        std::uint64_t encodedRef = ref.second;
                        int tid = static_cast<int>(encodedRef >> 48);
                        std::size_t localCol = encodedRef & 0xFFFFFFFFFFFFULL;
                        ref.second = colMapping[tid][localCol];
                    }
                }
            }
        }

        nodeStates[level].clear();
        nodeStates[level].shrink_to_fit();
        spec.destructLevel(level);
    }

    // Phase 2: Bottom-up MVZDD construction
    for (int level = 1; level <= rootLevel; ++level) {
        std::size_t numNodes = childRefs[level].size();
        nodeMVZDDs[level].resize(numNodes);

        for (std::size_t col = 0; col < numNodes; ++col) {
            std::vector<MVZDD> children(ARITY);

            for (int b = 0; b < ARITY; ++b) {
                int childLevel = childRefs[level][col][b].first;
                std::size_t childCol = childRefs[level][col][b].second;

                if (childLevel == 0) {
                    children[b] = empty_mvzdd;
                } else if (childLevel < 0) {
                    children[b] = base_term;
                } else {
                    children[b] = nodeMVZDDs[childLevel][childCol];
                }
            }

            bddvar mv = static_cast<bddvar>(rootLevel - level + 1);
            nodeMVZDDs[level][col] = MVZDD::ite(base_mvzdd, mv, children);
        }

        childRefs[level].clear();
        childRefs[level].shrink_to_fit();
    }

    return nodeMVZDDs[rootLevel][0];
}

/**
 * Build an MVBDD from a VarArity spec using OpenMP parallelism.
 *
 * @tparam SPEC The spec type (must have getArity() method)
 * @param mgr The DDManager to use
 * @param spec The spec instance
 * @return MVBDD
 */
template<typename SPEC>
MVBDD build_mvbdd_va_mp(DDManager& mgr, SPEC& spec) {
    int const datasize = spec.datasize();
    int const ARITY = spec.getArity();

    // Get root
    std::vector<char> rootState(datasize > 0 ? datasize : 1);
    int rootLevel = spec.get_root(rootState.data());

    // Create MVBDD with k = ARITY
    MVBDD base_mvbdd = MVBDD::zero(mgr, ARITY);

    if (rootLevel == 0) {
        return base_mvbdd;
    }
    if (rootLevel < 0) {
        return MVBDD::one(mgr, ARITY);
    }

    // Pre-create MVDD variables for all levels
    while (static_cast<int>(base_mvbdd.mvdd_var_count()) < rootLevel) {
        base_mvbdd.new_var();
    }

    // Terminal MVBDDs
    MVBDD zero_mvbdd = MVBDD(base_mvbdd.manager(), base_mvbdd.var_table(),
                              BDD::zero(mgr));
    MVBDD one_mvbdd = MVBDD(base_mvbdd.manager(), base_mvbdd.var_table(),
                             BDD::one(mgr));

    // Data structures
    std::vector<std::vector<std::vector<char>>> nodeStates(rootLevel + 1);
    std::vector<std::vector<std::vector<std::pair<int, std::size_t>>>> childRefs(rootLevel + 1);
    std::vector<std::vector<MVBDD>> nodeMVBDDs(rootLevel + 1);

    // Phase 1: Top-down state exploration
    nodeStates[rootLevel].push_back(rootState);

    for (int level = rootLevel; level >= 1; --level) {
        std::size_t numNodes = nodeStates[level].size();
        if (numNodes == 0) continue;

        childRefs[level].resize(numNodes);

        int numThreads = get_num_threads();
        std::vector<std::vector<std::vector<char>>> threadLocalStates(numThreads);
        std::vector<std::unordered_map<std::size_t, std::vector<std::size_t>>> threadLocalHash(numThreads);

        #ifdef _OPENMP
        #pragma omp parallel
        #endif
        {
            #ifdef _OPENMP
            int tid = omp_get_thread_num();
            #else
            int tid = 0;
            #endif

            #ifdef _OPENMP
            #pragma omp for schedule(dynamic)
            #endif
            for (std::size_t col = 0; col < numNodes; ++col) {
                childRefs[level][col].resize(ARITY);

                for (int b = 0; b < ARITY; ++b) {
                    std::vector<char> childState(datasize > 0 ? datasize : 1);
                    if (datasize > 0) {
                        spec.get_copy(childState.data(), nodeStates[level][col].data());
                    }

                    int childLevel = spec.get_child(childState.data(), level, b);

                    if (childLevel <= 0) {
                        childRefs[level][col][b] = {childLevel, 0};
                    } else {
                        assert(childLevel < level);

                        std::size_t hashCode = spec.hash_code(childState.data(), childLevel);

                        bool found = false;
                        std::size_t childCol = 0;

                        auto& localHash = threadLocalHash[tid];
                        auto it = localHash.find(hashCode);
                        if (it != localHash.end()) {
                            for (std::size_t existingIdx : it->second) {
                                if (spec.equal_to(childState.data(),
                                                  threadLocalStates[tid][existingIdx].data(),
                                                  childLevel)) {
                                    childCol = existingIdx;
                                    found = true;
                                    break;
                                }
                            }
                        }

                        if (!found) {
                            childCol = threadLocalStates[tid].size();
                            threadLocalStates[tid].push_back(childState);
                            localHash[hashCode].push_back(childCol);
                        }

                        std::uint64_t encodedRef = (static_cast<std::uint64_t>(tid) << 48) | childCol;
                        childRefs[level][col][b] = {childLevel, encodedRef};
                    }
                }

                if (datasize > 0) {
                    spec.destruct(nodeStates[level][col].data());
                }
            }
        }

        if (level > 1) {
            int nextLevel = level - 1;
            std::unordered_map<std::size_t, std::vector<std::size_t>> globalHash;
            std::vector<std::pair<int, std::size_t>> threadColMapping;

            for (int tid = 0; tid < numThreads; ++tid) {
                for (std::size_t localCol = 0; localCol < threadLocalStates[tid].size(); ++localCol) {
                    auto& state = threadLocalStates[tid][localCol];
                    std::size_t hashCode = spec.hash_code(state.data(), nextLevel);

                    bool found = false;
                    std::size_t globalCol = 0;

                    auto it = globalHash.find(hashCode);
                    if (it != globalHash.end()) {
                        for (std::size_t existingCol : it->second) {
                            if (spec.equal_to(state.data(),
                                              nodeStates[nextLevel][existingCol].data(),
                                              nextLevel)) {
                                globalCol = existingCol;
                                found = true;
                                break;
                            }
                        }
                    }

                    if (!found) {
                        globalCol = nodeStates[nextLevel].size();
                        nodeStates[nextLevel].push_back(state);
                        globalHash[hashCode].push_back(globalCol);
                    }

                    threadColMapping.push_back(std::make_pair(tid, globalCol));
                }
            }

            std::vector<std::vector<std::size_t>> colMapping(numThreads);
            std::size_t mappingIdx = 0;
            for (int tid = 0; tid < numThreads; ++tid) {
                colMapping[tid].resize(threadLocalStates[tid].size());
                for (std::size_t localCol = 0; localCol < threadLocalStates[tid].size(); ++localCol) {
                    colMapping[tid][localCol] = threadColMapping[mappingIdx++].second;
                }
            }

            for (std::size_t col = 0; col < numNodes; ++col) {
                for (int b = 0; b < ARITY; ++b) {
                    auto& ref = childRefs[level][col][b];
                    if (ref.first > 0) {
                        std::uint64_t encodedRef = ref.second;
                        int tid = static_cast<int>(encodedRef >> 48);
                        std::size_t localCol = encodedRef & 0xFFFFFFFFFFFFULL;
                        ref.second = colMapping[tid][localCol];
                    }
                }
            }
        }

        nodeStates[level].clear();
        nodeStates[level].shrink_to_fit();
        spec.destructLevel(level);
    }

    // Phase 2: Bottom-up MVBDD construction
    for (int level = 1; level <= rootLevel; ++level) {
        std::size_t numNodes = childRefs[level].size();
        nodeMVBDDs[level].resize(numNodes);

        for (std::size_t col = 0; col < numNodes; ++col) {
            std::vector<MVBDD> children(ARITY);

            for (int b = 0; b < ARITY; ++b) {
                int childLevel = childRefs[level][col][b].first;
                std::size_t childCol = childRefs[level][col][b].second;

                if (childLevel == 0) {
                    children[b] = zero_mvbdd;
                } else if (childLevel < 0) {
                    children[b] = one_mvbdd;
                } else {
                    children[b] = nodeMVBDDs[childLevel][childCol];
                }
            }

            bddvar mv = static_cast<bddvar>(rootLevel - level + 1);
            nodeMVBDDs[level][col] = MVBDD::ite(base_mvbdd, mv, children);
        }

        childRefs[level].clear();
        childRefs[level].shrink_to_fit();
    }

    return nodeMVBDDs[rootLevel][0];
}

} // namespace tdzdd
} // namespace sbdd2
