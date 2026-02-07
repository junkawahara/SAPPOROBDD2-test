/**
 * @file Sbdd2BuilderMP.hpp
 * @brief OpenMP並列版TdZddスタイルのZDD/BDDビルダー
 *
 * TdZdd互換のSpecからUnreducedZDD/ZDD/UnreducedBDD/BDD/MVZDD/MVBDDを
 * OpenMPによる並列幅優先トップダウン構築で生成する関数群を提供する。
 * スレッドローカルストレージを用いて状態の重複排除を並列化し、
 * その後グローバルにマージする方式を採用している。
 *
 * @see Sbdd2Builder.hpp
 * @see Sbdd2BuilderDFS.hpp
 * @see DdSpec.hpp
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
 * @brief 並列構築用のスレッド数を取得する
 *
 * OpenMPが有効な場合はomp_get_max_threads()の値を返し、
 * 無効な場合は1を返す。
 *
 * @return 使用可能なスレッド数
 */
inline int get_num_threads() {
#ifdef _OPENMP
    return omp_get_max_threads();
#else
    return 1;
#endif
}

/**
 * @brief OpenMP並列でTdZdd互換SpecからUnreducedZDDを構築する
 *
 * フェーズ1（トップダウン状態探索）をOpenMPで並列化する。
 * 各スレッドがスレッドローカルな状態ストレージとハッシュテーブルを使用し、
 * レベル処理後にグローバルにマージする。
 * フェーズ2（ボトムアップノード確定）は正確性のため逐次実行する。
 *
 * スレッドIDとローカル列インデックスを (tid << 48) | localCol として
 * エンコードしプレースホルダ参照に格納する。
 *
 * @tparam SPEC Spec型（TdZdd Specインターフェースを満たす必要がある）
 * @param mgr 使用するDDManager
 * @param spec Specインスタンス
 * @return 構築されたUnreducedZDD
 *
 * @see build_zdd_mp
 * @see build_unreduced_zdd
 */
template<typename SPEC>
UnreducedZDD build_unreduced_zdd_mp(DDManager& mgr, SPEC& spec) {
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

    // 変数の存在を確保
    while (static_cast<int>(mgr.var_count()) < rootLevel) {
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
        // TdZddレベルL → SAPPOROBDD2レベルL（同一セマンティクス：高レベル = ルートに近い）
        bddvar var = mgr.var_of_lev(level);
        for (std::size_t col = 0; col < numNodes; ++col) {
            bddindex placeholder_idx = mgr.create_placeholder_zdd(var);
            nodeArcs[level][col] = Arc::node(placeholder_idx, false);
        }

        // 次レベルノード用のスレッドローカルストレージ
        int numThreads = get_num_threads();
        std::vector<std::vector<std::vector<char>>> threadLocalStates(numThreads);
        std::vector<std::unordered_map<std::size_t, std::vector<std::size_t>>> threadLocalHash(numThreads);

        // 最終マージ用のグローバルハッシュ
        std::unordered_map<std::size_t, std::vector<std::size_t>> globalHash;
        std::mutex globalMutex;

        // 子ノードを並列処理
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

                        // まずスレッドローカルストレージで既存ノードを検索
                        bool found = false;
                        std::size_t childCol = 0;

                        // スレッドローカルハッシュを確認
                        auto& localHash = threadLocalHash[tid];
                        auto it = localHash.find(hashCode);
                        if (it != localHash.end()) {
                            for (std::size_t existingIdx : it->second) {
                                if (spec.equal_to(childState.data(),
                                                  threadLocalStates[tid][existingIdx].data(),
                                                  childLevel)) {
                                    // スレッドローカルで発見
                                    childCol = existingIdx;
                                    found = true;
                                    break;
                                }
                            }
                        }

                        if (!found) {
                            // スレッドローカルストレージに追加
                            childCol = threadLocalStates[tid].size();
                            threadLocalStates[tid].push_back(childState);
                            localHash[hashCode].push_back(childCol);
                        }

                        // プレースホルダ参照を格納（後で解決）
                        // 特殊エンコーディング：(tid << 48) | childCol
                        std::uint64_t encodedRef = (static_cast<std::uint64_t>(tid) << 48) | childCol;
                        nodeChildren[level][col][b] = Arc::placeholder(childLevel, encodedRef);
                    }
                }

                // このノードの状態を破棄
                if (datasize > 0) {
                    spec.destruct(nodeStates[level][col].data());
                }
            }
        }

        // スレッドローカル結果を次レベルのグローバル状態にマージ
        if (level > 1) {
            int nextLevel = level - 1;
            std::vector<std::pair<int, std::size_t>> threadColMapping;  // (tid, localCol) -> globalCol

            // 第1パス：全一意状態をマージ
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

            // マッピングテーブルを構築：(tid, localCol) -> globalCol
            std::vector<std::vector<std::size_t>> colMapping(numThreads);
            std::size_t mappingIdx = 0;
            for (int tid = 0; tid < numThreads; ++tid) {
                colMapping[tid].resize(threadLocalStates[tid].size());
                for (std::size_t localCol = 0; localCol < threadLocalStates[tid].size(); ++localCol) {
                    colMapping[tid][localCol] = threadColMapping[mappingIdx++].second;
                }
            }

            // プレースホルダ参照をグローバル列インデックスで更新
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

        // 処理済みレベルの状態記憶域をクリア
        nodeStates[level].clear();
        nodeStates[level].shrink_to_fit();
        spec.destructLevel(level);
    }

    // フェーズ2：ボトムアップノード確定（正確性のため逐次実行）
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

        nodeChildren[level].clear();
        nodeChildren[level].shrink_to_fit();
    }

    mgr.clear_unlinked_nodes();

    return UnreducedZDD(&mgr, nodeArcs[rootLevel][0]);
}

/**
 * @brief OpenMP並列でTdZdd互換SpecからリダクションされたZDDを構築する
 *
 * 内部でbuild_unreduced_zdd_mpを呼び出した後、reduce()を適用する。
 *
 * @tparam SPEC Spec型（TdZdd Specインターフェースを満たす必要がある）
 * @param mgr 使用するDDManager
 * @param spec Specインスタンス
 * @return リダクションされたZDD
 *
 * @see build_unreduced_zdd_mp
 * @see build_zdd
 */
template<typename SPEC>
ZDD build_zdd_mp(DDManager& mgr, SPEC& spec) {
    UnreducedZDD unreduced = build_unreduced_zdd_mp(mgr, spec);
    return unreduced.reduce();
}

/**
 * @brief OpenMP並列でTdZdd互換SpecからUnreducedBDDを構築する
 *
 * ZDD版と同様にスレッドローカルストレージとグローバルマージを用いて
 * 並列に状態探索を行う。BDD用のプレースホルダとファイナライズを使用する。
 *
 * @tparam SPEC Spec型（TdZdd Specインターフェースを満たす必要がある）
 * @param mgr 使用するDDManager
 * @param spec Specインスタンス
 * @return 構築されたUnreducedBDD
 *
 * @see build_bdd_mp
 * @see build_unreduced_bdd
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

        // 対応：TdZddレベルL → 変数 (rootLevel - L + 1)
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
 * @brief OpenMP並列でTdZdd互換SpecからリダクションされたBDDを構築する
 *
 * 内部でbuild_unreduced_bdd_mpを呼び出した後、reduce()を適用する。
 *
 * @tparam SPEC Spec型（TdZdd Specインターフェースを満たす必要がある）
 * @param mgr 使用するDDManager
 * @param spec Specインスタンス
 * @return リダクションされたBDD
 *
 * @see build_unreduced_bdd_mp
 * @see build_bdd
 */
template<typename SPEC>
BDD build_bdd_mp(DDManager& mgr, SPEC& spec) {
    UnreducedBDD unreduced = build_unreduced_bdd_mp(mgr, spec);
    return unreduced.reduce();
}

/**
 * @brief OpenMP並列で実行時可変ARITYのVarArity SpecからMVZDDを構築する
 *
 * スレッドローカルストレージとグローバルマージを用いて
 * 並列にフェーズ1の状態探索を行い、フェーズ2でボトムアップに
 * ITEを使用してMVZDDノードを構築する。
 *
 * @tparam SPEC Spec型（getArity()メソッドを持つ必要がある）
 * @param mgr 使用するDDManager
 * @param spec Specインスタンス
 * @return 構築されたMVZDD
 *
 * @see build_mvzdd_va
 * @see build_mvbdd_va_mp
 * @see VarArityDdSpec
 */
template<typename SPEC>
MVZDD build_mvzdd_va_mp(DDManager& mgr, SPEC& spec) {
    int const datasize = spec.datasize();
    int const ARITY = spec.getArity();

    // ルート状態を取得
    std::vector<char> rootState(datasize > 0 ? datasize : 1);
    int rootLevel = spec.get_root(rootState.data());

    // k = ARITYでMVZDDを作成
    MVZDD base_mvzdd = MVZDD::empty(mgr, ARITY);

    if (rootLevel == 0) {
        return base_mvzdd;
    }
    if (rootLevel < 0) {
        return MVZDD::single(mgr, ARITY);
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

        // 次レベルノード用のスレッドローカルストレージ
        int numThreads = get_num_threads();
        std::vector<std::vector<std::vector<char>>> threadLocalStates(numThreads);
        std::vector<std::unordered_map<std::size_t, std::vector<std::size_t>>> threadLocalHash(numThreads);

        // 子ノードを並列処理
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

                        // (tid, localCol)をプレースホルダ参照としてエンコード
                        std::uint64_t encodedRef = (static_cast<std::uint64_t>(tid) << 48) | childCol;
                        childRefs[level][col][b] = {childLevel, encodedRef};
                    }
                }

                if (datasize > 0) {
                    spec.destruct(nodeStates[level][col].data());
                }
            }
        }

        // スレッドローカル結果を次レベルのグローバル状態にマージ
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

            // マッピングテーブルを構築
            std::vector<std::vector<std::size_t>> colMapping(numThreads);
            std::size_t mappingIdx = 0;
            for (int tid = 0; tid < numThreads; ++tid) {
                colMapping[tid].resize(threadLocalStates[tid].size());
                for (std::size_t localCol = 0; localCol < threadLocalStates[tid].size(); ++localCol) {
                    colMapping[tid][localCol] = threadColMapping[mappingIdx++].second;
                }
            }

            // プレースホルダ参照をグローバル列インデックスで更新
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

            bddvar mv = static_cast<bddvar>(rootLevel - level + 1);
            nodeMVZDDs[level][col] = MVZDD::ite(base_mvzdd, mv, children);
        }

        childRefs[level].clear();
        childRefs[level].shrink_to_fit();
    }

    return nodeMVZDDs[rootLevel][0];
}

/**
 * @brief OpenMP並列で実行時可変ARITYのVarArity SpecからMVBDDを構築する
 *
 * MVZDD版と同様にスレッドローカルストレージとグローバルマージを用いて
 * 並列に状態探索を行い、ボトムアップにMVBDDノードを構築する。
 *
 * @tparam SPEC Spec型（getArity()メソッドを持つ必要がある）
 * @param mgr 使用するDDManager
 * @param spec Specインスタンス
 * @return 構築されたMVBDD
 *
 * @see build_mvbdd_va
 * @see build_mvzdd_va_mp
 * @see VarArityDdSpec
 */
template<typename SPEC>
MVBDD build_mvbdd_va_mp(DDManager& mgr, SPEC& spec) {
    int const datasize = spec.datasize();
    int const ARITY = spec.getArity();

    // ルート状態を取得
    std::vector<char> rootState(datasize > 0 ? datasize : 1);
    int rootLevel = spec.get_root(rootState.data());

    // k = ARITYでMVBDDを作成
    MVBDD base_mvbdd = MVBDD::zero(mgr, ARITY);

    if (rootLevel == 0) {
        return base_mvbdd;
    }
    if (rootLevel < 0) {
        return MVBDD::one(mgr, ARITY);
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
