/**
 * @file mvzdd.hpp
 * @brief SAPPOROBDD 2.0 - Multi-Valued ZDD
 * @author SAPPOROBDD Team
 * @copyright MIT License
 *
 * 多値変数を扱うZDD。各変数は k 個の値 {0, 1, ..., k-1} を取ることができます。
 * 内部的には通常のZDDでエミュレートして実装されています。
 */

#ifndef SBDD2_MVZDD_HPP
#define SBDD2_MVZDD_HPP

#include "mvdd_base.hpp"
#include "zdd.hpp"
#include <initializer_list>

namespace sbdd2 {

/**
 * @brief Multi-Valued ZDD
 *
 * 多値変数を扱うZDD。各変数は k 個の値 {0, 1, ..., k-1} を取ることができます。
 *
 * エンコード方式（One-Hot風、k=4の例）:
 * - MVZDD変数 x に対して、内部変数 x_1, x_2, x_3 を使用
 * - x=1: x_1 の 1-arc を選択
 * - x=2: x_1 の 0-arc → x_2 の 1-arc を選択
 * - x=3: x_1 の 0-arc → x_2 の 0-arc → x_3 の 1-arc を選択
 * - x=0: 全ての内部変数の 0-arc を選択
 *
 * 縮約規則: ZDD縮約のみ（1-arc以外が全て0終端なら縮約）
 *
 * @code{.cpp}
 * DDManager mgr;
 * MVZDD f = MVZDD::empty(mgr, 4);  // k=4
 * f.new_var();  // MVDD変数1を作成
 * f.new_var();  // MVDD変数2を作成
 *
 * // 単一要素: 変数1が値2を取る
 * MVZDD s = MVZDD::single(f, 1, 2);
 *
 * // ITE構築
 * MVZDD t = MVZDD::ite(f, 1, {child0, child1, child2, child3});
 * @endcode
 */
class MVZDD : public MVDDBase {
private:
    ZDD zdd_;  ///< 内部ZDD表現

public:
    /// @name コンストラクタ
    /// @{

    /// デフォルトコンストラクタ（無効なMVZDD）
    MVZDD() : MVDDBase(), zdd_() {}

    /**
     * @brief マネージャーと値域サイズから構築（空集合族）
     * @param mgr DDマネージャーへのポインタ
     * @param k 値域サイズ
     */
    MVZDD(DDManager* mgr, int k) : MVDDBase(mgr, k), zdd_(ZDD::empty(*mgr)) {}

    /**
     * @brief 変数テーブルと内部ZDDから構築
     * @param mgr DDマネージャーへのポインタ
     * @param table 変数マッピングテーブル
     * @param zdd 内部ZDD
     */
    MVZDD(DDManager* mgr, std::shared_ptr<MVDDVarTable> table, const ZDD& zdd)
        : MVDDBase(mgr, table), zdd_(zdd) {}

    /// コピーコンストラクタ
    MVZDD(const MVZDD& other) = default;

    /// ムーブコンストラクタ
    MVZDD(MVZDD&& other) noexcept = default;

    /// コピー代入演算子
    MVZDD& operator=(const MVZDD& other) = default;

    /// ムーブ代入演算子
    MVZDD& operator=(MVZDD&& other) noexcept = default;

    /// @}

    /// @name ファクトリメソッド
    /// @{

    /**
     * @brief 空集合族を作成
     * @param mgr DDマネージャー
     * @param k 値域サイズ
     * @return 空のMVZDD
     */
    static MVZDD empty(DDManager& mgr, int k) {
        return MVZDD(&mgr, k);
    }

    /**
     * @brief 基底集合族を作成
     * @param mgr DDマネージャー
     * @param k 値域サイズ
     * @return 空集合のみを含むMVZDD {()}
     */
    static MVZDD base(DDManager& mgr, int k) {
        MVZDD result(&mgr, k);
        result.zdd_ = ZDD::base(mgr);
        return result;
    }

    /**
     * @brief 単一要素を作成
     * @param base 変数マッピング情報を持つMVZDD
     * @param mv MVDD変数番号
     * @param value 値 (0 to k-1)
     * @return {{(mv, value)}}
     */
    static MVZDD single(const MVZDD& base, bddvar mv, int value);

    /// @}

    /// @name 変数管理
    /// @{

    /**
     * @brief 新しいMVDD変数を追加
     * @return 新しいMVDD変数番号
     */
    bddvar new_var() {
        auto dd_vars = manager_->new_mvdd_var(k());
        return register_mvdd_var(dd_vars);
    }

    /// @}

    /// @name ITE構築
    /// @{

    /**
     * @brief ITE形式でMVZDDノードを構築
     *
     * 変数mvの各値に対応する子ノードを指定して構築します。
     *
     * @param base 変数マッピング情報
     * @param mv MVDD変数番号
     * @param children k個の子MVZDD (children[i] = 変数mvが値iを取る場合)
     * @return 構築されたMVZDD
     */
    static MVZDD ite(const MVZDD& base, bddvar mv,
                     const std::vector<MVZDD>& children);

    /// initializer_list版
    static MVZDD ite(const MVZDD& base, bddvar mv,
                     std::initializer_list<MVZDD> children) {
        return ite(base, mv, std::vector<MVZDD>(children));
    }

    /// @}

    /// @name 子ノードアクセス
    /// @{

    /**
     * @brief 指定値に対応する子を取得
     * @param value 値 (0 to k-1)
     * @return 子MVZDD
     */
    MVZDD child(int value) const;

    /**
     * @brief トップ変数のMVDD変数番号を取得
     * @return MVDD変数番号 (終端の場合は0)
     */
    bddvar top_var() const;

    /// @}

    /// @name 集合族演算
    /// @{

    /// 和集合（Union）
    MVZDD operator+(const MVZDD& other) const {
        check_compatible(other);
        return MVZDD(manager_, var_table_, zdd_ + other.zdd_);
    }

    /// 差集合（Difference）
    MVZDD operator-(const MVZDD& other) const {
        check_compatible(other);
        return MVZDD(manager_, var_table_, zdd_ - other.zdd_);
    }

    /// 積集合（Intersection）
    MVZDD operator*(const MVZDD& other) const {
        check_compatible(other);
        return MVZDD(manager_, var_table_, zdd_ * other.zdd_);
    }

    /// 複合代入演算子
    MVZDD& operator+=(const MVZDD& other) {
        check_compatible(other);
        zdd_ += other.zdd_;
        return *this;
    }

    MVZDD& operator-=(const MVZDD& other) {
        check_compatible(other);
        zdd_ -= other.zdd_;
        return *this;
    }

    MVZDD& operator*=(const MVZDD& other) {
        check_compatible(other);
        zdd_ *= other.zdd_;
        return *this;
    }

    /// @}

    /// @name カウント演算
    /// @{

    /**
     * @brief 集合族に含まれる集合の数
     * @return |F|
     */
    double card() const { return zdd_.card(); }

    /// @}

    /// @name 評価
    /// @{

    /**
     * @brief 変数割り当てに対する評価
     * @param assignment MVDD変数番号 -> 値のマッピング (0-indexed配列、要素数はmvdd_var_count以上)
     * @return その割り当てが集合族に含まれるかどうか
     */
    bool evaluate(const std::vector<int>& assignment) const;

    /**
     * @brief 全ての充足割り当てを列挙
     * @return 各割り当て（MVDD変数番号 -> 値のベクタ）のリスト
     */
    std::vector<std::vector<int>> all_sat() const;

    /// @}

    /// @name 変換
    /// @{

    /// 内部ZDD表現を取得
    const ZDD& to_zdd() const { return zdd_; }

    /**
     * @brief ZDDからMVZDDを構築
     * @param base 変数マッピング情報
     * @param zdd 内部ZDD
     * @return MVZDD
     */
    static MVZDD from_zdd(const MVZDD& base, const ZDD& zdd) {
        return MVZDD(base.manager_, base.var_table_, zdd);
    }

    /// @}

    /// @name 判定
    /// @{

    /// 空集合族かどうか
    bool is_empty() const { return zdd_.is_zero(); }

    /// 基底集合族かどうか
    bool is_base() const { return zdd_.is_one(); }

    /// 終端かどうか
    bool is_terminal() const { return zdd_.is_terminal(); }

    /// @}

    /// @name ノード数
    /// @{

    /**
     * @brief MVZDDとしての論理ノード数を計算
     *
     * 内部ZDDノードではなく、MVDD変数ごとにカウントします。
     *
     * @return MVZDDノード数
     */
    std::size_t mvzdd_node_count() const;

    /// 内部ZDDのノード数
    std::size_t size() const { return zdd_.size(); }

    /// @}

    /// @name 比較
    /// @{

    bool operator==(const MVZDD& other) const {
        return k() == other.k() && zdd_ == other.zdd_;
    }

    bool operator!=(const MVZDD& other) const {
        return !(*this == other);
    }

    /// @}

private:
    /// 互換性チェック
    void check_compatible(const MVZDD& other) const {
        if (k() != other.k()) {
            throw DDArgumentException("MVZDD k values must match");
        }
    }

    /// 内部ヘルパー: MVDD変数のトップ内部変数を取得
    bddvar get_top_dd_var(bddvar mv) const {
        const auto& dd_vars = dd_vars_of(mv);
        return dd_vars.empty() ? 0 : dd_vars[0];
    }
};

// Implementation

inline MVZDD MVZDD::single(const MVZDD& base, bddvar mv, int value) {
    if (!base.is_valid()) {
        throw DDArgumentException("Base MVZDD is not valid");
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

    if (value == 0) {
        // value=0: 全ての内部変数の0-arcをたどる -> base（空集合を含む）
        return MVZDD(base.manager_, base.var_table_, ZDD::base(mgr));
    }

    // value=i (1 <= i <= k-1): x_{value}の1-arcを選択
    // 構築: 内部変数 dd_vars[value-1] を含む単一要素集合
    ZDD result = ZDD::single(mgr, dd_vars[value - 1]);

    return MVZDD(base.manager_, base.var_table_, result);
}

inline MVZDD MVZDD::ite(const MVZDD& base, bddvar mv,
                        const std::vector<MVZDD>& children) {
    if (!base.is_valid()) {
        throw DDArgumentException("Base MVZDD is not valid");
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
    // x_1 の 1-arc -> children[1]
    // x_1 の 0-arc -> x_2 へ
    // x_2 の 1-arc -> children[2]
    // x_2 の 0-arc -> x_3 へ
    // ...
    // x_{k-1} の 1-arc -> children[k-1]
    // x_{k-1} の 0-arc -> children[0]

    // 最下位から構築
    ZDD result = children[0].to_zdd();  // 0-arc の終着点

    for (int i = k - 2; i >= 0; --i) {
        bddvar dv = dd_vars[i];
        ZDD hi = children[i + 1].to_zdd();  // i+1 番目の値
        ZDD lo = result;

        // ZDD ITE: get_or_create_node_zdd を使用
        // hi が空なら lo を返す（ZDD縮約）
        if (hi.is_zero()) {
            result = lo;
        } else {
            Arc arc = mgr.get_or_create_node_zdd(dv, lo.arc(), hi.arc(), true);
            result = ZDD(&mgr, arc);
        }
    }

    return MVZDD(base.manager_, base.var_table_, result);
}

inline MVZDD MVZDD::child(int value) const {
    if (!is_valid()) {
        throw DDArgumentException("MVZDD is not valid");
    }
    if (value < 0 || value >= k()) {
        throw DDArgumentException("Value out of range");
    }

    if (zdd_.is_terminal()) {
        return *this;
    }

    // トップ変数を取得
    bddvar top_dv = zdd_.top();
    bddvar top_mv = mvdd_var_of(top_dv);

    if (top_mv == 0) {
        // 内部変数がMVDD変数にマッピングされていない場合
        return *this;
    }

    const auto& dd_vars = dd_vars_of(top_mv);

    // value に対応するアークをたどる
    ZDD current = zdd_;

    for (int i = 0; i < static_cast<int>(dd_vars.size()); ++i) {
        if (current.is_terminal()) break;
        if (current.top() != dd_vars[i]) {
            // この変数がスキップされている場合は 0-arc と同じ
            continue;
        }

        if (i == value - 1 && value > 0) {
            // この変数の1-arcを選択
            current = current.high();
            return MVZDD(manager_, var_table_, current);
        } else {
            // この変数の0-arcを選択
            current = current.low();
        }
    }

    // value == 0 または全ての変数をたどった場合
    return MVZDD(manager_, var_table_, current);
}

inline bddvar MVZDD::top_var() const {
    if (!is_valid() || zdd_.is_terminal()) {
        return 0;
    }

    bddvar top_dv = zdd_.top();
    return mvdd_var_of(top_dv);
}

inline bool MVZDD::evaluate(const std::vector<int>& assignment) const {
    if (!is_valid()) {
        return false;
    }

    ZDD current = zdd_;

    // 各MVDD変数について、対応する内部変数をたどる
    for (bddvar mv = 1; mv <= mvdd_var_count(); ++mv) {
        int value = (mv <= assignment.size()) ? assignment[mv - 1] : 0;
        if (value < 0 || value >= k()) {
            throw DDArgumentException("Invalid assignment value");
        }

        // ZDD が終端の場合
        if (current.is_terminal()) {
            // 0-終端なら false
            if (current.is_zero()) {
                return false;
            }
            // 1-終端で value > 0 なら false（選ぶべき変数がないのに値を選ぼうとしている）
            if (value > 0) {
                return false;
            }
            // 1-終端で value == 0 なら、残りの変数も全て 0 でなければならない
            continue;
        }

        const auto& dd_vars = dd_vars_of(mv);

        // この MVDD 変数の内部変数をたどる
        for (int i = 0; i < static_cast<int>(dd_vars.size()); ++i) {
            if (current.is_terminal()) {
                // 途中で終端に到達
                if (current.is_zero()) {
                    return false;
                }
                // 1-終端で value > 0 で、まだ対応する変数を見ていない
                if (value > 0 && i <= value - 1) {
                    return false;
                }
                break;
            }

            // 現在の内部変数がこのMVDD変数のものでない場合はスキップ
            if (current.top() != dd_vars[i]) {
                // ZDDでは、スキップされた変数は0-arcと同等
                // つまり、その変数の1-arcを選ばない
                if (i == value - 1 && value > 0) {
                    // value に対応する変数がスキップされている = その値は選べない
                    return false;
                }
                continue;
            }

            if (i == value - 1 && value > 0) {
                // この値を選択：1-arcをたどる
                current = current.high();
                break;  // この MVDD 変数の処理完了
            } else {
                // この値を選択しない：0-arcをたどる
                current = current.low();
            }
        }
    }

    // 終端に到達：1-終端なら true、0-終端なら false
    return current.is_one();
}

inline std::vector<std::vector<int>> MVZDD::all_sat() const {
    std::vector<std::vector<int>> result;

    if (!is_valid() || is_empty()) {
        return result;
    }

    // 内部 ZDD の全集合を列挙
    auto zdd_sets = zdd_.enumerate();

    for (const auto& zdd_set : zdd_sets) {
        std::vector<int> mvdd_assignment(mvdd_var_count(), 0);

        // 内部変数の集合から MVDD 割り当てを復元
        for (bddvar dv : zdd_set) {
            bddvar mv = mvdd_var_of(dv);
            if (mv > 0) {
                int idx = dd_var_index(dv);
                if (idx >= 0) {
                    // value = idx + 1
                    mvdd_assignment[mv - 1] = idx + 1;
                }
            }
        }

        result.push_back(mvdd_assignment);
    }

    return result;
}

inline std::size_t MVZDD::mvzdd_node_count() const {
    if (!is_valid() || zdd_.is_terminal()) {
        return 0;
    }

    // MVDD ノード数を数える
    // 同じ MVDD 変数に属する内部ノードは1つのMVDDノードとしてカウント

    std::unordered_map<bddindex, bool> visited;
    std::unordered_map<bddvar, bool> mvdd_nodes;  // MVDD変数ごとのノード存在

    std::function<void(Arc)> count_nodes = [&](Arc a) {
        if (a.is_constant()) return;

        bddindex idx = a.index();
        if (visited.find(idx) != visited.end()) return;
        visited[idx] = true;

        bddvar dv = manager_->node_at(idx).var();
        bddvar mv = mvdd_var_of(dv);
        if (mv > 0) {
            mvdd_nodes[mv] = true;
        }

        count_nodes(manager_->node_at(idx).arc0());
        count_nodes(manager_->node_at(idx).arc1());
    };

    count_nodes(zdd_.arc());
    return mvdd_nodes.size();
}

} // namespace sbdd2

#endif // SBDD2_MVZDD_HPP
