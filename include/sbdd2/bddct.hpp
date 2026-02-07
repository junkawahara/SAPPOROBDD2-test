/**
 * @file bddct.hpp
 * @brief BDDCT（BDDコストテーブル）クラスの定義。変数にコストを割り当て、コスト制約付き列挙を行う。
 *
 * 各変数にコストとラベルを関連付け、コスト上限を指定した
 * ZDD のフィルタリング（コスト制約付き部分集合列挙）を提供する。
 *
 * @see ZDD
 * @see DDManager
 */

// SAPPOROBDD 2.0 - BDDCT (BDD Cost Table) class
// MIT License

#ifndef SBDD2_BDDCT_HPP
#define SBDD2_BDDCT_HPP

#include "zdd.hpp"
#include <vector>
#include <string>
#include <map>
#include <cstdio>

namespace sbdd2 {

/** @brief コスト値の型 */
using bddcost = int;

/** @brief コスト値の無効値（未設定を示す） */
constexpr bddcost BDDCOST_NULL = 0x7FFFFFFF;

/** @brief コストテーブルのラベル文字列の最大長 */
constexpr int CT_STRLEN = 15;

/**
 * @brief BDD コストテーブルクラス。変数にコストを関連付け、コスト制約付き列挙を行う。
 *
 * 各変数にコスト（整数重み）を割り当て、ZDD で表現された集合族に対して
 * コスト上限によるフィルタリングや最小・最大コストの計算を提供する。
 * 内部キャッシュにより高速な繰り返し計算が可能。
 *
 * @see ZDD
 * @see DDManager
 * @see GBase
 */
class BDDCT {
public:
    /**
     * @brief コストマップ型。コスト値から対応する解の ZDD へのマッピング。
     */
    using CostMap = std::map<bddcost, ZDD>;

private:
    DDManager* manager_;
    int n_vars_;
    std::vector<bddcost> costs_;
    std::vector<std::string> labels_;

    /** @brief キャッシュエントリ（コストマップ付き） */
    struct CacheEntry {
        std::uint64_t id;
        CostMap* zmap;

        CacheEntry() : id(~0ULL), zmap(nullptr) {}
        ~CacheEntry() { delete zmap; }
    };

    /** @brief 簡易キャッシュエントリ（コスト値のみ） */
    struct Cache0Entry {
        std::uint64_t id;
        bddcost bound;
        std::uint8_t op;

        Cache0Entry() : id(~0ULL), bound(BDDCOST_NULL), op(255) {}
    };

    std::vector<CacheEntry> cache_;
    std::vector<Cache0Entry> cache0_;
    std::size_t cache_entries_;
    std::size_t cache0_entries_;
    std::size_t call_count_;

public:
    /**
     * @brief デフォルトコンストラクタ。マネージャなしで初期化する。
     */
    BDDCT();

    /**
     * @brief DDManager を指定して構築するコンストラクタ
     * @param mgr 使用する DDManager への参照
     */
    explicit BDDCT(DDManager& mgr);

    /**
     * @brief デストラクタ。キャッシュを解放する。
     */
    ~BDDCT();

    /** @brief コピーコンストラクタ（削除済み、キャッシュポインタのため） */
    BDDCT(const BDDCT&) = delete;
    /** @brief コピー代入演算子（削除済み） */
    BDDCT& operator=(const BDDCT&) = delete;

    /** @brief ムーブコンストラクタ */
    BDDCT(BDDCT&&) noexcept;
    /** @brief ムーブ代入演算子 */
    BDDCT& operator=(BDDCT&&) noexcept;

    /**
     * @brief コストテーブルの領域を確保する
     * @param n_vars 変数の数
     * @param default_cost 各変数のデフォルトコスト（デフォルト値: 1）
     * @return 確保に成功した場合 true
     */
    bool alloc(int n_vars, bddcost default_cost = 1);

    /**
     * @brief ファイルポインタからコストテーブルを読み込む
     * @param fp 入力ファイルポインタ
     * @return 読み込みに成功した場合 true
     */
    bool import(FILE* fp);

    /**
     * @brief ファイル名を指定してコストテーブルを読み込む
     * @param filename 入力ファイル名
     * @return 読み込みに成功した場合 true
     */
    bool import(const std::string& filename);

    /**
     * @brief ランダムなコスト値でテーブルを初期化する
     * @param n_vars 変数の数
     * @param min_cost コストの最小値
     * @param max_cost コストの最大値
     * @return 初期化に成功した場合 true
     */
    bool alloc_random(int n_vars, bddcost min_cost, bddcost max_cost);

    /**
     * @brief 変数番号を指定してコストを取得する
     * @param var_index 変数番号
     * @return 指定した変数のコスト
     */
    bddcost cost(int var_index) const;

    /**
     * @brief レベルを指定してコストを取得する
     * @param level レベル番号
     * @return 指定したレベルの変数のコスト
     */
    bddcost cost_of_level(int level) const;

    /**
     * @brief 変数番号を指定してコストを設定する
     * @param var_index 変数番号
     * @param cost 設定するコスト値
     * @return 設定に成功した場合 true
     */
    bool set_cost(int var_index, bddcost cost);

    /**
     * @brief レベルを指定してコストを設定する
     * @param level レベル番号
     * @param cost 設定するコスト値
     * @return 設定に成功した場合 true
     */
    bool set_cost_of_level(int level, bddcost cost);

    /**
     * @brief 変数番号を指定してラベルを取得する
     * @param var_index 変数番号
     * @return 指定した変数のラベル文字列への参照
     */
    const std::string& label(int var_index) const;

    /**
     * @brief レベルを指定してラベルを取得する
     * @param level レベル番号
     * @return 指定したレベルの変数のラベル文字列への参照
     */
    const std::string& label_of_level(int level) const;

    /**
     * @brief 変数番号を指定してラベルを設定する
     * @param var_index 変数番号
     * @param label 設定するラベル文字列
     * @return 設定に成功した場合 true
     */
    bool set_label(int var_index, const std::string& label);

    /**
     * @brief レベルを指定してラベルを設定する
     * @param level レベル番号
     * @param label 設定するラベル文字列
     * @return 設定に成功した場合 true
     */
    bool set_label_of_level(int level, const std::string& label);

    /**
     * @brief テーブルの変数の数を取得する
     * @return 変数の数
     */
    int size() const { return n_vars_; }

    /**
     * @brief 使用中の DDManager へのポインタを取得する
     * @return DDManager へのポインタ
     * @see DDManager
     */
    DDManager* manager() const { return manager_; }

    /**
     * @brief ZDD に対してコスト上限以下の部分集合のみを抽出する
     * @param f 入力 ZDD
     * @param bound コスト上限値
     * @return コストが bound 以下の部分集合のみを含む ZDD
     * @see ZDD
     */
    ZDD zdd_cost_le(const ZDD& f, bddcost bound);

    /**
     * @brief ZDD に対してコスト上限以下の部分集合のみを抽出する（詳細情報付き）
     * @param f 入力 ZDD
     * @param bound コスト上限値
     * @param[out] actual_weight 実際の最小コスト
     * @param[out] reduced_bound 削減後のコスト上限
     * @return コストが bound 以下の部分集合のみを含む ZDD
     * @see ZDD
     */
    ZDD zdd_cost_le(const ZDD& f, bddcost bound, bddcost& actual_weight, bddcost& reduced_bound);

    /**
     * @brief ZDD に対してコスト上限以下の部分集合のみを抽出する（簡易版）
     * @param f 入力 ZDD
     * @param bound コスト上限値
     * @return コストが bound 以下の部分集合のみを含む ZDD
     * @see ZDD
     */
    ZDD zdd_cost_le0(const ZDD& f, bddcost bound);

    /**
     * @brief ZDD で表現された集合族の最小コストを計算する
     * @param f 入力 ZDD
     * @return 最小コスト値
     */
    bddcost min_cost(const ZDD& f);

    /**
     * @brief ZDD で表現された集合族の最大コストを計算する
     * @param f 入力 ZDD
     * @return 最大コスト値
     */
    bddcost max_cost(const ZDD& f);

    /**
     * @brief メインキャッシュをクリアする
     */
    void cache_clear();

    /**
     * @brief メインキャッシュのサイズを拡大する
     */
    void cache_enlarge();

    /**
     * @brief 簡易キャッシュをクリアする
     */
    void cache0_clear();

    /**
     * @brief 簡易キャッシュのサイズを拡大する
     */
    void cache0_enlarge();

    /**
     * @brief コストテーブルの内容をファイルに出力する
     * @param fp 出力先ファイルポインタ（デフォルト: stdout）
     */
    void export_table(FILE* fp = stdout) const;

    /**
     * @brief コストテーブルの内容を文字列として取得する
     * @return テーブル情報の文字列表現
     */
    std::string to_string() const;

private:
    // Cache helpers
    ZDD cache_ref(const ZDD& f, bddcost bound, bddcost& aw, bddcost& rb);
    void cache_ent(const ZDD& f, const ZDD& result, bddcost bound, bddcost cost);
    bddcost cache0_ref(std::uint8_t op, std::uint64_t id) const;
    void cache0_ent(std::uint8_t op, std::uint64_t id, bddcost result);
};

} // namespace sbdd2

#endif // SBDD2_BDDCT_HPP
