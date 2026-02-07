/**
 * @file Sbdd2BuilderDFS.hpp
 * @brief TdZddスタイルのDFS（深さ優先探索）ベースのZDD/BDDビルダー
 *
 * TdZdd互換のSpecからZDD/BDDを深さ優先探索とメモ化キャッシュを用いて構築する。
 * BFS（幅優先）ベースのビルダーに比べ、多くの問題でメモリ効率が良い。
 *
 * @see Sbdd2Builder.hpp
 * @see Sbdd2BuilderMP.hpp
 * @see DdSpec.hpp
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
 * @brief DFS構築用のメモ化キャッシュ
 *
 * 状態(state) -> Arc の対応をキャッシュする。
 * (level, state_hash) をキーとし、状態の衝突はspec.equal_to()で解決する。
 *
 * @tparam SPEC Spec型
 *
 * @see build_zdd_dfs
 * @see build_bdd_dfs
 */
template<typename SPEC>
class DFSCache {
public:
    /**
     * @brief キャッシュエントリ
     *
     * 状態データ・計算結果Arc・有効フラグを保持する。
     */
    struct Entry {
        std::vector<char> state;  ///< 状態データ
        Arc result;               ///< 計算結果のArc
        bool valid;               ///< エントリが有効かどうか

        /**
         * @brief デフォルトコンストラクタ（無効状態で初期化）
         */
        Entry() : valid(false) {}
    };

private:
    int datasize_;  ///< 状態データのバイトサイズ
    std::unordered_map<std::size_t, std::vector<Entry>> table_;  ///< ハッシュテーブル

    /**
     * @brief レベルと状態ハッシュからキーを生成する
     * @param level レベル
     * @param state_hash 状態のハッシュ値
     * @return テーブルキー
     */
    std::size_t make_key(int level, std::size_t state_hash) const {
        return static_cast<std::size_t>(level) ^ (state_hash * 314159257);
    }

public:
    /**
     * @brief コンストラクタ
     * @param datasize 状態データのバイトサイズ
     */
    explicit DFSCache(int datasize) : datasize_(datasize) {}

    /**
     * @brief 指定状態のキャッシュ結果を検索する
     *
     * ハッシュが一致するエントリに対してspec.equal_to()で状態を比較し、
     * 一致するエントリがあればそのArc結果を返す。
     *
     * @param spec Spec（equal_to比較用）
     * @param level 現在のレベル
     * @param state 現在の状態
     * @param state_hash 状態のハッシュ値
     * @param[out] result 見つかった場合のキャッシュされたArc
     * @return 見つかった場合true、見つからなかった場合false
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
     * @brief キャッシュに結果を挿入する
     *
     * @param spec Spec（get_copyによる状態コピー用）
     * @param level 現在のレベル
     * @param state 現在の状態
     * @param state_hash 状態のハッシュ値
     * @param result キャッシュするArc
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
     * @brief キャッシュをクリアする
     */
    void clear() {
        table_.clear();
    }

    /**
     * @brief キャッシュされたエントリ数を取得する
     * @return エントリ数
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
 * @brief DFS方式によるZDD構築の内部再帰関数
 *
 * 0子・1子を再帰的に構築し、ZDDリダクションルール
 * （1辺が0終端なら0辺を返す）を適用してノードを作成する。
 * メモ化キャッシュにより同一状態の再計算を回避する。
 *
 * @tparam SPEC Spec型
 * @param mgr DDManager
 * @param spec Specインスタンス
 * @param rootLevel ルートレベル（変数マッピング用）
 * @param level 現在のレベル
 * @param state 現在の状態（変更される場合がある）
 * @param cache メモ化用DFSキャッシュ
 * @param datasize 状態データのバイトサイズ
 * @return この状態に対するZDDを表すArc
 */
template<typename SPEC>
Arc dfs_build_zdd_impl(DDManager& mgr, SPEC& spec, int rootLevel, int level,
                       void* state, DFSCache<SPEC>& cache, int datasize) {
    // 終端ケース
    if (level == 0) {
        return ARC_TERMINAL_0;
    }
    if (level < 0) {
        return ARC_TERMINAL_1;
    }

    // キャッシュを確認
    std::size_t state_hash = spec.hash_code(state, level);
    Arc cached_result;
    if (cache.lookup(spec, level, state, state_hash, cached_result)) {
        return cached_result;
    }

    // 0子を処理
    std::vector<char> state0(datasize > 0 ? datasize : 1);
    if (datasize > 0) {
        spec.get_copy(state0.data(), state);
    }
    int level0 = spec.get_child(state0.data(), level, 0);
    Arc arc0 = dfs_build_zdd_impl(mgr, spec, rootLevel, level0, state0.data(), cache, datasize);
    if (datasize > 0) {
        spec.destruct(state0.data());
    }

    // 1子を処理
    std::vector<char> state1(datasize > 0 ? datasize : 1);
    if (datasize > 0) {
        spec.get_copy(state1.data(), state);
    }
    int level1 = spec.get_child(state1.data(), level, 1);
    Arc arc1 = dfs_build_zdd_impl(mgr, spec, rootLevel, level1, state1.data(), cache, datasize);
    if (datasize > 0) {
        spec.destruct(state1.data());
    }

    // ZDDリダクションルール：1辺が0終端なら0辺を返す
    Arc result;
    if (arc1 == ARC_TERMINAL_0) {
        result = arc0;
    } else {
        // ノードを作成
        // TdZddレベルL → SAPPOROBDD2レベルL（同一セマンティクス：高レベル = ルートに近い）
        bddvar var = mgr.var_of_lev(level);
        result = mgr.get_or_create_node_zdd(var, arc0, arc1, true);
    }

    // 結果をキャッシュ
    cache.insert(spec, level, state, state_hash, result);

    return result;
}

/**
 * @brief DFS方式によるBDD構築の内部再帰関数
 *
 * 0子・1子を再帰的に構築し、BDDリダクションルール
 * （両方の子が同一なら子をそのまま返す）を適用してノードを作成する。
 * メモ化キャッシュにより同一状態の再計算を回避する。
 *
 * @tparam SPEC Spec型
 * @param mgr DDManager
 * @param spec Specインスタンス
 * @param rootLevel ルートレベル（変数マッピング用）
 * @param level 現在のレベル
 * @param state 現在の状態（変更される場合がある）
 * @param cache メモ化用DFSキャッシュ
 * @param datasize 状態データのバイトサイズ
 * @return この状態に対するBDDを表すArc
 */
template<typename SPEC>
Arc dfs_build_bdd_impl(DDManager& mgr, SPEC& spec, int rootLevel, int level,
                       void* state, DFSCache<SPEC>& cache, int datasize) {
    // 終端ケース
    if (level == 0) {
        return ARC_TERMINAL_0;
    }
    if (level < 0) {
        return ARC_TERMINAL_1;
    }

    // キャッシュを確認
    std::size_t state_hash = spec.hash_code(state, level);
    Arc cached_result;
    if (cache.lookup(spec, level, state, state_hash, cached_result)) {
        return cached_result;
    }

    // 0子を処理
    std::vector<char> state0(datasize > 0 ? datasize : 1);
    if (datasize > 0) {
        spec.get_copy(state0.data(), state);
    }
    int level0 = spec.get_child(state0.data(), level, 0);
    Arc arc0 = dfs_build_bdd_impl(mgr, spec, rootLevel, level0, state0.data(), cache, datasize);
    if (datasize > 0) {
        spec.destruct(state0.data());
    }

    // 1子を処理
    std::vector<char> state1(datasize > 0 ? datasize : 1);
    if (datasize > 0) {
        spec.get_copy(state1.data(), state);
    }
    int level1 = spec.get_child(state1.data(), level, 1);
    Arc arc1 = dfs_build_bdd_impl(mgr, spec, rootLevel, level1, state1.data(), cache, datasize);
    if (datasize > 0) {
        spec.destruct(state1.data());
    }

    // BDDリダクションルール：両方の子が同一なら子を返す
    Arc result;
    if (arc0 == arc1) {
        result = arc0;
    } else {
        // ノードを作成
        // TdZddレベルL → SAPPOROBDD2レベルL（同一セマンティクス：高レベル = ルートに近い）
        bddvar var = mgr.var_of_lev(level);
        result = mgr.get_or_create_node_bdd(var, arc0, arc1, true);
    }

    // 結果をキャッシュ
    cache.insert(spec, level, state, state_hash, result);

    return result;
}

} // namespace detail

/**
 * @brief DFS方式でTdZdd互換SpecからリダクションされたZDDを構築する
 *
 * メモ化付き深さ優先探索により、BFS方式に比べ多くの問題でメモリ効率が良い。
 * 構築中にZDDリダクションルールを適用するため、結果は直接リダクション済みZDDとなる。
 *
 * @tparam SPEC Spec型（TdZdd Specインターフェースを満たす必要がある）
 * @param mgr 使用するDDManager
 * @param spec Specインスタンス
 * @return リダクションされたZDD
 *
 * @see build_zdd
 * @see build_bdd_dfs
 * @see DFSCache
 */
template<typename SPEC>
ZDD build_zdd_dfs(DDManager& mgr, SPEC& spec) {
    int const datasize = spec.datasize();

    // ルート状態を取得
    std::vector<char> rootState(datasize > 0 ? datasize : 1);
    int rootLevel = spec.get_root(rootState.data());

    // 自明なケースを処理
    if (rootLevel == 0) {
        return ZDD::empty(mgr);
    }
    if (rootLevel < 0) {
        return ZDD::single(mgr);
    }

    // 変数の存在を確保
    while (static_cast<int>(mgr.var_count()) < rootLevel) {
        mgr.new_var();
    }

    // キャッシュを作成して構築
    DFSCache<SPEC> cache(datasize);
    Arc result = detail::dfs_build_zdd_impl(mgr, spec, rootLevel, rootLevel,
                                            rootState.data(), cache, datasize);

    // ルート状態をクリーンアップ
    if (datasize > 0) {
        spec.destruct(rootState.data());
    }

    return ZDD(&mgr, result);
}

/**
 * @brief DFS方式でTdZdd互換SpecからリダクションされたBDDを構築する
 *
 * メモ化付き深さ優先探索により構築する。
 * 構築中にBDDリダクションルールを適用するため、結果は直接リダクション済みBDDとなる。
 *
 * @tparam SPEC Spec型（TdZdd Specインターフェースを満たす必要がある）
 * @param mgr 使用するDDManager
 * @param spec Specインスタンス
 * @return リダクションされたBDD
 *
 * @see build_bdd
 * @see build_zdd_dfs
 * @see DFSCache
 */
template<typename SPEC>
BDD build_bdd_dfs(DDManager& mgr, SPEC& spec) {
    int const datasize = spec.datasize();

    // ルート状態を取得
    std::vector<char> rootState(datasize > 0 ? datasize : 1);
    int rootLevel = spec.get_root(rootState.data());

    // 自明なケースを処理
    if (rootLevel == 0) {
        return BDD::zero(mgr);
    }
    if (rootLevel < 0) {
        return BDD::one(mgr);
    }

    // 変数の存在を確保
    while (static_cast<int>(mgr.var_count()) < rootLevel) {
        mgr.new_var();
    }

    // キャッシュを作成して構築
    DFSCache<SPEC> cache(datasize);
    Arc result = detail::dfs_build_bdd_impl(mgr, spec, rootLevel, rootLevel,
                                            rootState.data(), cache, datasize);

    // ルート状態をクリーンアップ
    if (datasize > 0) {
        spec.destruct(rootState.data());
    }

    return BDD(&mgr, result);
}

} // namespace tdzdd
} // namespace sbdd2
