/*
 * DdEval.hpp - TdZdd-compatible Evaluation Interface for SAPPOROBDD2
 *
 * Based on TdZdd by Hiroaki Iwashita
 * Copyright (c) 2014 ERATO MINATO Project
 * Ported to SAPPOROBDD2 namespace
 */

#pragma once

#include <cassert>
#include <cstddef>
#include <functional>
#include <iostream>
#include <vector>

namespace sbdd2 {
namespace tdzdd {

/**
 * Collection of child node values/levels for
 * DdEval::evalNode function interface.
 * @tparam T data type of work area for each node.
 * @tparam ARITY the number of children for each node.
 */
template<typename T, int ARITY>
class DdValues {
    T const* value_[ARITY];
    int level_[ARITY];

public:
    /**
     * Returns the value of the b-th child.
     * @param b branch index.
     * @return value of the b-th child.
     */
    T const& get(int b) const {
        assert(0 <= b && b < ARITY);
        return *value_[b];
    }

    /**
     * Returns the level of the b-th child.
     * @param b branch index.
     * @return level of the b-th child.
     */
    int getLevel(int b) const {
        assert(0 <= b && b < ARITY);
        return level_[b];
    }

    void setReference(int b, T const& v) {
        assert(0 <= b && b < ARITY);
        value_[b] = &v;
    }

    void setLevel(int b, int i) {
        assert(0 <= b && b < ARITY);
        level_[b] = i;
    }

    friend std::ostream& operator<<(std::ostream& os, DdValues const& o) {
        os << "(";
        for (int b = 0; b < ARITY; ++b) {
            if (b != 0) os << ",";
            os << *o.value_[b] << "@" << o.level_[b];
        }
        return os << ")";
    }
};

/**
 * Base class of DD evaluators.
 *
 * Every implementation must define the following functions:
 * - void evalTerminal(T& v, int id)
 * - void evalNode(T& v, int level, DdValues<T,ARITY> const& values)
 *
 * Optionally, the following functions can be overloaded:
 * - bool showMessages()
 * - void initialize(int level)
 * - R getValue(T const& work)
 * - void destructLevel(int i)
 *
 * @tparam E the class implementing this class.
 * @tparam T data type of work area for each node.
 * @tparam R data type of return value.
 * @tparam ARITY arity of the DD (default 2).
 */
template<typename E, typename T, typename R = T, int ARITY = 2>
class DdEval {
public:
    E& entity() {
        return *static_cast<E*>(this);
    }

    E const& entity() const {
        return *static_cast<E const*>(this);
    }

    /**
     * Declares thread-safety.
     * @return true if this class is thread-safe.
     */
    bool isThreadSafe() const {
        return true;
    }

    /**
     * Declares preference to show messages.
     * @return true if messages are preferred.
     */
    bool showMessages() const {
        return false;
    }

    /**
     * Initialization.
     * @param level the maximum level of the DD.
     */
    void initialize(int level) {
        (void)level;
    }

    /**
     * Makes a result value.
     * @param v work area value for the root node.
     * @return final value of the evaluation.
     */
    R getValue(T const& v) {
        return R(v);
    }

    /**
     * Destructs i-th level of data storage.
     * @param i the level to be destructed.
     */
    void destructLevel(int i) {
        (void)i;
    }
};

/**
 * Cardinality evaluator - counts the number of solutions in a ZDD.
 * This is a simple evaluator that can be used with evaluate_spec().
 */
class CardinalityEval : public DdEval<CardinalityEval, double, double, 2> {
public:
    void evalTerminal(double& v, int id) {
        v = (id == 1) ? 1.0 : 0.0;
    }

    void evalNode(double& v, int level, DdValues<double, 2> const& values) {
        (void)level;
        v = values.get(0) + values.get(1);
    }
};

/**
 * Evaluate a spec using an evaluator.
 * This evaluates the spec by traversing it top-down and computing values bottom-up.
 *
 * @tparam SPEC The spec type
 * @tparam E Evaluator type
 * @tparam T Work area type
 * @tparam R Return type
 * @param spec The spec to evaluate
 * @param eval The evaluator instance
 * @return Evaluation result
 */
template<typename SPEC, typename E, typename T, typename R>
R evaluate_spec(SPEC& spec, DdEval<E, T, R, 2>& eval_base) {
    E& eval = eval_base.entity();
    int const datasize = spec.datasize();

    // Get root
    std::vector<char> rootState(datasize > 0 ? datasize : 1);
    int rootLevel = spec.get_root(rootState.data());

    if (rootLevel == 0) {
        T v;
        eval.evalTerminal(v, 0);
        return eval.getValue(v);
    }
    if (rootLevel < 0) {
        T v;
        eval.evalTerminal(v, 1);
        return eval.getValue(v);
    }

    eval.initialize(rootLevel);

    // Simple recursive evaluation (not optimized)
    struct EvalState {
        std::vector<char> state;
        int level;
    };

    std::function<T(EvalState)> evalRec = [&](EvalState es) -> T {
        T childVals[2];
        int childLevels[2];

        for (int b = 0; b < 2; ++b) {
            std::vector<char> childState(datasize > 0 ? datasize : 1);
            if (datasize > 0) {
                spec.get_copy(childState.data(), es.state.data());
            }
            int childLevel = spec.get_child(childState.data(), es.level, b);

            if (childLevel == 0) {
                eval.evalTerminal(childVals[b], 0);
                childLevels[b] = 0;
            } else if (childLevel < 0) {
                eval.evalTerminal(childVals[b], 1);
                childLevels[b] = 0;
            } else {
                EvalState childEs;
                childEs.state = childState;
                childEs.level = childLevel;
                childVals[b] = evalRec(childEs);
                childLevels[b] = childLevel;
            }

            if (datasize > 0) {
                spec.destruct(childState.data());
            }
        }

        DdValues<T, 2> values;
        values.setReference(0, childVals[0]);
        values.setLevel(0, childLevels[0]);
        values.setReference(1, childVals[1]);
        values.setLevel(1, childLevels[1]);

        T result;
        eval.evalNode(result, es.level, values);
        return result;
    };

    EvalState initial;
    initial.state = rootState;
    initial.level = rootLevel;
    T rootVal = evalRec(initial);

    if (datasize > 0) {
        spec.destruct(rootState.data());
    }

    return eval.getValue(rootVal);
}

/**
 * Evaluate a ZDD using a DdEval evaluator.
 * This traverses the ZDD bottom-up and applies the evaluator to compute a result.
 *
 * @tparam E Evaluator type (derived from DdEval)
 * @tparam T Work area type
 * @tparam R Return type
 * @tparam ARITY Arity of the DD (default 2)
 * @param zdd The ZDD to evaluate
 * @param eval_base The evaluator instance
 * @return Evaluation result
 */
template<typename E, typename T, typename R, int ARITY>
R zdd_evaluate(ZDD const& zdd, DdEval<E, T, R, ARITY>& eval_base);

} // namespace tdzdd
} // namespace sbdd2

// Include ZDD header for the implementation
#include "../zdd.hpp"
#include "../dd_node.hpp"

namespace sbdd2 {
namespace tdzdd {

template<typename E, typename T, typename R, int ARITY>
R zdd_evaluate(ZDD const& zdd, DdEval<E, T, R, ARITY>& eval_base) {
    E& eval = eval_base.entity();
    DDManager* mgr = zdd.manager();

    // Handle terminal cases
    if (zdd.is_terminal()) {
        T v;
        eval.evalTerminal(v, zdd.is_one() ? 1 : 0);
        return eval.getValue(v);
    }

    // Collect all nodes by level using BFS
    // Each node is identified by its Arc
    std::unordered_map<std::uint64_t, int> arcToIndex;  // arc.data -> index in nodeList
    std::vector<Arc> nodeList;  // All nodes
    std::vector<int> nodeLevel;  // Level of each node
    std::vector<std::vector<int>> nodesByLevel;  // Indices grouped by level

    // Get root level
    bddvar rootVar = zdd.top();
    int rootLevel = static_cast<int>(mgr->lev_of_var(rootVar));

    nodesByLevel.resize(rootLevel + 1);
    eval.initialize(rootLevel);

    // BFS to collect nodes
    std::vector<Arc> queue;
    queue.push_back(zdd.arc());
    arcToIndex[zdd.arc().data] = 0;
    nodeList.push_back(zdd.arc());
    nodeLevel.push_back(rootLevel);
    nodesByLevel[rootLevel].push_back(0);

    std::size_t queueIdx = 0;
    while (queueIdx < queue.size()) {
        Arc current = queue[queueIdx++];
        if (current.is_constant()) continue;

        DDNode const& node = mgr->node_at(current.index());
        Arc children[ARITY];
        children[0] = node.arc0();  // low
        children[1] = node.arc1();  // high

        for (int b = 0; b < ARITY; ++b) {
            Arc child = children[b];

            if (arcToIndex.find(child.data) == arcToIndex.end()) {
                int idx = static_cast<int>(nodeList.size());
                arcToIndex[child.data] = idx;
                nodeList.push_back(child);

                int childLevel = 0;
                if (!child.is_constant()) {
                    bddvar childVar = mgr->node_at(child.index()).var();
                    childLevel = static_cast<int>(mgr->lev_of_var(childVar));
                }
                nodeLevel.push_back(childLevel);

                if (childLevel >= 0 && childLevel <= rootLevel) {
                    nodesByLevel[childLevel].push_back(idx);
                }

                if (!child.is_constant()) {
                    queue.push_back(child);
                }
            }
        }
    }

    // Allocate work area for each node
    std::vector<T> work(nodeList.size());

    // Evaluate terminals (level 0)
    for (int idx : nodesByLevel[0]) {
        Arc arc = nodeList[idx];
        if (arc.is_constant()) {
            eval.evalTerminal(work[idx], arc.terminal_value() ? 1 : 0);
        }
    }

    // Also evaluate any terminal nodes not at level 0
    for (std::size_t idx = 0; idx < nodeList.size(); ++idx) {
        Arc arc = nodeList[idx];
        if (arc.is_constant()) {
            eval.evalTerminal(work[idx], arc.terminal_value() ? 1 : 0);
        }
    }

    // Bottom-up evaluation
    for (int level = 1; level <= rootLevel; ++level) {
        for (int idx : nodesByLevel[level]) {
            Arc arc = nodeList[idx];
            if (arc.is_constant()) continue;

            DDNode const& node = mgr->node_at(arc.index());
            Arc childArcs[ARITY];
            childArcs[0] = node.arc0();  // low
            childArcs[1] = node.arc1();  // high

            DdValues<T, ARITY> values;
            for (int b = 0; b < ARITY; ++b) {
                Arc childArc = childArcs[b];
                int childIdx = arcToIndex[childArc.data];
                values.setReference(b, work[childIdx]);
                values.setLevel(b, nodeLevel[childIdx]);
            }

            eval.evalNode(work[idx], level, values);
        }

        // Destruct lower levels if evaluator wants
        eval.destructLevel(level - 1);
    }

    // Get result from root
    int rootIdx = arcToIndex[zdd.arc().data];
    return eval.getValue(work[rootIdx]);
}

/**
 * Evaluate a BDD using a DdEval evaluator.
 * This traverses the BDD bottom-up and applies the evaluator to compute a result.
 *
 * @tparam E Evaluator type (derived from DdEval)
 * @tparam T Work area type
 * @tparam R Return type
 * @tparam ARITY Arity of the DD (default 2)
 * @param bdd The BDD to evaluate
 * @param eval_base The evaluator instance
 * @return Evaluation result
 */
template<typename E, typename T, typename R, int ARITY>
R bdd_evaluate(BDD const& bdd, DdEval<E, T, R, ARITY>& eval_base);

} // namespace tdzdd
} // namespace sbdd2

// Include BDD header for the implementation
#include "../bdd.hpp"

namespace sbdd2 {
namespace tdzdd {

template<typename E, typename T, typename R, int ARITY>
R bdd_evaluate(BDD const& bdd, DdEval<E, T, R, ARITY>& eval_base) {
    E& eval = eval_base.entity();
    DDManager* mgr = bdd.manager();

    // Handle terminal cases
    if (bdd.is_terminal()) {
        T v;
        eval.evalTerminal(v, bdd.is_one() ? 1 : 0);
        return eval.getValue(v);
    }

    // Collect all nodes by level using BFS
    std::unordered_map<std::uint64_t, int> arcToIndex;
    std::vector<Arc> nodeList;
    std::vector<int> nodeLevel;
    std::vector<std::vector<int>> nodesByLevel;

    bddvar rootVar = bdd.top();
    int rootLevel = static_cast<int>(mgr->lev_of_var(rootVar));

    nodesByLevel.resize(rootLevel + 1);
    eval.initialize(rootLevel);

    // BFS to collect nodes
    // For BDD, we normalize the arc (remove negation bit for indexing)
    // Negation bit is bit 0, so we mask with ~1ULL
    std::vector<Arc> queue;
    Arc rootArc = bdd.arc();
    Arc normalizedRoot = Arc(rootArc.data & ~1ULL);
    queue.push_back(normalizedRoot);
    arcToIndex[normalizedRoot.data] = 0;
    nodeList.push_back(normalizedRoot);
    nodeLevel.push_back(rootLevel);
    nodesByLevel[rootLevel].push_back(0);

    std::size_t queueIdx = 0;
    while (queueIdx < queue.size()) {
        Arc current = queue[queueIdx++];
        if (current.is_constant()) continue;

        DDNode const& node = mgr->node_at(current.index());
        Arc children[ARITY];
        children[0] = node.arc0();
        children[1] = node.arc1();

        for (int b = 0; b < ARITY; ++b) {
            Arc child = children[b];
            // Normalize (remove negation) for indexing
            Arc normalizedChild = Arc(child.data & ~1ULL);

            if (arcToIndex.find(normalizedChild.data) == arcToIndex.end()) {
                int idx = static_cast<int>(nodeList.size());
                arcToIndex[normalizedChild.data] = idx;
                nodeList.push_back(normalizedChild);

                int childLevel = 0;
                if (!normalizedChild.is_constant()) {
                    bddvar childVar = mgr->node_at(normalizedChild.index()).var();
                    childLevel = static_cast<int>(mgr->lev_of_var(childVar));
                }
                nodeLevel.push_back(childLevel);

                if (childLevel >= 0 && childLevel <= rootLevel) {
                    nodesByLevel[childLevel].push_back(idx);
                }

                if (!normalizedChild.is_constant()) {
                    queue.push_back(normalizedChild);
                }
            }
        }
    }

    // Allocate work area
    std::vector<T> work(nodeList.size());

    // Evaluate terminals
    for (std::size_t idx = 0; idx < nodeList.size(); ++idx) {
        Arc arc = nodeList[idx];
        if (arc.is_constant()) {
            eval.evalTerminal(work[idx], arc.terminal_value() ? 1 : 0);
        }
    }

    // Bottom-up evaluation
    for (int level = 1; level <= rootLevel; ++level) {
        for (int idx : nodesByLevel[level]) {
            Arc arc = nodeList[idx];
            if (arc.is_constant()) continue;

            DDNode const& node = mgr->node_at(arc.index());
            Arc childArcs[ARITY];
            childArcs[0] = node.arc0();
            childArcs[1] = node.arc1();

            DdValues<T, ARITY> values;
            for (int b = 0; b < ARITY; ++b) {
                Arc childArc = childArcs[b];
                Arc normalizedChild = Arc(childArc.data & ~1ULL);
                int childIdx = arcToIndex[normalizedChild.data];
                values.setReference(b, work[childIdx]);
                values.setLevel(b, nodeLevel[childIdx]);
            }

            eval.evalNode(work[idx], level, values);
        }

        eval.destructLevel(level - 1);
    }

    // Handle negation at root
    int rootIdx = arcToIndex[normalizedRoot.data];
    return eval.getValue(work[rootIdx]);
}

} // namespace tdzdd
} // namespace sbdd2
