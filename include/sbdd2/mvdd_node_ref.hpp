/**
 * @file mvdd_node_ref.hpp
 * @brief SAPPOROBDD 2.0 - MVDD Node Reference (read-only, no ref counting)
 * @author SAPPOROBDD Team
 * @copyright MIT License
 *
 * MVBDD/MVZDD 共通の軽量ノード参照クラス。
 * 参照カウントを管理しないため、トラバーサル時のパフォーマンスに優れます。
 */

#ifndef SBDD2_MVDD_NODE_REF_HPP
#define SBDD2_MVDD_NODE_REF_HPP

#include "types.hpp"
#include "dd_node_ref.hpp"
#include "mvdd_base.hpp"

namespace sbdd2 {

// Forward declarations
class MVBDD;
class MVZDD;

/**
 * @brief MVDD Node Reference - 読み取り専用のMVDDノード参照
 *
 * 参照カウントを管理しない軽量参照クラスです。
 * MVBDD と MVZDD の両方で共通して使用できます。
 *
 * @warning 元の MVBDD/MVZDD より長く生存してはいけません。
 *
 * @code{.cpp}
 * MVZDD mvzdd = ...;
 * MVDDNodeRef ref = mvzdd.root_ref();
 *
 * if (!ref.is_terminal()) {
 *     bddvar mv = ref.mvdd_var();
 *     for (int v = 0; v < ref.k(); ++v) {
 *         MVDDNodeRef child = ref.child(v);
 *         // ...
 *     }
 * }
 * @endcode
 */
class MVDDNodeRef {
public:
    /// @name コンストラクタ
    /// @{

    /// デフォルトコンストラクタ（無効な参照を作成）
    MVDDNodeRef() : manager_(nullptr), arc_(), var_table_(nullptr) {}

    /**
     * @brief Arc、マネージャー、変数テーブルから構築
     * @param manager DDマネージャーへのポインタ
     * @param arc 内部DD Arc
     * @param var_table MVDD変数テーブルへのポインタ
     */
    MVDDNodeRef(DDManager* manager, Arc arc, const MVDDVarTable* var_table)
        : manager_(manager), arc_(arc), var_table_(var_table) {}

    /// コピーコンストラクタ
    MVDDNodeRef(const MVDDNodeRef&) = default;

    /// コピー代入演算子
    MVDDNodeRef& operator=(const MVDDNodeRef&) = default;

    /// @}

    /// @name 有効性チェック
    /// @{

    /// 有効かどうか
    bool is_valid() const { return manager_ != nullptr && var_table_ != nullptr; }

    /// bool への明示的変換
    explicit operator bool() const { return is_valid(); }

    /// @}

    /// @name 終端判定
    /// @{

    /// 終端ノードかどうか
    bool is_terminal() const { return arc_.is_constant(); }

    /// 0終端かどうか
    bool is_terminal_zero() const { return arc_ == ARC_TERMINAL_0; }

    /// 1終端かどうか
    bool is_terminal_one() const { return arc_ == ARC_TERMINAL_1; }

    /**
     * @brief 終端の値を取得
     * @return 終端の値（is_terminal() が true の場合のみ有効）
     * @note 否定辺を考慮した論理値を返します
     */
    bool terminal_value() const {
        return arc_.terminal_value() != arc_.is_negated();
    }

    /// @}

    /// @name 変数アクセス
    /// @{

    /**
     * @brief トップの MVDD 変数番号を取得
     * @return MVDD 変数番号（終端の場合は 0）
     */
    bddvar mvdd_var() const;

    /**
     * @brief 値域サイズ k を取得
     * @return 値域サイズ
     */
    int k() const { return var_table_ ? var_table_->k() : 0; }

    /// @}

    /// @name 子ノードアクセス
    /// @{

    /**
     * @brief 指定値に対応する子を取得
     * @param value 値 (0 to k-1)
     * @return 子ノードへの参照
     * @note 終端ノードの場合は自身を返します
     */
    MVDDNodeRef child(int value) const;

    /// @}

    /// @name 変換
    /// @{

    /**
     * @brief MVBDD に変換（参照カウントが増加）
     * @return MVBDD
     * @warning 元が MVZDD の場合、結果は未定義です
     */
    MVBDD to_mvbdd() const;

    /**
     * @brief MVZDD に変換（参照カウントが増加）
     * @return MVZDD
     * @warning 元が MVBDD の場合、結果は未定義です
     */
    MVZDD to_mvzdd() const;

    /// @}

    /// @name 内部アクセス
    /// @{

    /**
     * @brief 内部の DDNodeRef を取得
     * @return DDNodeRef
     */
    DDNodeRef dd_node_ref() const {
        return DDNodeRef(manager_, arc_);
    }

    /// 内部 Arc を取得
    Arc arc() const { return arc_; }

    /// DDマネージャーを取得
    DDManager* manager() const { return manager_; }

    /// 変数テーブルを取得
    const MVDDVarTable* var_table() const { return var_table_; }

    /// @}

    /// @name 比較演算子
    /// @{

    bool operator==(const MVDDNodeRef& other) const {
        return manager_ == other.manager_ && arc_ == other.arc_;
    }

    bool operator!=(const MVDDNodeRef& other) const {
        return !(*this == other);
    }

    /// @}

private:
    DDManager* manager_;            ///< DDマネージャー
    Arc arc_;                       ///< 内部 DD Arc
    const MVDDVarTable* var_table_; ///< MVDD変数テーブル

    /// 内部ノードポインタを取得
    const DDNode* node_ptr() const;

    /// 内部DD変数番号を取得（内部用）
    bddvar dd_var() const;
};

} // namespace sbdd2

#endif // SBDD2_MVDD_NODE_REF_HPP
