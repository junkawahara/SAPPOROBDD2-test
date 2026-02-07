/**
 * @file dd_node.hpp
 * @brief 128ビット DDノード構造体 DDNode の定義
 * @copyright MIT License
 *
 * DDの各ノードを表す128ビット構造体を定義します。
 * 0-arc、1-arc、変数番号、参照カウント、簡約フラグを
 * 2つの64ビットワードにパックして格納します。
 */

// SAPPOROBDD 2.0 - DD Node structure
// MIT License

#ifndef SBDD2_DD_NODE_HPP
#define SBDD2_DD_NODE_HPP

#include "types.hpp"
#include <atomic>
#include <cassert>

namespace sbdd2 {

/**
 * @brief 128ビット DDノード構造体
 *
 * DDの各ノードを表現する128ビット（16バイト）の構造体です。
 * 2つの64ビットワードに以下の情報をパックして格納します。
 *
 * Word 0 (low, 64ビット):
 *   - ビット 0-43:  0-arc（44ビット）
 *   - ビット 44-63: 1-arc の下位20ビット
 *
 * Word 1 (high, 64ビット):
 *   - ビット 0-23:  1-arc の上位24ビット
 *   - ビット 24:    簡約フラグ
 *   - ビット 25-40: 参照カウント（16ビット）
 *   - ビット 41-60: 変数番号（20ビット）
 *   - ビット 61-63: 予約（3ビット）
 *
 * @see DDManager
 * @see DDNodeRef
 * @see Arc
 */
class DDNode {
public:
    /// @name Word 0 用のマスクとシフト量
    /// @{
    static constexpr std::uint64_t ARC0_MASK = (1ULL << 44) - 1;          ///< 0-arc用ビットマスク（44ビット）
    static constexpr int ARC1_LOW_SHIFT = 44;                              ///< 1-arc下位ビットのシフト量
    static constexpr std::uint64_t ARC1_LOW_MASK = (1ULL << 20) - 1;      ///< 1-arc下位20ビット用マスク
    /// @}

    /// @name Word 1 用のマスクとシフト量
    /// @{
    static constexpr int ARC1_HIGH_BITS = 24;                              ///< 1-arc上位ビット数
    static constexpr std::uint64_t ARC1_HIGH_MASK = (1ULL << 24) - 1;     ///< 1-arc上位24ビット用マスク
    static constexpr int REDUCED_SHIFT = 24;                               ///< 簡約フラグのシフト量
    static constexpr int REFCOUNT_SHIFT = 25;                              ///< 参照カウントのシフト量
    static constexpr std::uint64_t REFCOUNT_MASK = 0xFFFFULL;             ///< 参照カウント用マスク（16ビット）
    static constexpr int VAR_SHIFT = 41;                                   ///< 変数番号のシフト量
    static constexpr std::uint64_t VAR_MASK = (1ULL << 20) - 1;           ///< 変数番号用マスク（20ビット）
    /// @}

private:
    std::uint64_t low_;   ///< Word 0: 0-arc + 1-arc下位ビット
    std::uint64_t high_;  ///< Word 1: 1-arc上位ビット + フラグ + 参照カウント + 変数番号

public:
    /**
     * @brief デフォルトコンストラクタ（全ビットを0で初期化）
     */
    DDNode() : low_(0), high_(0) {}

    /**
     * @brief 全フィールドを指定するコンストラクタ
     * @param arc0 0-arc（低枝）
     * @param arc1 1-arc（高枝）
     * @param var 変数番号
     * @param reduced 簡約済みフラグ（デフォルト: false）
     * @param refcount 初期参照カウント（デフォルト: 0）
     */
    DDNode(Arc arc0, Arc arc1, bddvar var, bool reduced = false, bddrefcount refcount = 0)
        : low_(0), high_(0)
    {
        set_arc0(arc0);
        set_arc1(arc1);
        set_var(var);
        set_reduced(reduced);
        set_refcount_raw(refcount);
    }

    /**
     * @brief 0-arc（低枝）を取得する
     * @return 0-arcを表すArc
     * @see set_arc0()
     */
    Arc arc0() const {
        return Arc(low_ & ARC0_MASK);
    }

    /**
     * @brief 0-arc（低枝）を設定する
     * @param arc 設定する0-arc
     * @see arc0()
     */
    void set_arc0(Arc arc) {
        low_ = (low_ & ~ARC0_MASK) | (arc.data & ARC0_MASK);
    }

    /**
     * @brief 1-arc（高枝）を取得する
     * @return 1-arcを表すArc
     *
     * Word 0 の上位20ビットと Word 1 の下位24ビットを結合して復元します。
     *
     * @see set_arc1()
     */
    Arc arc1() const {
        std::uint64_t lo = (low_ >> ARC1_LOW_SHIFT) & ARC1_LOW_MASK;
        std::uint64_t hi = high_ & ARC1_HIGH_MASK;
        return Arc((hi << 20) | lo);
    }

    /**
     * @brief 1-arc（高枝）を設定する
     * @param arc 設定する1-arc
     *
     * Word 0 の上位20ビットと Word 1 の下位24ビットに分割して格納します。
     *
     * @see arc1()
     */
    void set_arc1(Arc arc) {
        std::uint64_t lo = arc.data & ARC1_LOW_MASK;
        std::uint64_t hi = (arc.data >> 20) & ARC1_HIGH_MASK;
        low_ = (low_ & ARC0_MASK) | (lo << ARC1_LOW_SHIFT);
        high_ = (high_ & ~ARC1_HIGH_MASK) | hi;
    }

    /**
     * @brief 変数番号を取得する
     * @return このノードの変数番号
     * @see set_var()
     */
    bddvar var() const {
        return static_cast<bddvar>((high_ >> VAR_SHIFT) & VAR_MASK);
    }

    /**
     * @brief 変数番号を設定する
     * @param v 設定する変数番号（BDDVAR_MAX以下であること）
     * @see var()
     */
    void set_var(bddvar v) {
        assert(v <= BDDVAR_MAX);
        high_ = (high_ & ~(VAR_MASK << VAR_SHIFT)) | (static_cast<std::uint64_t>(v) << VAR_SHIFT);
    }

    /**
     * @brief 簡約済みかどうかを判定する
     * @return 簡約済みであれば true
     * @see set_reduced()
     */
    bool is_reduced() const {
        return (high_ & (1ULL << REDUCED_SHIFT)) != 0;
    }

    /**
     * @brief 簡約フラグを設定する
     * @param r 簡約済みの場合 true
     * @see is_reduced()
     */
    void set_reduced(bool r) {
        if (r) {
            high_ |= (1ULL << REDUCED_SHIFT);
        } else {
            high_ &= ~(1ULL << REDUCED_SHIFT);
        }
    }

    /**
     * @brief 参照カウントを取得する
     * @return 現在の参照カウント値
     * @see inc_refcount()
     * @see dec_refcount()
     */
    bddrefcount refcount() const {
        return static_cast<bddrefcount>((high_ >> REFCOUNT_SHIFT) & REFCOUNT_MASK);
    }

    /**
     * @brief 参照カウントを直接設定する（非アトミック、初期化用）
     * @param count 設定する参照カウント値
     *
     * 初期化時にのみ使用してください。通常は inc_refcount() / dec_refcount() を使用します。
     *
     * @see inc_refcount()
     * @see dec_refcount()
     */
    void set_refcount_raw(bddrefcount count) {
        high_ = (high_ & ~(REFCOUNT_MASK << REFCOUNT_SHIFT)) |
                (static_cast<std::uint64_t>(count) << REFCOUNT_SHIFT);
    }

    /**
     * @brief 参照カウントをインクリメントする
     *
     * 参照カウントが最大値（BDDREFCOUNT_MAX）に達した場合は飽和し、増加しません。
     *
     * @see dec_refcount()
     * @see refcount()
     */
    void inc_refcount() {
        bddrefcount current = refcount();
        if (current < BDDREFCOUNT_MAX) {
            set_refcount_raw(current + 1);
        }
    }

    /**
     * @brief 参照カウントをデクリメントする
     * @return カウントが0になった場合 true
     *
     * 参照カウントが0または最大値（飽和状態）の場合はデクリメントしません。
     *
     * @see inc_refcount()
     * @see refcount()
     */
    bool dec_refcount() {
        bddrefcount current = refcount();
        if (current > 0 && current < BDDREFCOUNT_MAX) {
            set_refcount_raw(current - 1);
            return (current - 1) == 0;
        }
        return false;
    }

    /**
     * @brief ノードが使用中かどうかを判定する
     * @return 参照カウントが0より大きければ true
     */
    bool is_alive() const {
        return refcount() > 0;
    }

    /**
     * @brief Word 0 の生データを取得する（ハッシュ・シリアライズ用）
     * @return Word 0 の64ビット値
     * @see raw_high()
     * @see set_raw()
     */
    std::uint64_t raw_low() const { return low_; }

    /**
     * @brief Word 1 の生データを取得する（ハッシュ・シリアライズ用）
     * @return Word 1 の64ビット値
     * @see raw_low()
     * @see set_raw()
     */
    std::uint64_t raw_high() const { return high_; }

    /**
     * @brief 生データを直接設定する
     * @param lo Word 0 に設定する64ビット値
     * @param hi Word 1 に設定する64ビット値
     * @see raw_low()
     * @see raw_high()
     */
    void set_raw(std::uint64_t lo, std::uint64_t hi) {
        low_ = lo;
        high_ = hi;
    }

    /**
     * @brief スロットが空（未使用）かどうかを判定する
     * @return 両ワードが0であれば true
     * @see is_tombstone()
     */
    bool is_empty() const {
        return low_ == 0 && high_ == 0;
    }

    /**
     * @brief スロットがトゥームストーン（削除済み）かどうかを判定する
     * @return トゥームストーンマーカーであれば true
     *
     * トゥームストーンは low = ~0, high = 0 の特殊マーカーで表されます。
     *
     * @see mark_tombstone()
     * @see is_empty()
     */
    bool is_tombstone() const {
        // Use special marker: low = -1, high = 0
        return low_ == ~0ULL && high_ == 0;
    }

    /**
     * @brief トゥームストーン（削除済み）としてマークする
     *
     * ハッシュテーブルからノードを削除した際に、スロットを削除済みとしてマークします。
     *
     * @see is_tombstone()
     */
    void mark_tombstone() {
        low_ = ~0ULL;
        high_ = 0;
    }

    /**
     * @brief ノードをクリアする（全ビットを0にリセット）
     */
    void clear() {
        low_ = 0;
        high_ = 0;
    }

    /**
     * @brief 指定したフィールド値と一致するかを判定する（ハッシュテーブル用）
     * @param arc0 比較する0-arc
     * @param arc1 比較する1-arc
     * @param v 比較する変数番号
     * @return すべてのフィールドが一致すれば true
     */
    bool equals(Arc arc0, Arc arc1, bddvar v) const {
        return this->arc0() == arc0 && this->arc1() == arc1 && var() == v;
    }
};

static_assert(sizeof(DDNode) == 16, "DDNode must be 128 bits (16 bytes)");

} // namespace sbdd2

#endif // SBDD2_DD_NODE_HPP
