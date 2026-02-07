/**
 * @file unreduced_bdd.hpp
 * @brief UnreducedBDD（非簡約BDD）クラスの定義。簡約規則を適用しない BDD を表現する。
 *
 * トップダウン構築アルゴリズム向けに、重複ノードや冗長ノードを
 * 許容する非簡約 BDD を提供する。構築後に reduce() で簡約 BDD に変換できる。
 *
 * @see BDD
 * @see UnreducedZDD
 * @see QDD
 * @see DDBase
 */

// SAPPOROBDD 2.0 - UnreducedBDD class
// MIT License

#ifndef SBDD2_UNREDUCED_BDD_HPP
#define SBDD2_UNREDUCED_BDD_HPP

#include "dd_base.hpp"
#include <vector>

namespace sbdd2 {

// Forward declarations
class BDD;
class QDD;

/**
 * @brief 非簡約BDD（UnreducedBDD）クラス。簡約規則を適用しない BDD を表現する。
 *
 * 通常の BDD とは異なり、重複ノードの存在や冗長ノードの削除を行わない。
 * トップダウン構築アルゴリズムで使用され、構築後に reduce() を呼ぶことで
 * 簡約された BDD に変換できる。正規形ではないため、比較演算子は削除されている。
 *
 * @see BDD
 * @see UnreducedZDD
 * @see QDD
 * @see DDBase
 * @see DDManager
 */
class UnreducedBDD : public DDBase {
    friend class DDManager;
    friend class BDD;
    friend class QDD;

public:
    /**
     * @brief デフォルトコンストラクタ。無効な状態で初期化する。
     */
    UnreducedBDD() : DDBase() {}

    /**
     * @brief マネージャとアークを指定して構築するコンストラクタ（内部使用）
     * @param mgr DDManager へのポインタ
     * @param a アーク
     */
    UnreducedBDD(DDManager* mgr, Arc a) : DDBase(mgr, a) {}

    /** @brief コピーコンストラクタ */
    UnreducedBDD(const UnreducedBDD&) = default;
    /** @brief ムーブコンストラクタ */
    UnreducedBDD(UnreducedBDD&&) noexcept = default;
    /** @brief コピー代入演算子 */
    UnreducedBDD& operator=(const UnreducedBDD&) = default;
    /** @brief ムーブ代入演算子 */
    UnreducedBDD& operator=(UnreducedBDD&&) noexcept = default;

    /**
     * @brief 等値比較演算子（削除済み）。非簡約 BDD は正規形でないため比較不可。
     */
    bool operator==(const UnreducedBDD&) const = delete;

    /**
     * @brief 非等値比較演算子（削除済み）。非簡約 BDD は正規形でないため比較不可。
     */
    bool operator!=(const UnreducedBDD&) const = delete;

    /**
     * @brief 小なり比較演算子（削除済み）。非簡約 BDD は正規形でないため比較不可。
     */
    bool operator<(const UnreducedBDD&) const = delete;

    /**
     * @brief 定数 0（偽）を表す UnreducedBDD を生成する
     * @param mgr 使用する DDManager への参照
     * @return 定数 0 を表す UnreducedBDD
     */
    static UnreducedBDD zero(DDManager& mgr);

    /**
     * @brief 定数 1（真）を表す UnreducedBDD を生成する
     * @param mgr 使用する DDManager への参照
     * @return 定数 1 を表す UnreducedBDD
     */
    static UnreducedBDD one(DDManager& mgr);

    /**
     * @brief 簡約規則を適用せずにノードを生成する（冗長ノードを許容）
     * @param mgr 使用する DDManager への参照
     * @param var 変数番号
     * @param low 0-枝の子ノード
     * @param high 1-枝の子ノード
     * @return 生成された UnreducedBDD ノード
     */
    static UnreducedBDD node(DDManager& mgr, bddvar var,
                             const UnreducedBDD& low, const UnreducedBDD& high);

    /**
     * @brief 0-枝（low）の子ノードを取得する
     * @return 0-枝の子ノードを表す UnreducedBDD
     */
    UnreducedBDD low() const;

    /**
     * @brief 1-枝（high）の子ノードを取得する
     * @return 1-枝の子ノードを表す UnreducedBDD
     */
    UnreducedBDD high() const;

    /**
     * @brief 否定を計算する
     * @return 否定された UnreducedBDD
     */
    UnreducedBDD operator~() const;

    /**
     * @brief 簡約規則を適用して簡約 BDD に変換する
     * @return 簡約された BDD
     * @see BDD
     */
    BDD reduce() const;

    /**
     * @brief ノード共有のみを適用して QDD に変換する（簡約は行わない）
     * @return 変換された QDD
     * @see QDD
     */
    QDD to_qdd() const;

    /**
     * @brief 既に簡約済みかどうかを判定する
     * @return 簡約済みの場合 true
     */
    bool is_reduced() const;
};

} // namespace sbdd2

#endif // SBDD2_UNREDUCED_BDD_HPP
