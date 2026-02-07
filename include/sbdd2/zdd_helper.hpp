/**
 * @file zdd_helper.hpp
 * @brief SAPPOROBDD 2.0 - ZDDヘルパー関数
 * @author SAPPOROBDD Team
 * @copyright MIT License
 *
 * ZDDの便利な操作関数を提供するヘルパーライブラリ。
 */

#ifndef SBDD2_ZDD_HELPER_HPP
#define SBDD2_ZDD_HELPER_HPP

#include "zdd.hpp"
#include "dd_manager.hpp"
#include <vector>
#include <set>
#include <random>
#include <algorithm>

namespace sbdd2 {

// ============== Power Set Generation ==============

/**
 * @brief 変数集合のべき集合を生成する
 *
 * 指定された変数集合の全ての部分集合を表すZDDを構築する。
 *
 * @tparam Container 変数を格納するコンテナ型（begin/endをサポートする型）
 * @param mgr DDマネージャー
 * @param variables 変数のコンテナ
 * @return 全ての部分集合を含むZDD
 * @see get_power_set_with_card
 */
template<typename Container>
ZDD get_power_set(DDManager& mgr, const Container& variables) {
    ZDD result = ZDD::single(mgr);  // Start with empty set
    for (const auto& v : variables) {
        // For each variable, add both with and without it
        result = result + result.change(static_cast<bddvar>(v));
    }
    return result;
}

/**
 * @brief 変数1からnまでのべき集合を生成する
 *
 * 変数{1, 2, ..., n}の全ての部分集合を表すZDDを構築する。
 *
 * @param mgr DDマネージャー
 * @param n 変数の数（1以上）
 * @return 全ての部分集合を含むZDD（2^n個の集合）
 * @see get_power_set_with_card
 */
inline ZDD get_power_set(DDManager& mgr, int n) {
    std::vector<bddvar> vars;
    for (int i = 1; i <= n; ++i) {
        vars.push_back(static_cast<bddvar>(i));
    }
    return get_power_set(mgr, vars);
}

/**
 * @brief 指定濃度のべき集合を生成する
 *
 * 指定された変数集合から、要素数がちょうどkの部分集合のみを表すZDDを
 * 動的計画法で構築する。
 *
 * @tparam Container 変数を格納するコンテナ型（begin/endをサポートする型）
 * @param mgr DDマネージャー
 * @param variables 変数のコンテナ
 * @param k 濃度（部分集合の要素数）
 * @return 濃度がちょうどkの部分集合を含むZDD
 * @see get_power_set
 */
template<typename Container>
ZDD get_power_set_with_card(DDManager& mgr, const Container& variables, int k) {
    std::vector<bddvar> vars(variables.begin(), variables.end());
    int n = static_cast<int>(vars.size());

    if (k < 0 || k > n) {
        return ZDD::empty(mgr);
    }
    if (k == 0) {
        return ZDD::single(mgr);  // Only empty set
    }
    if (k == n) {
        // Only full set
        ZDD result = ZDD::single(mgr);
        for (bddvar v : vars) {
            result = result.change(v);
        }
        return result;
    }

    // Dynamic programming approach
    // dp[i][j] = ZDD of subsets of vars[0..i-1] with exactly j elements
    std::vector<ZDD> prev(k + 1, ZDD::empty(mgr));
    std::vector<ZDD> curr(k + 1, ZDD::empty(mgr));

    prev[0] = ZDD::single(mgr);

    for (int i = 0; i < n; ++i) {
        bddvar v = vars[i];
        for (int j = 0; j <= k; ++j) {
            curr[j] = prev[j];  // Don't include v
            if (j > 0) {
                curr[j] = curr[j] + prev[j - 1].change(v);  // Include v
            }
        }
        std::swap(prev, curr);
        for (int j = 0; j <= k; ++j) {
            curr[j] = ZDD::empty(mgr);
        }
    }

    return prev[k];
}

/**
 * @brief 変数1からnまでで濃度kのべき集合を生成する
 *
 * 変数{1, 2, ..., n}から要素数がちょうどkの部分集合のみを表すZDDを構築する。
 *
 * @param mgr DDマネージャー
 * @param n 変数の数（1以上）
 * @param k 濃度（部分集合の要素数、0以上n以下）
 * @return 濃度がちょうどkの部分集合を含むZDD（C(n,k)個の集合）
 * @see get_power_set
 */
inline ZDD get_power_set_with_card(DDManager& mgr, int n, int k) {
    std::vector<bddvar> vars;
    for (int i = 1; i <= n; ++i) {
        vars.push_back(static_cast<bddvar>(i));
    }
    return get_power_set_with_card(mgr, vars, k);
}

// ============== Single Set Generation ==============

/**
 * @brief 単一の集合を含むZDDを生成する
 *
 * 指定された変数の集合のみを含むZDDを構築する。
 * 重複する変数は自動的に除去される。
 *
 * @tparam Container 変数を格納するコンテナ型（begin/endをサポートする型）
 * @param mgr DDマネージャー
 * @param variables 集合に含める変数
 * @return 指定した変数を含む単一集合のZDD
 * @see get_power_set
 */
template<typename Container>
ZDD get_single_set(DDManager& mgr, const Container& variables) {
    // Remove duplicates and sort
    std::set<bddvar> unique_vars;
    for (const auto& v : variables) {
        unique_vars.insert(static_cast<bddvar>(v));
    }

    ZDD result = ZDD::single(mgr);  // Start with empty set
    for (bddvar v : unique_vars) {
        result = result.change(v);
    }
    return result;
}

// ============== Don't Care ==============

/**
 * @brief ドントケア条件を適用したZDDを作成する
 *
 * 指定された変数について、その変数が含まれる集合と含まれない集合の
 * 両方をZDDに追加する。各変数vに対して f := f + f.change(v) を適用する。
 *
 * @tparam Container 変数を格納するコンテナ型（begin/endをサポートする型）
 * @param f 元のZDD
 * @param variables ドントケアにする変数のコンテナ
 * @return 指定変数をドントケアにしたZDD
 */
template<typename Container>
ZDD make_dont_care(const ZDD& f, const Container& variables) {
    if (!f.manager()) {
        return f;
    }

    ZDD result = f;
    for (const auto& v : variables) {
        bddvar var = static_cast<bddvar>(v);
        // Add both with and without the variable
        result = result + result.change(var);
    }
    return result;
}

// ============== Membership Test ==============

/**
 * @brief 指定した集合がZDDに含まれるかを判定する
 *
 * ZDDを根から辿り、指定された変数集合がZDDの表す集合族に
 * 含まれているかどうかを判定する。
 *
 * @tparam Container 変数を格納するコンテナ型（begin/endをサポートする型）
 * @param f 判定対象のZDD
 * @param variables チェックする変数集合
 * @return variablesがfの表す集合族に含まれていればtrue、そうでなければfalse
 */
template<typename Container>
bool is_member(const ZDD& f, const Container& variables) {
    if (!f.manager()) {
        return false;
    }
    if (f.is_zero()) {
        return false;
    }

    // Collect and sort variables
    std::set<bddvar> target_vars;
    for (const auto& v : variables) {
        target_vars.insert(static_cast<bddvar>(v));
    }

    // Traverse the ZDD
    Arc current = f.arc();
    DDManager* mgr = f.manager();

    while (!current.is_constant()) {
        const DDNode& node = mgr->node_at(current.index());
        bddvar node_var = node.var();

        if (target_vars.count(node_var)) {
            // Variable should be in the set, take high branch
            current = node.arc1();
            target_vars.erase(node_var);
        } else {
            // Variable should not be in the set, take low branch
            current = node.arc0();
        }
    }

    // Must reach terminal 1 and have used all target variables
    return current == ARC_TERMINAL_1 && target_vars.empty();
}

// ============== Weight Operations ==============

/**
 * @brief 総重みが指定範囲内の集合のみを抽出する
 *
 * 各集合の総重み（各変数の重みの合計）が[lower_bound, upper_bound]の
 * 閉区間に含まれる集合のみを残すフィルタ。
 *
 * @param f フィルタ対象のZDD
 * @param lower_bound 重みの下限（この値を含む）
 * @param upper_bound 重みの上限（この値を含む）
 * @param weights 各変数の重み（インデックス0は変数1の重み）
 * @return 総重みが[lower_bound, upper_bound]の範囲にある集合のみを含むZDD
 * @see weight_le, weight_ge, weight_eq, WeightIterator
 */
ZDD weight_range(const ZDD& f, long long lower_bound, long long upper_bound,
                 const std::vector<long long>& weights);

/**
 * @brief 総重みが指定値以下の集合のみを抽出する
 * @param f フィルタ対象のZDD
 * @param bound 重みの上限（この値を含む）
 * @param weights 各変数の重み（インデックス0は変数1の重み）
 * @return 総重みがbound以下の集合のみを含むZDD
 * @see weight_lt, weight_range
 */
ZDD weight_le(const ZDD& f, long long bound, const std::vector<long long>& weights);

/**
 * @brief 総重みが指定値未満の集合のみを抽出する
 * @param f フィルタ対象のZDD
 * @param bound 重みの上限（この値を含まない）
 * @param weights 各変数の重み（インデックス0は変数1の重み）
 * @return 総重みがbound未満の集合のみを含むZDD
 * @see weight_le, weight_range
 */
ZDD weight_lt(const ZDD& f, long long bound, const std::vector<long long>& weights);

/**
 * @brief 総重みが指定値以上の集合のみを抽出する
 * @param f フィルタ対象のZDD
 * @param bound 重みの下限（この値を含む）
 * @param weights 各変数の重み（インデックス0は変数1の重み）
 * @return 総重みがbound以上の集合のみを含むZDD
 * @see weight_gt, weight_range
 */
ZDD weight_ge(const ZDD& f, long long bound, const std::vector<long long>& weights);

/**
 * @brief 総重みが指定値より大きい集合のみを抽出する
 * @param f フィルタ対象のZDD
 * @param bound 重みの下限（この値を含まない）
 * @param weights 各変数の重み（インデックス0は変数1の重み）
 * @return 総重みがboundより大きい集合のみを含むZDD
 * @see weight_ge, weight_range
 */
ZDD weight_gt(const ZDD& f, long long bound, const std::vector<long long>& weights);

/**
 * @brief 総重みが指定値と等しい集合のみを抽出する
 * @param f フィルタ対象のZDD
 * @param bound 目標の重み
 * @param weights 各変数の重み（インデックス0は変数1の重み）
 * @return 総重みがちょうどboundの集合のみを含むZDD
 * @see weight_ne, weight_range
 */
ZDD weight_eq(const ZDD& f, long long bound, const std::vector<long long>& weights);

/**
 * @brief 総重みが指定値と異なる集合のみを抽出する
 * @param f フィルタ対象のZDD
 * @param bound 除外する重み
 * @param weights 各変数の重み（インデックス0は変数1の重み）
 * @return 総重みがbound以外の集合のみを含むZDD
 * @see weight_eq, weight_range
 */
ZDD weight_ne(const ZDD& f, long long bound, const std::vector<long long>& weights);

// ============== Random Generation ==============

/**
 * @brief 均一ランダムなZDDを生成する
 *
 * 再帰的にランダムなZDDを構築する。各レベルで乱数に基づいて
 * ノードの構成を決定する。
 *
 * @tparam RNG 乱数生成器型（std::mt19937など、UniformRandomBitGenerator要件を満たす型）
 * @param mgr DDマネージャー
 * @param level 変数の数（ZDDの最大レベル）
 * @param rng 乱数生成器への参照
 * @return ランダムに生成されたZDD
 * @see get_random_zdd_with_card, RandomIterator
 */
template<typename RNG>
ZDD get_uniformly_random_zdd(DDManager& mgr, int level, RNG& rng) {
    if (level <= 0) {
        std::uniform_int_distribution<int> dist(0, 1);
        return dist(rng) ? ZDD::single(mgr) : ZDD::empty(mgr);
    }

    std::uniform_int_distribution<int> dist(0, 1);

    // Recursively build random ZDD
    ZDD low = get_uniformly_random_zdd(mgr, level - 1, rng);
    ZDD high = get_uniformly_random_zdd(mgr, level - 1, rng);

    // Randomly decide to include this level or merge
    if (dist(rng)) {
        // Include this variable
        if (high.is_zero()) {
            return low;  // ZDD reduction: skip if high is 0
        }
        Arc result = mgr.get_or_create_node_zdd(static_cast<bddvar>(level), low.arc(), high.arc(), true);
        return ZDD(&mgr, result);
    } else {
        // Merge (union of low and high)
        return low + high;
    }
}

/**
 * @brief 指定された濃度のランダムZDDを生成する
 *
 * ランダムな集合を繰り返し生成・追加し、ZDDの濃度（含まれる集合の数）が
 * 指定値に達するまで構築する。
 *
 * @tparam RNG 乱数生成器型（std::mt19937など、UniformRandomBitGenerator要件を満たす型）
 * @param mgr DDマネージャー
 * @param level 変数の数（ZDDの最大レベル）
 * @param card 目標濃度（ZDDに含まれる集合の数）
 * @param rng 乱数生成器への参照
 * @return 指定濃度のランダムZDD
 * @see get_uniformly_random_zdd, RandomIterator
 */
template<typename RNG>
ZDD get_random_zdd_with_card(DDManager& mgr, int level, long long card, RNG& rng) {
    if (card <= 0) {
        return ZDD::empty(mgr);
    }
    if (level <= 0) {
        return ZDD::single(mgr);
    }

    ZDD result = ZDD::empty(mgr);
    std::uniform_int_distribution<int> var_dist(1, level);

    while (static_cast<long long>(result.card()) < card) {
        // Generate a random set
        ZDD new_set = ZDD::single(mgr);
        for (int i = 1; i <= level; ++i) {
            if (var_dist(rng) <= level / 2) {
                new_set = new_set.change(static_cast<bddvar>(i));
            }
        }
        result = result + new_set;
    }

    return result;
}

} // namespace sbdd2

#endif // SBDD2_ZDD_HELPER_HPP
