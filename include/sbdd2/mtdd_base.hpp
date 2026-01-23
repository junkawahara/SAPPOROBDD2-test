/**
 * @file mtdd_base.hpp
 * @brief SAPPOROBDD 2.0 - Multi-Terminal DD 基底クラスとテーブル
 * @author SAPPOROBDD Team
 * @copyright MIT License
 *
 * MTBDD<T> と MTZDD<T> の共通機能を提供します。
 */

#ifndef SBDD2_MTDD_BASE_HPP
#define SBDD2_MTDD_BASE_HPP

#include "types.hpp"
#include "dd_manager.hpp"
#include <vector>
#include <unordered_map>
#include <mutex>
#include <functional>
#include <algorithm>

namespace sbdd2 {

/**
 * @brief MTBDD終端テーブルの基底クラス（型消去用）
 *
 * DDManagerが型パラメータを持たないため、この基底クラスを通じて
 * 異なる型の終端テーブルを管理します。
 */
class MTBDDTerminalTableBase {
public:
    virtual ~MTBDDTerminalTableBase() = default;

    /// 登録済み終端値の数
    virtual std::size_t size() const = 0;
};

/**
 * @brief MTBDD終端テーブル
 *
 * 型Tの終端値を管理するテーブル。値からインデックスへの逆引きを
 * サポートし、同じ値は同じインデックスを共有します。
 *
 * @tparam T 終端値の型（operator== が必要）
 */
template<typename T>
class MTBDDTerminalTable : public MTBDDTerminalTableBase {
public:
    /**
     * @brief コンストラクタ
     *
     * インデックス0にデフォルト値（T{}）を登録します。
     */
    MTBDDTerminalTable() {
        // Index 0 is always the "zero" terminal (T{})
        T zero_val = T{};
        values_.push_back(zero_val);
        value_to_index_[zero_val] = 0;
    }

    /**
     * @brief 値を登録してインデックスを取得
     *
     * 値が既に登録されている場合は既存のインデックスを返し、
     * そうでなければ新しいインデックスを割り当てます。
     *
     * @param value 登録する値
     * @return 値に対応するインデックス
     */
    bddindex get_or_insert(const T& value) {
        std::lock_guard<std::mutex> lock(mutex_);

        auto it = value_to_index_.find(value);
        if (it != value_to_index_.end()) {
            return it->second;
        }

        bddindex idx = static_cast<bddindex>(values_.size());
        values_.push_back(value);
        value_to_index_[value] = idx;
        return idx;
    }

    /**
     * @brief インデックスから値を取得
     *
     * @param index 終端インデックス
     * @return 対応する値への参照
     * @throw std::out_of_range インデックスが範囲外の場合
     */
    const T& get_value(bddindex index) const {
        std::lock_guard<std::mutex> lock(mutex_);
        if (index >= values_.size()) {
            throw std::out_of_range("Terminal index out of range");
        }
        return values_[index];
    }

    /**
     * @brief 値が登録済みかどうか
     *
     * @param value 検索する値
     * @return 登録済みならtrue
     */
    bool contains(const T& value) const {
        std::lock_guard<std::mutex> lock(mutex_);
        return value_to_index_.find(value) != value_to_index_.end();
    }

    /**
     * @brief 登録済み値の数
     */
    std::size_t size() const override {
        std::lock_guard<std::mutex> lock(mutex_);
        return values_.size();
    }

    /**
     * @brief ゼロ終端のインデックス
     *
     * MTZDDの縮約規則で使用されます。
     * @return 常に0
     */
    bddindex zero_index() const { return 0; }

    /**
     * @brief 終端Arcを作成
     *
     * @param index 終端インデックス
     * @return 終端を指すArc
     */
    static Arc make_terminal_arc(bddindex index) {
        // constant flag = 1 (bit 1), index shifted by 2
        return Arc((index << 2) | 0x2ULL);
    }

private:
    std::vector<T> values_;
    std::unordered_map<T, bddindex> value_to_index_;
    mutable std::mutex mutex_;
};

/**
 * @brief Multi-Terminal DD 共通基底クラス
 *
 * MTBDD<T> と MTZDD<T> の共通機能を提供します。
 *
 * @tparam T 終端値の型
 */
template<typename T>
class MTDDBase {
protected:
    DDManager* manager_;
    Arc arc_;

public:
    /// @name コンストラクタ・デストラクタ
    /// @{

    /// デフォルトコンストラクタ（無効なDD）
    MTDDBase() : manager_(nullptr), arc_() {}

    /// マネージャーとアークから構築
    MTDDBase(DDManager* mgr, Arc a) : manager_(mgr), arc_(a) {
        if (manager_ && !arc_.is_constant()) {
            manager_->inc_ref(arc_);
        }
    }

    /// コピーコンストラクタ
    MTDDBase(const MTDDBase& other) : manager_(other.manager_), arc_(other.arc_) {
        if (manager_ && !arc_.is_constant()) {
            manager_->inc_ref(arc_);
        }
    }

    /// ムーブコンストラクタ
    MTDDBase(MTDDBase&& other) noexcept
        : manager_(other.manager_), arc_(other.arc_) {
        other.manager_ = nullptr;
        other.arc_ = Arc();
    }

    /// デストラクタ
    ~MTDDBase() {
        if (manager_ && !arc_.is_constant()) {
            manager_->dec_ref(arc_);
        }
    }

    /// コピー代入演算子
    MTDDBase& operator=(const MTDDBase& other) {
        if (this != &other) {
            if (manager_ && !arc_.is_constant()) {
                manager_->dec_ref(arc_);
            }
            manager_ = other.manager_;
            arc_ = other.arc_;
            if (manager_ && !arc_.is_constant()) {
                manager_->inc_ref(arc_);
            }
        }
        return *this;
    }

    /// ムーブ代入演算子
    MTDDBase& operator=(MTDDBase&& other) noexcept {
        if (this != &other) {
            if (manager_ && !arc_.is_constant()) {
                manager_->dec_ref(arc_);
            }
            manager_ = other.manager_;
            arc_ = other.arc_;
            other.manager_ = nullptr;
            other.arc_ = Arc();
        }
        return *this;
    }

    /// @}

    /// @name アクセサ
    /// @{

    /// 有効なDDかどうか
    bool is_valid() const { return manager_ != nullptr; }

    /// 終端かどうか
    bool is_terminal() const { return arc_.is_constant(); }

    /// 終端値を取得
    T terminal_value() const {
        if (!is_terminal() || !manager_) {
            throw DDArgumentException("Not a terminal node");
        }
        MTBDDTerminalTable<T>& table = manager_->get_or_create_terminal_table<T>();
        return table.get_value(arc_.index());
    }

    /// トップ変数を取得（終端の場合は0）
    bddvar top() const {
        if (!manager_ || is_terminal()) return 0;
        return manager_->node_at(arc_.index()).var();
    }

    /// マネージャーを取得
    DDManager* manager() const { return manager_; }

    /// 内部アークを取得
    Arc arc() const { return arc_; }

    /// @}

    /// @name 子ノードアクセス
    /// @{

    /// 0枝の子を取得
    Arc low_arc() const {
        if (!manager_ || is_terminal()) return arc_;
        return manager_->node_at(arc_.index()).arc0();
    }

    /// 1枝の子を取得
    Arc high_arc() const {
        if (!manager_ || is_terminal()) return arc_;
        return manager_->node_at(arc_.index()).arc1();
    }

    /// @}

    /// @name 比較演算子
    /// @{

    bool operator==(const MTDDBase& other) const {
        return manager_ == other.manager_ && arc_ == other.arc_;
    }

    bool operator!=(const MTDDBase& other) const {
        return !(*this == other);
    }

    /// @}

    /// @name ノード数
    /// @{

    /// 内部ノード数を計算
    std::size_t size() const {
        if (!manager_ || is_terminal()) return 0;
        std::unordered_map<bddindex, bool> visited;
        return count_nodes(arc_, visited);
    }

    /// @}

protected:
    /// ノード数を再帰的にカウント
    std::size_t count_nodes(Arc a, std::unordered_map<bddindex, bool>& visited) const {
        if (a.is_constant()) return 0;
        bddindex idx = a.index();
        if (visited.find(idx) != visited.end()) return 0;
        visited[idx] = true;

        const DDNode& node = manager_->node_at(idx);
        return 1 + count_nodes(node.arc0(), visited) + count_nodes(node.arc1(), visited);
    }
};

// DDManager template method implementations

template<typename T>
MTBDDTerminalTable<T>& DDManager::get_or_create_terminal_table() {
    std::lock_guard<std::mutex> lock(mtbdd_tables_mutex_);
    std::type_index ti(typeid(T));

    auto it = mtbdd_tables_.find(ti);
    if (it != mtbdd_tables_.end()) {
        return *static_cast<MTBDDTerminalTable<T>*>(it->second.get());
    }

    auto table = std::unique_ptr<MTBDDTerminalTableBase>(new MTBDDTerminalTable<T>());
    MTBDDTerminalTable<T>& ref = *static_cast<MTBDDTerminalTable<T>*>(table.get());
    mtbdd_tables_[ti] = std::move(table);
    return ref;
}

template<typename T>
MTBDDTerminalTable<T>* DDManager::get_terminal_table() const {
    std::lock_guard<std::mutex> lock(mtbdd_tables_mutex_);
    std::type_index ti(typeid(T));

    auto it = mtbdd_tables_.find(ti);
    if (it != mtbdd_tables_.end()) {
        return static_cast<MTBDDTerminalTable<T>*>(it->second.get());
    }
    return nullptr;
}

} // namespace sbdd2

#endif // SBDD2_MTDD_BASE_HPP
