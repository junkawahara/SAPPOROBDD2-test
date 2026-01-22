/**
 * @file zdd.hpp
 * @brief SAPPOROBDD 2.0 - ZDDクラス
 * @author SAPPOROBDD Team
 * @copyright MIT License
 *
 * ゼロ抑制二分決定図（Zero-suppressed Binary Decision Diagram）の実装。
 */

#ifndef SBDD2_ZDD_HPP
#define SBDD2_ZDD_HPP

#include "dd_base.hpp"
#include <string>

namespace sbdd2 {

// Forward declarations
class BDD;
class UnreducedZDD;

/**
 * @brief 縮約ZDD（Zero-suppressed Binary Decision Diagram）クラス
 *
 * ZDDは集合族（集合の集合）を効率的に表現するデータ構造です。
 * BDDとは異なる縮約規則を使用します：
 * - 等価なノードの共有（一意性）
 * - 1枝が終端0を指すノードの除去（ゼロ抑制）
 *
 * ZDDは組合せ集合族の表現に特に適しています。
 *
 * @code{.cpp}
 * DDManager mgr;
 * mgr.new_var();
 * mgr.new_var();
 * mgr.new_var();
 *
 * // 単一要素集合 {1}, {2}, {3}
 * ZDD s1 = ZDD::single(mgr, 1);
 * ZDD s2 = ZDD::single(mgr, 2);
 * ZDD s3 = ZDD::single(mgr, 3);
 *
 * // 集合族の和: {{1}, {2}, {3}}
 * ZDD family = s1 + s2 + s3;
 *
 * // 集合族のカウント
 * double count = family.card();  // 3.0
 *
 * // 要素1を含む部分集合を取得
 * ZDD containing_1 = family.onset(1);
 * @endcode
 */
class ZDD : public DDBase {
    friend class DDManager;
    friend class DDNodeRef;
    friend class BDD;

public:
    /// @name コンストラクタ
    /// @{

    /// デフォルトコンストラクタ（無効なZDDを作成）
    ZDD() : DDBase() {}

    /**
     * @brief マネージャーとアークからの構築（内部使用）
     * @param mgr DDマネージャーへのポインタ
     * @param a アーク
     */
    ZDD(DDManager* mgr, Arc a) : DDBase(mgr, a) {}

    /// コピーコンストラクタ
    ZDD(const ZDD&) = default;
    /// ムーブコンストラクタ
    ZDD(ZDD&&) noexcept = default;
    /// コピー代入演算子
    ZDD& operator=(const ZDD&) = default;
    /// ムーブ代入演算子
    ZDD& operator=(ZDD&&) noexcept = default;

    /**
     * @brief UnreducedZDDからの変換コンストラクタ
     * @param unreduced 非縮約ZDD
     *
     * 非縮約ZDDを縮約して新しいZDDを作成します。
     */
    explicit ZDD(const UnreducedZDD& unreduced);

    /// @}

    /// @name ファクトリメソッド
    /// @{

    /**
     * @brief 空集合族を作成
     * @param mgr DDマネージャー
     * @return 空集合族 ∅（集合を1つも含まない）
     */
    static ZDD empty(DDManager& mgr);

    /**
     * @brief 基底集合族を作成
     * @param mgr DDマネージャー
     * @return 基底集合族 {∅}（空集合のみを含む）
     */
    static ZDD base(DDManager& mgr);

    /**
     * @brief 単一要素集合を作成
     * @param mgr DDマネージャー
     * @param v 要素番号
     * @return 集合族 {{v}}
     */
    static ZDD single(DDManager& mgr, bddvar v);

    /// @}

    /// @name 集合族演算
    /// @{

    /**
     * @brief onset演算（要素vを含む部分集合、vを除去）
     * @param v 要素番号
     * @return {S \ {v} | S ∈ F, v ∈ S}
     */
    ZDD onset(bddvar v) const;

    /**
     * @brief offset演算（要素vを含まない部分集合）
     * @param v 要素番号
     * @return {S | S ∈ F, v ∉ S}
     */
    ZDD offset(bddvar v) const;

    /**
     * @brief onset0演算（要素vを含む部分集合、vを保持）
     * @param v 要素番号
     * @return {S | S ∈ F, v ∈ S}
     */
    ZDD onset0(bddvar v) const;

    /**
     * @brief change演算（全集合でvをトグル）
     * @param v 要素番号
     * @return {S △ {v} | S ∈ F}
     */
    ZDD change(bddvar v) const;

    /// @}

    /// @name 子ノードアクセス
    /// @{

    /**
     * @brief 0枝（low）の子を取得
     * @return トップ変数を含まない集合の部分族
     */
    ZDD low() const;

    /**
     * @brief 1枝（high）の子を取得
     * @return トップ変数を含む集合の部分族（変数を除去した形）
     */
    ZDD high() const;

    /// @}

    /// @name 集合族演算（二項演算）
    /// @{

    /**
     * @brief 和集合（Union）
     * @param other 右オペランド
     * @return F ∪ G
     */
    ZDD operator+(const ZDD& other) const;

    /**
     * @brief 差集合（Difference）
     * @param other 右オペランド
     * @return F \ G
     */
    ZDD operator-(const ZDD& other) const;

    /**
     * @brief 積集合（Intersection）
     * @param other 右オペランド
     * @return F ∩ G
     */
    ZDD operator*(const ZDD& other) const;

    /**
     * @brief 商演算（Division）
     * @param other 右オペランド
     * @return {S | S ∪ T ∈ F for all T ∈ G}
     */
    ZDD operator/(const ZDD& other) const;

    /**
     * @brief 剰余演算（Remainder）
     * @param other 右オペランド
     * @return F - (F / G) * G
     */
    ZDD operator%(const ZDD& other) const;

    /// @}

    /// @name 複合代入演算子
    /// @{
    ZDD& operator+=(const ZDD& other);
    ZDD& operator-=(const ZDD& other);
    ZDD& operator*=(const ZDD& other);
    ZDD& operator/=(const ZDD& other);
    ZDD& operator%=(const ZDD& other);
    /// @}

    /**
     * @brief 直積演算（Cross Product）
     * @param other 右オペランド
     * @return {S ∪ T | S ∈ F, T ∈ G}
     */
    ZDD product(const ZDD& other) const;

    /// @name カウント演算
    /// @{

    /**
     * @brief 集合族に含まれる集合の数
     * @return |F|
     */
    double card() const;

    /**
     * @brief 集合族に含まれる集合の数（cardの別名）
     * @return |F|
     */
    double count() const;

    /// @}

    /// @name 列挙演算
    /// @{

    /**
     * @brief 全集合を列挙
     * @return 集合族に含まれる全ての集合
     *
     * @warning 集合数が大きい場合はメモリを大量に消費します
     */
    std::vector<std::vector<bddvar>> enumerate() const;

    /**
     * @brief 1つの集合を取得
     * @return 集合族に含まれる任意の1つの集合
     *
     * 空集合族の場合は空のベクタを返します。
     */
    std::vector<bddvar> one_set() const;

    /// @}

    /**
     * @brief BDDへの変換
     * @return 等価なBDD
     */
    BDD to_bdd() const;

    /// @name デバッグ・出力
    /// @{

    /**
     * @brief 文字列表現を取得
     * @return ZDDの文字列表現
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
inline ZDD operator+(ZDD&& a, const ZDD& b) { return a += b; }
inline ZDD operator*(ZDD&& a, const ZDD& b) { return a *= b; }
/// @}

} // namespace sbdd2

#endif // SBDD2_ZDD_HPP
