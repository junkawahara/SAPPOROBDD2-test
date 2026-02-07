/**
 * @file mvbdd.hpp
 * @brief SAPPOROBDD 2.0 - Multi-Valued BDD
 * @author SAPPOROBDD Team
 * @copyright MIT License
 *
 * 多値変数を扱うBDD。各変数は k 個の値 {0, 1, ..., k-1} を取ることができます。
 * 内部的には通常のBDDでエミュレートして実装されています。
 */

#ifndef SBDD2_MVBDD_HPP
#define SBDD2_MVBDD_HPP

#include "mvdd_base.hpp"
#include "mvdd_node_ref.hpp"
#include "bdd.hpp"
#include <initializer_list>

namespace sbdd2 {

/**
 * @brief Multi-Valued BDD
 *
 * 多値変数を扱うBDD。各変数は k 個の値 {0, 1, ..., k-1} を取ることができます。
 *
 * エンコード方式（One-Hot風、k=4の例）:
 * - MVBDD変数 x に対して、内部変数 x_1, x_2, x_3 を使用
 * - x=1: x_1 = 1
 * - x=2: x_1 = 0, x_2 = 1
 * - x=3: x_1 = 0, x_2 = 0, x_3 = 1
 * - x=0: x_1 = 0, x_2 = 0, x_3 = 0
 *
 * 縮約規則: BDD縮約（全arcが同じ先なら縮約）
 *
 * @code{.cpp}
 * DDManager mgr;
 * MVBDD f = MVBDD::one(mgr, 4);  // k=4
 * f.new_var();  // MVDD変数1を作成
 * f.new_var();  // MVDD変数2を作成
 *
 * // 単一リテラル: 変数1 = 2
 * MVBDD lit = MVBDD::single(f, 1, 2);
 *
 * // ITE構築
 * MVBDD t = MVBDD::ite(f, 1, {child0, child1, child2, child3});
 * @endcode
 *
 * @see MVZDD, DDManager, BDD
 */
class MVBDD : public MVDDBase {
private:
    BDD bdd_;  ///< 内部BDD表現

public:
    /// @name コンストラクタ
    /// @{

    /// デフォルトコンストラクタ（無効なMVBDD）
    MVBDD() : MVDDBase(), bdd_() {}

    /**
     * @brief マネージャーと値域サイズから構築（定数0）
     * @param mgr DDマネージャーへのポインタ
     * @param k 値域サイズ
     * @param value 初期値 (true=1, false=0)
     */
    MVBDD(DDManager* mgr, int k, bool value = false)
        : MVDDBase(mgr, k),
          bdd_(value ? BDD::one(*mgr) : BDD::zero(*mgr)) {}

    /**
     * @brief 変数テーブルと内部BDDから構築
     * @param mgr DDマネージャーへのポインタ
     * @param table 変数マッピングテーブル
     * @param bdd 内部BDD
     */
    MVBDD(DDManager* mgr, std::shared_ptr<MVDDVarTable> table, const BDD& bdd)
        : MVDDBase(mgr, table), bdd_(bdd) {}

    /// コピーコンストラクタ
    MVBDD(const MVBDD& other) = default;

    /// ムーブコンストラクタ
    MVBDD(MVBDD&& other) noexcept = default;

    /// コピー代入演算子
    MVBDD& operator=(const MVBDD& other) = default;

    /// ムーブ代入演算子
    MVBDD& operator=(MVBDD&& other) noexcept = default;

    /// @}

    /// @name ファクトリメソッド
    /// @{

    /**
     * @brief 定数0（偽）を作成
     * @param mgr DDマネージャー
     * @param k 値域サイズ
     * @return 定数0を表すMVBDD
     *
     * @see one(), MVZDD::empty()
     */
    static MVBDD zero(DDManager& mgr, int k) {
        return MVBDD(&mgr, k, false);
    }

    /**
     * @brief 定数1（真）を作成
     * @param mgr DDマネージャー
     * @param k 値域サイズ
     * @return 定数1を表すMVBDD
     *
     * @code{.cpp}
     * DDManager mgr;
     * MVBDD f = MVBDD::one(mgr, 4);  // k=4 の定数1
     * f.new_var();  // MVDD変数1を作成
     * f.new_var();  // MVDD変数2を作成
     * @endcode
     *
     * @see zero(), MVZDD::single()
     */
    static MVBDD one(DDManager& mgr, int k) {
        return MVBDD(&mgr, k, true);
    }

    /**
     * @brief 単一リテラルを作成（変数mvが値valueを取る）
     * @param base 変数マッピング情報を持つMVBDD
     * @param mv MVDD変数番号
     * @param value 値 (0 to k-1)
     * @return [mv = value]
     *
     * @code{.cpp}
     * DDManager mgr;
     * MVBDD f = MVBDD::one(mgr, 4);
     * f.new_var();
     * // 変数1 == 2 を表すリテラル
     * MVBDD lit = MVBDD::single(f, 1, 2);
     * @endcode
     *
     * @see ite(), MVZDD::singleton()
     */
    static MVBDD single(const MVBDD& base, bddvar mv, int value);

    /// @}

    /// @name 変数管理
    /// @{

    /**
     * @brief 新しいMVDD変数を追加
     * @return 新しいMVDD変数番号
     *
     * 内部的に k-1 個のDD変数が DDManager に作成されます。
     *
     * @see DDManager::new_mvdd_var(), MVZDD::new_var()
     */
    bddvar new_var() {
        auto dd_vars = manager_->new_mvdd_var(k());
        return register_mvdd_var(dd_vars);
    }

    /// @}

    /// @name ITE構築
    /// @{

    /**
     * @brief ITE形式でMVBDDノードを構築
     *
     * 変数mvの各値に対応する子ノードを指定して構築します。
     *
     * @param base 変数マッピング情報
     * @param mv MVDD変数番号
     * @param children k個の子MVBDD (children[i] = 変数mvが値iを取る場合)
     * @return 構築されたMVBDD
     *
     * @code{.cpp}
     * DDManager mgr;
     * MVBDD f = MVBDD::one(mgr, 3);  // k=3
     * f.new_var();
     * auto c0 = MVBDD::zero(mgr, 3);
     * auto c1 = MVBDD::one(mgr, 3);
     * auto c2 = MVBDD::one(mgr, 3);
     * MVBDD t = MVBDD::ite(f, 1, {c0, c1, c2});
     * @endcode
     *
     * @see single(), MVZDD::ite()
     */
    static MVBDD ite(const MVBDD& base, bddvar mv,
                     const std::vector<MVBDD>& children);

    /// initializer_list版
    static MVBDD ite(const MVBDD& base, bddvar mv,
                     std::initializer_list<MVBDD> children) {
        return ite(base, mv, std::vector<MVBDD>(children));
    }

    /// @}

    /// @name ノード参照
    /// @{

    /**
     * @brief ルートノードへの軽量参照を取得
     * @return MVDDNodeRef
     * @warning 返された参照はこの MVBDD より長く生存してはいけません
     */
    MVDDNodeRef root_ref() const {
        return MVDDNodeRef(manager_, bdd_.arc(), var_table_.get());
    }

    /// @}

    /// @name 子ノードアクセス
    /// @{

    /**
     * @brief 指定値に対応する子を取得
     * @param value 値 (0 to k-1)
     * @return 子MVBDD
     */
    MVBDD child(int value) const;

    /**
     * @brief トップ変数のMVDD変数番号を取得
     * @return MVDD変数番号 (終端の場合は0)
     */
    bddvar top_var() const;

    /// @}

    /// @name ブール演算
    /// @{

    /// 論理積（AND）
    MVBDD operator&(const MVBDD& other) const {
        check_compatible(other);
        return MVBDD(manager_, var_table_, bdd_ & other.bdd_);
    }

    /// 論理和（OR）
    MVBDD operator|(const MVBDD& other) const {
        check_compatible(other);
        return MVBDD(manager_, var_table_, bdd_ | other.bdd_);
    }

    /// 排他的論理和（XOR）
    MVBDD operator^(const MVBDD& other) const {
        check_compatible(other);
        return MVBDD(manager_, var_table_, bdd_ ^ other.bdd_);
    }

    /// 論理否定（NOT）
    MVBDD operator~() const {
        return MVBDD(manager_, var_table_, ~bdd_);
    }

    /// 複合代入演算子
    MVBDD& operator&=(const MVBDD& other) {
        check_compatible(other);
        bdd_ &= other.bdd_;
        return *this;
    }

    MVBDD& operator|=(const MVBDD& other) {
        check_compatible(other);
        bdd_ |= other.bdd_;
        return *this;
    }

    MVBDD& operator^=(const MVBDD& other) {
        check_compatible(other);
        bdd_ ^= other.bdd_;
        return *this;
    }

    /// @}

    /// @name 評価
    /// @{

    /**
     * @brief 変数割り当てに対する評価
     * @param assignment MVDD変数番号 -> 値のマッピング (0-indexed配列、要素数はmvdd_var_count以上)
     * @return ブール値
     *
     * @code{.cpp}
     * // MVDD変数1=2, 変数2=0 での評価
     * std::vector<int> assign = {2, 0};
     * bool val = f.evaluate(assign);
     * @endcode
     *
     * @see MVZDD::evaluate()
     */
    bool evaluate(const std::vector<int>& assignment) const;

    /// @}

    /// @name 変換
    /// @{

    /// 内部BDD表現を取得
    /// @see from_bdd(), MVZDD::to_zdd()
    const BDD& to_bdd() const { return bdd_; }

    /**
     * @brief BDDからMVBDDを構築
     * @param base 変数マッピング情報
     * @param bdd 内部BDD
     * @return MVBDD
     *
     * @see to_bdd(), MVZDD::from_zdd()
     */
    static MVBDD from_bdd(const MVBDD& base, const BDD& bdd) {
        return MVBDD(base.manager_, base.var_table_, bdd);
    }

    /// @}

    /// @name 判定
    /// @{

    /// 定数0かどうか
    bool is_zero() const { return bdd_.is_zero(); }

    /// 定数1かどうか
    bool is_one() const { return bdd_.is_one(); }

    /// 終端かどうか
    bool is_terminal() const { return bdd_.is_terminal(); }

    /// @}

    /// @name ノード数
    /// @{

    /**
     * @brief MVBDDとしての論理ノード数を計算
     *
     * 内部BDDノードではなく、MVDD変数ごとにカウントします。
     *
     * @return MVBDDノード数
     *
     * @see size(), MVZDD::mvzdd_node_count()
     */
    std::size_t mvbdd_node_count() const;

    /// 内部BDDのノード数
    std::size_t size() const { return bdd_.size(); }

    /// @}

    /// @name 比較
    /// @{

    bool operator==(const MVBDD& other) const {
        return k() == other.k() && bdd_ == other.bdd_;
    }

    bool operator!=(const MVBDD& other) const {
        return !(*this == other);
    }

    /// @}

private:
    /// 互換性チェック
    void check_compatible(const MVBDD& other) const {
        if (k() != other.k()) {
            throw DDArgumentException("MVBDD k values must match");
        }
    }
};

// Implementation

inline MVBDD MVBDD::single(const MVBDD& base, bddvar mv, int value) {
    if (!base.is_valid()) {
        throw DDArgumentException("Base MVBDD is not valid");
    }
    if (value < 0 || value >= base.k()) {
        throw DDArgumentException("Value out of range");
    }
    if (mv == 0 || mv > base.mvdd_var_count()) {
        throw DDArgumentException("Invalid MVDD variable number");
    }

    DDManager& mgr = *base.manager_;
    const auto& dd_vars = base.dd_vars_of(mv);
    int k = base.k();

    // [mv = value] を表すBDDを構築
    // value = 0: !x_1 & !x_2 & ... & !x_{k-1}
    // value = i (i > 0): !x_1 & ... & !x_{i-1} & x_i

    BDD result = BDD::one(mgr);

    for (int i = 0; i < k - 1; ++i) {
        BDD var_bdd = BDD::var(mgr, dd_vars[i]);

        if (value == 0) {
            // 全て x_i = 0
            result = result & ~var_bdd;
        } else if (i < value - 1) {
            // x_i = 0
            result = result & ~var_bdd;
        } else if (i == value - 1) {
            // x_i = 1
            result = result & var_bdd;
            break;  // これ以降の変数は don't care
        }
    }

    return MVBDD(base.manager_, base.var_table_, result);
}

inline MVBDD MVBDD::ite(const MVBDD& base, bddvar mv,
                        const std::vector<MVBDD>& children) {
    if (!base.is_valid()) {
        throw DDArgumentException("Base MVBDD is not valid");
    }
    if (static_cast<int>(children.size()) != base.k()) {
        throw DDArgumentException("Number of children must equal k");
    }
    if (mv == 0 || mv > base.mvdd_var_count()) {
        throw DDArgumentException("Invalid MVDD variable number");
    }

    DDManager& mgr = *base.manager_;
    const auto& dd_vars = base.dd_vars_of(mv);
    int k = base.k();

    // エンコード方式:
    // x_1 = 1 -> children[1]
    // x_1 = 0, x_2 = 1 -> children[2]
    // x_1 = 0, x_2 = 0, x_3 = 1 -> children[3]
    // ...
    // 全て 0 -> children[0]

    // 最下位から構築（ITE形式）
    BDD result = children[0].to_bdd();  // デフォルト（全て0の場合）

    for (int i = k - 2; i >= 0; --i) {
        bddvar dv = dd_vars[i];
        BDD hi = children[i + 1].to_bdd();  // i+1 番目の値
        BDD lo = result;

        // BDD ITE: if dv then hi else lo
        BDD var_bdd = BDD::var(mgr, dv);
        result = (var_bdd & hi) | (~var_bdd & lo);
    }

    return MVBDD(base.manager_, base.var_table_, result);
}

inline MVBDD MVBDD::child(int value) const {
    if (!is_valid()) {
        throw DDArgumentException("MVBDD is not valid");
    }
    if (value < 0 || value >= k()) {
        throw DDArgumentException("Value out of range");
    }

    if (bdd_.is_terminal()) {
        return *this;
    }

    // トップ変数を取得
    bddvar top_dv = bdd_.top();
    bddvar top_mv = mvdd_var_of(top_dv);

    if (top_mv == 0) {
        // 内部変数がMVDD変数にマッピングされていない場合
        return *this;
    }

    const auto& dd_vars = dd_vars_of(top_mv);

    // value に対応する余因子を取得
    BDD result = bdd_;

    for (int i = 0; i < static_cast<int>(dd_vars.size()); ++i) {
        if (value == 0) {
            // 全ての変数で 0-余因子を取る
            result = result.at0(dd_vars[i]);
        } else if (i < value - 1) {
            // x_i = 0
            result = result.at0(dd_vars[i]);
        } else if (i == value - 1) {
            // x_i = 1
            result = result.at1(dd_vars[i]);
            break;  // これ以降は don't care
        }
    }

    return MVBDD(manager_, var_table_, result);
}

inline bddvar MVBDD::top_var() const {
    if (!is_valid() || bdd_.is_terminal()) {
        return 0;
    }

    bddvar top_dv = bdd_.top();
    return mvdd_var_of(top_dv);
}

inline bool MVBDD::evaluate(const std::vector<int>& assignment) const {
    if (!is_valid()) {
        return false;
    }

    // MVDD割り当てを内部BDD割り当てに変換
    std::vector<bool> bdd_assignment(manager_->var_count() + 1, false);

    for (bddvar mv = 1; mv <= mvdd_var_count(); ++mv) {
        int value = (mv <= assignment.size()) ? assignment[mv - 1] : 0;
        if (value < 0 || value >= k()) {
            throw DDArgumentException("Invalid assignment value");
        }

        const auto& dd_vars = dd_vars_of(mv);

        // One-Hotエンコード: value-1番目の変数を1にする
        // value=0: 全て0
        // value=i (i>0): dd_vars[i-1]を1にする
        for (size_t i = 0; i < dd_vars.size(); ++i) {
            bddvar dv = dd_vars[i];
            if (dv < bdd_assignment.size()) {
                bdd_assignment[dv] = (value > 0 && static_cast<int>(i) == value - 1);
            }
        }
    }

    // BDDを評価
    Arc current = bdd_.arc();
    while (!current.is_constant()) {
        bddvar v = manager_->node_at(current.index()).var();
        bool val = (v < bdd_assignment.size()) ? bdd_assignment[v] : false;

        Arc arc0 = manager_->node_at(current.index()).arc0();
        Arc arc1 = manager_->node_at(current.index()).arc1();

        // 否定枝の処理
        if (current.is_negated()) {
            arc0 = arc0.negated();
            arc1 = arc1.negated();
        }

        current = val ? arc1 : arc0;
    }

    bool result = current.terminal_value();
    if (current.is_negated()) result = !result;
    return result;
}

inline std::size_t MVBDD::mvbdd_node_count() const {
    if (!is_valid() || bdd_.is_terminal()) {
        return 0;
    }

    // MVBDD ノード数を数える
    std::unordered_map<bddindex, bool> visited;
    std::unordered_map<bddvar, bool> mvbdd_nodes;

    std::function<void(Arc)> count_nodes = [&](Arc a) {
        if (a.is_constant()) return;

        bddindex idx = a.index();
        if (visited.find(idx) != visited.end()) return;
        visited[idx] = true;

        bddvar dv = manager_->node_at(idx).var();
        bddvar mv = mvdd_var_of(dv);
        if (mv > 0) {
            mvbdd_nodes[mv] = true;
        }

        count_nodes(manager_->node_at(idx).arc0());
        count_nodes(manager_->node_at(idx).arc1());
    };

    count_nodes(bdd_.arc());
    return mvbdd_nodes.size();
}

} // namespace sbdd2

#endif // SBDD2_MVBDD_HPP
