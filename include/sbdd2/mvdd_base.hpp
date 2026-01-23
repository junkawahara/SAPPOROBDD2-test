/**
 * @file mvdd_base.hpp
 * @brief SAPPOROBDD 2.0 - Multi-Valued DD 共通基底クラス
 * @author SAPPOROBDD Team
 * @copyright MIT License
 *
 * MVBDD と MVZDD の共通機能を提供します。
 * 多値変数を扱う Decision Diagram を、既存の BDD/ZDD でエミュレートして実装。
 */

#ifndef SBDD2_MVDD_BASE_HPP
#define SBDD2_MVDD_BASE_HPP

#include "types.hpp"
#include "exception.hpp"
#include <vector>
#include <memory>

namespace sbdd2 {

// Forward declaration
class DDManager;

/**
 * @brief MVDD変数マッピング情報
 *
 * 1つのMVDD変数に対応する内部DD変数群の情報を保持します。
 * MVDD変数 x (k値) は、内部的に k-1 個のDD変数 (x_1, x_2, ..., x_{k-1}) で表現されます。
 */
struct MVDDVarInfo {
    bddvar mvdd_var;               ///< MVDD変数番号 (1-indexed)
    int k;                         ///< この変数の値域サイズ (k値)
    std::vector<bddvar> dd_vars;   ///< 対応する内部DD変数番号のリスト (k-1個)

    /// デフォルトコンストラクタ
    MVDDVarInfo() : mvdd_var(0), k(0) {}

    /// 値を指定して構築
    MVDDVarInfo(bddvar mv, int kval, const std::vector<bddvar>& dvars)
        : mvdd_var(mv), k(kval), dd_vars(dvars) {}
};

/**
 * @brief MVDD変数マッピングテーブル
 *
 * 複数のMVDDオブジェクト間で共有される変数マッピング情報を管理します。
 * shared_ptrで共有されることを想定しています。
 */
class MVDDVarTable {
private:
    int k_;                                  ///< 値域サイズ (全変数共通)
    std::vector<MVDDVarInfo> var_map_;       ///< MVDD変数 -> 内部DD変数マッピング
    std::vector<bddvar> dd_to_mvdd_var_;     ///< 内部DD変数 -> MVDD変数の逆引き
    std::vector<int> dd_to_index_;           ///< 内部DD変数 -> MVDD変数内インデックス

public:
    /**
     * @brief コンストラクタ
     * @param k 値域サイズ (2 <= k <= 100)
     */
    explicit MVDDVarTable(int k) : k_(k) {
        if (k < 2 || k > 100) {
            throw DDArgumentException("k must be between 2 and 100");
        }
    }

    /// 値域サイズ k を取得
    int k() const { return k_; }

    /// MVDD変数数を取得
    bddvar mvdd_var_count() const { return static_cast<bddvar>(var_map_.size()); }

    /**
     * @brief 新しいMVDD変数を登録
     * @param dd_vars 対応する内部DD変数番号のリスト (k-1個)
     * @return 新しいMVDD変数番号
     */
    bddvar register_var(const std::vector<bddvar>& dd_vars) {
        if (static_cast<int>(dd_vars.size()) != k_ - 1) {
            throw DDArgumentException("Number of DD variables must be k-1");
        }

        bddvar mv = static_cast<bddvar>(var_map_.size() + 1);
        var_map_.emplace_back(mv, k_, dd_vars);

        // 逆引きテーブルを更新
        for (size_t i = 0; i < dd_vars.size(); ++i) {
            bddvar dv = dd_vars[i];
            if (dv >= dd_to_mvdd_var_.size()) {
                dd_to_mvdd_var_.resize(dv + 1, 0);
                dd_to_index_.resize(dv + 1, -1);
            }
            dd_to_mvdd_var_[dv] = mv;
            dd_to_index_[dv] = static_cast<int>(i);
        }

        return mv;
    }

    /**
     * @brief MVDD変数に対応する内部DD変数群を取得
     * @param mv MVDD変数番号 (1-indexed)
     * @return 内部DD変数番号のベクタ (k-1個)
     */
    const std::vector<bddvar>& dd_vars_of(bddvar mv) const {
        if (mv == 0 || mv > var_map_.size()) {
            throw DDArgumentException("Invalid MVDD variable number");
        }
        return var_map_[mv - 1].dd_vars;
    }

    /**
     * @brief 内部DD変数からMVDD変数を取得
     * @param dv 内部DD変数番号
     * @return MVDD変数番号 (0なら対応なし)
     */
    bddvar mvdd_var_of(bddvar dv) const {
        if (dv >= dd_to_mvdd_var_.size()) {
            return 0;
        }
        return dd_to_mvdd_var_[dv];
    }

    /**
     * @brief 内部DD変数のMVDD変数内でのインデックスを取得
     * @param dv 内部DD変数番号
     * @return インデックス (0 to k-2)、対応なしなら -1
     */
    int dd_var_index(bddvar dv) const {
        if (dv >= dd_to_index_.size()) {
            return -1;
        }
        return dd_to_index_[dv];
    }

    /**
     * @brief MVDD変数の情報を取得
     * @param mv MVDD変数番号 (1-indexed)
     * @return 変数情報
     */
    const MVDDVarInfo& var_info(bddvar mv) const {
        if (mv == 0 || mv > var_map_.size()) {
            throw DDArgumentException("Invalid MVDD variable number");
        }
        return var_map_[mv - 1];
    }
};

/**
 * @brief Multi-Valued DD 共通基底クラス
 *
 * MVBDD と MVZDD の共通機能を提供します。
 * - 変数マッピング管理（MVDD変数 <-> 内部DD変数）
 * - k値の保持
 *
 * @note テンプレートクラスではなく、具象クラスです
 */
class MVDDBase {
protected:
    DDManager* manager_;                         ///< DDマネージャー
    std::shared_ptr<MVDDVarTable> var_table_;    ///< 変数マッピングテーブル（共有）

public:
    /// @name コンストラクタ
    /// @{

    /// デフォルトコンストラクタ（無効なMVDD）
    MVDDBase() : manager_(nullptr), var_table_(nullptr) {}

    /**
     * @brief マネージャーと値域サイズから構築
     * @param mgr DDマネージャーへのポインタ
     * @param k 値域サイズ (2 <= k <= 100)
     */
    MVDDBase(DDManager* mgr, int k)
        : manager_(mgr), var_table_(std::make_shared<MVDDVarTable>(k)) {}

    /**
     * @brief マネージャーと変数テーブルから構築
     * @param mgr DDマネージャーへのポインタ
     * @param table 変数マッピングテーブル
     */
    MVDDBase(DDManager* mgr, std::shared_ptr<MVDDVarTable> table)
        : manager_(mgr), var_table_(table) {}

    /// コピーコンストラクタ
    MVDDBase(const MVDDBase& other) = default;

    /// ムーブコンストラクタ
    MVDDBase(MVDDBase&& other) noexcept = default;

    /// デストラクタ
    virtual ~MVDDBase() = default;

    /// コピー代入演算子
    MVDDBase& operator=(const MVDDBase& other) = default;

    /// ムーブ代入演算子
    MVDDBase& operator=(MVDDBase&& other) noexcept = default;

    /// @}

    /// @name アクセサ
    /// @{

    /// 有効かどうか
    bool is_valid() const { return manager_ != nullptr && var_table_ != nullptr; }

    /// 値域サイズ k を取得
    int k() const { return var_table_ ? var_table_->k() : 0; }

    /// MVDD変数数を取得
    bddvar mvdd_var_count() const {
        return var_table_ ? var_table_->mvdd_var_count() : 0;
    }

    /// マネージャーを取得
    DDManager* manager() const { return manager_; }

    /// 変数テーブルを取得
    std::shared_ptr<MVDDVarTable> var_table() const { return var_table_; }

    /// @}

    /// @name 変数マッピング
    /// @{

    /**
     * @brief MVDD変数に対応する内部DD変数群を取得
     * @param mv MVDD変数番号 (1-indexed)
     * @return 内部DD変数番号のベクタ (k-1個)
     */
    const std::vector<bddvar>& dd_vars_of(bddvar mv) const {
        if (!var_table_) {
            throw DDArgumentException("Variable table not initialized");
        }
        return var_table_->dd_vars_of(mv);
    }

    /**
     * @brief 内部DD変数からMVDD変数を取得
     * @param dv 内部DD変数番号
     * @return MVDD変数番号 (0なら対応なし)
     */
    bddvar mvdd_var_of(bddvar dv) const {
        if (!var_table_) return 0;
        return var_table_->mvdd_var_of(dv);
    }

    /**
     * @brief 内部DD変数のMVDD変数内でのインデックスを取得
     * @param dv 内部DD変数番号
     * @return インデックス (0 to k-2)、対応なしなら -1
     */
    int dd_var_index(bddvar dv) const {
        if (!var_table_) return -1;
        return var_table_->dd_var_index(dv);
    }

    /// @}

protected:
    /**
     * @brief 新しいMVDD変数を登録（内部用）
     * @param dd_vars 対応する内部DD変数番号のリスト
     * @return 新しいMVDD変数番号
     */
    bddvar register_mvdd_var(const std::vector<bddvar>& dd_vars) {
        if (!var_table_) {
            throw DDArgumentException("Variable table not initialized");
        }
        return var_table_->register_var(dd_vars);
    }
};

} // namespace sbdd2

#endif // SBDD2_MVDD_BASE_HPP
