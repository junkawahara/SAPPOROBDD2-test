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
#include "zdd_index.hpp"
#include <string>
#include <mutex>
#include <memory>

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

private:
    /// @name インデックスキャッシュ（mutableでconstメソッドから変更可能）
    /// @{
    /// once_flagはリセットできないためunique_ptrでラップ
    mutable std::unique_ptr<std::once_flag> index_once_flag_;
    mutable std::unique_ptr<ZDDIndexData> index_cache_;

#ifdef SBDD2_HAS_GMP
    mutable std::unique_ptr<std::once_flag> exact_index_once_flag_;
    mutable std::unique_ptr<ZDDExactIndexData> exact_index_cache_;
#endif
    /// @}

public:
    /// @name コンストラクタ
    /// @{

    /// デフォルトコンストラクタ（無効なZDDを作成）
    ZDD() : DDBase(),
        index_once_flag_(new std::once_flag()),
        index_cache_(nullptr)
#ifdef SBDD2_HAS_GMP
        , exact_index_once_flag_(new std::once_flag())
        , exact_index_cache_(nullptr)
#endif
    {}

    /**
     * @brief マネージャーとアークからの構築（内部使用）
     * @param mgr DDマネージャーへのポインタ
     * @param a アーク
     */
    ZDD(DDManager* mgr, Arc a) : DDBase(mgr, a),
        index_once_flag_(new std::once_flag()),
        index_cache_(nullptr)
#ifdef SBDD2_HAS_GMP
        , exact_index_once_flag_(new std::once_flag())
        , exact_index_cache_(nullptr)
#endif
    {}

    /**
     * @brief コピーコンストラクタ
     * @note インデックスキャッシュはコピーしない
     */
    ZDD(const ZDD& other) : DDBase(other),
        index_once_flag_(new std::once_flag()),
        index_cache_(nullptr)
#ifdef SBDD2_HAS_GMP
        , exact_index_once_flag_(new std::once_flag())
        , exact_index_cache_(nullptr)
#endif
    {}

    /**
     * @brief ムーブコンストラクタ
     * @note インデックスキャッシュはムーブしない
     */
    ZDD(ZDD&& other) noexcept : DDBase(std::move(other)),
        index_once_flag_(new std::once_flag()),
        index_cache_(nullptr)
#ifdef SBDD2_HAS_GMP
        , exact_index_once_flag_(new std::once_flag())
        , exact_index_cache_(nullptr)
#endif
    {}

    /**
     * @brief コピー代入演算子
     * @note インデックスキャッシュはコピーしない
     */
    ZDD& operator=(const ZDD& other) {
        if (this != &other) {
            DDBase::operator=(other);
            // 新しいonce_flagを作成して再構築可能にする
            index_once_flag_.reset(new std::once_flag());
            index_cache_.reset();
#ifdef SBDD2_HAS_GMP
            exact_index_once_flag_.reset(new std::once_flag());
            exact_index_cache_.reset();
#endif
        }
        return *this;
    }

    /**
     * @brief ムーブ代入演算子
     * @note インデックスキャッシュはムーブしない
     */
    ZDD& operator=(ZDD&& other) noexcept {
        if (this != &other) {
            DDBase::operator=(std::move(other));
            // 新しいonce_flagを作成して再構築可能にする
            index_once_flag_.reset(new std::once_flag());
            index_cache_.reset();
#ifdef SBDD2_HAS_GMP
            exact_index_once_flag_.reset(new std::once_flag());
            exact_index_cache_.reset();
#endif
        }
        return *this;
    }

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

#ifdef SBDD2_HAS_GMP
    /**
     * @brief 集合族に含まれる集合の数（厳密計算）
     * @return |F| を文字列で返す
     *
     * GMPを使用して任意精度で計算します。
     * 2^53を超える大きな値でも正確に計算できます。
     *
     * @note この機能はGMPがインストールされている場合のみ利用可能です
     */
    std::string exact_count() const;
#endif

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

    /// @name 変数操作
    /// @{

    /**
     * @brief 2つの変数を交換
     * @param v1 変数1
     * @param v2 変数2
     * @return 変数v1とv2を交換したZDD
     */
    ZDD swap(bddvar v1, bddvar v2) const;

    /**
     * @brief 別のZDDで制限
     * @param g 制限条件のZDD
     * @return gに含まれる要素を少なくとも1つ含む集合のみを残す
     */
    ZDD restrict(const ZDD& g) const;

    /**
     * @brief 別のZDDで許可
     * @param g 許可条件のZDD
     * @return gに含まれる集合の部分集合のみを残す
     */
    ZDD permit(const ZDD& g) const;

    /**
     * @brief 対称許可（濃度制限）
     * @param n 最大濃度
     * @return 濃度がn以下の集合のみを残す
     */
    ZDD permit_sym(int n) const;

    /**
     * @brief 必ず現れる要素を取得
     * @return 全ての集合に共通して含まれる要素の集合
     */
    ZDD always() const;

    /// @}

    /// @name カウント・サイズ演算
    /// @{

    /**
     * @brief リテラル総数を計算
     * @return 全集合の要素数の合計
     */
    std::uint64_t lit() const;

    /**
     * @brief 最大集合サイズを計算
     * @return 集合族に含まれる集合の最大濃度
     */
    std::uint64_t len() const;

    /// @}

    /// @name シフト演算子
    /// @{

    /**
     * @brief 左シフト（変数番号を増加）
     * @param s シフト量
     * @return 全変数番号をsだけ増加させたZDD
     */
    ZDD operator<<(int s) const;

    /**
     * @brief 右シフト（変数番号を減少）
     * @param s シフト量
     * @return 全変数番号をsだけ減少させたZDD
     */
    ZDD operator>>(int s) const;

    /**
     * @brief 左シフト代入
     * @param s シフト量
     * @return *this
     */
    ZDD& operator<<=(int s);

    /**
     * @brief 右シフト代入
     * @param s シフト量
     * @return *this
     */
    ZDD& operator>>=(int s);

    /// @}

    /// @name 対称性・インプリケーション
    /// @{

    /**
     * @brief 2変数の対称性チェック
     * @param v1 変数1
     * @param v2 変数2
     * @return 対称なら1、非対称なら0、エラーなら-1
     */
    int sym_chk(bddvar v1, bddvar v2) const;

    /**
     * @brief 対称グループを取得
     * @return 対称な変数グループの集合
     */
    ZDD sym_grp() const;

    /**
     * @brief 対称グループを取得（素朴版）
     * @return 対称な変数グループの集合
     */
    ZDD sym_grp_naive() const;

    /**
     * @brief 指定変数と対称な変数集合を取得
     * @param v 変数
     * @return vと対称な全変数の集合
     */
    ZDD sym_set(bddvar v) const;

    /**
     * @brief インプリケーションチェック
     * @param v1 変数1
     * @param v2 変数2
     * @return v1がv2をインプライするなら1、そうでなければ0
     */
    int imply_chk(bddvar v1, bddvar v2) const;

    /**
     * @brief 逆インプリケーションチェック
     * @param v1 変数1
     * @param v2 変数2
     * @return v1とv2が互いにインプライするなら1
     */
    int co_imply_chk(bddvar v1, bddvar v2) const;

    /**
     * @brief インプリケーション集合を取得
     * @param v 変数
     * @return vがインプライする全変数の集合
     */
    ZDD imply_set(bddvar v) const;

    /**
     * @brief 逆インプリケーション集合を取得
     * @param v 変数
     * @return vと互いにインプライする全変数の集合
     */
    ZDD co_imply_set(bddvar v) const;

    /// @}

    /// @name 多項式演算
    /// @{

    /**
     * @brief 多項式判定
     * @return 複数の集合を含むなら1、単一集合なら0
     */
    int is_poly() const;

    /**
     * @brief 最小因子を取得
     * @return 集合族の最小因子
     */
    ZDD divisor() const;

    /// @}

    /// @name インデックス操作（効率的な辞書順アクセス・サンプリング用）
    /// @{

    /**
     * @brief インデックスを構築（double版）
     *
     * 各ノードから1終端までの経路数をdoubleでキャッシュします。
     * このメソッドはスレッドセーフで、複数スレッドから同時に呼び出しても
     * 構築は1回のみ実行されます。
     *
     * @note 通常は明示的に呼び出す必要はありません。
     *       インデックスを使用するメソッドが自動的に構築します。
     */
    void build_index() const;

#ifdef SBDD2_HAS_GMP
    /**
     * @brief インデックスを構築（GMP版）
     *
     * 各ノードから1終端までの経路数をmpz_classでキャッシュします。
     * 2^53を超える大きな集合族でも正確にカウントできます。
     *
     * @note 通常は明示的に呼び出す必要はありません。
     *       exact_*メソッドが自動的に構築します。
     */
    void build_exact_index() const;
#endif

    /**
     * @brief インデックスキャッシュをクリア
     *
     * double版とGMP版両方のインデックスキャッシュを解放します。
     * メモリを節約したい場合に使用します。
     */
    void clear_index() const;

    /**
     * @brief インデックスが構築済みか確認（double版）
     * @return 構築済みならtrue
     */
    bool has_index() const;

#ifdef SBDD2_HAS_GMP
    /**
     * @brief インデックスが構築済みか確認（GMP版）
     * @return 構築済みならtrue
     */
    bool has_exact_index() const;
#endif

    /**
     * @brief インデックスの高さを取得
     * @return ZDDの高さ（ルートノードのレベル）
     */
    int index_height() const;

    /**
     * @brief インデックスのノード数を取得
     * @return 総ノード数
     */
    std::size_t index_size() const;

    /**
     * @brief 指定レベルのノード数を取得
     * @param level レベル
     * @return そのレベルのノード数
     */
    std::size_t index_size_at_level(int level) const;

    /**
     * @brief インデックスを使ったカウント（double版）
     * @return 集合族に含まれる集合の数
     *
     * インデックスが未構築の場合は自動的に構築します。
     */
    double indexed_count() const;

#ifdef SBDD2_HAS_GMP
    /**
     * @brief インデックスを使ったカウント（GMP版）
     * @return 集合族に含まれる集合の数（文字列形式）
     *
     * インデックスが未構築の場合は自動的に構築します。
     */
    std::string indexed_exact_count() const;
#endif

    /// @}

private:
    /// @name インデックス構築ヘルパー
    /// @{
    void ensure_index() const;
    void build_index_impl() const;
#ifdef SBDD2_HAS_GMP
    void ensure_exact_index() const;
    void build_exact_index_impl() const;
#endif
    /// @}
};

/// @name 非メンバ演算子
/// @{
inline ZDD operator+(ZDD&& a, const ZDD& b) { return a += b; }
inline ZDD operator*(ZDD&& a, const ZDD& b) { return a *= b; }
/// @}

/// @name 非メンバ関数
/// @{

/**
 * @brief Meet演算
 * @param f 左オペランド
 * @param g 右オペランド
 * @return fとgのMeet（集合族の要素ごとの共通部分の族）
 *
 * Knuth TAOCP Vol 4 Ex 203 に基づく実装。
 */
ZDD zdd_meet(const ZDD& f, const ZDD& g);

/// @}

} // namespace sbdd2

#endif // SBDD2_ZDD_HPP
