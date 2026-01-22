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

#include "../dd_manager.hpp"
#include "../zdd.hpp"
#include "../bdd.hpp"
#include "../unreduced_zdd.hpp"
#include "../unreduced_bdd.hpp"

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
        return UnreducedZDD::base(mgr);
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

} // namespace tdzdd
} // namespace sbdd2
