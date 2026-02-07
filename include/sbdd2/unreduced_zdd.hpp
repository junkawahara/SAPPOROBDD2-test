/**
 * @file unreduced_zdd.hpp
 * @brief UnreducedZDD（非簡約ZDD）クラスの定義。ZDD 簡約規則を適用しない ZDD を表現する。
 *
 * トップダウン構築アルゴリズム向けに、重複ノードや冗長ノードを
 * 許容する非簡約 ZDD を提供する。構築後に reduce() で簡約 ZDD に変換できる。
 *
 * @see ZDD
 * @see UnreducedBDD
 * @see QDD
 * @see DDBase
 */

// SAPPOROBDD 2.0 - UnreducedZDD class
// MIT License

#ifndef SBDD2_UNREDUCED_ZDD_HPP
#define SBDD2_UNREDUCED_ZDD_HPP

#include "dd_base.hpp"
#include <vector>

namespace sbdd2 {

// Forward declarations
class ZDD;
class QDD;

/**
 * @brief 非簡約ZDD（UnreducedZDD）クラス。ZDD 簡約規則を適用しない ZDD を表現する。
 *
 * 通常の ZDD とは異なり、重複ノードの存在やゼロ抑制規則の適用を行わない。
 * トップダウン構築アルゴリズムで使用され、構築後に reduce() を呼ぶことで
 * 簡約された ZDD に変換できる。正規形ではないため、比較演算子は削除されている。
 *
 * @see ZDD
 * @see UnreducedBDD
 * @see QDD
 * @see DDBase
 * @see DDManager
 */
class UnreducedZDD : public DDBase {
    friend class DDManager;
    friend class ZDD;
    friend class QDD;

public:
    /**
     * @brief デフォルトコンストラクタ。無効な状態で初期化する。
     */
    UnreducedZDD() : DDBase() {}

    /**
     * @brief マネージャとアークを指定して構築するコンストラクタ（内部使用）
     * @param mgr DDManager へのポインタ
     * @param a アーク
     */
    UnreducedZDD(DDManager* mgr, Arc a) : DDBase(mgr, a) {}

    /** @brief コピーコンストラクタ */
    UnreducedZDD(const UnreducedZDD&) = default;
    /** @brief ムーブコンストラクタ */
    UnreducedZDD(UnreducedZDD&&) noexcept = default;
    /** @brief コピー代入演算子 */
    UnreducedZDD& operator=(const UnreducedZDD&) = default;
    /** @brief ムーブ代入演算子 */
    UnreducedZDD& operator=(UnreducedZDD&&) noexcept = default;

    /**
     * @brief 等値比較演算子（削除済み）。非簡約 ZDD は正規形でないため比較不可。
     */
    bool operator==(const UnreducedZDD&) const = delete;

    /**
     * @brief 非等値比較演算子（削除済み）。非簡約 ZDD は正規形でないため比較不可。
     */
    bool operator!=(const UnreducedZDD&) const = delete;

    /**
     * @brief 小なり比較演算子（削除済み）。非簡約 ZDD は正規形でないため比較不可。
     */
    bool operator<(const UnreducedZDD&) const = delete;

    /**
     * @brief 空集合を表す UnreducedZDD を生成する
     * @param mgr 使用する DDManager への参照
     * @return 空集合を表す UnreducedZDD
     */
    static UnreducedZDD empty(DDManager& mgr);

    /**
     * @brief 空集合のみを含む集合族（単位元）を表す UnreducedZDD を生成する
     * @param mgr 使用する DDManager への参照
     * @return 単位元を表す UnreducedZDD
     */
    static UnreducedZDD single(DDManager& mgr);

    /**
     * @brief ZDD 簡約規則を適用せずにノードを生成する（冗長ノードを許容）
     * @param mgr 使用する DDManager への参照
     * @param var 変数番号
     * @param low 0-枝の子ノード
     * @param high 1-枝の子ノード
     * @return 生成された UnreducedZDD ノード
     */
    static UnreducedZDD node(DDManager& mgr, bddvar var,
                             const UnreducedZDD& low, const UnreducedZDD& high);

    /**
     * @brief 0-枝（low）の子ノードを取得する
     * @return 0-枝の子ノードを表す UnreducedZDD
     */
    UnreducedZDD low() const;

    /**
     * @brief 1-枝（high）の子ノードを取得する
     * @return 1-枝の子ノードを表す UnreducedZDD
     */
    UnreducedZDD high() const;

    /**
     * @brief ZDD 簡約規則を適用して簡約 ZDD に変換する
     * @return 簡約された ZDD
     * @see ZDD
     */
    ZDD reduce() const;

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

#endif // SBDD2_UNREDUCED_ZDD_HPP
