/**
 * @file DdEval.hpp
 * @brief TdZdd互換のDD評価（Eval）インターフェース
 *
 * DDをボトムアップに走査し、各ノードで評価値を計算するための
 * 評価器（Evaluator）基底クラスと評価関数を提供する。
 *
 * 元のTdZddライブラリ（岩下洋哉氏）に基づく。
 * Copyright (c) 2014 ERATO MINATO Project
 * SAPPOROBDD2名前空間に移植。
 *
 * @see DdSpec.hpp Spec基底クラス
 * @see Sbdd2Builder.hpp ビルダー関数群
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
 * @brief DdEval::evalNodeに渡す子ノード値・レベルのコレクション
 *
 * 各子ノードの評価値とレベルを保持し、evalNodeメソッドから参照できるようにする。
 *
 * @tparam T 各ノードの作業領域のデータ型
 * @tparam ARITY ノードの子の数
 * @see DdEval
 */
template<typename T, int ARITY>
class DdValues {
    T const* value_[ARITY];
    int level_[ARITY];

public:
    /**
     * @brief 第b番目の子ノードの評価値を取得する
     * @param b 分岐インデックス（0 ~ ARITY-1）
     * @return 子ノードの評価値への参照
     */
    T const& get(int b) const {
        assert(0 <= b && b < ARITY);
        return *value_[b];
    }

    /**
     * @brief 第b番目の子ノードのレベルを取得する
     * @param b 分岐インデックス（0 ~ ARITY-1）
     * @return 子ノードのレベル
     */
    int getLevel(int b) const {
        assert(0 <= b && b < ARITY);
        return level_[b];
    }

    /**
     * @brief 第b番目の子ノードの評価値への参照を設定する（内部用）
     * @param b 分岐インデックス
     * @param v 評価値への参照
     */
    void setReference(int b, T const& v) {
        assert(0 <= b && b < ARITY);
        value_[b] = &v;
    }

    /**
     * @brief 第b番目の子ノードのレベルを設定する（内部用）
     * @param b 分岐インデックス
     * @param i レベル値
     */
    void setLevel(int b, int i) {
        assert(0 <= b && b < ARITY);
        level_[b] = i;
    }

    /**
     * @brief DdValuesの内容を出力ストリームに書き出す
     * @param os 出力ストリーム
     * @param o DdValuesオブジェクト
     * @return 出力ストリームへの参照
     */
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
 * @brief DD評価器の基底クラス
 *
 * DDをボトムアップに走査し、各ノードで評価値を計算するための基底クラス。
 * CRTPパターンを使用する。
 *
 * 派生クラスは以下のメソッドを必ず実装する:
 * - void evalTerminal(T& v, int id) : 終端ノードの評価値を設定する
 *   - id=0: 0終端, id=1: 1終端
 * - void evalNode(T& v, int level, DdValues<T,ARITY> const& values) : 非終端ノードの評価値を計算する
 *
 * オプションでオーバーライド可能なメソッド:
 * - bool showMessages() : メッセージ表示の有無（デフォルト: false）
 * - void initialize(int level) : 最大レベルでの初期化処理
 * - R getValue(T const& work) : 最終結果値の変換（デフォルト: R(work)）
 * - void destructLevel(int i) : レベルごとのリソース解放
 *
 * @tparam E このクラスを実装する派生クラス（CRTP）
 * @tparam T 各ノードの作業領域のデータ型
 * @tparam R 評価結果のデータ型（デフォルト: T）
 * @tparam ARITY DDのアリティ（デフォルト: 2）
 * @see DdValues 子ノード値のコレクション
 * @see CardinalityEval 解の数を数える評価器
 * @see evaluate_spec Specの評価関数
 * @see zdd_evaluate ZDDの評価関数
 * @see bdd_evaluate BDDの評価関数
 */
template<typename E, typename T, typename R = T, int ARITY = 2>
class DdEval {
public:
    /**
     * @brief CRTP派生クラスの参照を取得する
     * @return 派生クラスへの参照
     */
    E& entity() {
        return *static_cast<E*>(this);
    }

    /**
     * @brief CRTP派生クラスのconst参照を取得する
     * @return 派生クラスへのconst参照
     */
    E const& entity() const {
        return *static_cast<E const*>(this);
    }

    /**
     * @brief スレッドセーフかどうかを宣言する
     * @return スレッドセーフならtrue
     */
    bool isThreadSafe() const {
        return true;
    }

    /**
     * @brief メッセージ表示を希望するかどうかを宣言する
     * @return メッセージを表示する場合true
     */
    bool showMessages() const {
        return false;
    }

    /**
     * @brief 評価の初期化処理を行う
     * @param level DDの最大レベル
     */
    void initialize(int level) {
        (void)level;
    }

    /**
     * @brief ルートノードの作業領域値から最終結果を生成する
     * @param v ルートノードの作業領域値
     * @return 評価の最終結果
     */
    R getValue(T const& v) {
        return R(v);
    }

    /**
     * @brief 指定レベルのデータストレージを破棄する
     * @param i 破棄するレベル
     */
    void destructLevel(int i) {
        (void)i;
    }
};

/**
 * @brief 解の数（カーディナリティ）を数える評価器
 *
 * ZDDに含まれる解集合の要素数を計算する。
 * 0終端は値0、1終端は値1とし、各ノードで0枝と1枝の値を合計する。
 *
 * @see DdEval 基底クラス
 * @see evaluate_spec Specの評価関数
 * @see zdd_evaluate ZDDの評価関数
 */
class CardinalityEval : public DdEval<CardinalityEval, double, double, 2> {
public:
    /**
     * @brief 終端ノードの評価値を設定する
     * @param v 評価値の格納先
     * @param id 終端ID（0: 0終端, 1: 1終端）
     */
    void evalTerminal(double& v, int id) {
        v = (id == 1) ? 1.0 : 0.0;
    }

    /**
     * @brief 非終端ノードの評価値を計算する（0枝+1枝の合計）
     * @param v 評価値の格納先
     * @param level 現在のレベル（未使用）
     * @param values 子ノードの評価値コレクション
     */
    void evalNode(double& v, int level, DdValues<double, 2> const& values) {
        (void)level;
        v = values.get(0) + values.get(1);
    }
};

/**
 * @brief Specを評価器で評価する
 *
 * Specをトップダウンに展開し、ボトムアップに評価値を計算する。
 * 再帰的な走査により結果を求める。
 *
 * @tparam SPEC Spec型
 * @tparam E 評価器型（DdEval派生クラス）
 * @tparam T 作業領域のデータ型
 * @tparam R 結果のデータ型
 * @param spec 評価対象のSpec
 * @param eval_base 評価器インスタンス
 * @return 評価結果
 * @see DdEval 評価器基底クラス
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
 * @brief ZDDを評価器で評価する
 *
 * ZDDをボトムアップに走査し、評価器を適用して結果を計算する。
 * BFSでノードを収集した後、レベル順に評価を行う。
 *
 * @tparam E 評価器型（DdEval派生クラス）
 * @tparam T 作業領域のデータ型
 * @tparam R 結果のデータ型
 * @tparam ARITY DDのアリティ（デフォルト: 2）
 * @param zdd 評価対象のZDD
 * @param eval_base 評価器インスタンス
 * @return 評価結果
 * @see DdEval 評価器基底クラス
 * @see bdd_evaluate BDDの評価関数
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
 * @brief BDDを評価器で評価する
 *
 * BDDをボトムアップに走査し、評価器を適用して結果を計算する。
 * BFSでノードを収集した後、レベル順に評価を行う。
 * BDDの否定辺はインデックスのマスクにより正規化される。
 *
 * @tparam E 評価器型（DdEval派生クラス）
 * @tparam T 作業領域のデータ型
 * @tparam R 結果のデータ型
 * @tparam ARITY DDのアリティ（デフォルト: 2）
 * @param bdd 評価対象のBDD
 * @param eval_base 評価器インスタンス
 * @return 評価結果
 * @see DdEval 評価器基底クラス
 * @see zdd_evaluate ZDDの評価関数
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
