/**
 * @file zdd_iterators.hpp
 * @brief SAPPOROBDD 2.0 - ZDDイテレータクラス
 * @author SAPPOROBDD Team
 * @copyright MIT License
 *
 * ZDDの集合を様々な順序で列挙するためのイテレータクラス。
 * - DictIterator: 辞書順
 * - WeightIterator: 重み順（昇順/降順）
 * - RandomIterator: ランダム順（重複なし）
 */

#ifndef SBDD2_ZDD_ITERATORS_HPP
#define SBDD2_ZDD_ITERATORS_HPP

#include "types.hpp"
#include <set>
#include <vector>
#include <cstdint>
#include <iterator>
#include <random>

namespace sbdd2 {

// Forward declaration
class ZDD;

/**
 * @brief 辞書順イテレータ
 *
 * ZDD内の集合を辞書順で列挙する入力イテレータ。
 * O(1)のメモリオーバーヘッドで効率的に列挙できる。
 * 昇順・降順の両方に対応している。
 *
 * @code{.cpp}
 * ZDD zdd = ...;
 * for (auto it = zdd.dict_begin(); it != zdd.dict_end(); ++it) {
 *     std::set<bddvar> s = *it;
 *     // sを処理
 * }
 * @endcode
 *
 * @see WeightIterator, RandomIterator, ZDDIndexData
 */
class DictIterator {
public:
    using iterator_category = std::input_iterator_tag;   ///< イテレータカテゴリ（入力イテレータ）
    using value_type = std::set<bddvar>;                 ///< 値型（変数の集合）
    using difference_type = std::ptrdiff_t;              ///< 差分型
    using pointer = const std::set<bddvar>*;             ///< ポインタ型
    using reference = const std::set<bddvar>&;           ///< 参照型

private:
    const ZDD* zdd_;
    int64_t current_;
    int64_t count_;
    bool reverse_;
    bool is_end_;
    mutable std::set<bddvar> cached_value_;

public:
    /**
     * @brief 終端イテレータ用デフォルトコンストラクタ
     *
     * 範囲ベースforループ等でend()として使用される終端イテレータを生成する。
     */
    DictIterator() : zdd_(nullptr), current_(0), count_(0), reverse_(false), is_end_(true) {}

    /**
     * @brief 通常コンストラクタ（beginイテレータ用）
     * @param zdd 列挙対象のZDDへのポインタ
     * @param count ZDDの濃度（集合の総数）
     * @param reverse trueの場合、辞書順降順で列挙する
     */
    DictIterator(const ZDD* zdd, int64_t count, bool reverse);

    /**
     * @brief イテレータが終端に達したかどうかを判定する
     * @return 終端に達している場合はtrue
     */
    bool at_end() const;

    /**
     * @brief 現在の集合を取得する（参照外し演算子）
     * @return 現在指している変数の集合
     */
    std::set<bddvar> operator*() const;

    /**
     * @brief イテレータを次の集合に進める（前置インクリメント）
     * @return インクリメント後の自身への参照
     */
    DictIterator& operator++();

    /**
     * @brief イテレータを次の集合に進める（後置インクリメント）
     * @return インクリメント前のイテレータのコピー
     */
    DictIterator operator++(int) {
        DictIterator tmp = *this;
        ++(*this);
        return tmp;
    }

    /**
     * @brief 等価比較演算子
     * @param other 比較対象のイテレータ
     * @return 二つのイテレータが同じ位置を指している場合はtrue
     */
    bool operator==(const DictIterator& other) const {
        // Both at end = equal, neither at end = compare current_
        if (at_end() && other.at_end()) return true;
        if (at_end() != other.at_end()) return false;
        return current_ == other.current_ && reverse_ == other.reverse_;
    }

    /**
     * @brief 不等価比較演算子
     * @param other 比較対象のイテレータ
     * @return 二つのイテレータが異なる位置を指している場合はtrue
     */
    bool operator!=(const DictIterator& other) const {
        return !(*this == other);
    }
};

/**
 * @brief 重み順イテレータ
 *
 * ZDD内の集合を重み順（昇順または降順）で列挙する入力イテレータ。
 * 内部的にZDDのコピーを保持し、各イテレーションで現在の最小/最大重みの
 * 集合を取り出して取り除く。
 *
 * @code{.cpp}
 * std::vector<int64_t> weights = {0, 1, 2, 3};
 * for (auto it = zdd.weight_min_begin(weights); it != zdd.weight_min_end(); ++it) {
 *     std::set<bddvar> s = *it;
 *     // 重み昇順でsを処理
 * }
 * @endcode
 *
 * @see DictIterator, RandomIterator, weight_range
 */
class WeightIterator {
public:
    using iterator_category = std::input_iterator_tag;   ///< イテレータカテゴリ（入力イテレータ）
    using value_type = std::set<bddvar>;                 ///< 値型（変数の集合）
    using difference_type = std::ptrdiff_t;              ///< 差分型
    using pointer = const std::set<bddvar>*;             ///< ポインタ型
    using reference = const std::set<bddvar>&;           ///< 参照型

private:
    ZDD* remaining_;  // owned pointer to remaining ZDD
    std::vector<int64_t> weights_;
    bool is_min_;
    std::set<bddvar> current_;

    /**
     * @brief 現在の集合を更新する
     *
     * 残りのZDDから最小/最大重みの集合を取得し、current_に格納する。
     */
    void updateCurrent();

public:
    /**
     * @brief 終端イテレータ用デフォルトコンストラクタ
     *
     * end()として使用される終端イテレータを生成する。
     */
    WeightIterator();

    /**
     * @brief 通常コンストラクタ
     * @param zdd 列挙対象のZDD
     * @param weights 各変数の重み（インデックス0は変数1の重み）
     * @param is_min trueの場合は昇順（最小重みから）、falseの場合は降順（最大重みから）
     */
    WeightIterator(const ZDD& zdd, const std::vector<int64_t>& weights, bool is_min);

    /**
     * @brief コピーコンストラクタ
     * @param other コピー元のイテレータ
     */
    WeightIterator(const WeightIterator& other);

    /**
     * @brief ムーブコンストラクタ
     * @param other ムーブ元のイテレータ
     */
    WeightIterator(WeightIterator&& other) noexcept;

    /**
     * @brief コピー代入演算子
     * @param other コピー元のイテレータ
     * @return 自身への参照
     */
    WeightIterator& operator=(const WeightIterator& other);

    /**
     * @brief ムーブ代入演算子
     * @param other ムーブ元のイテレータ
     * @return 自身への参照
     */
    WeightIterator& operator=(WeightIterator&& other) noexcept;

    /**
     * @brief デストラクタ
     *
     * 内部で保持しているZDDのコピーを解放する。
     */
    ~WeightIterator();

    /**
     * @brief 現在の集合を取得する（参照外し演算子）
     * @return 現在指している変数の集合
     */
    std::set<bddvar> operator*() const { return current_; }

    /**
     * @brief イテレータを次の集合に進める（前置インクリメント）
     *
     * 現在の集合をZDDから取り除き、次の最小/最大重みの集合に進む。
     *
     * @return インクリメント後の自身への参照
     */
    WeightIterator& operator++();

    /**
     * @brief イテレータを次の集合に進める（後置インクリメント）
     * @return インクリメント前のイテレータのコピー
     */
    WeightIterator operator++(int) {
        WeightIterator tmp = *this;
        ++(*this);
        return tmp;
    }

    /**
     * @brief 等価比較演算子
     * @param other 比較対象のイテレータ
     * @return 二つのイテレータが同じ状態にある場合はtrue
     */
    bool operator==(const WeightIterator& other) const;

    /**
     * @brief 不等価比較演算子
     * @param other 比較対象のイテレータ
     * @return 二つのイテレータが異なる状態にある場合はtrue
     */
    bool operator!=(const WeightIterator& other) const {
        return !(*this == other);
    }
};

/**
 * @brief ランダム順イテレータ
 *
 * ZDD内の集合をランダム順で重複なく列挙する入力イテレータ。
 * 内部的にZDDのコピーを保持し、各イテレーションでランダムに
 * サンプルした集合を取り出して取り除く。
 *
 * @code{.cpp}
 * std::mt19937 rng(42);
 * for (auto it = zdd.random_begin(rng); it != zdd.random_end(); ++it) {
 *     std::set<bddvar> s = *it;
 *     // ランダム順でsを処理
 * }
 * @endcode
 *
 * @see DictIterator, WeightIterator, get_uniformly_random_zdd
 */
class RandomIterator {
public:
    using iterator_category = std::input_iterator_tag;   ///< イテレータカテゴリ（入力イテレータ）
    using value_type = std::set<bddvar>;                 ///< 値型（変数の集合）
    using difference_type = std::ptrdiff_t;              ///< 差分型
    using pointer = const std::set<bddvar>*;             ///< ポインタ型
    using reference = const std::set<bddvar>&;           ///< 参照型

private:
    ZDD* remaining_;  // owned pointer to remaining ZDD
    std::mt19937* rng_;  // external RNG (not owned)
    std::set<bddvar> current_;

    /**
     * @brief 現在の集合を更新する
     *
     * 残りのZDDからランダムに一つの集合をサンプルし、current_に格納する。
     */
    void updateCurrent();

public:
    /**
     * @brief 終端イテレータ用デフォルトコンストラクタ
     *
     * end()として使用される終端イテレータを生成する。
     */
    RandomIterator();

    /**
     * @brief 通常コンストラクタ
     * @param zdd 列挙対象のZDD
     * @param rng 乱数生成器への参照（外部所有、イテレータより長い寿命が必要）
     */
    RandomIterator(const ZDD& zdd, std::mt19937& rng);

    /**
     * @brief コピーコンストラクタ
     * @param other コピー元のイテレータ
     */
    RandomIterator(const RandomIterator& other);

    /**
     * @brief ムーブコンストラクタ
     * @param other ムーブ元のイテレータ
     */
    RandomIterator(RandomIterator&& other) noexcept;

    /**
     * @brief コピー代入演算子
     * @param other コピー元のイテレータ
     * @return 自身への参照
     */
    RandomIterator& operator=(const RandomIterator& other);

    /**
     * @brief ムーブ代入演算子
     * @param other ムーブ元のイテレータ
     * @return 自身への参照
     */
    RandomIterator& operator=(RandomIterator&& other) noexcept;

    /**
     * @brief デストラクタ
     *
     * 内部で保持しているZDDのコピーを解放する。
     * 乱数生成器は外部所有のため解放しない。
     */
    ~RandomIterator();

    /**
     * @brief 現在の集合を取得する（参照外し演算子）
     * @return 現在指している変数の集合
     */
    std::set<bddvar> operator*() const { return current_; }

    /**
     * @brief イテレータを次の集合に進める（前置インクリメント）
     *
     * 現在の集合をZDDから取り除き、残りからランダムに次の集合をサンプルする。
     *
     * @return インクリメント後の自身への参照
     */
    RandomIterator& operator++();

    /**
     * @brief イテレータを次の集合に進める（後置インクリメント）
     * @return インクリメント前のイテレータのコピー
     */
    RandomIterator operator++(int) {
        RandomIterator tmp = *this;
        ++(*this);
        return tmp;
    }

    /**
     * @brief 等価比較演算子
     * @param other 比較対象のイテレータ
     * @return 二つのイテレータが同じ状態にある場合はtrue
     */
    bool operator==(const RandomIterator& other) const;

    /**
     * @brief 不等価比較演算子
     * @param other 比較対象のイテレータ
     * @return 二つのイテレータが異なる状態にある場合はtrue
     */
    bool operator!=(const RandomIterator& other) const {
        return !(*this == other);
    }
};

} // namespace sbdd2

#endif // SBDD2_ZDD_ITERATORS_HPP
