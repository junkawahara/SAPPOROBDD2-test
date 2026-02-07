/**
 * @file qdd.hpp
 * @brief 準簡約決定図 QDD (Quasi-reduced Decision Diagram) クラスの定義
 * @copyright MIT License
 *
 * ノード共有は行うが、スキップ規則を適用しない準簡約決定図を実装します。
 * すべてのレベルにノードが存在するため、レベルごとの走査が容易です。
 */

// SAPPOROBDD 2.0 - QDD (Quasi-reduced Decision Diagram) class
// MIT License

#ifndef SBDD2_QDD_HPP
#define SBDD2_QDD_HPP

#include "dd_base.hpp"

namespace sbdd2 {

// Forward declarations
class BDD;
class ZDD;
class UnreducedBDD;
class UnreducedZDD;

/**
 * @brief 準簡約決定図 (Quasi-reduced Decision Diagram) クラス
 *
 * ノード共有は保証されます（ハッシュテーブルによるユニーク性）が、
 * スキップ規則は適用されません（すべてのレベルにノードが存在します）。
 *
 * スキップ規則が適用されないため、同じ論理関数が異なる構造を持つ可能性があり、
 * 等価比較演算子は削除されています。
 *
 * @see DDBase
 * @see BDD
 * @see ZDD
 * @see UnreducedBDD
 * @see UnreducedZDD
 */
class QDD : public DDBase {
    friend class DDManager;
    friend class BDD;
    friend class ZDD;
    friend class UnreducedBDD;
    friend class UnreducedZDD;

public:
    /**
     * @brief デフォルトコンストラクタ（無効なQDDを生成）
     */
    QDD() : DDBase() {}

    /**
     * @brief マネージャとArcを指定するコンストラクタ（内部使用）
     * @param mgr DDマネージャへのポインタ
     * @param a ルート辺
     */
    QDD(DDManager* mgr, Arc a) : DDBase(mgr, a) {}

    /// @brief コピーコンストラクタ（デフォルト）
    QDD(const QDD&) = default;
    /// @brief ムーブコンストラクタ（デフォルト）
    QDD(QDD&&) noexcept = default;
    /// @brief コピー代入演算子（デフォルト）
    QDD& operator=(const QDD&) = default;
    /// @brief ムーブ代入演算子（デフォルト）
    QDD& operator=(QDD&&) noexcept = default;

    /**
     * @name 削除された比較演算子
     *
     * QDDは正準形ではないため（スキップ規則が適用されないため、
     * 同じ関数が異なる構造を持ちうる）、比較演算子は削除されています。
     * @{
     */
    bool operator==(const QDD&) const = delete;
    bool operator!=(const QDD&) const = delete;
    bool operator<(const QDD&) const = delete;
    /// @}

    /**
     * @brief UnreducedBDD から QDD を構築する
     * @param ubdd 変換元の UnreducedBDD
     * @see UnreducedBDD
     */
    explicit QDD(const UnreducedBDD& ubdd);

    /**
     * @brief UnreducedZDD から QDD を構築する
     * @param uzdd 変換元の UnreducedZDD
     * @see UnreducedZDD
     */
    explicit QDD(const UnreducedZDD& uzdd);

    /**
     * @brief 0終端（空集合/偽）を生成するファクトリメソッド
     * @param mgr DDマネージャ
     * @return 0終端を表す QDD
     * @see one()
     */
    static QDD zero(DDManager& mgr);

    /**
     * @brief 1終端（全集合/真）を生成するファクトリメソッド
     * @param mgr DDマネージャ
     * @return 1終端を表す QDD
     * @see zero()
     */
    static QDD one(DDManager& mgr);

    /**
     * @brief ノード共有ありだがスキップ規則なしでノードを生成する
     * @param mgr DDマネージャ
     * @param var 変数番号
     * @param low 0-child（低枝）
     * @param high 1-child（高枝）
     * @return 生成された QDD ノード
     *
     * ハッシュテーブルによるノード共有は行いますが、
     * BDD/ZDDのスキップ規則は適用しません。
     */
    static QDD node(DDManager& mgr, bddvar var, const QDD& low, const QDD& high);

    /**
     * @brief 0-child（低枝）を取得する
     * @return 0-child を表す QDD
     * @see high()
     */
    QDD low() const;

    /**
     * @brief 1-child（高枝）を取得する
     * @return 1-child を表す QDD
     * @see low()
     */
    QDD high() const;

    /**
     * @brief 否定演算子
     * @return 否定された QDD
     */
    QDD operator~() const;

    /**
     * @brief BDD に変換する
     * @return BDDスキップ規則を適用した BDD
     *
     * QDDにBDDの簡約規則（冗長ノードの除去）を適用して、
     * 正準形のBDDに変換します。
     *
     * @see to_zdd()
     * @see BDD
     */
    BDD to_bdd() const;

    /**
     * @brief ZDD に変換する
     * @return ZDDスキップ規則を適用した ZDD
     *
     * QDDにZDDの簡約規則（1-childが0終端のノードの除去）を適用して、
     * 正準形のZDDに変換します。
     *
     * @see to_bdd()
     * @see ZDD
     */
    ZDD to_zdd() const;
};

} // namespace sbdd2

#endif // SBDD2_QDD_HPP
