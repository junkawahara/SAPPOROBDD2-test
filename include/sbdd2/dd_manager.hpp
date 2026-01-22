/**
 * @file dd_manager.hpp
 * @brief SAPPOROBDD 2.0 - DDマネージャー
 * @author SAPPOROBDD Team
 * @copyright MIT License
 *
 * 決定図のノードテーブル、キャッシュ、ガベージコレクションを管理するクラス。
 */

#ifndef SBDD2_DD_MANAGER_HPP
#define SBDD2_DD_MANAGER_HPP

#include "types.hpp"
#include "dd_node.hpp"
#include "exception.hpp"
#include <vector>
#include <mutex>
#include <atomic>
#include <functional>

namespace sbdd2 {

// Forward declarations
class BDD;
class ZDD;
class DDBase;
class DDNodeRef;

/**
 * @name デフォルトサイズ定数
 * @{
 */
constexpr std::size_t DEFAULT_NODE_TABLE_SIZE = 1 << 20;  ///< デフォルトノードテーブルサイズ（1Mノード）
constexpr std::size_t DEFAULT_CACHE_SIZE = 1 << 18;       ///< デフォルトキャッシュサイズ（256Kエントリ）
/** @} */

/**
 * @brief キャッシュ操作タイプ
 *
 * 演算キャッシュで使用される操作種別を表す列挙型。
 */
enum class CacheOp : std::uint8_t {
    AND = 0,        ///< 論理積（BDD）
    OR = 1,         ///< 論理和（BDD）
    XOR = 2,        ///< 排他的論理和（BDD）
    DIFF = 3,       ///< 差（BDD: f & ~g, ZDD: f - g）
    ITE = 4,        ///< if-then-else（BDD）
    RESTRICT = 5,   ///< 制限演算
    COMPOSE = 6,    ///< 合成演算
    // ZDD specific
    PRODUCT = 10,   ///< 直積（ZDD）
    QUOTIENT = 11,  ///< 商（ZDD）
    REMAINDER = 12, ///< 剰余（ZDD）
    UNION = 13,     ///< 和集合（ZDD）
    INTERSECT = 14, ///< 積集合（ZDD）
    // ZDD advanced operations
    RESTRICT_ZDD = 15,  ///< ZDD制限演算
    PERMIT_ZDD = 16,    ///< ZDD許可演算
    PERMIT_SYM = 17,    ///< ZDD対称許可演算
    ALWAYS = 18,        ///< ZDD常在演算
    SYM_CHK = 19,       ///< ZDD対称性チェック
    SYM_SET = 20,       ///< ZDD対称集合
    CO_IMPLY_SET = 21,  ///< ZDD逆インプリケーション集合
    MEET = 22,          ///< ZDD Meet演算
    // Custom
    CUSTOM = 255    ///< カスタム操作
};

/**
 * @brief キャッシュエントリ
 *
 * 演算結果をキャッシュするためのエントリ構造体。
 */
struct CacheEntry {
    std::uint64_t key1;   ///< 第1オペランド＋操作タイプ
    std::uint64_t key2;   ///< 第2オペランド（ITEの場合は第3も）
    std::uint64_t result; ///< 結果アーク

    /// デフォルトコンストラクタ
    CacheEntry() : key1(0), key2(0), result(0) {}

    /// 空エントリかどうか
    bool is_empty() const { return key1 == 0 && key2 == 0; }

    /// エントリをクリア
    void clear() {
        key1 = 0;
        key2 = 0;
        result = 0;
    }
};

/**
 * @brief DDマネージャークラス
 *
 * BDD/ZDDのノード管理、演算キャッシュ、ガベージコレクションを担当する
 * 中心的なクラスです。
 *
 * 主な機能:
 * - ノードテーブル管理（内部ハッシュ法、二次探索）
 * - 変数の作成と管理
 * - 演算キャッシュ
 * - 参照カウントによるメモリ管理
 * - ガベージコレクション
 *
 * @note スレッドセーフ：内部でmutexを使用
 *
 * @code{.cpp}
 * // マネージャーの作成
 * DDManager mgr;
 *
 * // 変数の作成
 * mgr.new_var();
 * mgr.new_var();
 *
 * // BDDの作成
 * BDD x1 = mgr.var_bdd(1);
 * BDD x2 = mgr.var_bdd(2);
 *
 * // 演算
 * BDD f = x1 & x2;
 *
 * // 統計情報
 * std::cout << "ノード数: " << mgr.node_count() << std::endl;
 * @endcode
 */
class DDManager {
public:
    /// @name コンストラクタ・デストラクタ
    /// @{

    /**
     * @brief コンストラクタ
     * @param node_table_size ノードテーブルの初期サイズ
     * @param cache_size 演算キャッシュのサイズ
     */
    explicit DDManager(std::size_t node_table_size = DEFAULT_NODE_TABLE_SIZE,
                       std::size_t cache_size = DEFAULT_CACHE_SIZE);

    /// デストラクタ
    ~DDManager();

    /// コピー禁止
    DDManager(const DDManager&) = delete;
    /// コピー代入禁止
    DDManager& operator=(const DDManager&) = delete;

    /// ムーブコンストラクタ
    DDManager(DDManager&&) noexcept;
    /// ムーブ代入演算子
    DDManager& operator=(DDManager&&) noexcept;

    /// @}

    /// @name 変数管理
    /// @{

    /**
     * @brief 新しい変数を作成
     * @return 作成された変数番号（1から開始）
     */
    bddvar new_var();

    /**
     * @brief 現在の変数数を取得
     * @return 変数の総数
     */
    bddvar var_count() const { return var_count_; }

    /// @}

    /// @name BDD/ZDD作成
    /// @{

    /**
     * @brief 変数BDDを取得
     * @param v 変数番号
     * @return 変数vを表すBDD（x_v）
     */
    BDD var_bdd(bddvar v);

    /**
     * @brief 変数ZDDを取得
     * @param v 変数番号
     * @return 要素vのみを含む集合族 {{v}}
     */
    ZDD var_zdd(bddvar v);

    /**
     * @brief BDDの定数0を取得
     * @return 偽を表すBDD
     */
    BDD bdd_zero();

    /**
     * @brief BDDの定数1を取得
     * @return 真を表すBDD
     */
    BDD bdd_one();

    /**
     * @brief 空集合族を取得
     * @return 空のZDD
     */
    ZDD zdd_empty();

    /**
     * @brief 基底集合族を取得
     * @return 空集合のみを含むZDD {∅}
     */
    ZDD zdd_base();

    /// @}

    /// @name ノード作成（内部使用）
    /// @{

    /**
     * @brief BDDノードを取得または作成
     * @param var 変数番号
     * @param arc0 0枝アーク
     * @param arc1 1枝アーク
     * @param reduced 既約フラグ
     * @return ノードへのアーク（既存または新規）
     */
    Arc get_or_create_node_bdd(bddvar var, Arc arc0, Arc arc1, bool reduced = true);

    /**
     * @brief ZDDノードを取得または作成
     * @param var 変数番号
     * @param arc0 0枝アーク
     * @param arc1 1枝アーク
     * @param reduced 既約フラグ
     * @return ノードへのアーク（既存または新規）
     */
    Arc get_or_create_node_zdd(bddvar var, Arc arc0, Arc arc1, bool reduced = true);

    /// @}

    /// @name 参照カウント管理
    /// @{

    /**
     * @brief 参照カウントを増加
     * @param arc 対象アーク
     */
    void inc_ref(Arc arc);

    /**
     * @brief 参照カウントを減少
     * @param arc 対象アーク
     */
    void dec_ref(Arc arc);

    /// @}

    /// @name ガベージコレクション
    /// @{

    /// 強制的にGCを実行
    void gc();

    /// 必要に応じてGCを実行
    void gc_if_needed();

    /// @}

    /// @name 統計情報
    /// @{

    /// 総ノード数（削除済み含む）
    std::size_t node_count() const { return node_count_; }

    /// 生存ノード数
    std::size_t alive_count() const { return alive_count_; }

    /// テーブルサイズ
    std::size_t table_size() const { return table_size_; }

    /// キャッシュサイズ
    std::size_t cache_size() const { return cache_size_; }

    /**
     * @brief ロードファクター（充填率）を取得
     * @return ノード数 / テーブルサイズ
     */
    double load_factor() const;

    /// @}

    /// @name キャッシュ操作
    /// @{

    /**
     * @brief 2項演算の結果をキャッシュから検索
     * @param op 操作タイプ
     * @param f 第1オペランド
     * @param g 第2オペランド
     * @param[out] result 結果（見つかった場合）
     * @return キャッシュヒットした場合true
     */
    bool cache_lookup(CacheOp op, Arc f, Arc g, Arc& result) const;

    /**
     * @brief 3項演算の結果をキャッシュから検索
     * @param op 操作タイプ
     * @param f 第1オペランド
     * @param g 第2オペランド
     * @param h 第3オペランド
     * @param[out] result 結果（見つかった場合）
     * @return キャッシュヒットした場合true
     */
    bool cache_lookup3(CacheOp op, Arc f, Arc g, Arc h, Arc& result) const;

    /**
     * @brief 2項演算の結果をキャッシュに登録
     * @param op 操作タイプ
     * @param f 第1オペランド
     * @param g 第2オペランド
     * @param result 結果
     */
    void cache_insert(CacheOp op, Arc f, Arc g, Arc result);

    /**
     * @brief 3項演算の結果をキャッシュに登録
     * @param op 操作タイプ
     * @param f 第1オペランド
     * @param g 第2オペランド
     * @param h 第3オペランド
     * @param result 結果
     */
    void cache_insert3(CacheOp op, Arc f, Arc g, Arc h, Arc result);

    /// キャッシュをクリア
    void cache_clear();

    /// @}

    /// @name ノードアクセス（内部使用）
    /// @{

    /// 指定インデックスのノードを取得（const版）
    const DDNode& node_at(bddindex index) const;

    /// 指定インデックスのノードを取得
    DDNode& node_at(bddindex index);

    /// @}

    /// @name 終端判定（静的メソッド）
    /// @{

    /// アークが終端を指すか判定
    static bool is_terminal(Arc arc) { return arc.is_constant(); }

    /// アークが終端0を指すか判定
    static bool is_terminal_zero(Arc arc) { return arc == ARC_TERMINAL_0; }

    /// アークが終端1を指すか判定
    static bool is_terminal_one(Arc arc) { return arc == ARC_TERMINAL_1; }

    /// @}

private:
    // Node table (internal hash with quadratic probing)
    std::vector<DDNode> nodes_;
    std::size_t table_size_;
    std::size_t node_count_;      // Total nodes in table (including tombstones)
    std::size_t alive_count_;     // Nodes with refcount > 0

    // Avail list (indices of available slots)
    std::vector<bddindex> avail_;

    // Operation cache
    std::vector<CacheEntry> cache_;
    std::size_t cache_size_;

    // Variable count
    std::atomic<bddvar> var_count_;

    // Thread safety
    mutable std::mutex table_mutex_;
    mutable std::mutex cache_mutex_;

    // GC threshold
    double gc_threshold_;
    std::size_t gc_min_nodes_;

    // Internal hash function
    std::size_t hash_node(bddvar var, Arc arc0, Arc arc1) const;

    // Find or create node (internal)
    bddindex find_node(bddvar var, Arc arc0, Arc arc1) const;
    bddindex insert_node(bddvar var, Arc arc0, Arc arc1, bool reduced);

    // Resize table when needed
    void resize_table();

    // GC helper
    void mark_and_sweep();
    void mark_arc(Arc arc, std::vector<bool>& marked);
};

/**
 * @brief デフォルトマネージャーを取得
 * @return グローバルなデフォルトDDManager
 *
 * @note シングルスレッド環境での簡易使用を想定
 */
DDManager& default_manager();

} // namespace sbdd2

#endif // SBDD2_DD_MANAGER_HPP
