/**
 * @file bdd.hpp
 * @brief SAPPOROBDD 2.0 - BDDクラス
 * @author SAPPOROBDD Team
 * @copyright MIT License
 *
 * 縮約された二分決定図（Reduced Binary Decision Diagram）の実装。
 */

#ifndef SBDD2_BDD_HPP
#define SBDD2_BDD_HPP

#include "dd_base.hpp"
#include <string>

namespace sbdd2 {

// Forward declarations
class ZDD;
class UnreducedBDD;

/**
 * @brief 縮約BDD（Reduced Binary Decision Diagram）クラス
 *
 * 縮約BDDはブール関数を表現するためのデータ構造です。
 * 以下の縮約規則が適用されています：
 * - 等価なノードの共有（一意性）
 * - 冗長ノードの除去（0枝と1枝が同じ場合）
 *
 * @note 否定枝表現を使用しており、NOT演算はO(1)で実行可能です。
 *
 * @code{.cpp}
 * DDManager mgr;
 * mgr.new_var();
 * mgr.new_var();
 *
 * // 変数BDDの作成
 * BDD x1 = mgr.var_bdd(1);
 * BDD x2 = mgr.var_bdd(2);
 *
 * // ブール演算
 * BDD f = x1 & x2;        // AND
 * BDD g = x1 | x2;        // OR
 * BDD h = x1 ^ x2;        // XOR
 * BDD not_f = ~f;         // NOT
 *
 * // 量化
 * BDD exists_x1 = f.exist(1);  // ∃x1. f
 * @endcode
 */
class BDD : public DDBase {
    friend class DDManager;
    friend class DDNodeRef;
    friend class ZDD;

public:
    /// @name コンストラクタ
    /// @{

    /// デフォルトコンストラクタ（無効なBDDを作成）
    BDD() : DDBase() {}

    /**
     * @brief マネージャーとアークからの構築（内部使用）
     * @param mgr DDマネージャーへのポインタ
     * @param a アーク
     */
    BDD(DDManager* mgr, Arc a) : DDBase(mgr, a) {}

    /// コピーコンストラクタ
    BDD(const BDD&) = default;
    /// ムーブコンストラクタ
    BDD(BDD&&) noexcept = default;
    /// コピー代入演算子
    BDD& operator=(const BDD&) = default;
    /// ムーブ代入演算子
    BDD& operator=(BDD&&) noexcept = default;

    /**
     * @brief UnreducedBDDからの変換コンストラクタ
     * @param unreduced 非縮約BDD
     *
     * 非縮約BDDを縮約して新しいBDDを作成します。
     */
    explicit BDD(const UnreducedBDD& unreduced);

    /// @}

    /// @name ファクトリメソッド
    /// @{

    /**
     * @brief 定数0（偽）を作成
     * @param mgr DDマネージャー
     * @return 定数0を表すBDD
     */
    static BDD zero(DDManager& mgr);

    /**
     * @brief 定数1（真）を作成
     * @param mgr DDマネージャー
     * @return 定数1を表すBDD
     */
    static BDD one(DDManager& mgr);

    /**
     * @brief 変数BDDを作成
     * @param mgr DDマネージャー
     * @param v 変数番号
     * @return 変数vを表すBDD
     */
    static BDD var(DDManager& mgr, bddvar v);

    /// @}

    /// @name 余因子（Shannon展開）
    /// @{

    /**
     * @brief 0-余因子を計算
     * @param v 変数番号
     * @return f|_{x_v=0}
     */
    BDD at0(bddvar v) const;

    /**
     * @brief 1-余因子を計算
     * @param v 変数番号
     * @return f|_{x_v=1}
     */
    BDD at1(bddvar v) const;

    /// @}

    /// @name 子ノードアクセス
    /// @{

    /**
     * @brief 0枝（low）の子を取得
     * @return トップノードの0枝先
     */
    BDD low() const;

    /**
     * @brief 1枝（high）の子を取得
     * @return トップノードの1枝先
     */
    BDD high() const;

    /// @}

    /// @name 否定演算
    /// @{

    /**
     * @brief 論理否定
     * @return ¬f
     * @note 否定枝表現により O(1) で実行
     */
    BDD operator~() const;

    /**
     * @brief 論理否定（メソッド形式）
     * @return ¬f
     */
    BDD negated() const { return ~(*this); }

    /// @}

    /// @name ブール演算
    /// @{

    /**
     * @brief 論理積（AND）
     * @param other 右オペランド
     * @return f ∧ g
     */
    BDD operator&(const BDD& other) const;

    /**
     * @brief 論理和（OR）
     * @param other 右オペランド
     * @return f ∨ g
     */
    BDD operator|(const BDD& other) const;

    /**
     * @brief 排他的論理和（XOR）
     * @param other 右オペランド
     * @return f ⊕ g
     */
    BDD operator^(const BDD& other) const;

    /**
     * @brief 差（f AND NOT g）
     * @param other 右オペランド
     * @return f ∧ ¬g
     */
    BDD operator-(const BDD& other) const;

    /// @}

    /// @name 複合代入演算子
    /// @{
    BDD& operator&=(const BDD& other);
    BDD& operator|=(const BDD& other);
    BDD& operator^=(const BDD& other);
    BDD& operator-=(const BDD& other);
    /// @}

    /**
     * @brief ITE（if-then-else）演算
     * @param t then節
     * @param e else節
     * @return this ? t : e
     *
     * f.ite(g, h) は f ? g : h を計算します。
     * すべてのブール演算はITEで表現可能です。
     */
    BDD ite(const BDD& t, const BDD& e) const;

    /// @name 量化演算
    /// @{

    /**
     * @brief 存在量化
     * @param v 量化する変数
     * @return ∃x_v. f = f|_{x_v=0} ∨ f|_{x_v=1}
     */
    BDD exist(bddvar v) const;

    /**
     * @brief 全称量化
     * @param v 量化する変数
     * @return ∀x_v. f = f|_{x_v=0} ∧ f|_{x_v=1}
     */
    BDD forall(bddvar v) const;

    /**
     * @brief 複数変数の存在量化
     * @param vars 量化する変数のリスト
     * @return ∃vars. f
     */
    BDD exist(const std::vector<bddvar>& vars) const;

    /**
     * @brief 複数変数の全称量化
     * @param vars 量化する変数のリスト
     * @return ∀vars. f
     */
    BDD forall(const std::vector<bddvar>& vars) const;

    /// @}

    /**
     * @brief 制限演算
     * @param v 変数番号
     * @param value 代入値
     * @return f|_{x_v=value}
     */
    BDD restrict(bddvar v, bool value) const;

    /**
     * @brief 合成演算
     * @param v 置換する変数
     * @param g 置換先のBDD
     * @return f[x_v := g]
     *
     * 変数 x_v を BDD g で置き換えます。
     */
    BDD compose(bddvar v, const BDD& g) const;

    /// @name カウント演算
    /// @{

    /**
     * @brief 充足割当数を計算
     * @return 充足割当の数（2^n スケール）
     */
    double card() const;

    /**
     * @brief 指定範囲での充足割当数を計算
     * @param max_var 最大変数番号
     * @return 充足割当の数
     */
    double count(bddvar max_var) const;

#if defined(SBDD2_HAS_GMP) || defined(SBDD2_HAS_BIGINT)
    /**
     * @brief 充足割当数を計算（厳密計算）
     * @return 充足割当数を文字列で返す
     *
     * GMPを使用して任意精度で計算します。
     * 2^53を超える大きな値でも正確に計算できます。
     *
     * @note この機能はGMPがインストールされている場合のみ利用可能です
     */
    std::string exact_count() const;
#endif

    /// @}

    /**
     * @brief 充足割当を1つ求める
     * @return 各変数への割当（-1=don't care, 0=false, 1=true）
     *
     * BDDが偽定数の場合は空のベクタを返します。
     */
    std::vector<int> one_sat() const;

    /**
     * @brief ZDDへの変換
     * @return 等価なZDD
     */
    ZDD to_zdd() const;

    /// @name デバッグ・出力
    /// @{

    /**
     * @brief 文字列表現を取得
     * @return BDDの文字列表現
     */
    std::string to_string() const;

    /**
     * @brief 標準出力に出力
     */
    void print() const;

    /// @}
};

/// @name 非メンバ演算子
/// @{
inline BDD operator&(BDD&& a, const BDD& b) { return a &= b; }
inline BDD operator|(BDD&& a, const BDD& b) { return a |= b; }
inline BDD operator^(BDD&& a, const BDD& b) { return a ^= b; }
/// @}

} // namespace sbdd2

#endif // SBDD2_BDD_HPP
