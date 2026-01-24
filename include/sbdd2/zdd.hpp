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
#include "zdd_iterators.hpp"
#include <string>
#include <set>
#include <mutex>
#include <memory>
#include <random>

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
 * ZDD s1 = ZDD::singleton(mgr, 1);
 * ZDD s2 = ZDD::singleton(mgr, 2);
 * ZDD s3 = ZDD::singleton(mgr, 3);
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
     * @brief 単一集合族（1終端）を作成
     * @param mgr DDマネージャー
     * @return 単一集合族 {∅}（空集合のみを含む）
     */
    static ZDD single(DDManager& mgr);

    /**
     * @brief 単一要素集合を作成
     * @param mgr DDマネージャー
     * @param v 要素番号
     * @return 集合族 {{v}}
     */
    static ZDD singleton(DDManager& mgr, bddvar v);

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
    ZDD operator&(const ZDD& other) const;

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
    ZDD& operator&=(const ZDD& other);
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

    /// @name 辞書順アクセス
    /// @{

    /**
     * @brief 集合から辞書順番号を取得
     * @param s 検索する集合
     * @return sの辞書順番号（0始まり）、集合が存在しない場合は-1
     *
     * 辞書順は、ZDDの構造に基づいて定義されます：
     * - まず1枝側（その要素を含む集合）が先
     * - 次に0枝側（その要素を含まない集合）
     */
    int64_t order_of(const std::set<bddvar>& s) const;

#ifdef SBDD2_HAS_GMP
    /**
     * @brief 集合から辞書順番号を取得（GMP版）
     * @param s 検索する集合
     * @return sの辞書順番号（0始まり）の文字列表現、存在しない場合は"-1"
     */
    std::string exact_order_of(const std::set<bddvar>& s) const;
#endif

    /**
     * @brief 辞書順番号から集合を取得
     * @param order 辞書順番号（0始まり）
     * @return 指定された番号の集合
     *
     * @pre 0 <= order < indexed_count()
     */
    std::set<bddvar> get_set(int64_t order) const;

#ifdef SBDD2_HAS_GMP
    /**
     * @brief 辞書順番号から集合を取得（GMP版）
     * @param order 辞書順番号（0始まり）の文字列表現
     * @return 指定された番号の集合
     *
     * @pre 0 <= order < indexed_exact_count()
     */
    std::set<bddvar> exact_get_set(const std::string& order) const;
#endif

    /// @}

    /// @name 重み付き最適化
    /// @{

    /**
     * @brief 最大重みを持つ集合を取得
     * @param weights 各変数の重み（weights[v] = 変数vの重み）
     * @param result_set 最大重みの集合を格納
     * @return 最大重み
     *
     * 集合の重みは、その集合に含まれる変数の重みの総和です。
     * 空集合の重みは0です。
     */
    int64_t max_weight(const std::vector<int64_t>& weights, std::set<bddvar>& result_set) const;

    /**
     * @brief 最大重みを取得（集合は返さない）
     * @param weights 各変数の重み
     * @return 最大重み
     */
    int64_t max_weight(const std::vector<int64_t>& weights) const;

    /**
     * @brief 最小重みを持つ集合を取得
     * @param weights 各変数の重み
     * @param result_set 最小重みの集合を格納
     * @return 最小重み
     */
    int64_t min_weight(const std::vector<int64_t>& weights, std::set<bddvar>& result_set) const;

    /**
     * @brief 最小重みを取得（集合は返さない）
     * @param weights 各変数の重み
     * @return 最小重み
     */
    int64_t min_weight(const std::vector<int64_t>& weights) const;

    /**
     * @brief 全集合の重みの総和を計算
     * @param weights 各変数の重み
     * @return 全集合の重みの総和
     *
     * 各集合の重みはその集合に含まれる変数の重みの総和。
     * この関数は全集合の重みを合計します。
     */
    int64_t sum_weight(const std::vector<int64_t>& weights) const;

#ifdef SBDD2_HAS_GMP
    /**
     * @brief 全集合の重みの総和を計算（GMP版）
     * @param weights 各変数の重み
     * @return 全集合の重みの総和（文字列形式）
     */
    std::string exact_sum_weight(const std::vector<int64_t>& weights) const;
#endif

    /// @}

    /// @name ランダムサンプリング
    /// @{

    /**
     * @brief ZDD内から一様ランダムに集合を1つサンプリング
     * @tparam RNG C++11乱数生成器の型（std::mt19937など）
     * @param rng 乱数生成器への参照
     * @return ランダムに選択された集合
     *
     * ZDD内の全集合から一様ランダムに1つの集合を選択します。
     * 内部では辞書順番号をランダムに生成し、get_set()を呼び出します。
     *
     * @code{.cpp}
     * std::mt19937 rng(42);  // シード42で初期化
     * std::set<bddvar> s = zdd.sample_randomly(rng);
     * @endcode
     *
     * @pre ZDDが空でないこと（is_zero()がfalse）
     * @note 空のZDDに対して呼び出した場合は空集合を返します
     */
    template<typename RNG>
    std::set<bddvar> sample_randomly(RNG& rng) const {
        // 空の場合は空集合を返す
        if (is_zero()) {
            return std::set<bddvar>();
        }

        // インデックスを構築（未構築の場合）
        ensure_index();

        // ルートノードのカウントを取得
        double total = indexed_count();
        if (total <= 0) {
            return std::set<bddvar>();
        }

        // 0からtotal-1までの一様分布で辞書順番号を生成
        std::uniform_int_distribution<int64_t> dist(0, static_cast<int64_t>(total) - 1);
        int64_t order = dist(rng);

        return get_set(order);
    }

#ifdef SBDD2_HAS_GMP
    /**
     * @brief ZDD内から一様ランダムに集合を1つサンプリング（GMP版）
     * @tparam RNG C++11乱数生成器の型
     * @param rng 乱数生成器への参照
     * @return ランダムに選択された集合
     *
     * 2^53を超える大きな集合族でも一様サンプリングが可能です。
     * 内部ではGMPの乱数生成を使用します。
     *
     * @pre ZDDが空でないこと
     */
    template<typename RNG>
    std::set<bddvar> exact_sample_randomly(RNG& rng) const {
        // 空の場合は空集合を返す
        if (is_zero()) {
            return std::set<bddvar>();
        }

        // GMP版インデックスを構築してカウントを取得
        std::string total_str = indexed_exact_count();
        mpz_class total(total_str);

        if (total <= 0) {
            return std::set<bddvar>();
        }

        // GMP乱数生成器を使用
        gmp_randclass gmp_rng(gmp_randinit_default);
        // C++乱数生成器からシードを取得
        std::uniform_int_distribution<unsigned long> seed_dist;
        gmp_rng.seed(seed_dist(rng));

        // 0からtotal-1までの一様分布で辞書順番号を生成
        mpz_class order = gmp_rng.get_z_range(total);

        return exact_get_set(order.get_str());
    }
#endif

    /// @}

    /// @name イテレータ
    /// @{

    /**
     * @brief 辞書順イテレータの開始位置
     * @return 辞書順で最初の集合を指すイテレータ
     */
    DictIterator dict_begin() const;

    /**
     * @brief 辞書順イテレータの終端位置
     * @return 終端を指すイテレータ
     */
    DictIterator dict_end() const;

    /**
     * @brief 逆辞書順イテレータの開始位置
     * @return 逆辞書順で最初の集合（辞書順で最後の集合）を指すイテレータ
     */
    DictIterator dict_rbegin() const;

    /**
     * @brief 逆辞書順イテレータの終端位置
     * @return 終端を指すイテレータ
     */
    DictIterator dict_rend() const;

    /**
     * @brief 重み昇順イテレータの開始位置
     * @param weights 各変数の重み
     * @return 最小重み集合を指すイテレータ
     */
    WeightIterator weight_min_begin(const std::vector<int64_t>& weights) const;

    /**
     * @brief 重み昇順イテレータの終端位置
     * @return 終端を指すイテレータ
     */
    WeightIterator weight_min_end() const;

    /**
     * @brief 重み降順イテレータの開始位置
     * @param weights 各変数の重み
     * @return 最大重み集合を指すイテレータ
     */
    WeightIterator weight_max_begin(const std::vector<int64_t>& weights) const;

    /**
     * @brief 重み降順イテレータの終端位置
     * @return 終端を指すイテレータ
     */
    WeightIterator weight_max_end() const;

    /**
     * @brief ランダムイテレータの開始位置
     * @param rng 乱数生成器
     * @return ランダムに選んだ集合を指すイテレータ
     */
    RandomIterator random_begin(std::mt19937& rng) const;

    /**
     * @brief ランダムイテレータの終端位置
     * @return 終端を指すイテレータ
     */
    RandomIterator random_end() const;

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
inline ZDD operator&(ZDD&& a, const ZDD& b) { return a &= b; }
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
