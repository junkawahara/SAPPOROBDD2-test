/**
 * @file dd_node_ref.hpp
 * @brief DDノードの読み取り専用参照 DDNodeRef の定義
 * @copyright MIT License
 *
 * DDの木構造を走査するための軽量な読み取り専用参照を提供します。
 * 参照カウントを管理しないため、高速な走査が可能ですが、
 * 元のBDD/ZDDより長く生存させてはいけません。
 */

// SAPPOROBDD 2.0 - DD Node Reference (read-only, no ref counting)
// MIT License

#ifndef SBDD2_DD_NODE_REF_HPP
#define SBDD2_DD_NODE_REF_HPP

#include "types.hpp"
#include "dd_node.hpp"
#include "dd_manager.hpp"

namespace sbdd2 {

// Forward declarations
class BDD;
class ZDD;

/**
 * @brief DDノードへの読み取り専用参照
 *
 * DDの木構造を走査するための軽量な参照クラスです。
 * 参照カウントを管理しないため、走査中のオーバーヘッドがありません。
 *
 * @warning 元のBDD/ZDDオブジェクトより長く生存させないでください。
 *          元のオブジェクトが破棄されると、この参照は無効になります。
 *
 * @see DDBase::ref()
 * @see DDNode
 * @see BDD
 * @see ZDD
 */
class DDNodeRef {
public:
    /**
     * @brief デフォルトコンストラクタ（無効な参照を生成）
     */
    DDNodeRef() : manager_(nullptr), arc_() {}

    /**
     * @brief マネージャとArcを指定するコンストラクタ
     * @param manager DDマネージャへのポインタ
     * @param arc ノードを指すArc（辺）
     */
    DDNodeRef(DDManager* manager, Arc arc)
        : manager_(manager), arc_(arc) {}

    /// @brief コピーコンストラクタ（デフォルト）
    DDNodeRef(const DDNodeRef&) = default;
    /// @brief コピー代入演算子（デフォルト）
    DDNodeRef& operator=(const DDNodeRef&) = default;

    /**
     * @brief 参照が有効かどうかを判定する
     * @return マネージャが設定されていれば true
     */
    bool is_valid() const { return manager_ != nullptr; }

    /**
     * @brief bool への明示的変換演算子
     * @return is_valid() の結果
     * @see is_valid()
     */
    explicit operator bool() const { return is_valid(); }

    /**
     * @brief 終端ノードかどうかを判定する
     * @return 終端ノードであれば true
     * @see is_terminal_zero()
     * @see is_terminal_one()
     */
    bool is_terminal() const { return arc_.is_constant(); }

    /**
     * @brief 0終端かどうかを判定する
     * @return 0終端であれば true
     * @see is_terminal_one()
     * @see is_terminal()
     */
    bool is_terminal_zero() const { return arc_ == ARC_TERMINAL_0; }

    /**
     * @brief 1終端かどうかを判定する
     * @return 1終端であれば true
     * @see is_terminal_zero()
     * @see is_terminal()
     */
    bool is_terminal_one() const { return arc_ == ARC_TERMINAL_1; }

    /**
     * @brief 終端の値を取得する
     * @return 終端値（否定辺を考慮した論理値）
     *
     * is_terminal() が true の場合にのみ有効です。
     * 否定辺が付いている場合は値が反転されます。
     *
     * @see is_terminal()
     */
    bool terminal_value() const {
        return arc_.terminal_value() != arc_.is_negated();
    }

    /**
     * @brief 変数番号を取得する
     * @return このノードの変数番号
     *
     * 終端ノードの場合は未定義です。
     */
    bddvar var() const;

    /**
     * @brief ノードが簡約済みかどうかを判定する
     * @return 簡約済みであれば true
     */
    bool is_reduced() const;

    /**
     * @brief 生の0-child（否定辺を無視）を取得する
     * @return 0-child への DDNodeRef
     *
     * ノードに格納されている0-arcをそのまま返します。
     * 否定辺の影響は考慮しません。
     *
     * @see child0()
     * @see raw_child1()
     */
    DDNodeRef raw_child0() const;

    /**
     * @brief 生の1-child（否定辺を無視）を取得する
     * @return 1-child への DDNodeRef
     *
     * ノードに格納されている1-arcをそのまま返します。
     * 否定辺の影響は考慮しません。
     *
     * @see child1()
     * @see raw_child0()
     */
    DDNodeRef raw_child1() const;

    /**
     * @brief 論理的な0-child（否定辺を考慮）を取得する
     * @return 0-child への DDNodeRef
     *
     * このノードへの辺に否定が付いている場合、子の否定フラグを反転します。
     *
     * @see raw_child0()
     * @see child1()
     */
    DDNodeRef child0() const;

    /**
     * @brief 論理的な1-child（否定辺を考慮）を取得する
     * @return 1-child への DDNodeRef
     *
     * このノードへの辺に否定が付いている場合、子の否定フラグを反転します。
     *
     * @see raw_child1()
     * @see child0()
     */
    DDNodeRef child1() const;

    /**
     * @brief 0-child への辺が否定されているかを判定する
     * @return 否定されていれば true
     * @see is_child1_negated()
     */
    bool is_child0_negated() const;

    /**
     * @brief 1-child への辺が否定されているかを判定する
     * @return 否定されていれば true
     * @see is_child0_negated()
     */
    bool is_child1_negated() const;

    /**
     * @brief この参照自体が否定されているかを判定する
     * @return 否定辺であれば true
     * @see positive()
     * @see negated()
     */
    bool is_negated() const { return arc_.is_negated(); }

    /**
     * @brief 否定を除去したバージョンを取得する
     * @return 否定フラグを除去した DDNodeRef
     * @see negated()
     * @see is_negated()
     */
    DDNodeRef positive() const;

    /**
     * @brief 否定したバージョンを取得する
     * @return 否定フラグを反転した DDNodeRef
     * @see positive()
     * @see is_negated()
     */
    DDNodeRef negated() const;

    /**
     * @brief BDD に変換する（参照カウントをインクリメント）
     * @return このノードを根とするBDD
     * @see to_zdd()
     * @see BDD
     */
    BDD to_bdd() const;

    /**
     * @brief ZDD に変換する（参照カウントをインクリメント）
     * @return このノードを根とするZDD
     * @see to_bdd()
     * @see ZDD
     */
    ZDD to_zdd() const;

    /**
     * @brief 内部のArc（辺）を取得する
     * @return このノード参照のArc
     * @see Arc
     */
    Arc arc() const { return arc_; }

    /**
     * @brief DDマネージャへのポインタを取得する
     * @return DDManagerへのポインタ
     * @see DDManager
     */
    DDManager* manager() const { return manager_; }

    /**
     * @brief 等価比較演算子
     * @param other 比較対象の DDNodeRef
     * @return 同一のマネージャかつ同一のArcであれば true
     */
    bool operator==(const DDNodeRef& other) const {
        return manager_ == other.manager_ && arc_ == other.arc_;
    }

    /**
     * @brief 非等価比較演算子
     * @param other 比較対象の DDNodeRef
     * @return 等価でなければ true
     */
    bool operator!=(const DDNodeRef& other) const {
        return !(*this == other);
    }

    /**
     * @brief ノードインデックスを取得する（デバッグ・シリアライズ用）
     * @return Arcのインデックス値
     */
    bddindex index() const { return arc_.index(); }

private:
    DDManager* manager_;  ///< DDマネージャへのポインタ
    Arc arc_;             ///< ノードを指すArc（辺）

    /**
     * @brief 実際のノードへのポインタを取得する（内部使用）
     * @return DDNodeへのconstポインタ
     */
    const DDNode* node_ptr() const;
};

} // namespace sbdd2

#endif // SBDD2_DD_NODE_REF_HPP
