/**
 * @file zdd_index.hpp
 * @brief SAPPOROBDD 2.0 - ZDDインデックスデータ構造
 * @author SAPPOROBDD Team
 * @copyright MIT License
 *
 * ZDDの各ノードから1終端までの経路数を格納するインデックス構造。
 * 効率的な辞書順アクセス、サンプリング、最適化を実現する。
 */

#ifndef SBDD2_ZDD_INDEX_HPP
#define SBDD2_ZDD_INDEX_HPP

#include "types.hpp"
#include <vector>
#include <unordered_map>
#include <cstdint>
#include <memory>
#include <mutex>

#if defined(SBDD2_HAS_GMP) || defined(SBDD2_HAS_BIGINT)
#include "exact_int.hpp"
#endif

namespace sbdd2 {

/**
 * @brief Arcのハッシュ関数オブジェクト
 *
 * unordered_mapなどのハッシュコンテナでArcをキーとして使用するためのハッシュ関数。
 *
 * @see ArcEqual, ZDDIndexData
 */
struct ArcHash {
    /**
     * @brief Arcのハッシュ値を計算する
     * @param arc ハッシュ対象のArc
     * @return 計算されたハッシュ値
     */
    std::size_t operator()(const Arc& arc) const {
        return std::hash<std::uint64_t>()(arc.data);
    }
};

/**
 * @brief Arcの等価比較関数オブジェクト
 *
 * unordered_mapなどのハッシュコンテナでArcをキーとして使用するための等価比較関数。
 *
 * @see ArcHash, ZDDIndexData
 */
struct ArcEqual {
    /**
     * @brief 二つのArcが等しいかどうかを判定する
     * @param a 比較対象のArc
     * @param b 比較対象のArc
     * @return 二つのArcが等しければtrue、そうでなければfalse
     */
    bool operator()(const Arc& a, const Arc& b) const {
        return a.data == b.data;
    }
};

/**
 * @brief ZDDインデックスデータ（double版）
 *
 * 各ノードから1終端までの経路数をdoubleで格納するデータ構造。
 * 2^53までの精度で正確な値を保持できる。
 * 辞書順アクセスやランダムサンプリングに使用する。
 *
 * @see ZDDExactIndexData, DictIterator, RandomIterator
 */
struct ZDDIndexData {
    /// @brief レベルごとのノードリスト
    /// level_nodes[level] でそのレベルの全ノードを取得できる。
    /// level 0 は未使用、level 1 から height まで使用する。
    std::vector<std::vector<Arc>> level_nodes;

    /// @brief ノードからレベル内インデックスへのマッピング
    std::unordered_map<Arc, std::size_t, ArcHash, ArcEqual> node_to_idx;

    /// @brief ノードから1終端までの経路数へのマッピング
    std::unordered_map<Arc, double, ArcHash, ArcEqual> count_cache;

    /// @brief ZDDの高さ（ルートノードのレベル = 最高レベル）
    int height;

    /// @brief 最低レベル（終端に最も近い非終端レベル）
    int min_level;

    /**
     * @brief デフォルトコンストラクタ
     *
     * height と min_level を0に初期化する。
     */
    ZDDIndexData() : height(0), min_level(0) {}
};

#if defined(SBDD2_HAS_GMP) || defined(SBDD2_HAS_BIGINT)
/**
 * @brief ZDDインデックスデータ（厳密整数版）
 *
 * 各ノードから1終端までの経路数を厳密整数型で格納する
 * データ構造。2^53を超える大きな濃度のZDDでも正確な値を保持できる。
 *
 * @note このクラスはSBDD2_HAS_GMPまたはSBDD2_HAS_BIGINTが定義されている場合のみ利用可能。
 * @see ZDDIndexData
 */
struct ZDDExactIndexData {
    /// @brief レベルごとのノードリスト
    /// level_nodes[level] でそのレベルの全ノードを取得できる。
    std::vector<std::vector<Arc>> level_nodes;

    /// @brief ノードからレベル内インデックスへのマッピング
    std::unordered_map<Arc, std::size_t, ArcHash, ArcEqual> node_to_idx;

    /// @brief ノードから1終端までの経路数へのマッピング（厳密整数型）
    std::unordered_map<Arc, exact_int_t, ArcHash, ArcEqual> count_cache;

    /// @brief ZDDの高さ（ルートノードのレベル = 最高レベル）
    int height;

    /// @brief 最低レベル（終端に最も近い非終端レベル）
    int min_level;

    /**
     * @brief デフォルトコンストラクタ
     *
     * height と min_level を0に初期化する。
     */
    ZDDExactIndexData() : height(0), min_level(0) {}
};
#endif

} // namespace sbdd2

#endif // SBDD2_ZDD_INDEX_HPP
