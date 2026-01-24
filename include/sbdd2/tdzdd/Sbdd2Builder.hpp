/*
 * Sbdd2Builder.hpp - TdZdd-style ZDD/BDD Builder for SAPPOROBDD2
 *
 * Builds UnreducedZDD/ZDD from TdZdd-compatible specs using
 * breadth-first top-down construction with placeholder nodes.
 */

#pragma once

#include <cassert>
#include <cstddef>
#include <cstring>
#include <vector>
#include <unordered_map>
#include <array>
#include <functional>

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
 * SpecNode: Holds state information for a node during construction.
 */
class SpecNode {
    std::size_t code_;      // Hash code
    std::size_t col_;       // Column index at this level
    char* state_;           // State data (if any)

public:
    SpecNode() : code_(0), col_(0), state_(nullptr) {}
    SpecNode(std::size_t code, std::size_t col, char* state)
        : code_(code), col_(col), state_(state) {}

    std::size_t code() const { return code_; }
    std::size_t col() const { return col_; }
    void* state() { return state_; }
    void const* state() const { return state_; }
};

/**
 * Build an UnreducedZDD from a TdZdd-compatible spec.
 *
 * @tparam SPEC The spec type (must satisfy TdZdd spec interface)
 * @param mgr The DDManager to use
 * @param spec The spec instance
 * @return UnreducedZDD
 */
template<typename SPEC>
UnreducedZDD build_unreduced_zdd(DDManager& mgr, SPEC& spec) {
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
    // nodeStates[level] = vector of state data for nodes at that level
    // nodeChildren[level][col] = array of child references (Arc::placeholder or terminal)
    // nodeArcs[level][col] = the Arc for the node at (level, col)
    std::vector<std::vector<std::vector<char>>> nodeStates(rootLevel + 1);
    std::vector<std::vector<std::array<Arc, 2>>> nodeChildren(rootLevel + 1);
    std::vector<std::vector<Arc>> nodeArcs(rootLevel + 1);

    // Hash table for state deduplication at each level
    // Maps (hash_code, state_ptr) -> column index
    struct StateHash {
        std::size_t operator()(const std::pair<std::size_t, std::size_t>& p) const {
            return p.first;
        }
    };

    // Phase 1: Top-down state exploration
    // Start with root
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

        // Hash table for next level's state deduplication
        std::unordered_map<std::size_t, std::vector<std::size_t>> nextLevelHash;

        // Process children
        for (std::size_t col = 0; col < numNodes; ++col) {
            for (int b = 0; b < ARITY; ++b) {
                // Copy state for child computation
                std::vector<char> childState(datasize > 0 ? datasize : 1);
                if (datasize > 0) {
                    spec.get_copy(childState.data(), nodeStates[level][col].data());
                }

                int childLevel = spec.get_child(childState.data(), level, b);

                if (childLevel == 0) {
                    // 0-terminal
                    nodeChildren[level][col][b] = ARC_TERMINAL_0;
                } else if (childLevel < 0) {
                    // 1-terminal
                    nodeChildren[level][col][b] = ARC_TERMINAL_1;
                } else {
                    // Non-terminal: find or create child node
                    assert(childLevel < level);

                    // Ensure child level storage exists
                    if (nodeStates[childLevel].empty()) {
                        // Initialize for this level
                    }

                    // Compute hash
                    std::size_t hashCode = spec.hash_code(childState.data(), childLevel);

                    // Look for existing node with same state
                    bool found = false;
                    std::size_t childCol = 0;

                    auto it = nextLevelHash.find(hashCode);
                    if (it != nextLevelHash.end()) {
                        for (std::size_t existingCol : it->second) {
                            if (spec.equal_to(childState.data(),
                                              nodeStates[childLevel][existingCol].data(),
                                              childLevel)) {
                                childCol = existingCol;
                                found = true;
                                break;
                            }
                        }
                    }

                    if (!found) {
                        // Create new node
                        childCol = nodeStates[childLevel].size();
                        nodeStates[childLevel].push_back(childState);
                        nextLevelHash[hashCode].push_back(childCol);
                    }

                    // Store placeholder reference
                    nodeChildren[level][col][b] = Arc::placeholder(childLevel, childCol);
                }
            }

            // Destruct the state for this node
            if (datasize > 0) {
                spec.destruct(nodeStates[level][col].data());
            }
        }

        // Clear state storage for processed level (no longer needed)
        nodeStates[level].clear();
        nodeStates[level].shrink_to_fit();

        spec.destructLevel(level);
    }

    // Phase 2: Bottom-up node finalization
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

        // Clear children storage for processed level
        nodeChildren[level].clear();
        nodeChildren[level].shrink_to_fit();
    }

    // Clean up unlinked nodes
    mgr.clear_unlinked_nodes();

    // Return root
    return UnreducedZDD(&mgr, nodeArcs[rootLevel][0]);
}

/**
 * Build a reduced ZDD from a TdZdd-compatible spec.
 *
 * @tparam SPEC The spec type (must satisfy TdZdd spec interface)
 * @param mgr The DDManager to use
 * @param spec The spec instance
 * @return ZDD (reduced)
 */
template<typename SPEC>
ZDD build_zdd(DDManager& mgr, SPEC& spec) {
    UnreducedZDD unreduced = build_unreduced_zdd(mgr, spec);
    return unreduced.reduce();
}

/**
 * Build an UnreducedBDD from a TdZdd-compatible spec.
 *
 * @tparam SPEC The spec type (must satisfy TdZdd spec interface)
 * @param mgr The DDManager to use
 * @param spec The spec instance
 * @return UnreducedBDD
 */
template<typename SPEC>
UnreducedBDD build_unreduced_bdd(DDManager& mgr, SPEC& spec) {
    int const datasize = spec.datasize();
    int const ARITY = SPEC::ARITY;

    // Get root
    std::vector<char> rootState(datasize > 0 ? datasize : 1);
    int rootLevel = spec.get_root(rootState.data());

    if (rootLevel == 0) {
        return UnreducedBDD::zero(mgr);
    }
    if (rootLevel < 0) {
        return UnreducedBDD::one(mgr);
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
            bddindex placeholder_idx = mgr.create_placeholder_bdd(var);
            nodeArcs[level][col] = Arc::node(placeholder_idx, false);
        }

        // Hash table for next level's state deduplication
        std::unordered_map<std::size_t, std::vector<std::size_t>> nextLevelHash;

        // Process children
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

                    bool found = false;
                    std::size_t childCol = 0;

                    auto it = nextLevelHash.find(hashCode);
                    if (it != nextLevelHash.end()) {
                        for (std::size_t existingCol : it->second) {
                            if (spec.equal_to(childState.data(),
                                              nodeStates[childLevel][existingCol].data(),
                                              childLevel)) {
                                childCol = existingCol;
                                found = true;
                                break;
                            }
                        }
                    }

                    if (!found) {
                        childCol = nodeStates[childLevel].size();
                        nodeStates[childLevel].push_back(childState);
                        nextLevelHash[hashCode].push_back(childCol);
                    }

                    nodeChildren[level][col][b] = Arc::placeholder(childLevel, childCol);
                }
            }

            if (datasize > 0) {
                spec.destruct(nodeStates[level][col].data());
            }
        }

        nodeStates[level].clear();
        nodeStates[level].shrink_to_fit();
        spec.destructLevel(level);
    }

    // Phase 2: Bottom-up node finalization
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
 * Build a reduced BDD from a TdZdd-compatible spec.
 *
 * @tparam SPEC The spec type (must satisfy TdZdd spec interface)
 * @param mgr The DDManager to use
 * @param spec The spec instance
 * @return BDD (reduced)
 */
template<typename SPEC>
BDD build_bdd(DDManager& mgr, SPEC& spec) {
    UnreducedBDD unreduced = build_unreduced_bdd(mgr, spec);
    return unreduced.reduce();
}

/**
 * Build an MVZDD from a TdZdd-compatible spec with ARITY >= 2.
 *
 * This function is intended for specs with ARITY >= 3, but also works for ARITY = 2.
 * For ARITY = 2, consider using build_zdd instead for better performance.
 *
 * @tparam SPEC The spec type (must satisfy TdZdd spec interface)
 * @param mgr The DDManager to use
 * @param spec The spec instance
 * @return MVZDD
 */
template<typename SPEC>
MVZDD build_mvzdd(DDManager& mgr, SPEC& spec) {
    int const datasize = spec.datasize();
    int const ARITY = SPEC::ARITY;

    // Get root
    std::vector<char> rootState(datasize > 0 ? datasize : 1);
    int rootLevel = spec.get_root(rootState.data());

    // Create MVZDD with k = ARITY
    MVZDD base_mvzdd = MVZDD::empty(mgr, ARITY);

    if (rootLevel == 0) {
        return base_mvzdd;  // Empty set
    }
    if (rootLevel < 0) {
        return MVZDD::single(mgr, ARITY);  // Base set
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

        // Hash table for next level's state deduplication
        std::unordered_map<std::size_t, std::vector<std::size_t>> nextLevelHash;

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

                    auto it = nextLevelHash.find(hashCode);
                    if (it != nextLevelHash.end()) {
                        for (std::size_t existingCol : it->second) {
                            if (spec.equal_to(childState.data(),
                                              nodeStates[childLevel][existingCol].data(),
                                              childLevel)) {
                                childCol = existingCol;
                                found = true;
                                break;
                            }
                        }
                    }

                    if (!found) {
                        childCol = nodeStates[childLevel].size();
                        nodeStates[childLevel].push_back(childState);
                        nextLevelHash[hashCode].push_back(childCol);
                    }

                    childRefs[level][col][b] = {childLevel, childCol};
                }
            }

            if (datasize > 0) {
                spec.destruct(nodeStates[level][col].data());
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

            // Build MVZDD node using ITE
            // TdZdd level numbering: rootLevel is the top, 1 is the bottom
            // MVDD variable numbering: 1 is the first variable created (top)
            // Map: TdZdd level L → MVDD variable (rootLevel - L + 1)
            // So level rootLevel → MVDD var 1 (top), level 1 → MVDD var rootLevel (bottom)
            bddvar mv = static_cast<bddvar>(rootLevel - level + 1);
            nodeMVZDDs[level][col] = MVZDD::ite(base_mvzdd, mv, children);
        }

        childRefs[level].clear();
        childRefs[level].shrink_to_fit();
    }

    // Return root
    return nodeMVZDDs[rootLevel][0];
}

/**
 * Build an MVBDD from a TdZdd-compatible spec with ARITY >= 2.
 *
 * This function is intended for specs with ARITY >= 3, but also works for ARITY = 2.
 * For ARITY = 2, consider using build_bdd instead for better performance.
 *
 * @tparam SPEC The spec type (must satisfy TdZdd spec interface)
 * @param mgr The DDManager to use
 * @param spec The spec instance
 * @return MVBDD
 */
template<typename SPEC>
MVBDD build_mvbdd(DDManager& mgr, SPEC& spec) {
    int const datasize = spec.datasize();
    int const ARITY = SPEC::ARITY;

    // Get root
    std::vector<char> rootState(datasize > 0 ? datasize : 1);
    int rootLevel = spec.get_root(rootState.data());

    // Create MVBDD with k = ARITY
    MVBDD base_mvbdd = MVBDD::zero(mgr, ARITY);

    if (rootLevel == 0) {
        return base_mvbdd;  // Constant 0
    }
    if (rootLevel < 0) {
        return MVBDD::one(mgr, ARITY);  // Constant 1
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

        std::unordered_map<std::size_t, std::vector<std::size_t>> nextLevelHash;

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

                    auto it = nextLevelHash.find(hashCode);
                    if (it != nextLevelHash.end()) {
                        for (std::size_t existingCol : it->second) {
                            if (spec.equal_to(childState.data(),
                                              nodeStates[childLevel][existingCol].data(),
                                              childLevel)) {
                                childCol = existingCol;
                                found = true;
                                break;
                            }
                        }
                    }

                    if (!found) {
                        childCol = nodeStates[childLevel].size();
                        nodeStates[childLevel].push_back(childState);
                        nextLevelHash[hashCode].push_back(childCol);
                    }

                    childRefs[level][col][b] = {childLevel, childCol};
                }
            }

            if (datasize > 0) {
                spec.destruct(nodeStates[level][col].data());
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

            // Map: TdZdd level L → MVDD variable (rootLevel - L + 1)
            bddvar mv = static_cast<bddvar>(rootLevel - level + 1);
            nodeMVBDDs[level][col] = MVBDD::ite(base_mvbdd, mv, children);
        }

        childRefs[level].clear();
        childRefs[level].shrink_to_fit();
    }

    return nodeMVBDDs[rootLevel][0];
}

/**
 * Build an MVZDD from a VarArity spec with runtime-configurable ARITY.
 *
 * This function works with specs that use getArity() instead of SPEC::ARITY.
 *
 * @tparam SPEC The spec type (must have getArity() method)
 * @param mgr The DDManager to use
 * @param spec The spec instance
 * @return MVZDD
 */
template<typename SPEC>
MVZDD build_mvzdd_va(DDManager& mgr, SPEC& spec) {
    int const datasize = spec.datasize();
    int const ARITY = spec.getArity();

    // Get root
    std::vector<char> rootState(datasize > 0 ? datasize : 1);
    int rootLevel = spec.get_root(rootState.data());

    // Create MVZDD with k = ARITY
    MVZDD base_mvzdd = MVZDD::empty(mgr, ARITY);

    if (rootLevel == 0) {
        return base_mvzdd;  // Empty set
    }
    if (rootLevel < 0) {
        return MVZDD::single(mgr, ARITY);  // Base set
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

        // Hash table for next level's state deduplication
        std::unordered_map<std::size_t, std::vector<std::size_t>> nextLevelHash;

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

                    auto it = nextLevelHash.find(hashCode);
                    if (it != nextLevelHash.end()) {
                        for (std::size_t existingCol : it->second) {
                            if (spec.equal_to(childState.data(),
                                              nodeStates[childLevel][existingCol].data(),
                                              childLevel)) {
                                childCol = existingCol;
                                found = true;
                                break;
                            }
                        }
                    }

                    if (!found) {
                        childCol = nodeStates[childLevel].size();
                        nodeStates[childLevel].push_back(childState);
                        nextLevelHash[hashCode].push_back(childCol);
                    }

                    childRefs[level][col][b] = {childLevel, childCol};
                }
            }

            if (datasize > 0) {
                spec.destruct(nodeStates[level][col].data());
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

            // Build MVZDD node using ITE
            // Map: TdZdd level L -> MVDD variable (rootLevel - L + 1)
            bddvar mv = static_cast<bddvar>(rootLevel - level + 1);
            nodeMVZDDs[level][col] = MVZDD::ite(base_mvzdd, mv, children);
        }

        childRefs[level].clear();
        childRefs[level].shrink_to_fit();
    }

    // Return root
    return nodeMVZDDs[rootLevel][0];
}

/**
 * Build an MVBDD from a VarArity spec with runtime-configurable ARITY.
 *
 * This function works with specs that use getArity() instead of SPEC::ARITY.
 *
 * @tparam SPEC The spec type (must have getArity() method)
 * @param mgr The DDManager to use
 * @param spec The spec instance
 * @return MVBDD
 */
template<typename SPEC>
MVBDD build_mvbdd_va(DDManager& mgr, SPEC& spec) {
    int const datasize = spec.datasize();
    int const ARITY = spec.getArity();

    // Get root
    std::vector<char> rootState(datasize > 0 ? datasize : 1);
    int rootLevel = spec.get_root(rootState.data());

    // Create MVBDD with k = ARITY
    MVBDD base_mvbdd = MVBDD::zero(mgr, ARITY);

    if (rootLevel == 0) {
        return base_mvbdd;  // Constant 0
    }
    if (rootLevel < 0) {
        return MVBDD::one(mgr, ARITY);  // Constant 1
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

        std::unordered_map<std::size_t, std::vector<std::size_t>> nextLevelHash;

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

                    auto it = nextLevelHash.find(hashCode);
                    if (it != nextLevelHash.end()) {
                        for (std::size_t existingCol : it->second) {
                            if (spec.equal_to(childState.data(),
                                              nodeStates[childLevel][existingCol].data(),
                                              childLevel)) {
                                childCol = existingCol;
                                found = true;
                                break;
                            }
                        }
                    }

                    if (!found) {
                        childCol = nodeStates[childLevel].size();
                        nodeStates[childLevel].push_back(childState);
                        nextLevelHash[hashCode].push_back(childCol);
                    }

                    childRefs[level][col][b] = {childLevel, childCol};
                }
            }

            if (datasize > 0) {
                spec.destruct(nodeStates[level][col].data());
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

            // Map: TdZdd level L -> MVDD variable (rootLevel - L + 1)
            bddvar mv = static_cast<bddvar>(rootLevel - level + 1);
            nodeMVBDDs[level][col] = MVBDD::ite(base_mvbdd, mv, children);
        }

        childRefs[level].clear();
        childRefs[level].shrink_to_fit();
    }

    return nodeMVBDDs[rootLevel][0];
}

// ============================================================
// ZDD Subset Operation
// ============================================================

/**
 * Build a ZDD by applying a spec as a filter (subset operation) on an existing ZDD.
 *
 * This is equivalent to TdZdd's DdStructure::zddSubset().
 * The result is the intersection of the input ZDD with the set defined by the spec.
 *
 * @tparam SPEC The spec type (must satisfy TdZdd spec interface)
 * @param mgr The DDManager to use
 * @param input The input ZDD to subset
 * @param spec The spec instance defining the filter
 * @return ZDD representing input ∩ spec
 */
template<typename SPEC>
ZDD zdd_subset(DDManager& mgr, ZDD const& input, SPEC& spec) {
    // Handle terminal cases
    if (input == ZDD::empty(mgr)) {
        return ZDD::empty(mgr);
    }

    // Use recursive memoized approach
    // Key: (input ZDD id, state hash) -> result ZDD
    typedef std::pair<bddindex, std::size_t> MemoKey;
    struct MemoKeyHash {
        std::size_t operator()(MemoKey const& k) const {
            return k.first * 314159257 + k.second;
        }
    };
    std::unordered_map<MemoKey, ZDD, MemoKeyHash> memo;

    int const stateSize = spec.datasize();

    // Helper to compute state hash
    auto stateHash = [stateSize](void const* state) -> std::size_t {
        if (stateSize == 0) return 0;
        std::size_t h = 0;
        char const* p = static_cast<char const*>(state);
        for (int i = 0; i < stateSize; ++i) {
            h = h * 31 + static_cast<unsigned char>(p[i]);
        }
        return h;
    };

    // Recursive subset function
    std::function<ZDD(ZDD const&, void*, int)> subsetRec;
    subsetRec = [&](ZDD const& f, void* state, int specLev) -> ZDD {
        // Terminal cases
        if (specLev == 0) {
            return ZDD::empty(mgr);
        }
        if (f == ZDD::empty(mgr)) {
            return ZDD::empty(mgr);
        }
        if (specLev < 0) {
            // Spec accepts - return input as-is
            return f;
        }
        if (f == ZDD::single(mgr)) {
            // Input is 1-terminal - follow spec's 0-edges to see if it accepts
            std::vector<char> tmpState(stateSize > 0 ? stateSize : 1);
            if (stateSize > 0) {
                spec.get_copy(tmpState.data(), state);
            }
            int lev = specLev;
            while (lev > 0) {
                lev = spec.get_child(tmpState.data(), lev, 0);
            }
            spec.destruct(tmpState.data());
            return (lev < 0) ? ZDD::single(mgr) : ZDD::empty(mgr);
        }

        // Get input ZDD level
        int inputLev = static_cast<int>(mgr.lev_of_var(f.top()));

        // Synchronize levels
        if (specLev > inputLev) {
            // Spec is higher - follow 0-edge of spec
            std::vector<char> newState(stateSize > 0 ? stateSize : 1);
            if (stateSize > 0) {
                spec.get_copy(newState.data(), state);
            }
            int newSpecLev = spec.get_child(newState.data(), specLev, 0);
            ZDD result = subsetRec(f, newState.data(), newSpecLev);
            spec.destruct(newState.data());
            return result;
        }
        if (specLev < inputLev) {
            // Input is higher - follow 0-edge of input
            return subsetRec(f.low(), state, specLev);
        }

        // Same level - check memo
        bddindex inputId = f.is_terminal() ? 0 : f.id();
        std::size_t sh = stateHash(state);
        MemoKey key(inputId, sh);
        typename std::unordered_map<MemoKey, ZDD, MemoKeyHash>::iterator it = memo.find(key);
        if (it != memo.end()) {
            return it->second;
        }

        // Process children
        ZDD low, high;

        // 0-child
        {
            std::vector<char> childState(stateSize > 0 ? stateSize : 1);
            if (stateSize > 0) {
                spec.get_copy(childState.data(), state);
            }
            int childSpecLev = spec.get_child(childState.data(), specLev, 0);
            low = subsetRec(f.low(), childState.data(), childSpecLev);
            spec.destruct(childState.data());
        }

        // 1-child
        {
            std::vector<char> childState(stateSize > 0 ? stateSize : 1);
            if (stateSize > 0) {
                spec.get_copy(childState.data(), state);
            }
            int childSpecLev = spec.get_child(childState.data(), specLev, 1);
            high = subsetRec(f.high(), childState.data(), childSpecLev);
            spec.destruct(childState.data());
        }

        // Build result node (with ZDD reduction)
        ZDD result;
        if (high == ZDD::empty(mgr)) {
            result = low;
        } else {
            bddvar var = mgr.var_of_lev(specLev);
            Arc lowArc = low.arc();
            Arc highArc = high.arc();
            Arc resultArc = mgr.get_or_create_node_zdd(var, lowArc, highArc, true);
            result = resultArc.is_constant() ?
                (resultArc.terminal_value() ? ZDD::single(mgr) : ZDD::empty(mgr)) :
                ZDD(&mgr, resultArc);
        }

        memo[key] = result;
        return result;
    };

    // Get spec root
    std::vector<char> rootState(stateSize > 0 ? stateSize : 1);
    int specRootLev = spec.get_root(rootState.data());

    ZDD result = subsetRec(input, rootState.data(), specRootLev);
    spec.destruct(rootState.data());

    return result;
}

} // namespace tdzdd
} // namespace sbdd2
