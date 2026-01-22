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
 * ZDD内の集合を辞書順で列挙します。
 * O(1)のメモリオーバーヘッドで効率的に列挙できます。
 *
 * @code{.cpp}
 * ZDD zdd = ...;
 * for (auto it = zdd.dict_begin(); it != zdd.dict_end(); ++it) {
 *     std::set<bddvar> s = *it;
 *     // sを処理
 * }
 * @endcode
 */
class DictIterator {
public:
    using iterator_category = std::input_iterator_tag;
    using value_type = std::set<bddvar>;
    using difference_type = std::ptrdiff_t;
    using pointer = const std::set<bddvar>*;
    using reference = const std::set<bddvar>&;

private:
    const ZDD* zdd_;
    int64_t current_;
    int64_t count_;
    bool reverse_;
    bool is_end_;
    mutable std::set<bddvar> cached_value_;

public:
    /// 終端イテレータ用コンストラクタ
    DictIterator() : zdd_(nullptr), current_(0), count_(0), reverse_(false), is_end_(true) {}

    /// 通常コンストラクタ（beginイテレータ用）
    DictIterator(const ZDD* zdd, int64_t count, bool reverse);

    /// 終端判定
    bool at_end() const;

    /// 参照外し
    std::set<bddvar> operator*() const;

    /// 前置インクリメント
    DictIterator& operator++();

    /// 後置インクリメント
    DictIterator operator++(int) {
        DictIterator tmp = *this;
        ++(*this);
        return tmp;
    }

    /// 等価比較
    bool operator==(const DictIterator& other) const {
        // Both at end = equal, neither at end = compare current_
        if (at_end() && other.at_end()) return true;
        if (at_end() != other.at_end()) return false;
        return current_ == other.current_ && reverse_ == other.reverse_;
    }

    /// 不等価比較
    bool operator!=(const DictIterator& other) const {
        return !(*this == other);
    }
};

/**
 * @brief 重み順イテレータ
 *
 * ZDD内の集合を重み順（昇順または降順）で列挙します。
 * 内部的にZDDのコピーを保持し、各イテレーションで現在の最小/最大集合を取り除きます。
 *
 * @code{.cpp}
 * std::vector<int64_t> weights = {0, 1, 2, 3};
 * for (auto it = zdd.weight_min_begin(weights); it != zdd.weight_min_end(); ++it) {
 *     std::set<bddvar> s = *it;
 *     // 重み昇順でsを処理
 * }
 * @endcode
 */
class WeightIterator {
public:
    using iterator_category = std::input_iterator_tag;
    using value_type = std::set<bddvar>;
    using difference_type = std::ptrdiff_t;
    using pointer = const std::set<bddvar>*;
    using reference = const std::set<bddvar>&;

private:
    ZDD* remaining_;  // owned pointer to remaining ZDD
    std::vector<int64_t> weights_;
    bool is_min_;
    std::set<bddvar> current_;

    void updateCurrent();

public:
    /// 終端イテレータ用コンストラクタ
    WeightIterator();

    /// 通常コンストラクタ
    WeightIterator(const ZDD& zdd, const std::vector<int64_t>& weights, bool is_min);

    /// コピーコンストラクタ
    WeightIterator(const WeightIterator& other);

    /// ムーブコンストラクタ
    WeightIterator(WeightIterator&& other) noexcept;

    /// コピー代入
    WeightIterator& operator=(const WeightIterator& other);

    /// ムーブ代入
    WeightIterator& operator=(WeightIterator&& other) noexcept;

    /// デストラクタ
    ~WeightIterator();

    /// 参照外し
    std::set<bddvar> operator*() const { return current_; }

    /// 前置インクリメント
    WeightIterator& operator++();

    /// 後置インクリメント
    WeightIterator operator++(int) {
        WeightIterator tmp = *this;
        ++(*this);
        return tmp;
    }

    /// 等価比較
    bool operator==(const WeightIterator& other) const;

    /// 不等価比較
    bool operator!=(const WeightIterator& other) const {
        return !(*this == other);
    }
};

/**
 * @brief ランダム順イテレータ
 *
 * ZDD内の集合をランダム順で列挙します（重複なし）。
 * 内部的にZDDのコピーを保持し、各イテレーションでサンプルした集合を取り除きます。
 *
 * @code{.cpp}
 * std::mt19937 rng(42);
 * for (auto it = zdd.random_begin(rng); it != zdd.random_end(); ++it) {
 *     std::set<bddvar> s = *it;
 *     // ランダム順でsを処理
 * }
 * @endcode
 */
class RandomIterator {
public:
    using iterator_category = std::input_iterator_tag;
    using value_type = std::set<bddvar>;
    using difference_type = std::ptrdiff_t;
    using pointer = const std::set<bddvar>*;
    using reference = const std::set<bddvar>&;

private:
    ZDD* remaining_;  // owned pointer to remaining ZDD
    std::mt19937* rng_;  // external RNG (not owned)
    std::set<bddvar> current_;

    void updateCurrent();

public:
    /// 終端イテレータ用コンストラクタ
    RandomIterator();

    /// 通常コンストラクタ
    RandomIterator(const ZDD& zdd, std::mt19937& rng);

    /// コピーコンストラクタ
    RandomIterator(const RandomIterator& other);

    /// ムーブコンストラクタ
    RandomIterator(RandomIterator&& other) noexcept;

    /// コピー代入
    RandomIterator& operator=(const RandomIterator& other);

    /// ムーブ代入
    RandomIterator& operator=(RandomIterator&& other) noexcept;

    /// デストラクタ
    ~RandomIterator();

    /// 参照外し
    std::set<bddvar> operator*() const { return current_; }

    /// 前置インクリメント
    RandomIterator& operator++();

    /// 後置インクリメント
    RandomIterator operator++(int) {
        RandomIterator tmp = *this;
        ++(*this);
        return tmp;
    }

    /// 等価比較
    bool operator==(const RandomIterator& other) const;

    /// 不等価比較
    bool operator!=(const RandomIterator& other) const {
        return !(*this == other);
    }
};

} // namespace sbdd2

#endif // SBDD2_ZDD_ITERATORS_HPP
