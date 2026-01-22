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

#ifdef SBDD2_HAS_GMP
#include <gmpxx.h>
#endif

namespace sbdd2 {

/**
 * @brief Arcのハッシュ関数
 */
struct ArcHash {
    std::size_t operator()(const Arc& arc) const {
        return std::hash<std::uint64_t>()(arc.data);
    }
};

/**
 * @brief Arcの等価比較関数
 */
struct ArcEqual {
    bool operator()(const Arc& a, const Arc& b) const {
        return a.data == b.data;
    }
};

/**
 * @brief ZDDインデックスデータ（double版）
 *
 * 各ノードから1終端までの経路数をdoubleで格納する。
 * 2^53までの精度で正確な値を保持できる。
 */
struct ZDDIndexData {
    /// レベルごとのノードリスト（level_nodes[level] = そのレベルの全ノード）
    /// level 0 は未使用、level 1 から height まで使用
    std::vector<std::vector<Arc>> level_nodes;

    /// ノードからレベル内インデックスへのマッピング
    std::unordered_map<Arc, std::size_t, ArcHash, ArcEqual> node_to_idx;

    /// ノードから経路数へのマッピング
    std::unordered_map<Arc, double, ArcHash, ArcEqual> count_cache;

    /// ZDDの高さ（ルートノードのレベル）
    int height;

    /// デフォルトコンストラクタ
    ZDDIndexData() : height(0) {}
};

#ifdef SBDD2_HAS_GMP
/**
 * @brief ZDDインデックスデータ（GMP版）
 *
 * 各ノードから1終端までの経路数をmpz_classで格納する。
 * 任意精度の整数値を扱える。
 */
struct ZDDExactIndexData {
    /// レベルごとのノードリスト
    std::vector<std::vector<Arc>> level_nodes;

    /// ノードからレベル内インデックスへのマッピング
    std::unordered_map<Arc, std::size_t, ArcHash, ArcEqual> node_to_idx;

    /// ノードから経路数へのマッピング（GMP）
    std::unordered_map<Arc, mpz_class, ArcHash, ArcEqual> count_cache;

    /// ZDDの高さ
    int height;

    /// デフォルトコンストラクタ
    ZDDExactIndexData() : height(0) {}
};
#endif

} // namespace sbdd2

#endif // SBDD2_ZDD_INDEX_HPP
