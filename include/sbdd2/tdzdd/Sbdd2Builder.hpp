/**
 * @file Sbdd2Builder.hpp
 * @brief TdZddスタイルのBFS（幅優先探索）ベースのZDD/BDDビルダー
 *
 * TdZdd互換のSpecからUnreducedZDD/ZDD/UnreducedBDD/BDD/MVZDD/MVBDDを
 * 幅優先トップダウン構築する関数群を提供する。
 *
 * @see DdSpec.hpp
 * @see VarArityDdSpec.hpp
 * @see Sbdd2BuilderDFS.hpp
 * @see Sbdd2BuilderMP.hpp
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
 * @brief 構築中のノードの状態情報を保持するクラス
 *
 * 各レベルのノードに対して、ハッシュコード・列インデックス・状態データを管理する。
 *
 * @see build_unreduced_zdd
 * @see build_unreduced_bdd
 */
class SpecNode {
    std::size_t code_;      ///< ハッシュコード
    std::size_t col_;       ///< このレベルにおける列インデックス
    char* state_;           ///< 状態データ（存在する場合）

public:
    /**
     * @brief デフォルトコンストラクタ
     */
    SpecNode() : code_(0), col_(0), state_(nullptr) {}

    /**
     * @brief コンストラクタ
     * @param code ハッシュコード
     * @param col 列インデックス
     * @param state 状態データへのポインタ
     */
    SpecNode(std::size_t code, std::size_t col, char* state)
        : code_(code), col_(col), state_(state) {}

    /**
     * @brief ハッシュコードを取得する
     * @return ハッシュコード
     */
    std::size_t code() const { return code_; }

    /**
     * @brief 列インデックスを取得する
     * @return 列インデックス
     */
    std::size_t col() const { return col_; }

    /**
     * @brief 状態データへのポインタを取得する
     * @return 状態データへのポインタ
     */
    void* state() { return state_; }

    /**
     * @brief 状態データへのconstポインタを取得する
     * @return 状態データへのconstポインタ
     */
    void const* state() const { return state_; }
};

/**
 * @brief TdZdd互換SpecからUnreducedZDDを構築する
 *
 * 幅優先トップダウン構築により、プレースホルダノードを使用して
 * UnreducedZDDを構築する。
 * フェーズ1（トップダウン状態探索）とフェーズ2（ボトムアップノード確定）
 * の2段階で処理する。
 *
 * @tparam SPEC Spec型（TdZdd Specインターフェースを満たす必要がある）
 * @param mgr 使用するDDManager
 * @param spec Specインスタンス
 * @param offset レベルオフセット：SpecレベルLをSAPPOROBDD2レベル(L + offset)に対応させる
 * @return 構築されたUnreducedZDD
 *
 * @see build_zdd
 * @see build_unreduced_bdd
 * @see DdSpec
 */
template<typename SPEC>
UnreducedZDD build_unreduced_zdd(DDManager& mgr, SPEC& spec, int offset = 0) {
    int const datasize = spec.datasize();
    int const ARITY = SPEC::ARITY;

    // ルート状態を取得
    std::vector<char> rootState(datasize > 0 ? datasize : 1);
    int rootLevel = spec.get_root(rootState.data());

    if (rootLevel == 0) {
        return UnreducedZDD::empty(mgr);
    }
    if (rootLevel < 0) {
        return UnreducedZDD::single(mgr);
    }

    // レベル (rootLevel + offset) までの変数を確保
    while (static_cast<int>(mgr.var_count()) < rootLevel + offset) {
        mgr.new_var();
    }

    // 各レベル用のデータ構造
    // nodeStates[level] = そのレベルのノードの状態データ
    // nodeChildren[level][col] = 子参照の配列（プレースホルダまたは終端）
    // nodeArcs[level][col] = (level, col) のノードのArc
    std::vector<std::vector<std::vector<char>>> nodeStates(rootLevel + 1);
    std::vector<std::vector<std::array<Arc, 2>>> nodeChildren(rootLevel + 1);
    std::vector<std::vector<Arc>> nodeArcs(rootLevel + 1);

    // 状態重複排除用ハッシュテーブル
    // (hash_code, state_ptr) -> 列インデックスの対応
    struct StateHash {
        std::size_t operator()(const std::pair<std::size_t, std::size_t>& p) const {
            return p.first;
        }
    };

    // フェーズ1：トップダウン状態探索
    // ルートから開始
    nodeStates[rootLevel].push_back(rootState);

    for (int level = rootLevel; level >= 1; --level) {
        std::size_t numNodes = nodeStates[level].size();
        if (numNodes == 0) continue;

        nodeChildren[level].resize(numNodes);
        nodeArcs[level].resize(numNodes);

        // このレベルのプレースホルダノードを作成
        // TdZddレベルL → SAPPOROBDD2レベル(L + offset)
        bddvar var = mgr.var_of_lev(level + offset);
        for (std::size_t col = 0; col < numNodes; ++col) {
            bddindex placeholder_idx = mgr.create_placeholder_zdd(var);
            nodeArcs[level][col] = Arc::node(placeholder_idx, false);
        }

        // 次レベルの状態重複排除用ハッシュテーブル
        std::unordered_map<std::size_t, std::vector<std::size_t>> nextLevelHash;

        // 子ノードを処理
        for (std::size_t col = 0; col < numNodes; ++col) {
            for (int b = 0; b < ARITY; ++b) {
                // 子計算用に状態をコピー
                std::vector<char> childState(datasize > 0 ? datasize : 1);
                if (datasize > 0) {
                    spec.get_copy(childState.data(), nodeStates[level][col].data());
                }

                int childLevel = spec.get_child(childState.data(), level, b);

                if (childLevel == 0) {
                    // 0終端
                    nodeChildren[level][col][b] = ARC_TERMINAL_0;
                } else if (childLevel < 0) {
                    // 1終端
                    nodeChildren[level][col][b] = ARC_TERMINAL_1;
                } else {
                    // 非終端：子ノードを検索または作成
                    assert(childLevel < level);

                    // 子レベルの記憶域を確保
                    if (nodeStates[childLevel].empty()) {
                        // このレベルを初期化
                    }

                    // ハッシュ値を計算
                    std::size_t hashCode = spec.hash_code(childState.data(), childLevel);

                    // 同じ状態を持つ既存ノードを検索
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
                        // 新規ノードを作成
                        childCol = nodeStates[childLevel].size();
                        nodeStates[childLevel].push_back(childState);
                        nextLevelHash[hashCode].push_back(childCol);
                    }

                    // プレースホルダ参照を格納
                    nodeChildren[level][col][b] = Arc::placeholder(childLevel, childCol);
                }
            }

            // このノードの状態を破棄
            if (datasize > 0) {
                spec.destruct(nodeStates[level][col].data());
            }
        }

        // 処理済みレベルの状態記憶域をクリア（不要になったため）
        nodeStates[level].clear();
        nodeStates[level].shrink_to_fit();

        spec.destructLevel(level);
    }

    // フェーズ2：ボトムアップノード確定
    for (int level = 1; level <= rootLevel; ++level) {
        std::size_t numNodes = nodeArcs[level].size();

        for (std::size_t col = 0; col < numNodes; ++col) {
            Arc arc0 = nodeChildren[level][col][0];
            Arc arc1 = nodeChildren[level][col][1];

            // プレースホルダArcを実際のArcに解決
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

            // ノードを確定
            bddindex placeholder_idx = nodeArcs[level][col].index();
            Arc finalArc = mgr.finalize_node_zdd(placeholder_idx, arc0, arc1, false);
            nodeArcs[level][col] = finalArc;
        }

        // 処理済みレベルの子記憶域をクリア
        nodeChildren[level].clear();
        nodeChildren[level].shrink_to_fit();
    }

    // 未リンクノードをクリーンアップ
    mgr.clear_unlinked_nodes();

    // ルートを返す
    return UnreducedZDD(&mgr, nodeArcs[rootLevel][0]);
}

/**
 * @brief TdZdd互換SpecからリダクションされたZDDを構築する
 *
 * 内部でbuild_unreduced_zddを呼び出した後、reduce()を適用する。
 *
 * @tparam SPEC Spec型（TdZdd Specインターフェースを満たす必要がある）
 * @param mgr 使用するDDManager
 * @param spec Specインスタンス
 * @param offset レベルオフセット：SpecレベルLをSAPPOROBDD2レベル(L + offset)に対応させる
 * @return リダクションされたZDD
 *
 * @see build_unreduced_zdd
 * @see build_zdd_dfs
 */
template<typename SPEC>
ZDD build_zdd(DDManager& mgr, SPEC& spec, int offset = 0) {
    UnreducedZDD unreduced = build_unreduced_zdd(mgr, spec, offset);
    return unreduced.reduce();
}

/**
 * @brief TdZdd互換SpecからUnreducedBDDを構築する
 *
 * 幅優先トップダウン構築により、プレースホルダノードを使用して
 * UnreducedBDDを構築する。
 * ZDD版と同様に2段階で処理するが、BDD用のプレースホルダとファイナライズを使用する。
 *
 * @tparam SPEC Spec型（TdZdd Specインターフェースを満たす必要がある）
 * @param mgr 使用するDDManager
 * @param spec Specインスタンス
 * @param offset レベルオフセット：SpecレベルLをSAPPOROBDD2レベル(L + offset)に対応させる
 * @return 構築されたUnreducedBDD
 *
 * @see build_bdd
 * @see build_unreduced_zdd
 */
template<typename SPEC>
UnreducedBDD build_unreduced_bdd(DDManager& mgr, SPEC& spec, int offset = 0) {
    int const datasize = spec.datasize();
    int const ARITY = SPEC::ARITY;

    // ルート状態を取得
    std::vector<char> rootState(datasize > 0 ? datasize : 1);
    int rootLevel = spec.get_root(rootState.data());

    if (rootLevel == 0) {
        return UnreducedBDD::zero(mgr);
    }
    if (rootLevel < 0) {
        return UnreducedBDD::one(mgr);
    }

    // レベル (rootLevel + offset) までの変数を確保
    while (static_cast<int>(mgr.var_count()) < rootLevel + offset) {
        mgr.new_var();
    }

    // 各レベル用のデータ構造
    std::vector<std::vector<std::vector<char>>> nodeStates(rootLevel + 1);
    std::vector<std::vector<std::array<Arc, 2>>> nodeChildren(rootLevel + 1);
    std::vector<std::vector<Arc>> nodeArcs(rootLevel + 1);

    // フェーズ1：トップダウン状態探索
    nodeStates[rootLevel].push_back(rootState);

    for (int level = rootLevel; level >= 1; --level) {
        std::size_t numNodes = nodeStates[level].size();
        if (numNodes == 0) continue;

        nodeChildren[level].resize(numNodes);
        nodeArcs[level].resize(numNodes);

        // このレベルのプレースホルダノードを作成
        // TdZddレベルL → SAPPOROBDD2レベル(L + offset)
        bddvar var = mgr.var_of_lev(level + offset);
        for (std::size_t col = 0; col < numNodes; ++col) {
            bddindex placeholder_idx = mgr.create_placeholder_bdd(var);
            nodeArcs[level][col] = Arc::node(placeholder_idx, false);
        }

        // 次レベルの状態重複排除用ハッシュテーブル
        std::unordered_map<std::size_t, std::vector<std::size_t>> nextLevelHash;

        // 子ノードを処理
        for (std::size_t col = 0; col < numNodes; ++col) {
            for (int b = 0; b < ARITY; ++b) {
                // 子計算用に状態をコピー
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

    // フェーズ2：ボトムアップノード確定
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
 * @brief TdZdd互換SpecからリダクションされたBDDを構築する
 *
 * 内部でbuild_unreduced_bddを呼び出した後、reduce()を適用する。
 *
 * @tparam SPEC Spec型（TdZdd Specインターフェースを満たす必要がある）
 * @param mgr 使用するDDManager
 * @param spec Specインスタンス
 * @param offset レベルオフセット：SpecレベルLをSAPPOROBDD2レベル(L + offset)に対応させる
 * @return リダクションされたBDD
 *
 * @see build_unreduced_bdd
 * @see build_bdd_dfs
 */
template<typename SPEC>
BDD build_bdd(DDManager& mgr, SPEC& spec, int offset = 0) {
    UnreducedBDD unreduced = build_unreduced_bdd(mgr, spec, offset);
    return unreduced.reduce();
}

/**
 * @brief TdZdd互換Spec（ARITY >= 2）からMVZDDを構築する
 *
 * ARITY >= 3のSpecを想定しているが、ARITY = 2でも動作する。
 * ARITY = 2の場合は、パフォーマンスの観点からbuild_zddの使用を推奨する。
 *
 * フェーズ1でトップダウン状態探索を行い、フェーズ2でボトムアップに
 * ITEを使用してMVZDDノードを構築する。
 * TdZddレベルL → MVDD変数 (rootLevel - L + 1) の対応で変換する。
 *
 * @tparam SPEC Spec型（TdZdd Specインターフェースを満たす必要がある）
 * @param mgr 使用するDDManager
 * @param spec Specインスタンス
 * @return 構築されたMVZDD
 *
 * @see build_zdd
 * @see build_mvzdd_va
 * @see build_mvbdd
 */
template<typename SPEC>
MVZDD build_mvzdd(DDManager& mgr, SPEC& spec) {
    int const datasize = spec.datasize();
    int const ARITY = SPEC::ARITY;

    // ルート状態を取得
    std::vector<char> rootState(datasize > 0 ? datasize : 1);
    int rootLevel = spec.get_root(rootState.data());

    // k = ARITYでMVZDDを作成
    MVZDD base_mvzdd = MVZDD::empty(mgr, ARITY);

    if (rootLevel == 0) {
        return base_mvzdd;  // 空集合
    }
    if (rootLevel < 0) {
        return MVZDD::single(mgr, ARITY);  // 基底集合
    }

    // 全レベル分のMVDD変数を事前作成
    while (static_cast<int>(base_mvzdd.mvdd_var_count()) < rootLevel) {
        base_mvzdd.new_var();
    }

    // 終端MVZDD
    MVZDD empty_mvzdd = MVZDD(base_mvzdd.manager(), base_mvzdd.var_table(),
                               ZDD::empty(mgr));
    MVZDD base_term = MVZDD(base_mvzdd.manager(), base_mvzdd.var_table(),
                             ZDD::single(mgr));

    // データ構造
    std::vector<std::vector<std::vector<char>>> nodeStates(rootLevel + 1);
    std::vector<std::vector<std::vector<std::pair<int, std::size_t>>>> childRefs(rootLevel + 1);
    std::vector<std::vector<MVZDD>> nodeMVZDDs(rootLevel + 1);

    // フェーズ1：トップダウン状態探索
    nodeStates[rootLevel].push_back(rootState);

    for (int level = rootLevel; level >= 1; --level) {
        std::size_t numNodes = nodeStates[level].size();
        if (numNodes == 0) continue;

        childRefs[level].resize(numNodes);

        // 次レベルの状態重複排除用ハッシュテーブル
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

    // フェーズ2：ボトムアップMVZDD構築
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

            // ITEを使用してMVZDDノードを構築
            // TdZddレベル番号：rootLevelが最上位、1が最下位
            // MVDD変数番号：1が最初に作成された変数（最上位）
            // 対応：TdZddレベルL → MVDD変数 (rootLevel - L + 1)
            // つまりレベルrootLevel → MVDD変数1（上）、レベル1 → MVDD変数rootLevel（下）
            bddvar mv = static_cast<bddvar>(rootLevel - level + 1);
            nodeMVZDDs[level][col] = MVZDD::ite(base_mvzdd, mv, children);
        }

        childRefs[level].clear();
        childRefs[level].shrink_to_fit();
    }

    // ルートを返す
    return nodeMVZDDs[rootLevel][0];
}

/**
 * @brief TdZdd互換Spec（ARITY >= 2）からMVBDDを構築する
 *
 * ARITY >= 3のSpecを想定しているが、ARITY = 2でも動作する。
 * ARITY = 2の場合は、パフォーマンスの観点からbuild_bddの使用を推奨する。
 *
 * MVZDD版と同様に2段階で処理し、ITEを使用してMVBDDノードを構築する。
 *
 * @tparam SPEC Spec型（TdZdd Specインターフェースを満たす必要がある）
 * @param mgr 使用するDDManager
 * @param spec Specインスタンス
 * @return 構築されたMVBDD
 *
 * @see build_bdd
 * @see build_mvbdd_va
 * @see build_mvzdd
 */
template<typename SPEC>
MVBDD build_mvbdd(DDManager& mgr, SPEC& spec) {
    int const datasize = spec.datasize();
    int const ARITY = SPEC::ARITY;

    // ルート状態を取得
    std::vector<char> rootState(datasize > 0 ? datasize : 1);
    int rootLevel = spec.get_root(rootState.data());

    // k = ARITYでMVBDDを作成
    MVBDD base_mvbdd = MVBDD::zero(mgr, ARITY);

    if (rootLevel == 0) {
        return base_mvbdd;  // 定数0
    }
    if (rootLevel < 0) {
        return MVBDD::one(mgr, ARITY);  // 定数1
    }

    // 全レベル分のMVDD変数を事前作成
    while (static_cast<int>(base_mvbdd.mvdd_var_count()) < rootLevel) {
        base_mvbdd.new_var();
    }

    // 終端MVBDD
    MVBDD zero_mvbdd = MVBDD(base_mvbdd.manager(), base_mvbdd.var_table(),
                              BDD::zero(mgr));
    MVBDD one_mvbdd = MVBDD(base_mvbdd.manager(), base_mvbdd.var_table(),
                             BDD::one(mgr));

    // データ構造
    std::vector<std::vector<std::vector<char>>> nodeStates(rootLevel + 1);
    std::vector<std::vector<std::vector<std::pair<int, std::size_t>>>> childRefs(rootLevel + 1);
    std::vector<std::vector<MVBDD>> nodeMVBDDs(rootLevel + 1);

    // フェーズ1：トップダウン状態探索
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

    // フェーズ2：ボトムアップMVBDD構築
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

            // 対応：TdZddレベルL → MVDD変数 (rootLevel - L + 1)
            bddvar mv = static_cast<bddvar>(rootLevel - level + 1);
            nodeMVBDDs[level][col] = MVBDD::ite(base_mvbdd, mv, children);
        }

        childRefs[level].clear();
        childRefs[level].shrink_to_fit();
    }

    return nodeMVBDDs[rootLevel][0];
}

/**
 * @brief 実行時可変ARITYのVarArity SpecからMVZDDを構築する
 *
 * SPEC::ARITYの代わりにgetArity()を使用するSpecと連携する。
 * 構築アルゴリズムはbuild_mvzddと同一だが、ARITYを実行時に取得する。
 *
 * @tparam SPEC Spec型（getArity()メソッドを持つ必要がある）
 * @param mgr 使用するDDManager
 * @param spec Specインスタンス
 * @return 構築されたMVZDD
 *
 * @see build_mvzdd
 * @see build_mvbdd_va
 * @see VarArityDdSpec
 * @see VarArityHybridDdSpec
 */
template<typename SPEC>
MVZDD build_mvzdd_va(DDManager& mgr, SPEC& spec) {
    int const datasize = spec.datasize();
    int const ARITY = spec.getArity();

    // ルート状態を取得
    std::vector<char> rootState(datasize > 0 ? datasize : 1);
    int rootLevel = spec.get_root(rootState.data());

    // k = ARITYでMVZDDを作成
    MVZDD base_mvzdd = MVZDD::empty(mgr, ARITY);

    if (rootLevel == 0) {
        return base_mvzdd;  // 空集合
    }
    if (rootLevel < 0) {
        return MVZDD::single(mgr, ARITY);  // 基底集合
    }

    // 全レベル分のMVDD変数を事前作成
    while (static_cast<int>(base_mvzdd.mvdd_var_count()) < rootLevel) {
        base_mvzdd.new_var();
    }

    // 終端MVZDD
    MVZDD empty_mvzdd = MVZDD(base_mvzdd.manager(), base_mvzdd.var_table(),
                               ZDD::empty(mgr));
    MVZDD base_term = MVZDD(base_mvzdd.manager(), base_mvzdd.var_table(),
                             ZDD::single(mgr));

    // データ構造
    std::vector<std::vector<std::vector<char>>> nodeStates(rootLevel + 1);
    std::vector<std::vector<std::vector<std::pair<int, std::size_t>>>> childRefs(rootLevel + 1);
    std::vector<std::vector<MVZDD>> nodeMVZDDs(rootLevel + 1);

    // フェーズ1：トップダウン状態探索
    nodeStates[rootLevel].push_back(rootState);

    for (int level = rootLevel; level >= 1; --level) {
        std::size_t numNodes = nodeStates[level].size();
        if (numNodes == 0) continue;

        childRefs[level].resize(numNodes);

        // 次レベルの状態重複排除用ハッシュテーブル
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

    // フェーズ2：ボトムアップMVZDD構築
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

            // ITEを使用してMVZDDノードを構築
            // 対応：TdZddレベルL -> MVDD変数 (rootLevel - L + 1)
            bddvar mv = static_cast<bddvar>(rootLevel - level + 1);
            nodeMVZDDs[level][col] = MVZDD::ite(base_mvzdd, mv, children);
        }

        childRefs[level].clear();
        childRefs[level].shrink_to_fit();
    }

    // ルートを返す
    return nodeMVZDDs[rootLevel][0];
}

/**
 * @brief 実行時可変ARITYのVarArity SpecからMVBDDを構築する
 *
 * SPEC::ARITYの代わりにgetArity()を使用するSpecと連携する。
 * 構築アルゴリズムはbuild_mvbddと同一だが、ARITYを実行時に取得する。
 *
 * @tparam SPEC Spec型（getArity()メソッドを持つ必要がある）
 * @param mgr 使用するDDManager
 * @param spec Specインスタンス
 * @return 構築されたMVBDD
 *
 * @see build_mvbdd
 * @see build_mvzdd_va
 * @see VarArityDdSpec
 * @see VarArityHybridDdSpec
 */
template<typename SPEC>
MVBDD build_mvbdd_va(DDManager& mgr, SPEC& spec) {
    int const datasize = spec.datasize();
    int const ARITY = spec.getArity();

    // ルート状態を取得
    std::vector<char> rootState(datasize > 0 ? datasize : 1);
    int rootLevel = spec.get_root(rootState.data());

    // k = ARITYでMVBDDを作成
    MVBDD base_mvbdd = MVBDD::zero(mgr, ARITY);

    if (rootLevel == 0) {
        return base_mvbdd;  // 定数0
    }
    if (rootLevel < 0) {
        return MVBDD::one(mgr, ARITY);  // 定数1
    }

    // 全レベル分のMVDD変数を事前作成
    while (static_cast<int>(base_mvbdd.mvdd_var_count()) < rootLevel) {
        base_mvbdd.new_var();
    }

    // 終端MVBDD
    MVBDD zero_mvbdd = MVBDD(base_mvbdd.manager(), base_mvbdd.var_table(),
                              BDD::zero(mgr));
    MVBDD one_mvbdd = MVBDD(base_mvbdd.manager(), base_mvbdd.var_table(),
                             BDD::one(mgr));

    // データ構造
    std::vector<std::vector<std::vector<char>>> nodeStates(rootLevel + 1);
    std::vector<std::vector<std::vector<std::pair<int, std::size_t>>>> childRefs(rootLevel + 1);
    std::vector<std::vector<MVBDD>> nodeMVBDDs(rootLevel + 1);

    // フェーズ1：トップダウン状態探索
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

    // フェーズ2：ボトムアップMVBDD構築
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

            // 対応：TdZddレベルL -> MVDD変数 (rootLevel - L + 1)
            bddvar mv = static_cast<bddvar>(rootLevel - level + 1);
            nodeMVBDDs[level][col] = MVBDD::ite(base_mvbdd, mv, children);
        }

        childRefs[level].clear();
        childRefs[level].shrink_to_fit();
    }

    return nodeMVBDDs[rootLevel][0];
}

// ============================================================
// ZDDサブセット操作
// ============================================================

/**
 * @brief 既存ZDDにSpecをフィルタとして適用しサブセットZDDを構築する
 *
 * TdZddのDdStructure::zddSubset()に相当する操作。
 * 入力ZDDとSpecが定義する集合の共通部分を計算する。
 * 再帰的メモ化アプローチを使用して効率的に処理する。
 *
 * @tparam SPEC Spec型（TdZdd Specインターフェースを満たす必要がある）
 * @param mgr 使用するDDManager
 * @param input サブセット対象の入力ZDD
 * @param spec フィルタを定義するSpecインスタンス
 * @param offset レベルオフセット：SpecレベルLをSAPPOROBDD2レベル(L + offset)に対応させる
 * @return 入力ZDDとSpecの共通部分を表すZDD（input ∩ spec）
 *
 * @see build_zdd
 */
template<typename SPEC>
ZDD zdd_subset(DDManager& mgr, ZDD const& input, SPEC& spec, int offset = 0) {
    // 終端ケースの処理
    if (input == ZDD::empty(mgr)) {
        return ZDD::empty(mgr);
    }

    // 再帰的メモ化アプローチを使用
    // キー：(入力ZDD id, 状態ハッシュ) -> 結果ZDD
    typedef std::pair<bddindex, std::size_t> MemoKey;
    struct MemoKeyHash {
        std::size_t operator()(MemoKey const& k) const {
            return k.first * 314159257 + k.second;
        }
    };
    std::unordered_map<MemoKey, ZDD, MemoKeyHash> memo;

    int const stateSize = spec.datasize();

    // 状態ハッシュ計算用ヘルパー
    auto stateHash = [stateSize](void const* state) -> std::size_t {
        if (stateSize == 0) return 0;
        std::size_t h = 0;
        char const* p = static_cast<char const*>(state);
        for (int i = 0; i < stateSize; ++i) {
            h = h * 31 + static_cast<unsigned char>(p[i]);
        }
        return h;
    };

    // 再帰的サブセット関数
    // SpecレベルLはSAPPOROBDD2レベルLに対応
    std::function<ZDD(ZDD const&, void*, int)> subsetRec;
    subsetRec = [&](ZDD const& f, void* state, int specLev) -> ZDD {
        // 終端ケース
        if (specLev == 0) {
            return ZDD::empty(mgr);
        }
        if (f == ZDD::empty(mgr)) {
            return ZDD::empty(mgr);
        }
        if (specLev < 0) {
            // Specが受理 - 残りの変数はすべて0（非選択）でなければならない
            // 終端までlow辺をたどる
            ZDD current = f;
            while (!current.is_terminal()) {
                current = current.low();
            }
            return current;  // emptyまたはsingle
        }
        if (f == ZDD::single(mgr)) {
            // 入力が1終端 - Specの0辺をたどって受理するか確認
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

        // SpecレベルLはSAPPOROBDD2レベル(L + offset)に対応
        // レベル(specLev + offset)の変数を取得
        bddvar specVar = mgr.var_of_lev(specLev + offset);
        int specZddLev = specLev + offset;

        // 入力ZDDの現在の変数とそのレベルを取得
        bddvar inputVar = f.top();
        int inputZddLev = static_cast<int>(mgr.lev_of_var(inputVar));


        // ZDDレベルで同期（高いレベル = ルートに近い）
        if (specZddLev > inputZddLev) {
            // Spec変数の方が高い - Specの0辺をたどる
            std::vector<char> newState(stateSize > 0 ? stateSize : 1);
            if (stateSize > 0) {
                spec.get_copy(newState.data(), state);
            }
            int newSpecLev = spec.get_child(newState.data(), specLev, 0);
            ZDD result = subsetRec(f, newState.data(), newSpecLev);
            spec.destruct(newState.data());
            return result;
        }
        if (specZddLev < inputZddLev) {
            // 入力変数の方が高い - 入力の0辺をたどる
            return subsetRec(f.low(), state, specLev);
        }

        // 同じZDDレベル - メモを確認
        bddindex inputId = f.is_terminal() ? 0 : f.id();
        std::size_t sh = stateHash(state);
        MemoKey key(inputId, sh);
        typename std::unordered_map<MemoKey, ZDD, MemoKeyHash>::iterator it = memo.find(key);
        if (it != memo.end()) {
            return it->second;
        }

        // 子ノードを処理
        ZDD low, high;

        // 0子
        {
            std::vector<char> childState(stateSize > 0 ? stateSize : 1);
            if (stateSize > 0) {
                spec.get_copy(childState.data(), state);
            }
            int childSpecLev = spec.get_child(childState.data(), specLev, 0);
            low = subsetRec(f.low(), childState.data(), childSpecLev);
            spec.destruct(childState.data());
        }

        // 1子
        {
            std::vector<char> childState(stateSize > 0 ? stateSize : 1);
            if (stateSize > 0) {
                spec.get_copy(childState.data(), state);
            }
            int childSpecLev = spec.get_child(childState.data(), specLev, 1);
            high = subsetRec(f.high(), childState.data(), childSpecLev);
            spec.destruct(childState.data());
        }

        // 結果ノードを構築（ZDDリダクション付き）
        ZDD result;
        if (high == ZDD::empty(mgr)) {
            result = low;
        } else {
            // Specレベルに対応する変数を使用
            Arc lowArc = low.arc();
            Arc highArc = high.arc();
            Arc resultArc = mgr.get_or_create_node_zdd(specVar, lowArc, highArc, true);
            result = resultArc.is_constant() ?
                (resultArc.terminal_value() ? ZDD::single(mgr) : ZDD::empty(mgr)) :
                ZDD(&mgr, resultArc);
        }

        memo[key] = result;
        return result;
    };

    // Specルートを取得
    std::vector<char> rootState(stateSize > 0 ? stateSize : 1);
    int specRootLev = spec.get_root(rootState.data());

    ZDD result = subsetRec(input, rootState.data(), specRootLev);
    spec.destruct(rootState.data());

    return result;
}

} // namespace tdzdd
} // namespace sbdd2
