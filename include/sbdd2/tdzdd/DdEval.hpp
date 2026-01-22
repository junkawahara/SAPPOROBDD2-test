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

} // namespace tdzdd
} // namespace sbdd2
