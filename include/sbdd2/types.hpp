/**
 * @file types.hpp
 * @brief SAPPOROBDD 2.0 - 基本型定義
 * @author SAPPOROBDD Team
 * @copyright MIT License
 *
 * このファイルはSAPPOROBDD 2.0で使用される基本的な型を定義します。
 */

#ifndef SBDD2_TYPES_HPP
#define SBDD2_TYPES_HPP

#include <cstdint>
#include <cstddef>

namespace sbdd2 {

/**
 * @brief 変数番号型
 *
 * 1から開始、最大値は2^20 - 1 = 1,048,575
 */
using bddvar = std::uint32_t;

/**
 * @brief ノードインデックス型
 *
 * 最大2^42個のノードを表現可能（約4.4兆個）
 */
using bddindex = std::uint64_t;

/**
 * @brief 参照カウント型
 *
 * 最大65535、飽和後は増減しない
 */
using bddrefcount = std::uint16_t;

/**
 * @name 最大値定数
 * @{
 */
constexpr bddvar BDDVAR_MAX = (1U << 20) - 1;       ///< 変数番号の最大値
constexpr bddindex BDDINDEX_MAX = (1ULL << 42) - 1; ///< ノードインデックスの最大値
constexpr bddrefcount BDDREFCOUNT_MAX = 65535;      ///< 参照カウントの最大値
/** @} */

/// 無効な変数番号を表す定数
constexpr bddvar BDDVAR_INVALID = 0;

/**
 * @brief アーク型（44ビット）
 *
 * ノード間の枝を表現する構造体。64ビット整数にパックされています。
 *
 * ビットレイアウト:
 * - bit 0: 否定フラグ
 * - bit 1: 定数フラグ（終端ノードかどうか）
 * - bits 2-43: ノードインデックス（42ビット）
 *
 * @code{.cpp}
 * // 終端ノード0へのアーク
 * Arc zero = Arc::terminal(false);
 *
 * // 終端ノード1へのアーク
 * Arc one = Arc::terminal(true);
 *
 * // ノードインデックス100への通常アーク
 * Arc node_arc = Arc::node(100);
 *
 * // ノードインデックス100への否定アーク
 * Arc neg_arc = Arc::node(100, true);
 * @endcode
 */
struct Arc {
    std::uint64_t data;  ///< パックされたアークデータ

    /// デフォルトコンストラクタ（ゼロ初期化）
    Arc() : data(0) {}

    /// 生データからの構築
    explicit Arc(std::uint64_t d) : data(d) {}

    /**
     * @brief 終端ノードへのアークを作成
     * @param value 終端値（false=0, true=1）
     * @return 終端ノードを指すアーク
     */
    static Arc terminal(bool value) {
        // constant flag = 1 (bit 1), index = value (0 or 1)
        return Arc((value ? 1ULL : 0ULL) << 2 | 0x2ULL);
    }

    /**
     * @brief 通常ノードへのアークを作成
     * @param index ノードインデックス
     * @param negated 否定フラグ（デフォルト: false）
     * @return ノードを指すアーク
     */
    static Arc node(bddindex index, bool negated = false) {
        return Arc((index << 2) | (negated ? 1ULL : 0ULL));
    }

    /// @name アクセサ
    /// @{

    /// 否定フラグを取得
    bool is_negated() const { return (data & 1ULL) != 0; }

    /// 定数（終端）フラグを取得
    bool is_constant() const { return (data & 2ULL) != 0; }

    /// ノードインデックスを取得
    bddindex index() const { return data >> 2; }

    /**
     * @brief 終端値を取得
     * @pre is_constant() == true
     * @return 終端値（0または1）
     */
    bool terminal_value() const { return index() != 0; }

    /// @}

    /**
     * @brief 否定したアークを返す
     * @return 否定フラグを反転したアーク
     */
    Arc negated() const { return Arc(data ^ 1ULL); }

    /// @name 比較演算子
    /// @{
    bool operator==(const Arc& other) const { return data == other.data; }
    bool operator!=(const Arc& other) const { return data != other.data; }
    /// @}

    /// @name プレースホルダ Arc（TdZdd 移植用）
    /// @{

    /**
     * @brief プレースホルダ Arc を作成
     *
     * top-down DD 構築時に、子ノードがまだ作成されていない場合に使用。
     * 後から resolve_placeholder() で実際の Arc に解決する。
     *
     * ビットレイアウト:
     * - bit 0: 0 (negation なし)
     * - bit 1: 0 (非 terminal)
     * - bit 2: 1 (プレースホルダフラグ)
     * - bits 3-22: level (20 bits)
     * - bits 23-63: col (41 bits)
     *
     * @param level ノードの level (1 以上)
     * @param col そのレベル内での列番号
     * @return プレースホルダ Arc
     */
    static Arc placeholder(int level, std::uint64_t col) {
        // bit 2 = 1 (placeholder flag)
        // bits 3-22 = level (20 bits)
        // bits 23-63 = col (41 bits)
        return Arc(0x4ULL | (static_cast<std::uint64_t>(level) << 3) | (col << 23));
    }

    /**
     * @brief プレースホルダ Arc かどうか判定
     * @return プレースホルダなら true
     */
    bool is_placeholder() const {
        // bit 0 = 0, bit 1 = 0, bit 2 = 1
        return (data & 0x7ULL) == 0x4ULL;
    }

    /**
     * @brief プレースホルダの level を取得
     * @pre is_placeholder() == true
     * @return level (1 以上)
     */
    int placeholder_level() const {
        return static_cast<int>((data >> 3) & 0xFFFFFULL);  // 20 bits
    }

    /**
     * @brief プレースホルダの col を取得
     * @pre is_placeholder() == true
     * @return col (0 以上)
     */
    std::uint64_t placeholder_col() const {
        return data >> 23;  // 41 bits
    }

    /// @}
};

/// 終端0を指すアーク定数
const Arc ARC_TERMINAL_0 = Arc::terminal(false);

/// 終端1を指すアーク定数
const Arc ARC_TERMINAL_1 = Arc::terminal(true);

} // namespace sbdd2

#endif // SBDD2_TYPES_HPP
