/**
 * @file DdSpecOp.hpp
 * @brief TdZdd互換のSpec演算クラス群
 *
 * BDD/ZDDのSpec同士の二項演算（AND, OR, Union, Intersection）、
 * 先読み最適化（Lookahead）、および非簡約化（Unreduction）を提供する。
 *
 * 元のTdZddライブラリ（岩下洋哉氏）に基づく。
 * Copyright (c) 2014 ERATO MINATO Project
 * SAPPOROBDD2名前空間に移植。
 *
 * @see DdSpec.hpp Spec基底クラス
 * @see VarArityDdSpec.hpp 可変アリティSpec
 */

#pragma once

#include <algorithm>
#include <cassert>
#include <cstddef>
#include <iostream>
#include <vector>

#include "DdSpec.hpp"
#include "VarArityDdSpec.hpp"
#include "../exception.hpp"

namespace sbdd2 {
namespace tdzdd {

/**
 * @brief Specの二項演算の基底クラス
 *
 * 2つのSpec (S1, S2) を組み合わせた二項演算の共通機能を提供する。
 * 各ノードの状態として、両Specのレベル情報と状態データを保持する。
 *
 * @tparam S このクラスを実装する派生クラス（CRTP）
 * @tparam S1 第1オペランドのSpec型
 * @tparam S2 第2オペランドのSpec型
 * @see BddAnd BDD AND演算
 * @see BddOr BDD OR演算
 * @see ZddUnion ZDD和集合演算
 */
template<typename S, typename S1, typename S2>
class BinaryOperation: public PodArrayDdSpec<S, std::size_t, 2> {
protected:
    typedef S1 Spec1;
    typedef S2 Spec2;
    typedef std::size_t Word;

    /** @brief レベル情報（2つのint）を格納するのに必要なワード数 */
    static std::size_t const levelWords = (sizeof(int[2]) + sizeof(Word) - 1)
            / sizeof(Word);

    Spec1 spec1;   /**< @brief 第1オペランドのSpec */
    Spec2 spec2;   /**< @brief 第2オペランドのSpec */
    int const stateWords1;  /**< @brief 第1Specの状態サイズ（ワード数） */
    int const stateWords2;  /**< @brief 第2Specの状態サイズ（ワード数） */

    static int wordSize(int size) {
        return (size + sizeof(Word) - 1) / sizeof(Word);
    }

    void setLevel1(void* p, int level) const {
        static_cast<int*>(p)[0] = level;
    }

    int level1(void const* p) const {
        return static_cast<int const*>(p)[0];
    }

    void setLevel2(void* p, int level) const {
        static_cast<int*>(p)[1] = level;
    }

    int level2(void const* p) const {
        return static_cast<int const*>(p)[1];
    }

    void* state1(void* p) const {
        return static_cast<Word*>(p) + levelWords;
    }

    void const* state1(void const* p) const {
        return static_cast<Word const*>(p) + levelWords;
    }

    void* state2(void* p) const {
        return static_cast<Word*>(p) + levelWords + stateWords1;
    }

    void const* state2(void const* p) const {
        return static_cast<Word const*>(p) + levelWords + stateWords1;
    }

public:
    /**
     * @brief コンストラクタ
     * @param s1 第1オペランドのSpec
     * @param s2 第2オペランドのSpec
     */
    BinaryOperation(S1 const& s1, S2 const& s2) :
                    spec1(s1),
                    spec2(s2),
                    stateWords1(wordSize(spec1.datasize())),
                    stateWords2(wordSize(spec2.datasize())) {
        BinaryOperation::setArraySize(levelWords + stateWords1 + stateWords2);
    }

    /**
     * @brief 状態を複製する
     * @param to コピー先
     * @param from コピー元
     */
    void get_copy(void* to, void const* from) {
        setLevel1(to, level1(from));
        setLevel2(to, level2(from));
        spec1.get_copy(state1(to), state1(from));
        spec2.get_copy(state2(to), state2(from));
    }

    /**
     * @brief 2つの状態をマージする
     * @param p1 状態1のデータ領域
     * @param p2 状態2のデータ領域
     * @return マージ結果コード
     */
    int merge_states(void* p1, void* p2) {
        return spec1.merge_states(state1(p1), state1(p2))
                | spec2.merge_states(state2(p1), state2(p2));
    }

    /**
     * @brief 状態を破棄する
     * @param p 状態データ領域
     */
    void destruct(void* p) {
        spec1.destruct(state1(p));
        spec2.destruct(state2(p));
    }

    /**
     * @brief 指定レベルのリソースを解放する
     * @param level レベル番号
     */
    void destructLevel(int level) {
        spec1.destructLevel(level);
        spec2.destructLevel(level);
    }

    /**
     * @brief 状態のハッシュ値を計算する
     * @param p 状態データ領域
     * @param level レベル番号
     * @return ハッシュ値
     */
    std::size_t hash_code(void const* p, int level) const {
        std::size_t h = std::size_t(level1(p)) * 314159257
                + std::size_t(level2(p)) * 271828171;
        if (level1(p) > 0)
            h += spec1.hash_code(state1(p), level1(p)) * 171828143;
        if (level2(p) > 0)
            h += spec2.hash_code(state2(p), level2(p)) * 141421333;
        return h;
    }

    /**
     * @brief 2つの状態が等しいか判定する
     * @param p 状態1のデータ領域
     * @param q 状態2のデータ領域
     * @param level レベル番号
     * @return 等しければtrue
     */
    bool equal_to(void const* p, void const* q, int level) const {
        (void)level;
        if (level1(p) != level1(q)) return false;
        if (level2(p) != level2(q)) return false;
        if (level1(p) > 0 && !spec1.equal_to(state1(p), state1(q), level1(p)))
            return false;
        if (level2(p) > 0 && !spec2.equal_to(state2(p), state2(q), level2(p)))
            return false;
        return true;
    }
};

/**
 * @brief BDD AND演算のSpec
 *
 * 2つのBDD Specの論理積を表すSpecを生成する。
 * いずれかのオペランドが0終端になると、結果も0終端になる。
 *
 * @tparam S1 第1オペランドのSpec型
 * @tparam S2 第2オペランドのSpec型
 * @see bddAnd() 便利関数
 * @see BddOr BDD OR演算
 */
template<typename S1, typename S2>
struct BddAnd: public BinaryOperation<BddAnd<S1,S2>, S1, S2> {
    typedef BinaryOperation<BddAnd<S1,S2>, S1, S2> base;
    typedef typename base::Word Word;

    /**
     * @brief コンストラクタ
     * @param s1 第1オペランドのSpec
     * @param s2 第2オペランドのSpec
     */
    BddAnd(S1 const& s1, S2 const& s2) : base(s1, s2) {}

    /**
     * @brief ルートノードを初期化する
     * @param p 状態配列
     * @return ルートノードのレベル（0: いずれかが0終端）
     */
    int getRoot(Word* p) {
        int i1 = base::spec1.get_root(base::state1(p));
        if (i1 == 0) return 0;
        int i2 = base::spec2.get_root(base::state2(p));
        if (i2 == 0) return 0;
        base::setLevel1(p, i1);
        base::setLevel2(p, i2);
        return std::max(base::level1(p), base::level2(p));
    }

    /**
     * @brief 子ノードのレベルを計算する
     * @param p 状態配列
     * @param level 現在のレベル
     * @param take 分岐の値
     * @return 子ノードのレベル
     */
    int getChild(Word* p, int level, int take) {
        assert(base::level1(p) <= level && base::level2(p) <= level);
        if (base::level1(p) == level) {
            int i1 = base::spec1.get_child(base::state1(p), level, take);
            if (i1 == 0) return 0;
            base::setLevel1(p, i1);
        }
        if (base::level2(p) == level) {
            int i2 = base::spec2.get_child(base::state2(p), level, take);
            if (i2 == 0) return 0;
            base::setLevel2(p, i2);
        }
        return std::max(base::level1(p), base::level2(p));
    }

    /**
     * @brief 状態を出力ストリームに書き出す
     * @param os 出力ストリーム
     * @param p 状態データ領域
     * @param level レベル番号
     */
    void print_state(std::ostream& os, void const* p, int level) const {
        Word const* q = static_cast<Word const*>(p);
        os << "<" << base::level1(q) << ",";
        base::spec1.print_state(os, base::state1(q), level);
        os << "> AND <" << base::level2(q) << ",";
        base::spec2.print_state(os, base::state2(q), level);
        os << ">";
    }
};

/**
 * @brief BDD OR演算のSpec
 *
 * 2つのBDD Specの論理和を表すSpecを生成する。
 * いずれかのオペランドが1終端になると、結果も1終端になる。
 *
 * @tparam S1 第1オペランドのSpec型
 * @tparam S2 第2オペランドのSpec型
 * @see bddOr() 便利関数
 * @see BddAnd BDD AND演算
 */
template<typename S1, typename S2>
struct BddOr: public BinaryOperation<BddOr<S1,S2>, S1, S2> {
    typedef BinaryOperation<BddOr<S1,S2>, S1, S2> base;
    typedef typename base::Word Word;

    /**
     * @brief コンストラクタ
     * @param s1 第1オペランドのSpec
     * @param s2 第2オペランドのSpec
     */
    BddOr(S1 const& s1, S2 const& s2) : base(s1, s2) {}

    /**
     * @brief ルートノードを初期化する
     * @param p 状態配列
     * @return ルートノードのレベル（-1: いずれかが1終端）
     */
    int getRoot(Word* p) {
        int i1 = base::spec1.get_root(base::state1(p));
        if (i1 < 0) return -1;
        int i2 = base::spec2.get_root(base::state2(p));
        if (i2 < 0) return -1;
        base::setLevel1(p, i1);
        base::setLevel2(p, i2);
        return std::max(base::level1(p), base::level2(p));
    }

    /**
     * @brief 子ノードのレベルを計算する
     * @param p 状態配列
     * @param level 現在のレベル
     * @param take 分岐の値
     * @return 子ノードのレベル
     */
    int getChild(Word* p, int level, int take) {
        assert(base::level1(p) <= level && base::level2(p) <= level);

        if (base::level1(p) == level) {
            int i1 = base::spec1.get_child(base::state1(p), level, take);
            if (i1 < 0) return -1;
            base::setLevel1(p, i1);
        }

        if (base::level2(p) == level) {
            int i2 = base::spec2.get_child(base::state2(p), level, take);
            if (i2 < 0) return -1;
            base::setLevel2(p, i2);
        }

        return std::max(base::level1(p), base::level2(p));
    }

    /**
     * @brief 状態を出力ストリームに書き出す
     * @param os 出力ストリーム
     * @param p 状態データ領域
     * @param level レベル番号
     */
    void print_state(std::ostream& os, void const* p, int level) const {
        Word const* q = static_cast<Word const*>(p);
        os << "<" << base::level1(q) << ",";
        base::spec1.print_state(os, base::state1(q), level);
        os << "> OR <" << base::level2(q) << ",";
        base::spec2.print_state(os, base::state2(q), level);
        os << ">";
    }
};

/**
 * @brief ZDD積集合演算のSpec
 *
 * 2つのZDD Specの集合積（共通部分）を表すSpecを生成する。
 * スキップレベルでは0枝を辿ることで、ZDDのセマンティクスに従う。
 *
 * @tparam S1 第1オペランドのSpec型
 * @tparam S2 第2オペランドのSpec型
 * @see zddIntersection() 便利関数
 * @see ZddUnion ZDD和集合演算
 */
template<typename S1, typename S2>
struct ZddIntersection: public PodArrayDdSpec<ZddIntersection<S1,S2>, std::size_t, 2> {
    typedef S1 Spec1;
    typedef S2 Spec2;
    typedef std::size_t Word;

    Spec1 spec1;  /**< @brief 第1オペランドのSpec */
    Spec2 spec2;  /**< @brief 第2オペランドのSpec */
    int const stateWords1;  /**< @brief 第1Specの状態サイズ（ワード数） */
    int const stateWords2;  /**< @brief 第2Specの状態サイズ（ワード数） */

    static int wordSize(int size) {
        return (size + sizeof(Word) - 1) / sizeof(Word);
    }

    void* state1(void* p) const {
        return p;
    }

    void const* state1(void const* p) const {
        return p;
    }

    void* state2(void* p) const {
        return static_cast<Word*>(p) + stateWords1;
    }

    void const* state2(void const* p) const {
        return static_cast<Word const*>(p) + stateWords1;
    }

public:
    /**
     * @brief コンストラクタ
     * @param s1 第1オペランドのSpec
     * @param s2 第2オペランドのSpec
     */
    ZddIntersection(S1 const& s1, S2 const& s2) :
                    spec1(s1),
                    spec2(s2),
                    stateWords1(wordSize(spec1.datasize())),
                    stateWords2(wordSize(spec2.datasize())) {
        ZddIntersection::setArraySize(stateWords1 + stateWords2);
    }

    /**
     * @brief ルートノードを初期化する
     *
     * 両Specのルートレベルが異なる場合、高い方のSpec側で0枝を辿って
     * レベルを揃える。
     *
     * @param p 状態配列
     * @return ルートノードのレベル（0: いずれかが0終端）
     */
    int getRoot(Word* p) {
        int i1 = spec1.get_root(state1(p));
        if (i1 == 0) return 0;
        int i2 = spec2.get_root(state2(p));
        if (i2 == 0) return 0;

        while (i1 != i2) {
            if (i1 > i2) {
                i1 = spec1.get_child(state1(p), i1, 0);
                if (i1 == 0) return 0;
            }
            else {
                i2 = spec2.get_child(state2(p), i2, 0);
                if (i2 == 0) return 0;
            }
        }

        return i1;
    }

    /**
     * @brief 子ノードのレベルを計算する
     *
     * 両Specの子レベルが異なる場合、高い方のSpec側で0枝を辿って
     * レベルを揃える。
     *
     * @param p 状態配列
     * @param level 現在のレベル
     * @param take 分岐の値
     * @return 子ノードのレベル
     */
    int getChild(Word* p, int level, int take) {
        int i1 = spec1.get_child(state1(p), level, take);
        if (i1 == 0) return 0;
        int i2 = spec2.get_child(state2(p), level, take);
        if (i2 == 0) return 0;

        while (i1 != i2) {
            if (i1 > i2) {
                i1 = spec1.get_child(state1(p), i1, 0);
                if (i1 == 0) return 0;
            }
            else {
                i2 = spec2.get_child(state2(p), i2, 0);
                if (i2 == 0) return 0;
            }
        }

        return i1;
    }

    /**
     * @brief 状態を複製する
     * @param to コピー先
     * @param from コピー元
     */
    void get_copy(void* to, void const* from) {
        spec1.get_copy(state1(to), state1(from));
        spec2.get_copy(state2(to), state2(from));
    }

    /**
     * @brief 2つの状態をマージする
     * @param p1 状態1のデータ領域
     * @param p2 状態2のデータ領域
     * @return マージ結果コード
     */
    int merge_states(void* p1, void* p2) {
        return spec1.merge_states(state1(p1), state1(p2))
                | spec2.merge_states(state2(p1), state2(p2));
    }

    /**
     * @brief 状態を破棄する
     * @param p 状態データ領域
     */
    void destruct(void* p) {
        spec1.destruct(state1(p));
        spec2.destruct(state2(p));
    }

    /**
     * @brief 指定レベルのリソースを解放する
     * @param level レベル番号
     */
    void destructLevel(int level) {
        spec1.destructLevel(level);
        spec2.destructLevel(level);
    }

    /**
     * @brief 状態のハッシュ値を計算する
     * @param p 状態データ領域
     * @param level レベル番号
     * @return ハッシュ値
     */
    std::size_t hash_code(void const* p, int level) const {
        return spec1.hash_code(state1(p), level) * 314159257
                + spec2.hash_code(state2(p), level) * 271828171;
    }

    /**
     * @brief 2つの状態が等しいか判定する
     * @param p 状態1のデータ領域
     * @param q 状態2のデータ領域
     * @param level レベル番号
     * @return 等しければtrue
     */
    bool equal_to(void const* p, void const* q, int level) const {
        return spec1.equal_to(state1(p), state1(q), level)
                && spec2.equal_to(state2(p), state2(q), level);
    }

    /**
     * @brief 状態を出力ストリームに書き出す
     * @param os 出力ストリーム
     * @param p 状態データ領域
     * @param level レベル番号
     */
    void print_state(std::ostream& os, void const* p, int level) const {
        Word const* q = static_cast<Word const*>(p);
        os << "<";
        spec1.print_state(os, state1(q), level);
        os << "> INTERSECT <";
        spec2.print_state(os, state2(q), level);
        os << ">";
    }
};

/**
 * @brief ZDD和集合演算のSpec
 *
 * 2つのZDD Specの集合和（合併）を表すSpecを生成する。
 * スキップレベルでは、そのSpecが要素を持たないものとして扱う（ZDDルール）。
 *
 * @tparam S1 第1オペランドのSpec型
 * @tparam S2 第2オペランドのSpec型
 * @see zddUnion() 便利関数
 * @see ZddIntersection ZDD積集合演算
 */
template<typename S1, typename S2>
struct ZddUnion: public BinaryOperation<ZddUnion<S1,S2>, S1, S2> {
    typedef BinaryOperation<ZddUnion<S1,S2>, S1, S2> base;
    typedef typename base::Word Word;

    /**
     * @brief コンストラクタ
     * @param s1 第1オペランドのSpec
     * @param s2 第2オペランドのSpec
     */
    ZddUnion(S1 const& s1, S2 const& s2) : base(s1, s2) {}

    /**
     * @brief ルートノードを初期化する
     * @param p 状態配列
     * @return ルートノードのレベル
     */
    int getRoot(Word* p) {
        int i1 = base::spec1.get_root(base::state1(p));
        int i2 = base::spec2.get_root(base::state2(p));
        if (i1 == 0 && i2 == 0) return 0;
        if (i1 <= 0 && i2 <= 0) return -1;
        base::setLevel1(p, i1);
        base::setLevel2(p, i2);
        return std::max(base::level1(p), base::level2(p));
    }

    /**
     * @brief 子ノードのレベルを計算する
     *
     * take=1（1枝）でスキップレベルの場合、そのSpecを0終端に設定する（ZDDルール）。
     *
     * @param p 状態配列
     * @param level 現在のレベル
     * @param take 分岐の値（0: 0枝, 1: 1枝）
     * @return 子ノードのレベル
     */
    int getChild(Word* p, int level, int take) {
        assert(base::level1(p) <= level && base::level2(p) <= level);

        if (base::level1(p) == level) {
            int i1 = base::spec1.get_child(base::state1(p), level, take);
            base::setLevel1(p, i1);
        }
        else if (take) {
            base::setLevel1(p, 0);
        }

        if (base::level2(p) == level) {
            int i2 = base::spec2.get_child(base::state2(p), level, take);
            base::setLevel2(p, i2);
        }
        else if (take) {
            base::setLevel2(p, 0);
        }

        if (base::level1(p) == 0 && base::level2(p) == 0) return 0;
        if (base::level1(p) <= 0 && base::level2(p) <= 0) return -1;
        return std::max(base::level1(p), base::level2(p));
    }

    /**
     * @brief 状態を出力ストリームに書き出す
     * @param os 出力ストリーム
     * @param p 状態データ領域
     * @param level レベル番号
     */
    void print_state(std::ostream& os, void const* p, int level) const {
        Word const* q = static_cast<Word const*>(p);
        os << "<" << base::level1(q) << ",";
        base::spec1.print_state(os, base::state1(q), level);
        os << "> UNION <" << base::level2(q) << ",";
        base::spec2.print_state(os, base::state2(q), level);
        os << ">";
    }
};

/**
 * @brief BDD先読み最適化のSpecラッパー
 *
 * BDDの仕様を先読みし、BDDノード削除規則に基づいて冗長なノードを
 * スキップする最適化を行う。すべての分岐先が同一なノードを検出して除去する。
 *
 * @tparam S 内部Specの型
 * @see bddLookahead() 便利関数
 * @see ZddLookahead ZDD用の先読み最適化
 */
template<typename S>
class BddLookahead: public DdSpecBase<BddLookahead<S>, S::ARITY> {
    typedef S Spec;

    Spec spec;
    std::vector<char> work0;
    std::vector<char> work1;

    int lookahead(void* p, int level) {
        while (level >= 1) {
            spec.get_copy(work0.data(), p);
            int level0 = spec.get_child(work0.data(), level, 0);

            for (int b = 1; b < Spec::ARITY; ++b) {
                spec.get_copy(work1.data(), p);
                int level1 = spec.get_child(work1.data(), level, b);
                if (!(level0 == level1
                        && (level0 <= 0
                                || spec.equal_to(work0.data(), work1.data(),
                                        level0)))) {
                    spec.destruct(work0.data());
                    spec.destruct(work1.data());
                    return level;
                }
                spec.destruct(work1.data());
            }

            spec.destruct(p);
            spec.get_copy(p, work0.data());
            spec.destruct(work0.data());
            level = level0;
        }

        return level;
    }

public:
    /**
     * @brief コンストラクタ
     * @param s 最適化対象のBDD Spec
     */
    BddLookahead(S const& s)
            : spec(s), work0(spec.datasize()), work1(spec.datasize()) {
    }

    /**
     * @brief 状態データのバイトサイズを返す
     * @return 内部Specのデータサイズ
     */
    int datasize() const {
        return spec.datasize();
    }

    /**
     * @brief ルートノードを初期化し、先読み最適化後のレベルを返す
     * @param p 状態データ領域
     * @return 最適化後のルートレベル
     */
    int get_root(void* p) {
        return lookahead(p, spec.get_root(p));
    }

    /**
     * @brief 子ノードのレベルを計算し、先読み最適化を適用する
     * @param p 状態データ領域
     * @param level 現在のレベル
     * @param b 分岐の値
     * @return 最適化後の子ノードのレベル
     */
    int get_child(void* p, int level, int b) {
        return lookahead(p, spec.get_child(p, level, b));
    }

    /**
     * @brief 状態を複製する
     * @param to コピー先
     * @param from コピー元
     */
    void get_copy(void* to, void const* from) {
        spec.get_copy(to, from);
    }

    /**
     * @brief 2つの状態をマージする
     * @param p1 状態1のデータ領域
     * @param p2 状態2のデータ領域
     * @return マージ結果コード
     */
    int merge_states(void* p1, void* p2) {
        return spec.merge_states(p1, p2);
    }

    /**
     * @brief 状態を破棄する
     * @param p 状態データ領域
     */
    void destruct(void* p) {
        spec.destruct(p);
    }

    /**
     * @brief 指定レベルのリソースを解放する
     * @param level レベル番号
     */
    void destructLevel(int level) {
        spec.destructLevel(level);
    }

    /**
     * @brief 状態のハッシュ値を計算する
     * @param p 状態データ領域
     * @param level レベル番号
     * @return ハッシュ値
     */
    std::size_t hash_code(void const* p, int level) const {
        return spec.hash_code(p, level);
    }

    /**
     * @brief 2つの状態が等しいか判定する
     * @param p 状態1のデータ領域
     * @param q 状態2のデータ領域
     * @param level レベル番号
     * @return 等しければtrue
     */
    bool equal_to(void const* p, void const* q, int level) const {
        return spec.equal_to(p, q, level);
    }

    /**
     * @brief 状態を出力ストリームに書き出す
     * @param os 出力ストリーム
     * @param p 状態データ領域
     * @param level レベル番号
     */
    void print_state(std::ostream& os, void const* p, int level) const {
        spec.print_state(os, p, level);
    }
};

/**
 * @brief ZDD先読み最適化のSpecラッパー
 *
 * ZDDの仕様を先読みし、ZDDノード削除規則に基づいて冗長なノードを
 * スキップする最適化を行う。1枝がすべて0終端になるノードを検出して除去する。
 *
 * @tparam S 内部Specの型
 * @see zddLookahead() 便利関数
 * @see BddLookahead BDD用の先読み最適化
 */
template<typename S>
class ZddLookahead: public DdSpecBase<ZddLookahead<S>, S::ARITY> {
    typedef S Spec;

    Spec spec;
    std::vector<char> work;

    int lookahead(void* p, int level) {
        void* const q = work.data();
        while (level >= 1) {
            for (int b = 1; b < Spec::ARITY; ++b) {
                spec.get_copy(q, p);
                if (spec.get_child(q, level, b) != 0) {
                    spec.destruct(q);
                    return level;
                }
                spec.destruct(q);
            }
            level = spec.get_child(p, level, 0);
        }

        return level;
    }

public:
    /**
     * @brief コンストラクタ
     * @param s 最適化対象のZDD Spec
     */
    ZddLookahead(S const& s)
            : spec(s), work(spec.datasize()) {
    }

    /**
     * @brief 状態データのバイトサイズを返す
     * @return 内部Specのデータサイズ
     */
    int datasize() const {
        return spec.datasize();
    }

    /**
     * @brief ルートノードを初期化し、先読み最適化後のレベルを返す
     * @param p 状態データ領域
     * @return 最適化後のルートレベル
     */
    int get_root(void* p) {
        return lookahead(p, spec.get_root(p));
    }

    /**
     * @brief 子ノードのレベルを計算し、先読み最適化を適用する
     * @param p 状態データ領域
     * @param level 現在のレベル
     * @param b 分岐の値
     * @return 最適化後の子ノードのレベル
     */
    int get_child(void* p, int level, int b) {
        return lookahead(p, spec.get_child(p, level, b));
    }

    /**
     * @brief 状態を複製する
     * @param to コピー先
     * @param from コピー元
     */
    void get_copy(void* to, void const* from) {
        spec.get_copy(to, from);
    }

    /**
     * @brief 2つの状態をマージする
     * @param p1 状態1のデータ領域
     * @param p2 状態2のデータ領域
     * @return マージ結果コード
     */
    int merge_states(void* p1, void* p2) {
        return spec.merge_states(p1, p2);
    }

    /**
     * @brief 状態を破棄する
     * @param p 状態データ領域
     */
    void destruct(void* p) {
        spec.destruct(p);
    }

    /**
     * @brief 指定レベルのリソースを解放する
     * @param level レベル番号
     */
    void destructLevel(int level) {
        spec.destructLevel(level);
    }

    /**
     * @brief 状態のハッシュ値を計算する
     * @param p 状態データ領域
     * @param level レベル番号
     * @return ハッシュ値
     */
    std::size_t hash_code(void const* p, int level) const {
        return spec.hash_code(p, level);
    }

    /**
     * @brief 2つの状態が等しいか判定する
     * @param p 状態1のデータ領域
     * @param q 状態2のデータ領域
     * @param level レベル番号
     * @return 等しければtrue
     */
    bool equal_to(void const* p, void const* q, int level) const {
        return spec.equal_to(p, q, level);
    }

    /**
     * @brief 状態を出力ストリームに書き出す
     * @param os 出力ストリーム
     * @param p 状態データ領域
     * @param level レベル番号
     */
    void print_state(std::ostream& os, void const* p, int level) const {
        spec.print_state(os, p, level);
    }
};

/**
 * @brief BDD非簡約化のSpecラッパー
 *
 * BDD仕様からQDD（準簡約DD）仕様を生成する。
 * BDDノード削除規則でスキップされたノードを補完し、
 * すべてのレベルにノードが存在するQDDを構築する。
 *
 * @tparam S 内部Specの型
 * @see bddUnreduction() 便利関数
 * @see ZddUnreduction ZDD用の非簡約化
 */
template<typename S>
class BddUnreduction: public PodArrayDdSpec<BddUnreduction<S>, std::size_t, S::ARITY> {
protected:
    typedef S Spec;
    typedef std::size_t Word;

    static std::size_t const levelWords = (sizeof(int) + sizeof(Word) - 1)
            / sizeof(Word);

    Spec spec;
    int const stateWords;
    int numVars;

    static int wordSize(int size) {
        return (size + sizeof(Word) - 1) / sizeof(Word);
    }

    int& level(void* p) const {
        return *static_cast<int*>(p);
    }

    int level(void const* p) const {
        return *static_cast<int const*>(p);
    }

    void* state(void* p) const {
        return static_cast<Word*>(p) + levelWords;
    }

    void const* state(void const* p) const {
        return static_cast<Word const*>(p) + levelWords;
    }

public:
    /**
     * @brief コンストラクタ
     * @param s 非簡約化対象のBDD Spec
     * @param numVars_ 変数の数
     */
    BddUnreduction(S const& s, int numVars_)
            : spec(s), stateWords(wordSize(spec.datasize())), numVars(numVars_) {
        BddUnreduction::setArraySize(levelWords + stateWords);
    }

    /**
     * @brief ルートノードを初期化する
     * @param p 状態配列
     * @return ルートノードのレベル
     */
    int getRoot(Word* p) {
        level(p) = spec.get_root(state(p));
        if (level(p) == 0) return 0;
        if (level(p) >= numVars) numVars = level(p);
        return (numVars > 0) ? numVars : -1;
    }

    /**
     * @brief 子ノードのレベルを計算する
     *
     * 現在のレベルが内部Specのレベルと一致する場合のみ内部Specの
     * get_childを呼び出し、それ以外は単にレベルを減少させる（BDD補完）。
     *
     * @param p 状態配列
     * @param i 現在のレベル
     * @param value 分岐の値
     * @return 子ノードのレベル
     */
    int getChild(Word* p, int i, int value) {
        if (level(p) == i) {
            level(p) = spec.get_child(state(p), i, value);
            if (level(p) == 0) return 0;
        }

        --i;
        assert(level(p) <= i);
        return (i > 0) ? i : level(p);
    }

    /**
     * @brief 状態を複製する
     * @param to コピー先
     * @param from コピー元
     */
    void get_copy(void* to, void const* from) {
        level(to) = level(from);
        spec.get_copy(state(to), state(from));
    }

    /**
     * @brief 状態を破棄する
     * @param p 状態データ領域
     */
    void destruct(void* p) {
        spec.destruct(state(p));
    }

    /**
     * @brief 指定レベルのリソースを解放する
     * @param lvl レベル番号
     */
    void destructLevel(int lvl) {
        spec.destructLevel(lvl);
    }

    /**
     * @brief 2つの状態をマージする
     * @param p1 状態1のデータ領域
     * @param p2 状態2のデータ領域
     * @return マージ結果コード
     */
    int merge_states(void* p1, void* p2) {
        return spec.merge_states(state(p1), state(p2));
    }

    /**
     * @brief 状態のハッシュ値を計算する
     * @param p 状態データ領域
     * @param i レベル番号
     * @return ハッシュ値
     */
    std::size_t hash_code(void const* p, int i) const {
        (void)i;
        std::size_t h = std::size_t(level(p)) * 314159257;
        if (level(p) > 0) h += spec.hash_code(state(p), level(p)) * 271828171;
        return h;
    }

    /**
     * @brief 2つの状態が等しいか判定する
     * @param p 状態1のデータ領域
     * @param q 状態2のデータ領域
     * @param i レベル番号
     * @return 等しければtrue
     */
    bool equal_to(void const* p, void const* q, int i) const {
        (void)i;
        if (level(p) != level(q)) return false;
        if (level(p) > 0 && !spec.equal_to(state(p), state(q), level(p))) return false;
        return true;
    }

    /**
     * @brief 状態を出力ストリームに書き出す
     * @param os 出力ストリーム
     * @param p 状態データ領域
     * @param l レベル番号
     */
    void print_state(std::ostream& os, void const* p, int l) const {
        Word const* q = static_cast<Word const*>(p);
        os << "<" << level(q) << ",";
        spec.print_state(os, state(q), l);
        os << ">";
    }
};

/**
 * @brief ZDD非簡約化のSpecラッパー
 *
 * ZDD仕様からQDD（準簡約DD）仕様を生成する。
 * ZDDノード削除規則でスキップされたノードを補完し、
 * すべてのレベルにノードが存在するQDDを構築する。
 * スキップレベルへの1枝は0終端に接続される（ZDDルール）。
 *
 * @tparam S 内部Specの型
 * @see zddUnreduction() 便利関数
 * @see BddUnreduction BDD用の非簡約化
 */
template<typename S>
class ZddUnreduction: public PodArrayDdSpec<ZddUnreduction<S>, std::size_t, S::ARITY> {
protected:
    typedef S Spec;
    typedef std::size_t Word;

    static std::size_t const levelWords = (sizeof(int) + sizeof(Word) - 1)
            / sizeof(Word);

    Spec spec;
    int const stateWords;
    int numVars;

    static int wordSize(int size) {
        return (size + sizeof(Word) - 1) / sizeof(Word);
    }

    int& level(void* p) const {
        return *static_cast<int*>(p);
    }

    int level(void const* p) const {
        return *static_cast<int const*>(p);
    }

    void* state(void* p) const {
        return static_cast<Word*>(p) + levelWords;
    }

    void const* state(void const* p) const {
        return static_cast<Word const*>(p) + levelWords;
    }

public:
    /**
     * @brief コンストラクタ
     * @param s 非簡約化対象のZDD Spec
     * @param numVars_ 変数の数
     */
    ZddUnreduction(S const& s, int numVars_)
            : spec(s), stateWords(wordSize(spec.datasize())), numVars(numVars_) {
        ZddUnreduction::setArraySize(levelWords + stateWords);
    }

    /**
     * @brief ルートノードを初期化する
     * @param p 状態配列
     * @return ルートノードのレベル
     */
    int getRoot(Word* p) {
        level(p) = spec.get_root(state(p));
        if (level(p) == 0) return 0;
        if (level(p) >= numVars) numVars = level(p);
        return (numVars > 0) ? numVars : -1;
    }

    /**
     * @brief 子ノードのレベルを計算する
     *
     * 現在のレベルが内部Specのレベルと一致する場合のみ内部Specの
     * get_childを呼び出す。スキップレベルへの1枝は0終端を返す（ZDDルール）。
     *
     * @param p 状態配列
     * @param i 現在のレベル
     * @param value 分岐の値
     * @return 子ノードのレベル
     */
    int getChild(Word* p, int i, int value) {
        if (level(p) == i) {
            level(p) = spec.get_child(state(p), i, value);
            if (level(p) == 0) return 0;
        }
        else if (value) {
            return 0;  // ZDD: スキップレベルへの1枝は0終端
        }

        --i;
        assert(level(p) <= i);
        return (i > 0) ? i : level(p);
    }

    /**
     * @brief 状態を複製する
     * @param to コピー先
     * @param from コピー元
     */
    void get_copy(void* to, void const* from) {
        level(to) = level(from);
        spec.get_copy(state(to), state(from));
    }

    /**
     * @brief 状態を破棄する
     * @param p 状態データ領域
     */
    void destruct(void* p) {
        spec.destruct(state(p));
    }

    /**
     * @brief 指定レベルのリソースを解放する
     * @param lvl レベル番号
     */
    void destructLevel(int lvl) {
        spec.destructLevel(lvl);
    }

    /**
     * @brief 2つの状態をマージする
     * @param p1 状態1のデータ領域
     * @param p2 状態2のデータ領域
     * @return マージ結果コード
     */
    int merge_states(void* p1, void* p2) {
        return spec.merge_states(state(p1), state(p2));
    }

    /**
     * @brief 状態のハッシュ値を計算する
     * @param p 状態データ領域
     * @param i レベル番号
     * @return ハッシュ値
     */
    std::size_t hash_code(void const* p, int i) const {
        (void)i;
        std::size_t h = std::size_t(level(p)) * 314159257;
        if (level(p) > 0) h += spec.hash_code(state(p), level(p)) * 271828171;
        return h;
    }

    /**
     * @brief 2つの状態が等しいか判定する
     * @param p 状態1のデータ領域
     * @param q 状態2のデータ領域
     * @param i レベル番号
     * @return 等しければtrue
     */
    bool equal_to(void const* p, void const* q, int i) const {
        (void)i;
        if (level(p) != level(q)) return false;
        if (level(p) > 0 && !spec.equal_to(state(p), state(q), level(p))) return false;
        return true;
    }

    /**
     * @brief 状態を出力ストリームに書き出す
     * @param os 出力ストリーム
     * @param p 状態データ領域
     * @param l レベル番号
     */
    void print_state(std::ostream& os, void const* p, int l) const {
        Word const* q = static_cast<Word const*>(p);
        os << "<" << level(q) << ",";
        spec.print_state(os, state(q), l);
        os << ">";
    }
};

// ============================================================
// 便利関数
// ============================================================

/**
 * @brief 2つのBDD SpecのAND演算を返す
 * @tparam S1 第1オペランドのSpec型
 * @tparam S2 第2オペランドのSpec型
 * @param spec1 第1オペランドのSpec
 * @param spec2 第2オペランドのSpec
 * @return BDD AND演算のSpec
 * @see BddAnd
 */
template<typename S1, typename S2>
BddAnd<S1,S2> bddAnd(S1 const& spec1, S2 const& spec2) {
    return BddAnd<S1,S2>(spec1, spec2);
}

/**
 * @brief 2つのBDD SpecのOR演算を返す
 * @tparam S1 第1オペランドのSpec型
 * @tparam S2 第2オペランドのSpec型
 * @param spec1 第1オペランドのSpec
 * @param spec2 第2オペランドのSpec
 * @return BDD OR演算のSpec
 * @see BddOr
 */
template<typename S1, typename S2>
BddOr<S1,S2> bddOr(S1 const& spec1, S2 const& spec2) {
    return BddOr<S1,S2>(spec1, spec2);
}

/**
 * @brief 2つのZDD Specの積集合演算を返す
 * @tparam S1 第1オペランドのSpec型
 * @tparam S2 第2オペランドのSpec型
 * @param spec1 第1オペランドのSpec
 * @param spec2 第2オペランドのSpec
 * @return ZDD積集合演算のSpec
 * @see ZddIntersection
 */
template<typename S1, typename S2>
ZddIntersection<S1,S2> zddIntersection(S1 const& spec1, S2 const& spec2) {
    return ZddIntersection<S1,S2>(spec1, spec2);
}

/**
 * @brief 2つのZDD Specの和集合演算を返す
 * @tparam S1 第1オペランドのSpec型
 * @tparam S2 第2オペランドのSpec型
 * @param spec1 第1オペランドのSpec
 * @param spec2 第2オペランドのSpec
 * @return ZDD和集合演算のSpec
 * @see ZddUnion
 */
template<typename S1, typename S2>
ZddUnion<S1,S2> zddUnion(S1 const& spec1, S2 const& spec2) {
    return ZddUnion<S1,S2>(spec1, spec2);
}

/**
 * @brief BDD SpecにBDD先読み最適化を適用する
 * @tparam S Spec型
 * @param spec 最適化対象のSpec
 * @return 先読み最適化済みのSpec
 * @see BddLookahead
 */
template<typename S>
BddLookahead<S> bddLookahead(S const& spec) {
    return BddLookahead<S>(spec);
}

/**
 * @brief ZDD SpecにZDD先読み最適化を適用する
 * @tparam S Spec型
 * @param spec 最適化対象のSpec
 * @return 先読み最適化済みのSpec
 * @see ZddLookahead
 */
template<typename S>
ZddLookahead<S> zddLookahead(S const& spec) {
    return ZddLookahead<S>(spec);
}

/**
 * @brief BDD SpecからQDD仕様（非簡約化）を生成する
 * @tparam S Spec型
 * @param spec 非簡約化対象のSpec
 * @param numVars 変数の数
 * @return 非簡約化されたSpec（QDD仕様）
 * @see BddUnreduction
 */
template<typename S>
BddUnreduction<S> bddUnreduction(S const& spec, int numVars) {
    return BddUnreduction<S>(spec, numVars);
}

/**
 * @brief ZDD SpecからQDD仕様（非簡約化）を生成する
 * @tparam S Spec型
 * @param spec 非簡約化対象のSpec
 * @param numVars 変数の数
 * @return 非簡約化されたSpec（QDD仕様）
 * @see ZddUnreduction
 */
template<typename S>
ZddUnreduction<S> zddUnreduction(S const& spec, int numVars) {
    return ZddUnreduction<S>(spec, numVars);
}

// ============================================================
// 可変アリティSpec演算
// ============================================================

/**
 * @brief 可変アリティSpecの二項演算の基底クラス
 *
 * 実行時にアリティが決定されるSpec同士の二項演算の共通機能を提供する。
 * 両Specのアリティが一致している必要がある。
 *
 * @tparam S このクラスを実装する派生クラス（CRTP）
 * @tparam S1 第1オペランドのSpec型
 * @tparam S2 第2オペランドのSpec型
 * @see VarArityZddUnion 可変アリティZDD和集合演算
 * @see VarArityZddIntersection 可変アリティZDD積集合演算
 */
template<typename S, typename S1, typename S2>
class VarArityBinaryOperation {
protected:
    typedef S1 Spec1;
    typedef S2 Spec2;
    typedef std::size_t Word;

    /** @brief レベル情報を格納するのに必要なワード数 */
    static std::size_t const levelWords = (sizeof(int[2]) + sizeof(Word) - 1)
            / sizeof(Word);

    Spec1 spec1;   /**< @brief 第1オペランドのSpec */
    Spec2 spec2;   /**< @brief 第2オペランドのSpec */
    int const stateWords1;  /**< @brief 第1Specの状態サイズ（ワード数） */
    int const stateWords2;  /**< @brief 第2Specの状態サイズ（ワード数） */
    int arity_;             /**< @brief アリティ */
    int arraySize_;         /**< @brief 配列サイズ（ワード数） */

    static int wordSize(int size) {
        return (size + sizeof(Word) - 1) / sizeof(Word);
    }

    void setLevel1(void* p, int level) const {
        static_cast<int*>(p)[0] = level;
    }

    int level1(void const* p) const {
        return static_cast<int const*>(p)[0];
    }

    void setLevel2(void* p, int level) const {
        static_cast<int*>(p)[1] = level;
    }

    int level2(void const* p) const {
        return static_cast<int const*>(p)[1];
    }

    void* state1(void* p) const {
        return static_cast<Word*>(p) + levelWords;
    }

    void const* state1(void const* p) const {
        return static_cast<Word const*>(p) + levelWords;
    }

    void* state2(void* p) const {
        return static_cast<Word*>(p) + levelWords + stateWords1;
    }

    void const* state2(void const* p) const {
        return static_cast<Word const*>(p) + levelWords + stateWords1;
    }

    void setArraySize(int n) {
        arraySize_ = n;
    }

    int getArraySize() const {
        return arraySize_;
    }

public:
    /**
     * @brief コンストラクタ
     * @param s1 第1オペランドのSpec
     * @param s2 第2オペランドのSpec
     * @throws DDArgumentException 両Specのアリティが異なる場合
     */
    VarArityBinaryOperation(S1 const& s1, S2 const& s2)
            : spec1(s1),
              spec2(s2),
              stateWords1(wordSize(spec1.datasize())),
              stateWords2(wordSize(spec2.datasize())),
              arity_(0),
              arraySize_(levelWords + stateWords1 + stateWords2) {
        // Verify both specs have the same arity
        if (spec1.getArity() != spec2.getArity()) {
            throw DDArgumentException(
                "VarArity spec operations require both specs to have the same ARITY");
        }
        arity_ = spec1.getArity();
    }

    /**
     * @brief CRTP派生クラスの参照を取得する
     * @return 派生クラスへの参照
     */
    S& entity() {
        return *static_cast<S*>(this);
    }

    /**
     * @brief CRTP派生クラスのconst参照を取得する
     * @return 派生クラスへのconst参照
     */
    S const& entity() const {
        return *static_cast<S const*>(this);
    }

    /**
     * @brief アリティを取得する
     * @return アリティの値
     */
    int getArity() const {
        return arity_;
    }

    /**
     * @brief 状態データのバイトサイズを返す
     * @return データサイズ（バイト）
     */
    int datasize() const {
        return arraySize_ * sizeof(Word);
    }

    /**
     * @brief 状態を複製する
     * @param to コピー先
     * @param from コピー元
     */
    void get_copy(void* to, void const* from) {
        setLevel1(to, level1(from));
        setLevel2(to, level2(from));
        spec1.get_copy(state1(to), state1(from));
        spec2.get_copy(state2(to), state2(from));
    }

    /**
     * @brief 2つの状態をマージする
     * @param p1 状態1のデータ領域
     * @param p2 状態2のデータ領域
     * @return マージ結果コード
     */
    int merge_states(void* p1, void* p2) {
        return spec1.merge_states(state1(p1), state1(p2))
                | spec2.merge_states(state2(p1), state2(p2));
    }

    /**
     * @brief 状態を破棄する
     * @param p 状態データ領域
     */
    void destruct(void* p) {
        spec1.destruct(state1(p));
        spec2.destruct(state2(p));
    }

    /**
     * @brief 指定レベルのリソースを解放する
     * @param level レベル番号
     */
    void destructLevel(int level) {
        spec1.destructLevel(level);
        spec2.destructLevel(level);
    }

    /**
     * @brief 状態のハッシュ値を計算する
     * @param p 状態データ領域
     * @param level レベル番号
     * @return ハッシュ値
     */
    std::size_t hash_code(void const* p, int level) const {
        std::size_t h = std::size_t(level1(p)) * 314159257
                + std::size_t(level2(p)) * 271828171;
        if (level1(p) > 0)
            h += spec1.hash_code(state1(p), level1(p)) * 171828143;
        if (level2(p) > 0)
            h += spec2.hash_code(state2(p), level2(p)) * 141421333;
        return h;
    }

    /**
     * @brief 2つの状態が等しいか判定する
     * @param p 状態1のデータ領域
     * @param q 状態2のデータ領域
     * @param level レベル番号
     * @return 等しければtrue
     */
    bool equal_to(void const* p, void const* q, int level) const {
        (void)level;
        if (level1(p) != level1(q)) return false;
        if (level2(p) != level2(q)) return false;
        if (level1(p) > 0 && !spec1.equal_to(state1(p), state1(q), level1(p)))
            return false;
        if (level2(p) > 0 && !spec2.equal_to(state2(p), state2(q), level2(p)))
            return false;
        return true;
    }

    /**
     * @brief レベル番号を出力ストリームに書き出す
     * @param os 出力ストリーム
     * @param level レベル番号
     */
    void printLevel(std::ostream& os, int level) const {
        os << level;
    }
};

/**
 * @brief 可変アリティZDD和集合演算のSpec
 *
 * 実行時にアリティが決定される2つのZDD Specの和集合を表すSpec。
 * 両Specのアリティが一致している必要がある。
 *
 * @tparam S1 第1オペランドのSpec型
 * @tparam S2 第2オペランドのSpec型
 * @see zddUnionVA() 便利関数
 * @see VarArityZddIntersection 可変アリティZDD積集合演算
 */
template<typename S1, typename S2>
struct VarArityZddUnion
        : public VarArityBinaryOperation<VarArityZddUnion<S1,S2>, S1, S2> {
    typedef VarArityBinaryOperation<VarArityZddUnion<S1,S2>, S1, S2> base;
    typedef typename base::Word Word;

    /**
     * @brief コンストラクタ
     * @param s1 第1オペランドのSpec
     * @param s2 第2オペランドのSpec
     */
    VarArityZddUnion(S1 const& s1, S2 const& s2) : base(s1, s2) {}

    /**
     * @brief ルートノードを初期化する（内部用）
     * @param p 状態配列
     * @return ルートノードのレベル
     */
    int getRoot(Word* p) {
        int i1 = base::spec1.get_root(base::state1(p));
        int i2 = base::spec2.get_root(base::state2(p));
        if (i1 == 0 && i2 == 0) return 0;
        if (i1 <= 0 && i2 <= 0) return -1;
        base::setLevel1(p, i1);
        base::setLevel2(p, i2);
        return std::max(base::level1(p), base::level2(p));
    }

    /**
     * @brief ルートノードを初期化する（外部インターフェース）
     * @param p 状態データ領域
     * @return ルートノードのレベル
     */
    int get_root(void* p) {
        return getRoot(static_cast<Word*>(p));
    }

    /**
     * @brief 子ノードのレベルを計算する（内部用）
     * @param p 状態配列
     * @param level 現在のレベル
     * @param take 分岐の値
     * @return 子ノードのレベル
     */
    int getChild(Word* p, int level, int take) {
        assert(base::level1(p) <= level && base::level2(p) <= level);

        if (base::level1(p) == level) {
            int i1 = base::spec1.get_child(base::state1(p), level, take);
            base::setLevel1(p, i1);
        }
        else if (take) {
            base::setLevel1(p, 0);
        }

        if (base::level2(p) == level) {
            int i2 = base::spec2.get_child(base::state2(p), level, take);
            base::setLevel2(p, i2);
        }
        else if (take) {
            base::setLevel2(p, 0);
        }

        if (base::level1(p) == 0 && base::level2(p) == 0) return 0;
        if (base::level1(p) <= 0 && base::level2(p) <= 0) return -1;
        return std::max(base::level1(p), base::level2(p));
    }

    /**
     * @brief 子ノードのレベルを計算する（外部インターフェース）
     * @param p 状態データ領域
     * @param level 現在のレベル
     * @param value 分岐の値
     * @return 子ノードのレベル
     */
    int get_child(void* p, int level, int value) {
        return getChild(static_cast<Word*>(p), level, value);
    }

    /**
     * @brief 状態を出力ストリームに書き出す
     * @param os 出力ストリーム
     * @param p 状態データ領域
     * @param level レベル番号
     */
    void print_state(std::ostream& os, void const* p, int level) const {
        Word const* q = static_cast<Word const*>(p);
        os << "<" << base::level1(q) << ",";
        base::spec1.print_state(os, base::state1(q), level);
        os << "> UNION <" << base::level2(q) << ",";
        base::spec2.print_state(os, base::state2(q), level);
        os << ">";
    }
};

/**
 * @brief 可変アリティZDD積集合演算のSpec
 *
 * 実行時にアリティが決定される2つのZDD Specの積集合を表すSpec。
 * 両Specのアリティが一致している必要がある。
 * スキップレベルでは0枝を辿ることでZDDセマンティクスに従う。
 *
 * @tparam S1 第1オペランドのSpec型
 * @tparam S2 第2オペランドのSpec型
 * @see zddIntersectionVA() 便利関数
 * @see VarArityZddUnion 可変アリティZDD和集合演算
 */
template<typename S1, typename S2>
struct VarArityZddIntersection {
    typedef S1 Spec1;
    typedef S2 Spec2;
    typedef std::size_t Word;

    Spec1 spec1;  /**< @brief 第1オペランドのSpec */
    Spec2 spec2;  /**< @brief 第2オペランドのSpec */
    int const stateWords1;  /**< @brief 第1Specの状態サイズ（ワード数） */
    int const stateWords2;  /**< @brief 第2Specの状態サイズ（ワード数） */
    int arity_;             /**< @brief アリティ */
    int arraySize_;         /**< @brief 配列サイズ（ワード数） */

    static int wordSize(int size) {
        return (size + sizeof(Word) - 1) / sizeof(Word);
    }

    void* state1(void* p) const {
        return p;
    }

    void const* state1(void const* p) const {
        return p;
    }

    void* state2(void* p) const {
        return static_cast<Word*>(p) + stateWords1;
    }

    void const* state2(void const* p) const {
        return static_cast<Word const*>(p) + stateWords1;
    }

public:
    /**
     * @brief コンストラクタ
     * @param s1 第1オペランドのSpec
     * @param s2 第2オペランドのSpec
     * @throws DDArgumentException 両Specのアリティが異なる場合
     */
    VarArityZddIntersection(S1 const& s1, S2 const& s2)
            : spec1(s1),
              spec2(s2),
              stateWords1(wordSize(spec1.datasize())),
              stateWords2(wordSize(spec2.datasize())),
              arity_(0),
              arraySize_(stateWords1 + stateWords2) {
        if (spec1.getArity() != spec2.getArity()) {
            throw DDArgumentException(
                "VarArity spec operations require both specs to have the same ARITY");
        }
        arity_ = spec1.getArity();
    }

    /**
     * @brief アリティを取得する
     * @return アリティの値
     */
    int getArity() const {
        return arity_;
    }

    /**
     * @brief 状態データのバイトサイズを返す
     * @return データサイズ（バイト）
     */
    int datasize() const {
        return arraySize_ * sizeof(Word);
    }

    /**
     * @brief ルートノードを初期化する（内部用）
     * @param p 状態配列
     * @return ルートノードのレベル
     */
    int getRoot(Word* p) {
        int i1 = spec1.get_root(state1(p));
        if (i1 == 0) return 0;
        int i2 = spec2.get_root(state2(p));
        if (i2 == 0) return 0;

        while (i1 != i2) {
            if (i1 > i2) {
                i1 = spec1.get_child(state1(p), i1, 0);
                if (i1 == 0) return 0;
            }
            else {
                i2 = spec2.get_child(state2(p), i2, 0);
                if (i2 == 0) return 0;
            }
        }

        return i1;
    }

    /**
     * @brief ルートノードを初期化する（外部インターフェース）
     * @param p 状態データ領域
     * @return ルートノードのレベル
     */
    int get_root(void* p) {
        return getRoot(static_cast<Word*>(p));
    }

    /**
     * @brief 子ノードのレベルを計算する（内部用）
     * @param p 状態配列
     * @param level 現在のレベル
     * @param take 分岐の値
     * @return 子ノードのレベル
     */
    int getChild(Word* p, int level, int take) {
        int i1 = spec1.get_child(state1(p), level, take);
        if (i1 == 0) return 0;
        int i2 = spec2.get_child(state2(p), level, take);
        if (i2 == 0) return 0;

        while (i1 != i2) {
            if (i1 > i2) {
                i1 = spec1.get_child(state1(p), i1, 0);
                if (i1 == 0) return 0;
            }
            else {
                i2 = spec2.get_child(state2(p), i2, 0);
                if (i2 == 0) return 0;
            }
        }

        return i1;
    }

    /**
     * @brief 子ノードのレベルを計算する（外部インターフェース）
     * @param p 状態データ領域
     * @param level 現在のレベル
     * @param value 分岐の値
     * @return 子ノードのレベル
     */
    int get_child(void* p, int level, int value) {
        return getChild(static_cast<Word*>(p), level, value);
    }

    /**
     * @brief 状態を複製する
     * @param to コピー先
     * @param from コピー元
     */
    void get_copy(void* to, void const* from) {
        spec1.get_copy(state1(to), state1(from));
        spec2.get_copy(state2(to), state2(from));
    }

    /**
     * @brief 2つの状態をマージする
     * @param p1 状態1のデータ領域
     * @param p2 状態2のデータ領域
     * @return マージ結果コード
     */
    int merge_states(void* p1, void* p2) {
        return spec1.merge_states(state1(p1), state1(p2))
                | spec2.merge_states(state2(p1), state2(p2));
    }

    /**
     * @brief 状態を破棄する
     * @param p 状態データ領域
     */
    void destruct(void* p) {
        spec1.destruct(state1(p));
        spec2.destruct(state2(p));
    }

    /**
     * @brief 指定レベルのリソースを解放する
     * @param level レベル番号
     */
    void destructLevel(int level) {
        spec1.destructLevel(level);
        spec2.destructLevel(level);
    }

    /**
     * @brief 状態のハッシュ値を計算する
     * @param p 状態データ領域
     * @param level レベル番号
     * @return ハッシュ値
     */
    std::size_t hash_code(void const* p, int level) const {
        return spec1.hash_code(state1(p), level) * 314159257
                + spec2.hash_code(state2(p), level) * 271828171;
    }

    /**
     * @brief 2つの状態が等しいか判定する
     * @param p 状態1のデータ領域
     * @param q 状態2のデータ領域
     * @param level レベル番号
     * @return 等しければtrue
     */
    bool equal_to(void const* p, void const* q, int level) const {
        return spec1.equal_to(state1(p), state1(q), level)
                && spec2.equal_to(state2(p), state2(q), level);
    }

    /**
     * @brief 状態を出力ストリームに書き出す
     * @param os 出力ストリーム
     * @param p 状態データ領域
     * @param level レベル番号
     */
    void print_state(std::ostream& os, void const* p, int level) const {
        Word const* q = static_cast<Word const*>(p);
        os << "<";
        spec1.print_state(os, state1(q), level);
        os << "> INTERSECT <";
        spec2.print_state(os, state2(q), level);
        os << ">";
    }

    /**
     * @brief レベル番号を出力ストリームに書き出す
     * @param os 出力ストリーム
     * @param level レベル番号
     */
    void printLevel(std::ostream& os, int level) const {
        os << level;
    }
};

/**
 * @brief 2つの可変アリティZDD Specの和集合演算を返す
 *
 * 両Specのアリティが一致している必要がある。
 *
 * @tparam S1 第1オペランドのSpec型
 * @tparam S2 第2オペランドのSpec型
 * @param spec1 第1オペランドのSpec
 * @param spec2 第2オペランドのSpec
 * @return 可変アリティZDD和集合演算のSpec
 * @throws DDArgumentException 両Specのアリティが異なる場合
 * @see VarArityZddUnion
 */
template<typename S1, typename S2>
VarArityZddUnion<S1,S2> zddUnionVA(S1 const& spec1, S2 const& spec2) {
    return VarArityZddUnion<S1,S2>(spec1, spec2);
}

/**
 * @brief 2つの可変アリティZDD Specの積集合演算を返す
 *
 * 両Specのアリティが一致している必要がある。
 *
 * @tparam S1 第1オペランドのSpec型
 * @tparam S2 第2オペランドのSpec型
 * @param spec1 第1オペランドのSpec
 * @param spec2 第2オペランドのSpec
 * @return 可変アリティZDD積集合演算のSpec
 * @throws DDArgumentException 両Specのアリティが異なる場合
 * @see VarArityZddIntersection
 */
template<typename S1, typename S2>
VarArityZddIntersection<S1,S2> zddIntersectionVA(S1 const& spec1, S2 const& spec2) {
    return VarArityZddIntersection<S1,S2>(spec1, spec2);
}

} // namespace tdzdd
} // namespace sbdd2
