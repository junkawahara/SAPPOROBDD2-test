/**
 * @file mtbdd.hpp
 * @brief SAPPOROBDD 2.0 - Multi-Terminal BDD
 * @author SAPPOROBDD Team
 * @copyright MIT License
 *
 * 終端が任意の型Tの値を持てるBDD（Multi-Terminal BDD）を提供します。
 */

#ifndef SBDD2_MTBDD_HPP
#define SBDD2_MTBDD_HPP

#include "mtdd_base.hpp"
#include "bdd.hpp"
#include <functional>

namespace sbdd2 {

// Forward declaration
template<typename T> class MTZDD;

/**
 * @brief Multi-Terminal BDD
 *
 * 終端が0/1だけでなく任意の型Tの値を持てるBDD。
 * 代数的決定図（ADD: Algebraic Decision Diagram）としても知られています。
 *
 * 縮約規則: 0枝と1枝が同じなら子ノードを返す（BDD規則）
 * 否定枝は使用しません。
 *
 * @tparam T 終端値の型（+, -, *, <, == をサポートし、std::hash が定義されている必要があります）
 *
 * @code{.cpp}
 * DDManager mgr;
 * mgr.new_var();
 * mgr.new_var();
 *
 * // 定数MTBDD
 * MTBDD<double> c = MTBDD<double>::constant(mgr, 3.14);
 *
 * // 変数で分岐するMTBDD
 * MTBDD<double> hi = MTBDD<double>::constant(mgr, 1.0);
 * MTBDD<double> lo = MTBDD<double>::constant(mgr, 0.0);
 * MTBDD<double> x1 = MTBDD<double>::ite(mgr, 1, hi, lo);
 *
 * // 演算
 * MTBDD<double> result = x1 + c;
 * @endcode
 */
template<typename T>
class MTBDD : public MTDDBase<T> {
public:
    using MTDDBase<T>::manager_;
    using MTDDBase<T>::arc_;
    using MTDDBase<T>::is_terminal;
    using MTDDBase<T>::is_valid;

    /// @name コンストラクタ
    /// @{

    /// デフォルトコンストラクタ（無効なMTBDD）
    MTBDD() : MTDDBase<T>() {}

    /// マネージャーとアークから構築
    MTBDD(DDManager* mgr, Arc a) : MTDDBase<T>(mgr, a) {}

    /// コピーコンストラクタ
    MTBDD(const MTBDD& other) : MTDDBase<T>(other) {}

    /// ムーブコンストラクタ
    MTBDD(MTBDD&& other) noexcept : MTDDBase<T>(std::move(other)) {}

    /// コピー代入演算子
    MTBDD& operator=(const MTBDD& other) {
        MTDDBase<T>::operator=(other);
        return *this;
    }

    /// ムーブ代入演算子
    MTBDD& operator=(MTBDD&& other) noexcept {
        MTDDBase<T>::operator=(std::move(other));
        return *this;
    }

    /// @}

    /// @name ファクトリメソッド
    /// @{

    /**
     * @brief 定数MTBDDを作成
     *
     * @param mgr DDマネージャー
     * @param value 終端値
     * @return 単一の終端値を持つMTBDD
     */
    static MTBDD constant(DDManager& mgr, const T& value) {
        MTBDDTerminalTable<T>& table = mgr.template get_or_create_terminal_table<T>();
        bddindex idx = table.get_or_insert(value);
        Arc arc = MTBDDTerminalTable<T>::make_terminal_arc(idx);
        return MTBDD(&mgr, arc);
    }

    /**
     * @brief 変数で分岐するMTBDDを作成（ITE形式）
     *
     * 変数vで分岐し、v=1ならhigh、v=0ならlowを返すMTBDDを作成します。
     *
     * @param mgr DDマネージャー
     * @param v 変数番号
     * @param high v=1の場合の子MTBDD
     * @param low v=0の場合の子MTBDD
     * @return 作成されたMTBDD
     */
    static MTBDD ite(DDManager& mgr, bddvar v, const MTBDD& high, const MTBDD& low) {
        if (v == 0 || v > mgr.var_count()) {
            throw DDArgumentException("Invalid variable number");
        }
        if (!high.is_valid() || !low.is_valid()) {
            throw DDArgumentException("Invalid MTBDD operand");
        }
        Arc arc = mgr.get_or_create_node_mtbdd(v, low.arc_, high.arc_);
        return MTBDD(&mgr, arc);
    }

    /**
     * @brief BDDからMTBDDに変換
     *
     * BDDの終端0/1をそれぞれzero_val/one_valに対応させます。
     *
     * @param bdd 変換元のBDD
     * @param zero_val BDDの終端0に対応する値（デフォルト: T{}）
     * @param one_val BDDの終端1に対応する値（デフォルト: T{1}）
     * @return 変換されたMTBDD
     */
    static MTBDD from_bdd(const BDD& bdd,
                          const T& zero_val = T{},
                          const T& one_val = T{1}) {
        if (!bdd.is_valid()) {
            return MTBDD();
        }
        DDManager* mgr = bdd.manager();
        MTBDDTerminalTable<T>& table = mgr->template get_or_create_terminal_table<T>();

        bddindex zero_idx = table.get_or_insert(zero_val);
        bddindex one_idx = table.get_or_insert(one_val);

        std::unordered_map<std::uint64_t, Arc> memo;
        Arc result = convert_from_bdd(mgr, table, bdd.arc(), zero_idx, one_idx, memo);
        return MTBDD(mgr, result);
    }

    /// @}

    /// @name 子ノードアクセス
    /// @{

    /// 0枝の子をMTBDDとして取得
    MTBDD low() const {
        if (!is_valid() || is_terminal()) return *this;
        return MTBDD(manager_, this->low_arc());
    }

    /// 1枝の子をMTBDDとして取得
    MTBDD high() const {
        if (!is_valid() || is_terminal()) return *this;
        return MTBDD(manager_, this->high_arc());
    }

    /// @}

    /// @name 二項演算
    /// @{

    /// 加算
    MTBDD operator+(const MTBDD& other) const {
        return apply(other, [](const T& a, const T& b) { return a + b; }, CacheOp::MTBDD_PLUS);
    }

    /// 減算
    MTBDD operator-(const MTBDD& other) const {
        return apply(other, [](const T& a, const T& b) { return a - b; }, CacheOp::MTBDD_MINUS);
    }

    /// 乗算
    MTBDD operator*(const MTBDD& other) const {
        return apply(other, [](const T& a, const T& b) { return a * b; }, CacheOp::MTBDD_TIMES);
    }

    /// 最小値
    static MTBDD min(const MTBDD& a, const MTBDD& b) {
        return a.apply(b, [](const T& x, const T& y) { return std::min(x, y); }, CacheOp::MTBDD_MIN);
    }

    /// 最大値
    static MTBDD max(const MTBDD& a, const MTBDD& b) {
        return a.apply(b, [](const T& x, const T& y) { return std::max(x, y); }, CacheOp::MTBDD_MAX);
    }

    /**
     * @brief 汎用apply演算
     *
     * @tparam BinaryOp 二項演算関数型
     * @param other 第2オペランド
     * @param op 二項演算（(T, T) -> T）
     * @param cache_op キャッシュ用の操作タイプ（省略可）
     * @return 演算結果
     */
    template<typename BinaryOp>
    MTBDD apply(const MTBDD& other, BinaryOp op, CacheOp cache_op = CacheOp::CUSTOM) const {
        if (!is_valid() || !other.is_valid()) {
            throw DDArgumentException("Invalid MTBDD operand");
        }
        if (manager_ != other.manager_) {
            throw DDArgumentException("MTBDDs must share the same manager");
        }

        MTBDDTerminalTable<T>& table = manager_->template get_or_create_terminal_table<T>();
        Arc result = apply_impl(manager_, table, arc_, other.arc_, op, cache_op, false);
        return MTBDD(manager_, result);
    }

    /// @}

    /// @name ITE演算
    /// @{

    /**
     * @brief ITE演算 (if this then then_case else else_case)
     *
     * @param then_case thisが「真」（非ゼロ）の場合の値
     * @param else_case thisが「偽」（ゼロ）の場合の値
     * @return ITE結果
     */
    MTBDD ite(const MTBDD& then_case, const MTBDD& else_case) const {
        if (!is_valid() || !then_case.is_valid() || !else_case.is_valid()) {
            throw DDArgumentException("Invalid MTBDD operand");
        }
        if (manager_ != then_case.manager_ || manager_ != else_case.manager_) {
            throw DDArgumentException("MTBDDs must share the same manager");
        }

        MTBDDTerminalTable<T>& table = manager_->template get_or_create_terminal_table<T>();
        Arc result = ite_impl(manager_, table, arc_, then_case.arc_, else_case.arc_);
        return MTBDD(manager_, result);
    }

    /// @}

    /// @name 評価
    /// @{

    /**
     * @brief 変数割り当てに対する値を評価
     *
     * @param assignment 変数番号からbool値へのマッピング（インデックス0は未使用）
     * @return 評価結果の終端値
     */
    T evaluate(const std::vector<bool>& assignment) const {
        if (!is_valid()) {
            throw DDArgumentException("Invalid MTBDD");
        }

        Arc current = arc_;
        while (!current.is_constant()) {
            const DDNode& node = manager_->node_at(current.index());
            bddvar v = node.var();
            if (v < assignment.size() && assignment[v]) {
                current = node.arc1();
            } else {
                current = node.arc0();
            }
        }

        MTBDDTerminalTable<T>& table = manager_->template get_or_create_terminal_table<T>();
        return table.get_value(current.index());
    }

    /// @}

private:
    /// BDDからの変換ヘルパー
    static Arc convert_from_bdd(DDManager* mgr, MTBDDTerminalTable<T>& table,
                                Arc bdd_arc, bddindex zero_idx, bddindex one_idx,
                                std::unordered_map<std::uint64_t, Arc>& memo) {
        // 終端の場合
        if (bdd_arc.is_constant()) {
            bool val = bdd_arc.terminal_value() != bdd_arc.is_negated();
            bddindex idx = val ? one_idx : zero_idx;
            return MTBDDTerminalTable<T>::make_terminal_arc(idx);
        }

        // メモをチェック
        auto it = memo.find(bdd_arc.data);
        if (it != memo.end()) {
            return it->second;
        }

        const DDNode& node = mgr->node_at(bdd_arc.index());
        bddvar v = node.var();

        Arc bdd_arc0 = node.arc0();
        Arc bdd_arc1 = node.arc1();

        // 否定枝を処理
        if (bdd_arc.is_negated()) {
            bdd_arc0 = bdd_arc0.negated();
            bdd_arc1 = bdd_arc1.negated();
        }

        Arc arc0 = convert_from_bdd(mgr, table, bdd_arc0, zero_idx, one_idx, memo);
        Arc arc1 = convert_from_bdd(mgr, table, bdd_arc1, zero_idx, one_idx, memo);

        Arc result = mgr->get_or_create_node_mtbdd(v, arc0, arc1);
        memo[bdd_arc.data] = result;
        return result;
    }

    /// apply演算の実装
    template<typename BinaryOp>
    static Arc apply_impl(DDManager* mgr, MTBDDTerminalTable<T>& table,
                          Arc f, Arc g, BinaryOp op, CacheOp cache_op, bool use_zdd_rule) {
        // 両方終端の場合
        if (f.is_constant() && g.is_constant()) {
            T f_val = table.get_value(f.index());
            T g_val = table.get_value(g.index());
            T result_val = op(f_val, g_val);
            bddindex result_idx = table.get_or_insert(result_val);
            return MTBDDTerminalTable<T>::make_terminal_arc(result_idx);
        }

        // キャッシュを検索
        Arc result;
        if (cache_op != CacheOp::CUSTOM && mgr->cache_lookup(cache_op, f, g, result)) {
            return result;
        }

        // 変数を取得
        bddvar f_var = f.is_constant() ? BDDVAR_MAX : mgr->node_at(f.index()).var();
        bddvar g_var = g.is_constant() ? BDDVAR_MAX : mgr->node_at(g.index()).var();
        bddvar top_var = mgr->var_of_min_lev(f_var, g_var);

        // Shannon展開
        Arc f0, f1, g0, g1;
        if (!f.is_constant() && mgr->node_at(f.index()).var() == top_var) {
            f0 = mgr->node_at(f.index()).arc0();
            f1 = mgr->node_at(f.index()).arc1();
        } else {
            f0 = f1 = f;
        }
        if (!g.is_constant() && mgr->node_at(g.index()).var() == top_var) {
            g0 = mgr->node_at(g.index()).arc0();
            g1 = mgr->node_at(g.index()).arc1();
        } else {
            g0 = g1 = g;
        }

        // 再帰
        Arc r0 = apply_impl(mgr, table, f0, g0, op, cache_op, use_zdd_rule);
        Arc r1 = apply_impl(mgr, table, f1, g1, op, cache_op, use_zdd_rule);

        // ノード作成（縮約規則を適用）
        if (use_zdd_rule) {
            result = mgr->get_or_create_node_mtzdd(top_var, r0, r1, table.zero_index());
        } else {
            result = mgr->get_or_create_node_mtbdd(top_var, r0, r1);
        }

        // キャッシュに登録
        if (cache_op != CacheOp::CUSTOM) {
            mgr->cache_insert(cache_op, f, g, result);
        }

        return result;
    }

    /// ITE演算の実装
    static Arc ite_impl(DDManager* mgr, MTBDDTerminalTable<T>& table,
                        Arc f, Arc g, Arc h) {
        // 終端ケース
        if (f.is_constant()) {
            T f_val = table.get_value(f.index());
            // ゼロと比較（T{}がゼロとみなす）
            if (f_val == T{}) {
                return h;  // else
            } else {
                return g;  // then
            }
        }

        // キャッシュを検索（3項演算なので完全なキャッシュはここでは省略）
        Arc result;
        if (mgr->cache_lookup3(CacheOp::MTBDD_ITE, f, g, h, result)) {
            return result;
        }

        // 変数を取得
        bddvar f_var = mgr->node_at(f.index()).var();
        bddvar g_var = g.is_constant() ? BDDVAR_MAX : mgr->node_at(g.index()).var();
        bddvar h_var = h.is_constant() ? BDDVAR_MAX : mgr->node_at(h.index()).var();

        bddvar top_var = f_var;
        if (g_var != BDDVAR_MAX && mgr->var_is_above_or_equal(g_var, top_var)) {
            top_var = mgr->var_of_min_lev(top_var, g_var);
        }
        if (h_var != BDDVAR_MAX && mgr->var_is_above_or_equal(h_var, top_var)) {
            top_var = mgr->var_of_min_lev(top_var, h_var);
        }

        // Shannon展開
        Arc f0, f1, g0, g1, h0, h1;

        if (f_var == top_var) {
            f0 = mgr->node_at(f.index()).arc0();
            f1 = mgr->node_at(f.index()).arc1();
        } else {
            f0 = f1 = f;
        }
        if (!g.is_constant() && mgr->node_at(g.index()).var() == top_var) {
            g0 = mgr->node_at(g.index()).arc0();
            g1 = mgr->node_at(g.index()).arc1();
        } else {
            g0 = g1 = g;
        }
        if (!h.is_constant() && mgr->node_at(h.index()).var() == top_var) {
            h0 = mgr->node_at(h.index()).arc0();
            h1 = mgr->node_at(h.index()).arc1();
        } else {
            h0 = h1 = h;
        }

        // 再帰
        Arc r0 = ite_impl(mgr, table, f0, g0, h0);
        Arc r1 = ite_impl(mgr, table, f1, g1, h1);

        result = mgr->get_or_create_node_mtbdd(top_var, r0, r1);

        // キャッシュに登録
        mgr->cache_insert3(CacheOp::MTBDD_ITE, f, g, h, result);

        return result;
    }
};

/// ADDはMTBDDのエイリアス
template<typename T>
using ADD = MTBDD<T>;

} // namespace sbdd2

#endif // SBDD2_MTBDD_HPP
