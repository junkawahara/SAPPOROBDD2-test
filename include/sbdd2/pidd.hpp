/**
 * @file pidd.hpp
 * @brief PiDD（順列決定図）クラスの定義。順列の集合を ZDD で表現する。
 *
 * 順列を隣接互換の積として ZDD 上にエンコードし、
 * 順列集合に対する集合演算や合成演算を効率的に行う。
 *
 * @see ZDD
 * @see DDManager
 * @see SeqBDD
 */

// SAPPOROBDD 2.0 - PiDD (Permutation DD) class
// MIT License

#ifndef SBDD2_PIDD_HPP
#define SBDD2_PIDD_HPP

#include "zdd.hpp"
#include <vector>

namespace sbdd2 {

/**
 * @brief 順列決定図（PiDD）クラス。順列の集合を ZDD で表現する。
 *
 * PiDD は順列を隣接互換（transposition）の積としてエンコードし、
 * ZDD 上で順列集合を効率的に表現する。集合演算（和・積・差）に加えて、
 * 順列の合成・商・剰余などの代数的演算をサポートする。
 *
 * @see ZDD
 * @see DDManager
 * @see SeqBDD
 */
class PiDD {
public:
    /** @brief 使用可能な最大変数数 */
    static constexpr int MAX_VAR = 254;

private:
    ZDD zdd_;

    // Variable mapping tables
    static std::vector<int> lev_of_x_;  // Level of variable x
    static std::vector<int> x_of_lev_;  // Variable at level
    static int top_var_;
    static DDManager* manager_;

public:
    /**
     * @brief デフォルトコンストラクタ。空集合で初期化する。
     */
    PiDD() : zdd_() {}

    /**
     * @brief ZDD から PiDD を構築するコンストラクタ
     * @param zdd 元となる ZDD
     */
    explicit PiDD(const ZDD& zdd) : zdd_(zdd) {}

    /** @brief コピーコンストラクタ */
    PiDD(const PiDD&) = default;
    /** @brief ムーブコンストラクタ */
    PiDD(PiDD&&) noexcept = default;
    /** @brief コピー代入演算子 */
    PiDD& operator=(const PiDD&) = default;
    /** @brief ムーブ代入演算子 */
    PiDD& operator=(PiDD&&) noexcept = default;

    /**
     * @brief PiDD システムを初期化する
     * @param mgr 使用する DDManager への参照
     */
    static void init(DDManager& mgr);

    /**
     * @brief 新しい変数を生成する
     * @return 生成された変数の番号
     */
    static int new_var();

    /**
     * @brief 使用中の変数の数を取得する
     * @return 使用中の変数数
     */
    static int var_used();

    /**
     * @brief 空集合を生成する
     * @return 空集合を表す PiDD
     */
    static PiDD empty();

    /**
     * @brief 恒等順列のみを含む集合を生成する
     * @return 恒等順列のみを含む PiDD
     */
    static PiDD single();

    /**
     * @brief 単一の互換 (x, y) のみを含む集合を生成する
     * @param x 互換の第1要素
     * @param y 互換の第2要素
     * @return 互換 (x, y) のみを含む PiDD
     */
    static PiDD singleton(int x, int y);

    /**
     * @brief 積集合（共通部分）を計算する
     * @param other もう一方の PiDD
     * @return 積集合を表す PiDD
     */
    PiDD operator&(const PiDD& other) const;

    /**
     * @brief 和集合を計算する
     * @param other もう一方の PiDD
     * @return 和集合を表す PiDD
     */
    PiDD operator+(const PiDD& other) const;

    /**
     * @brief 差集合を計算する
     * @param other もう一方の PiDD
     * @return 差集合を表す PiDD
     */
    PiDD operator-(const PiDD& other) const;

    /**
     * @brief 順列の合成を計算する
     * @param other もう一方の PiDD
     * @return 合成結果を表す PiDD
     */
    PiDD operator*(const PiDD& other) const;

    /**
     * @brief 順列の商を計算する
     * @param other もう一方の PiDD
     * @return 商を表す PiDD
     */
    PiDD operator/(const PiDD& other) const;

    /**
     * @brief 順列の剰余を計算する
     * @param other もう一方の PiDD
     * @return 剰余を表す PiDD
     */
    PiDD operator%(const PiDD& other) const;

    /**
     * @brief 積集合の複合代入演算子
     * @param other もう一方の PiDD
     * @return 自身への参照
     */
    PiDD& operator&=(const PiDD& other);

    /**
     * @brief 和集合の複合代入演算子
     * @param other もう一方の PiDD
     * @return 自身への参照
     */
    PiDD& operator+=(const PiDD& other);

    /**
     * @brief 差集合の複合代入演算子
     * @param other もう一方の PiDD
     * @return 自身への参照
     */
    PiDD& operator-=(const PiDD& other);

    /**
     * @brief 順列合成の複合代入演算子
     * @param other もう一方の PiDD
     * @return 自身への参照
     */
    PiDD& operator*=(const PiDD& other);

    /**
     * @brief 順列商の複合代入演算子
     * @param other もう一方の PiDD
     * @return 自身への参照
     */
    PiDD& operator/=(const PiDD& other);

    /**
     * @brief 順列剰余の複合代入演算子
     * @param other もう一方の PiDD
     * @return 自身への参照
     */
    PiDD& operator%=(const PiDD& other);

    /**
     * @brief 全順列に対して要素 x と y を交換する
     * @param x 交換する第1要素
     * @param y 交換する第2要素
     * @return 交換後の順列集合を表す PiDD
     */
    PiDD swap(int x, int y) const;

    /**
     * @brief (x, y) に関するコファクターを計算する
     * @param x コファクターの第1要素
     * @param y コファクターの第2要素
     * @return コファクター結果を表す PiDD
     */
    PiDD cofact(int x, int y) const;

    /**
     * @brief 奇順列のみを抽出する
     * @return 奇順列のみを含む PiDD
     */
    PiDD odd() const;

    /**
     * @brief 偶順列のみを抽出する
     * @return 偶順列のみを含む PiDD
     */
    PiDD even() const;

    /**
     * @brief 互換数が n 以下の順列のみを抽出する
     * @param n 互換数の上限
     * @return 条件を満たす順列集合を表す PiDD
     */
    PiDD swap_bound(int n) const;

    /**
     * @brief 根ノードの x 値を取得する
     * @return 根ノードの x 値
     */
    int top_x() const;

    /**
     * @brief 根ノードの y 値を取得する
     * @return 根ノードの y 値
     */
    int top_y() const;

    /**
     * @brief 根ノードのレベルを取得する
     * @return 根ノードのレベル
     */
    int top_lev() const;

    /**
     * @brief ZDD ノード数を取得する
     * @return ノード数
     */
    std::size_t size() const;

    /**
     * @brief 含まれる順列の数を取得する
     * @return 順列の数（浮動小数点数）
     */
    double card() const;

    /**
     * @brief 内部の ZDD への const 参照を取得する
     * @return 内部 ZDD への const 参照
     * @see ZDD
     */
    const ZDD& get_zdd() const { return zdd_; }

    /**
     * @brief 等値比較演算子
     * @param other 比較対象の PiDD
     * @return 等しい場合 true
     */
    bool operator==(const PiDD& other) const { return zdd_ == other.zdd_; }

    /**
     * @brief 非等値比較演算子
     * @param other 比較対象の PiDD
     * @return 等しくない場合 true
     */
    bool operator!=(const PiDD& other) const { return zdd_ != other.zdd_; }

    /**
     * @brief PiDD の内容を文字列として取得する
     * @return 文字列表現
     */
    std::string to_string() const;

    /**
     * @brief PiDD の内容を標準出力に表示する
     */
    void print() const;

    /**
     * @brief 含まれる全順列を列挙する
     * @return 各順列を int のベクタとして格納したベクタ
     */
    std::vector<std::vector<int>> enumerate() const;

private:
    // Helper functions
    static int x_of_level(int lev);
    static int y_of_level(int lev);
    static int level_of_xy(int x, int y);
};

} // namespace sbdd2

#endif // SBDD2_PIDD_HPP
