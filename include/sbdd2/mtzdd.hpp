/**
 * @file mtzdd.hpp
 * @brief SAPPOROBDD 2.0 - Multi-Terminal ZDD
 * @author SAPPOROBDD Team
 * @copyright MIT License
 *
 * 終端が任意の型Tの値を持てるZDD（Multi-Terminal ZDD）を提供します。
 */

#ifndef SBDD2_MTZDD_HPP
#define SBDD2_MTZDD_HPP

#include "mtdd_base.hpp"
#include "zdd.hpp"
#include <functional>

namespace sbdd2 {

// Forward declaration
template<typename T> class MTBDD;

/**
 * @brief Multi-Terminal ZDD
 *
 * 終端が0/1だけでなく任意の型Tの値を持てるZDD。
 *
 * 縮約規則:
 * 1. 1枝がゼロ終端を指す場合、0枝を返す（ZDD規則）
 * 2. 0枝と1枝が同じなら子ノードを返す
 *
 * 否定枝は使用しません。
 *
 * @tparam T 終端値の型（+, -, *, <, == をサポートし、std::hash が定義されている必要があります）
 *
 * @code{.cpp}
 * DDManager mgr;
 * mgr.new_var();
 * mgr.new_var();
 *
 * // 定数MTZDD
 * MTZDD<int> c = MTZDD<int>::constant(mgr, 42);
 *
 * // 変数で分岐するMTZDD
 * MTZDD<int> hi = MTZDD<int>::constant(mgr, 1);
 * MTZDD<int> lo = MTZDD<int>::constant(mgr, 0);
 * MTZDD<int> x1 = MTZDD<int>::ite(mgr, 1, hi, lo);
 * // ZDD規則により、hiがゼロならloが返される
 *
 * // 演算
 * MTZDD<int> result = x1 + c;
 * @endcode
 *
 * @see MTBDD, DDManager, ZDD
 */
template<typename T>
class MTZDD : public MTDDBase<T> {
public:
    using MTDDBase<T>::manager_;
    using MTDDBase<T>::arc_;
    using MTDDBase<T>::is_terminal;
    using MTDDBase<T>::is_valid;

    /// @name コンストラクタ
    /// @{

    /// デフォルトコンストラクタ（無効なMTZDD）
    MTZDD() : MTDDBase<T>() {}

    /// マネージャーとアークから構築
    MTZDD(DDManager* mgr, Arc a) : MTDDBase<T>(mgr, a) {}

    /// コピーコンストラクタ
    MTZDD(const MTZDD& other) : MTDDBase<T>(other) {}

    /// ムーブコンストラクタ
    MTZDD(MTZDD&& other) noexcept : MTDDBase<T>(std::move(other)) {}

    /// コピー代入演算子
    MTZDD& operator=(const MTZDD& other) {
        MTDDBase<T>::operator=(other);
        return *this;
    }

    /// ムーブ代入演算子
    MTZDD& operator=(MTZDD&& other) noexcept {
        MTDDBase<T>::operator=(std::move(other));
        return *this;
    }

    /// @}

    /// @name ファクトリメソッド
    /// @{

    /**
     * @brief 定数MTZDDを作成
     *
     * @param mgr DDマネージャー
     * @param value 終端値
     * @return 単一の終端値を持つMTZDD
     *
     * @code{.cpp}
     * DDManager mgr;
     * MTZDD<int> c = MTZDD<int>::constant(mgr, 42);
     * MTZDD<double> d = MTZDD<double>::constant(mgr, 0.5);
     * @endcode
     *
     * @see ite(), from_zdd(), MTBDD::constant()
     */
    static MTZDD constant(DDManager& mgr, const T& value) {
        MTBDDTerminalTable<T>& table = mgr.template get_or_create_terminal_table<T>();
        bddindex idx = table.get_or_insert(value);
        Arc arc = MTBDDTerminalTable<T>::make_terminal_arc(idx);
        return MTZDD(&mgr, arc);
    }

    /**
     * @brief 変数で分岐するMTZDDを作成（ITE形式）
     *
     * 変数vで分岐し、v=1ならhigh、v=0ならlowを返すMTZDDを作成します。
     *
     * ZDD縮約規則: highがゼロ終端の場合、lowがそのまま返されます。
     *
     * @param mgr DDマネージャー
     * @param v 変数番号
     * @param high v=1の場合の子MTZDD
     * @param low v=0の場合の子MTZDD
     * @return 作成されたMTZDD
     *
     * @code{.cpp}
     * DDManager mgr;
     * mgr.new_var();
     * auto hi = MTZDD<int>::constant(mgr, 1);
     * auto lo = MTZDD<int>::constant(mgr, 0);
     * auto x1 = MTZDD<int>::ite(mgr, 1, hi, lo);
     * @endcode
     *
     * @see constant(), MTBDD::ite()
     */
    static MTZDD ite(DDManager& mgr, bddvar v, const MTZDD& high, const MTZDD& low) {
        if (v == 0 || v > mgr.var_count()) {
            throw DDArgumentException("Invalid variable number");
        }
        if (!high.is_valid() || !low.is_valid()) {
            throw DDArgumentException("Invalid MTZDD operand");
        }

        MTBDDTerminalTable<T>& table = mgr.template get_or_create_terminal_table<T>();
        Arc arc = mgr.get_or_create_node_mtzdd(v, low.arc_, high.arc_, table.zero_index());
        return MTZDD(&mgr, arc);
    }

    /**
     * @brief ZDDからMTZDDに変換
     *
     * ZDDの終端0/1をそれぞれzero_val/one_valに対応させます。
     *
     * @param zdd 変換元のZDD
     * @param zero_val ZDDの終端0に対応する値（デフォルト: T{}）
     * @param one_val ZDDの終端1に対応する値（デフォルト: T{1}）
     * @return 変換されたMTZDD
     *
     * @code{.cpp}
     * DDManager mgr;
     * mgr.new_var();
     * ZDD z1 = mgr.var_zdd(1);  // {{1}}
     * // ZDDをint型のMTZDDに変換（0->0, 1->1）
     * auto mt = MTZDD<int>::from_zdd(z1);
     * @endcode
     *
     * @see MTBDD::from_bdd(), ZDD
     */
    static MTZDD from_zdd(const ZDD& zdd,
                          const T& zero_val = T{},
                          const T& one_val = T{1}) {
        if (!zdd.is_valid()) {
            return MTZDD();
        }
        DDManager* mgr = zdd.manager();
        MTBDDTerminalTable<T>& table = mgr->template get_or_create_terminal_table<T>();

        bddindex zero_idx = table.get_or_insert(zero_val);
        bddindex one_idx = table.get_or_insert(one_val);

        std::unordered_map<std::uint64_t, Arc> memo;
        Arc result = convert_from_zdd(mgr, table, zdd.arc(), zero_idx, one_idx, memo);
        return MTZDD(mgr, result);
    }

    /// @}

    /// @name 子ノードアクセス
    /// @{

    /// 0枝の子をMTZDDとして取得
    MTZDD low() const {
        if (!is_valid() || is_terminal()) return *this;
        return MTZDD(manager_, this->low_arc());
    }

    /// 1枝の子をMTZDDとして取得
    MTZDD high() const {
        if (!is_valid() || is_terminal()) return *this;
        return MTZDD(manager_, this->high_arc());
    }

    /// @}

    /// @name 二項演算
    /// @{

    /// 加算
    MTZDD operator+(const MTZDD& other) const {
        return apply(other, [](const T& a, const T& b) { return a + b; }, CacheOp::MTBDD_PLUS);
    }

    /// 減算
    MTZDD operator-(const MTZDD& other) const {
        return apply(other, [](const T& a, const T& b) { return a - b; }, CacheOp::MTBDD_MINUS);
    }

    /// 乗算
    MTZDD operator*(const MTZDD& other) const {
        return apply(other, [](const T& a, const T& b) { return a * b; }, CacheOp::MTBDD_TIMES);
    }

    /// 最小値
    static MTZDD min(const MTZDD& a, const MTZDD& b) {
        return a.apply(b, [](const T& x, const T& y) { return std::min(x, y); }, CacheOp::MTBDD_MIN);
    }

    /// 最大値
    static MTZDD max(const MTZDD& a, const MTZDD& b) {
        return a.apply(b, [](const T& x, const T& y) { return std::max(x, y); }, CacheOp::MTBDD_MAX);
    }

    /**
     * @brief 汎用apply演算
     *
     * 2つのMTZDDに対して終端値同士で二項演算を適用します。
     * ZDD縮約規則に従ってノードが作成されます。
     *
     * @tparam BinaryOp 二項演算関数型
     * @param other 第2オペランド
     * @param op 二項演算（(T, T) -> T）
     * @param cache_op キャッシュ用の操作タイプ（省略可）
     * @return 演算結果
     *
     * @code{.cpp}
     * // カスタム演算: 2つのMTZDDの終端値の最大値
     * auto result = a.apply(b, [](int x, int y) {
     *     return std::max(x, y);
     * });
     * @endcode
     *
     * @see MTBDD::apply()
     */
    template<typename BinaryOp>
    MTZDD apply(const MTZDD& other, BinaryOp op, CacheOp cache_op = CacheOp::CUSTOM) const {
        if (!is_valid() || !other.is_valid()) {
            throw DDArgumentException("Invalid MTZDD operand");
        }
        if (manager_ != other.manager_) {
            throw DDArgumentException("MTZDDs must share the same manager");
        }

        MTBDDTerminalTable<T>& table = manager_->template get_or_create_terminal_table<T>();
        Arc result = apply_impl(manager_, table, arc_, other.arc_, op, cache_op);
        return MTZDD(manager_, result);
    }

    /// @}

    /// @name ITE演算
    /// @{

    /**
     * @brief ITE演算 (if this then then_case else else_case)
     *
     * thisの終端値がゼロ（T{}）なら else_case、非ゼロなら then_case を選択します。
     * ZDD縮約規則に従ってノードが作成されます。
     *
     * @param then_case thisが「真」（非ゼロ）の場合の値
     * @param else_case thisが「偽」（ゼロ）の場合の値
     * @return ITE結果
     *
     * @see MTBDD::ite()
     */
    MTZDD ite(const MTZDD& then_case, const MTZDD& else_case) const {
        if (!is_valid() || !then_case.is_valid() || !else_case.is_valid()) {
            throw DDArgumentException("Invalid MTZDD operand");
        }
        if (manager_ != then_case.manager_ || manager_ != else_case.manager_) {
            throw DDArgumentException("MTZDDs must share the same manager");
        }

        MTBDDTerminalTable<T>& table = manager_->template get_or_create_terminal_table<T>();
        Arc result = ite_impl(manager_, table, arc_, then_case.arc_, else_case.arc_);
        return MTZDD(manager_, result);
    }

    /// @}

    /// @name 評価
    /// @{

    /**
     * @brief 変数割り当てに対する値を評価
     *
     * @param assignment 変数番号からbool値へのマッピング（インデックス0は未使用）
     * @return 評価結果の終端値
     *
     * @code{.cpp}
     * // 変数1=true, 変数2=false での評価
     * std::vector<bool> assign = {false, true, false};  // index 0は未使用
     * int val = mt.evaluate(assign);
     * @endcode
     *
     * @see MTBDD::evaluate()
     */
    T evaluate(const std::vector<bool>& assignment) const {
        if (!is_valid()) {
            throw DDArgumentException("Invalid MTZDD");
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
    /// ZDDからの変換ヘルパー
    static Arc convert_from_zdd(DDManager* mgr, MTBDDTerminalTable<T>& table,
                                Arc zdd_arc, bddindex zero_idx, bddindex one_idx,
                                std::unordered_map<std::uint64_t, Arc>& memo) {
        // 終端の場合
        if (zdd_arc.is_constant()) {
            // ZDDは否定枝を使わないので、そのままindex()で値を取得
            bddindex idx = zdd_arc.index() == 0 ? zero_idx : one_idx;
            return MTBDDTerminalTable<T>::make_terminal_arc(idx);
        }

        // メモをチェック
        auto it = memo.find(zdd_arc.data);
        if (it != memo.end()) {
            return it->second;
        }

        const DDNode& node = mgr->node_at(zdd_arc.index());
        bddvar v = node.var();

        Arc zdd_arc0 = node.arc0();
        Arc zdd_arc1 = node.arc1();

        Arc arc0 = convert_from_zdd(mgr, table, zdd_arc0, zero_idx, one_idx, memo);
        Arc arc1 = convert_from_zdd(mgr, table, zdd_arc1, zero_idx, one_idx, memo);

        // ZDD縮約規則を適用
        Arc result = mgr->get_or_create_node_mtzdd(v, arc0, arc1, zero_idx);
        memo[zdd_arc.data] = result;
        return result;
    }

    /// apply演算の実装（ZDD縮約規則）
    template<typename BinaryOp>
    static Arc apply_impl(DDManager* mgr, MTBDDTerminalTable<T>& table,
                          Arc f, Arc g, BinaryOp op, CacheOp cache_op) {
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
        bddvar top_var = mgr->var_of_top_lev(f_var, g_var);

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
        Arc r0 = apply_impl(mgr, table, f0, g0, op, cache_op);
        Arc r1 = apply_impl(mgr, table, f1, g1, op, cache_op);

        // ノード作成（ZDD縮約規則を適用）
        result = mgr->get_or_create_node_mtzdd(top_var, r0, r1, table.zero_index());

        // キャッシュに登録
        if (cache_op != CacheOp::CUSTOM) {
            mgr->cache_insert(cache_op, f, g, result);
        }

        return result;
    }

    /// ITE演算の実装（ZDD縮約規則）
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

        // 変数を取得
        bddvar f_var = mgr->node_at(f.index()).var();
        bddvar g_var = g.is_constant() ? BDDVAR_MAX : mgr->node_at(g.index()).var();
        bddvar h_var = h.is_constant() ? BDDVAR_MAX : mgr->node_at(h.index()).var();

        bddvar top_var = f_var;
        if (g_var != BDDVAR_MAX && mgr->var_is_above_or_equal(g_var, top_var)) {
            top_var = mgr->var_of_top_lev(top_var, g_var);
        }
        if (h_var != BDDVAR_MAX && mgr->var_is_above_or_equal(h_var, top_var)) {
            top_var = mgr->var_of_top_lev(top_var, h_var);
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

        // ZDD縮約規則を適用
        Arc result = mgr->get_or_create_node_mtzdd(top_var, r0, r1, table.zero_index());
        return result;
    }
};

} // namespace sbdd2

#endif // SBDD2_MTZDD_HPP
