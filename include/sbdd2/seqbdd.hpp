/**
 * @file seqbdd.hpp
 * @brief SeqBDD（系列BDD）クラスの定義。系列（シーケンス）の集合を ZDD で表現する。
 *
 * 系列の集合を ZDD 上にエンコードし、集合演算や連接演算などの
 * 系列操作を効率的に行う。
 *
 * @see ZDD
 * @see DDManager
 * @see PiDD
 */

// SAPPOROBDD 2.0 - SeqBDD (Sequence BDD) class
// MIT License

#ifndef SBDD2_SEQBDD_HPP
#define SBDD2_SEQBDD_HPP

#include "zdd.hpp"
#include <vector>
#include <string>

namespace sbdd2 {

/**
 * @brief 系列BDD（SeqBDD）クラス。系列（シーケンス）の集合を ZDD で表現する。
 *
 * SeqBDD は記号列（系列）の集合を ZDD 上で効率的に表現する。
 * 集合演算（和・積・差）に加えて、連接（concatenation）、
 * 商、剰余などの系列固有の演算をサポートする。
 *
 * @see ZDD
 * @see DDManager
 * @see PiDD
 */
class SeqBDD {
private:
    ZDD zdd_;

public:
    /**
     * @brief デフォルトコンストラクタ。空集合で初期化する。
     */
    SeqBDD() : zdd_() {}

    /**
     * @brief ZDD から SeqBDD を構築するコンストラクタ
     * @param zdd 元となる ZDD
     */
    explicit SeqBDD(const ZDD& zdd) : zdd_(zdd) {}

    /** @brief コピーコンストラクタ */
    SeqBDD(const SeqBDD&) = default;
    /** @brief ムーブコンストラクタ */
    SeqBDD(SeqBDD&&) noexcept = default;
    /** @brief コピー代入演算子 */
    SeqBDD& operator=(const SeqBDD&) = default;
    /** @brief ムーブ代入演算子 */
    SeqBDD& operator=(SeqBDD&&) noexcept = default;

    /**
     * @brief 空集合を生成する
     * @param mgr 使用する DDManager への参照
     * @return 空集合を表す SeqBDD
     */
    static SeqBDD empty(DDManager& mgr);

    /**
     * @brief 空系列のみを含む集合を生成する
     * @param mgr 使用する DDManager への参照
     * @return 空系列のみを含む SeqBDD
     */
    static SeqBDD single(DDManager& mgr);

    /**
     * @brief 単一要素 v のみからなる系列の集合を生成する
     * @param mgr 使用する DDManager への参照
     * @param v 要素の変数番号
     * @return 系列 {v} のみを含む SeqBDD
     */
    static SeqBDD singleton(DDManager& mgr, bddvar v);

    /**
     * @brief 積集合（共通部分）を計算する
     * @param other もう一方の SeqBDD
     * @return 積集合を表す SeqBDD
     */
    SeqBDD operator&(const SeqBDD& other) const;

    /**
     * @brief 和集合を計算する
     * @param other もう一方の SeqBDD
     * @return 和集合を表す SeqBDD
     */
    SeqBDD operator+(const SeqBDD& other) const;

    /**
     * @brief 差集合を計算する
     * @param other もう一方の SeqBDD
     * @return 差集合を表す SeqBDD
     */
    SeqBDD operator-(const SeqBDD& other) const;

    /**
     * @brief 系列の連接（concatenation）を計算する
     * @param other もう一方の SeqBDD
     * @return 連接結果を表す SeqBDD
     */
    SeqBDD operator*(const SeqBDD& other) const;

    /**
     * @brief 系列の商を計算する
     * @param other もう一方の SeqBDD
     * @return 商を表す SeqBDD
     */
    SeqBDD operator/(const SeqBDD& other) const;

    /**
     * @brief 系列の剰余を計算する
     * @param other もう一方の SeqBDD
     * @return 剰余を表す SeqBDD
     */
    SeqBDD operator%(const SeqBDD& other) const;

    /**
     * @brief 積集合の複合代入演算子
     * @param other もう一方の SeqBDD
     * @return 自身への参照
     */
    SeqBDD& operator&=(const SeqBDD& other);

    /**
     * @brief 和集合の複合代入演算子
     * @param other もう一方の SeqBDD
     * @return 自身への参照
     */
    SeqBDD& operator+=(const SeqBDD& other);

    /**
     * @brief 差集合の複合代入演算子
     * @param other もう一方の SeqBDD
     * @return 自身への参照
     */
    SeqBDD& operator-=(const SeqBDD& other);

    /**
     * @brief 連接の複合代入演算子
     * @param other もう一方の SeqBDD
     * @return 自身への参照
     */
    SeqBDD& operator*=(const SeqBDD& other);

    /**
     * @brief 商の複合代入演算子
     * @param other もう一方の SeqBDD
     * @return 自身への参照
     */
    SeqBDD& operator/=(const SeqBDD& other);

    /**
     * @brief 剰余の複合代入演算子
     * @param other もう一方の SeqBDD
     * @return 自身への参照
     */
    SeqBDD& operator%=(const SeqBDD& other);

    /**
     * @brief 変数 v で始まらない系列のみを抽出する
     * @param v 変数番号
     * @return v で始まらない系列の集合を表す SeqBDD
     */
    SeqBDD offset(bddvar v) const;

    /**
     * @brief 変数 v で始まる系列を抽出し、先頭の v を除去する
     * @param v 変数番号
     * @return v で始まる系列から先頭を除去した集合を表す SeqBDD
     */
    SeqBDD onset(bddvar v) const;

    /**
     * @brief 変数 v で始まる系列を抽出する（先頭の v を保持）
     * @param v 変数番号
     * @return v で始まる系列の集合を表す SeqBDD
     */
    SeqBDD onset0(bddvar v) const;

    /**
     * @brief 全系列の先頭に変数 v を追加する
     * @param v 追加する変数番号
     * @return 先頭に v を追加した系列集合を表す SeqBDD
     */
    SeqBDD push(bddvar v) const;

    /**
     * @brief 根ノードの変数番号を取得する
     * @return 根ノードの変数番号
     */
    bddvar top() const;

    /**
     * @brief ZDD ノード数を取得する
     * @return ノード数
     */
    std::size_t size() const;

    /**
     * @brief 含まれる系列の数を取得する
     * @return 系列の数（浮動小数点数）
     */
    double card() const;

    /**
     * @brief 全系列に含まれるリテラルの総数を取得する
     * @return リテラルの総数
     */
    std::size_t lit() const;

    /**
     * @brief 最長系列の長さを取得する
     * @return 最長系列の長さ
     */
    std::size_t len() const;

    /**
     * @brief 内部の ZDD への const 参照を取得する
     * @return 内部 ZDD への const 参照
     * @see ZDD
     */
    const ZDD& get_zdd() const { return zdd_; }

    /**
     * @brief 使用中の DDManager へのポインタを取得する
     * @return DDManager へのポインタ
     * @see DDManager
     */
    DDManager* manager() const { return zdd_.manager(); }

    /**
     * @brief 等値比較演算子
     * @param other 比較対象の SeqBDD
     * @return 等しい場合 true
     */
    bool operator==(const SeqBDD& other) const { return zdd_ == other.zdd_; }

    /**
     * @brief 非等値比較演算子
     * @param other 比較対象の SeqBDD
     * @return 等しくない場合 true
     */
    bool operator!=(const SeqBDD& other) const { return zdd_ != other.zdd_; }

    /**
     * @brief SeqBDD の内容を文字列として取得する
     * @return 文字列表現
     */
    std::string to_string() const;

    /**
     * @brief SeqBDD の内容を標準出力に表示する
     */
    void print() const;

    /**
     * @brief 含まれる全系列を列挙する
     * @return 各系列を bddvar のベクタとして格納したベクタ
     */
    std::vector<std::vector<bddvar>> enumerate() const;
};

} // namespace sbdd2

#endif // SBDD2_SEQBDD_HPP
