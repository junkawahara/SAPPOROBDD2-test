/**
 * @file DdSpec.hpp
 * @brief TdZdd互換のDD仕様（Spec）インターフェースの基底クラス群
 *
 * TdZddのSpec設計に基づいたDD仕様の抽象基底クラスを提供する。
 * 各Specクラスは、DDの構造をトップダウンに定義するためのインターフェースを持つ。
 *
 * 元のTdZddライブラリ（岩下洋哉氏）に基づく。
 * Copyright (c) 2014 ERATO MINATO Project
 * SAPPOROBDD2名前空間に移植。
 *
 * @see DdSpecOp.hpp Spec演算クラス
 * @see Sbdd2Builder.hpp BFS方式のビルダー
 * @see Sbdd2BuilderDFS.hpp DFS方式のビルダー
 */

#pragma once

#include <cassert>
#include <cstddef>
#include <iostream>
#include <new>
#include <stdexcept>
#include <vector>

namespace sbdd2 {
namespace tdzdd {

/**
 * @brief DD仕様の基底クラス（CRTP）
 *
 * すべてのDD Specクラスの共通基底。CRTPパターンにより派生クラスの
 * メソッドを静的に呼び出す。
 *
 * 派生クラスは以下のメソッドを実装する必要がある:
 * - int datasize() const : 状態データのバイトサイズ
 * - int get_root(void* p) : ルートノードの初期化
 * - int get_child(void* p, int level, int value) : 子ノードの計算
 * - void get_copy(void* to, void const* from) : 状態のコピー
 * - int merge_states(void* p1, void* p2) : 状態のマージ
 * - void destruct(void* p) : 状態の破棄
 * - void destructLevel(int level) : レベルごとのリソース解放
 * - size_t hash_code(void const* p, int level) const : 状態のハッシュ値計算
 * - bool equal_to(void const* p, void const* q, int level) const : 状態の等価比較
 * - void print_state(std::ostream& os, void const* p, int level) const : 状態の出力
 *
 * get_root / get_child の戻り値:
 * - 0: 0終端ノード
 * - -1: 1終端ノード
 * - 正の値: 非終端ノードのレベル
 *
 * merge_states の戻り値:
 * - 0: マージ成功（第1の状態に統合）
 * - 1: マージ不可（第1の状態を0終端へ転送）
 * - 2: マージ不可（第2の状態を0終端へ転送）
 *
 * @tparam S このクラスを実装する派生クラス（CRTP）
 * @tparam AR ノードの分岐数（アリティ）
 * @see StatelessDdSpec 状態なしSpec
 * @see DdSpec スカラー状態Spec
 * @see PodArrayDdSpec POD配列状態Spec
 * @see HybridDdSpec ハイブリッド状態Spec
 */
template<typename S, int AR>
class DdSpecBase {
public:
    /** @brief ノードの分岐数（アリティ） */
    static int const ARITY = AR;

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
     * @brief レベル番号を出力ストリームに書き出す
     * @param os 出力ストリーム
     * @param level レベル番号
     */
    void printLevel(std::ostream& os, int level) const {
        os << level;
    }

private:
    template<typename T, typename I>
    static std::size_t rawHashCode_(void const* p) {
        std::size_t h = 0;
        I const* a = static_cast<I const*>(p);
        for (std::size_t i = 0; i < sizeof(T) / sizeof(I); ++i) {
            h += a[i];
            h *= 314159257;
        }
        return h;
    }

    template<typename T, typename I>
    static std::size_t rawEqualTo_(void const* p1, void const* p2) {
        I const* a1 = static_cast<I const*>(p1);
        I const* a2 = static_cast<I const*>(p2);
        for (std::size_t i = 0; i < sizeof(T) / sizeof(I); ++i) {
            if (a1[i] != a2[i]) return false;
        }
        return true;
    }

protected:
    /**
     * @brief PODデータのバイト列に基づくハッシュ値を計算する
     * @tparam T ハッシュ対象の型
     * @param o ハッシュ対象のオブジェクト
     * @return ハッシュ値
     */
    template<typename T>
    static std::size_t rawHashCode(T const& o) {
        if (sizeof(T) % sizeof(std::size_t) == 0) {
            return rawHashCode_<T,std::size_t>(&o);
        }
        if (sizeof(T) % sizeof(unsigned int) == 0) {
            return rawHashCode_<T,unsigned int>(&o);
        }
        if (sizeof(T) % sizeof(unsigned short) == 0) {
            return rawHashCode_<T,unsigned short>(&o);
        }
        return rawHashCode_<T,unsigned char>(&o);
    }

    /**
     * @brief PODデータのバイト列に基づく等価比較を行う
     * @tparam T 比較対象の型
     * @param o1 比較対象1
     * @param o2 比較対象2
     * @return 等しければtrue
     */
    template<typename T>
    static std::size_t rawEqualTo(T const& o1, T const& o2) {
        if (sizeof(T) % sizeof(std::size_t) == 0) {
            return rawEqualTo_<T,std::size_t>(&o1, &o2);
        }
        if (sizeof(T) % sizeof(unsigned int) == 0) {
            return rawEqualTo_<T,unsigned int>(&o1, &o2);
        }
        if (sizeof(T) % sizeof(unsigned short) == 0) {
            return rawEqualTo_<T,unsigned short>(&o1, &o2);
        }
        return rawEqualTo_<T,unsigned char>(&o1, &o2);
    }
};

/**
 * @brief 状態を持たないDD仕様の抽象クラス
 *
 * ノードに状態を保持しないDD仕様を定義する。
 * 状態管理が不要なため、最も軽量なSpecクラスである。
 *
 * 派生クラスは以下のメソッドを実装する必要がある:
 * - int getRoot() : ルートノードのレベルを返す
 * - int getChild(int level, int value) : 子ノードのレベルを返す
 *
 * @tparam S このクラスを実装する派生クラス（CRTP）
 * @tparam AR ノードの分岐数（デフォルト: 2）
 * @see DdSpec 状態ありのSpec
 * @see DdSpecBase 基底クラス
 */
template<typename S, int AR = 2>
class StatelessDdSpec: public DdSpecBase<S,AR> {
public:
    /**
     * @brief 状態データのバイトサイズを返す
     * @return 常に0（状態なし）
     */
    int datasize() const {
        return 0;
    }

    /**
     * @brief ルートノードを初期化し、ルートレベルを返す
     * @param p 状態データ領域（未使用）
     * @return ルートノードのレベル（0: 0終端, -1: 1終端, 正: 非終端レベル）
     */
    int get_root(void* p) {
        (void)p;
        return this->entity().getRoot();
    }

    /**
     * @brief 子ノードのレベルを計算する
     * @param p 状態データ領域（未使用）
     * @param level 現在のノードのレベル
     * @param value 分岐の値（0 ~ ARITY-1）
     * @return 子ノードのレベル（0: 0終端, -1: 1終端, 正: 非終端レベル）
     */
    int get_child(void* p, int level, int value) {
        (void)p;
        assert(0 <= value && value < S::ARITY);
        return this->entity().getChild(level, value);
    }

    /**
     * @brief 状態をコピーする（状態なしのため何もしない）
     * @param to コピー先（未使用）
     * @param from コピー元（未使用）
     */
    void get_copy(void* to, void const* from) {
        (void)to;
        (void)from;
    }

    /**
     * @brief 2つの状態をマージする（状態なしのため常に成功）
     * @param p1 状態1（未使用）
     * @param p2 状態2（未使用）
     * @return 常に0（マージ成功）
     */
    int merge_states(void* p1, void* p2) {
        (void)p1;
        (void)p2;
        return 0;
    }

    /**
     * @brief 状態を破棄する（状態なしのため何もしない）
     * @param p 状態データ領域（未使用）
     */
    void destruct(void* p) {
        (void)p;
    }

    /**
     * @brief 指定レベルのリソースを解放する（状態なしのため何もしない）
     * @param level レベル番号（未使用）
     */
    void destructLevel(int level) {
        (void)level;
    }

    /**
     * @brief 状態のハッシュ値を計算する（状態なしのため常に0）
     * @param p 状態データ領域（未使用）
     * @param level レベル番号（未使用）
     * @return 常に0
     */
    std::size_t hash_code(void const* p, int level) const {
        (void)p;
        (void)level;
        return 0;
    }

    /**
     * @brief 2つの状態が等しいか判定する（状態なしのため常にtrue）
     * @param p 状態1（未使用）
     * @param q 状態2（未使用）
     * @param level レベル番号（未使用）
     * @return 常にtrue
     */
    bool equal_to(void const* p, void const* q, int level) const {
        (void)p;
        (void)q;
        (void)level;
        return true;
    }

    /**
     * @brief 状態を出力ストリームに書き出す（状態なしのため"*"を出力）
     * @param os 出力ストリーム
     * @param p 状態データ領域（未使用）
     * @param level レベル番号（未使用）
     */
    void print_state(std::ostream& os, void const* p, int level) const {
        (void)p;
        (void)level;
        os << "*";
    }
};

/**
 * @brief スカラー状態を持つDD仕様の抽象クラス
 *
 * 各ノードにスカラー型Tの状態を保持するDD仕様を定義する。
 * 状態はノードの展開時に自動的に管理される。
 *
 * 派生クラスは以下のメソッドを実装する必要がある:
 * - int getRoot(T& state) : ルートノードの状態を初期化し、レベルを返す
 * - int getChild(T& state, int level, int value) : 子ノードの状態を更新し、レベルを返す
 *
 * オプションでオーバーライド可能なメソッド:
 * - void getCopy(void* p, State const& s) : 状態のコピー（デフォルト: コピーコンストラクタ使用）
 * - int mergeStates(State& s1, State& s2) : 状態のマージ（デフォルト: 常に成功）
 * - std::size_t hashCode(State const& s) const : 状態のハッシュ計算
 * - bool equalTo(State const& s1, State const& s2) const : 状態の等価比較
 *
 * @tparam S このクラスを実装する派生クラス（CRTP）
 * @tparam T 状態のデータ型
 * @tparam AR ノードの分岐数（デフォルト: 2）
 * @see StatelessDdSpec 状態なしSpec
 * @see PodArrayDdSpec POD配列状態Spec
 * @see HybridDdSpec ハイブリッド状態Spec
 */
template<typename S, typename T, int AR = 2>
class DdSpec: public DdSpecBase<S,AR> {
public:
    /** @brief 状態の型 */
    typedef T State;

private:
    static State& state(void* p) {
        return *static_cast<State*>(p);
    }

    static State const& state(void const* p) {
        return *static_cast<State const*>(p);
    }

public:
    /**
     * @brief 状態データのバイトサイズを返す
     * @return sizeof(State)
     */
    int datasize() const {
        return sizeof(State);
    }

    /**
     * @brief 状態をデフォルトコンストラクタで初期化する
     * @param p 状態データ領域（placement newで初期化される）
     */
    void construct(void* p) {
        new (p) State();
    }

    /**
     * @brief ルートノードを初期化し、ルートレベルを返す
     * @param p 状態データ領域（初期化後にgetRootに渡される）
     * @return ルートノードのレベル（0: 0終端, -1: 1終端, 正: 非終端レベル）
     */
    int get_root(void* p) {
        this->entity().construct(p);
        return this->entity().getRoot(state(p));
    }

    /**
     * @brief 子ノードのレベルを計算し、状態を更新する
     * @param p 状態データ領域（更新される）
     * @param level 現在のノードのレベル
     * @param value 分岐の値（0 ~ ARITY-1）
     * @return 子ノードのレベル（0: 0終端, -1: 1終端, 正: 非終端レベル）
     */
    int get_child(void* p, int level, int value) {
        assert(0 <= value && value < S::ARITY);
        return this->entity().getChild(state(p), level, value);
    }

    /**
     * @brief 状態をコピーコンストラクタで複製する
     * @param p コピー先の領域（placement newで初期化される）
     * @param s コピー元の状態
     */
    void getCopy(void* p, State const& s) {
        new (p) State(s);
    }

    /**
     * @brief 状態を複製する（内部インターフェース）
     * @param to コピー先の領域
     * @param from コピー元の領域
     */
    void get_copy(void* to, void const* from) {
        this->entity().getCopy(to, state(from));
    }

    /**
     * @brief 2つの状態をマージする（デフォルト実装: 常に成功）
     * @param s1 状態1（マージ先）
     * @param s2 状態2（マージ元）
     * @return 0: マージ成功, 1: 失敗（s1を0終端へ）, 2: 失敗（s2を0終端へ）
     */
    int mergeStates(State& s1, State& s2) {
        (void)s1;
        (void)s2;
        return 0;
    }

    /**
     * @brief 2つの状態をマージする（内部インターフェース）
     * @param p1 状態1のデータ領域
     * @param p2 状態2のデータ領域
     * @return マージ結果コード
     */
    int merge_states(void* p1, void* p2) {
        return this->entity().mergeStates(state(p1), state(p2));
    }

    /**
     * @brief 状態を破棄する（デストラクタを呼び出す）
     * @param p 状態データ領域
     */
    void destruct(void* p) {
        state(p).~State();
    }

    /**
     * @brief 指定レベルのリソースを解放する（デフォルト: 何もしない）
     * @param level レベル番号
     */
    void destructLevel(int level) {
        (void)level;
    }

    /**
     * @brief 状態のハッシュ値を計算する（デフォルト: バイト列ベース）
     * @param s 状態
     * @return ハッシュ値
     */
    std::size_t hashCode(State const& s) const {
        return this->rawHashCode(s);
    }

    /**
     * @brief 指定レベルでの状態のハッシュ値を計算する
     * @param s 状態
     * @param level レベル番号（デフォルト実装では未使用）
     * @return ハッシュ値
     */
    std::size_t hashCodeAtLevel(State const& s, int level) const {
        (void)level;
        return this->entity().hashCode(s);
    }

    /**
     * @brief 状態のハッシュ値を計算する（内部インターフェース）
     * @param p 状態データ領域
     * @param level レベル番号
     * @return ハッシュ値
     */
    std::size_t hash_code(void const* p, int level) const {
        return this->entity().hashCodeAtLevel(state(p), level);
    }

    /**
     * @brief 2つの状態が等しいか判定する（デフォルト: operator==使用）
     * @param s1 状態1
     * @param s2 状態2
     * @return 等しければtrue
     */
    bool equalTo(State const& s1, State const& s2) const {
        return s1 == s2;
    }

    /**
     * @brief 指定レベルでの2つの状態の等価比較を行う
     * @param s1 状態1
     * @param s2 状態2
     * @param level レベル番号（デフォルト実装では未使用）
     * @return 等しければtrue
     */
    bool equalToAtLevel(State const& s1, State const& s2, int level) const {
        (void)level;
        return this->entity().equalTo(s1, s2);
    }

    /**
     * @brief 2つの状態が等しいか判定する（内部インターフェース）
     * @param p 状態1のデータ領域
     * @param q 状態2のデータ領域
     * @param level レベル番号
     * @return 等しければtrue
     */
    bool equal_to(void const* p, void const* q, int level) const {
        return this->entity().equalToAtLevel(state(p), state(q), level);
    }

    /**
     * @brief 状態を出力ストリームに書き出す（デフォルト: operator<<使用）
     * @param os 出力ストリーム
     * @param s 状態
     */
    void printState(std::ostream& os, State const& s) const {
        os << s;
    }

    /**
     * @brief 指定レベルでの状態を出力ストリームに書き出す
     * @param os 出力ストリーム
     * @param s 状態
     * @param level レベル番号（デフォルト実装では未使用）
     */
    void printStateAtLevel(std::ostream& os, State const& s, int level) const {
        (void)level;
        this->entity().printState(os, s);
    }

    /**
     * @brief 状態を出力ストリームに書き出す（内部インターフェース）
     * @param os 出力ストリーム
     * @param p 状態データ領域
     * @param level レベル番号
     */
    void print_state(std::ostream& os, void const* p, int level) const {
        this->entity().printStateAtLevel(os, state(p), level);
    }
};

/**
 * @brief POD配列状態を持つDD仕様の抽象クラス
 *
 * 各ノードにPOD型Tの配列を状態として保持するDD仕様を定義する。
 * 配列サイズはコンストラクタ内で setArraySize() を呼び出して設定する。
 * メモリ管理はワード単位のコピーで行われるため、POD型のみ使用可能。
 *
 * 派生クラスは以下のメソッドを実装する必要がある:
 * - int getRoot(T* state) : ルートノードの状態配列を初期化し、レベルを返す
 * - int getChild(T* state, int level, int value) : 子ノードの状態を更新し、レベルを返す
 *
 * @tparam S このクラスを実装する派生クラス（CRTP）
 * @tparam T 配列要素のデータ型（POD型であること）
 * @tparam AR ノードの分岐数（デフォルト: 2）
 * @see DdSpec スカラー状態Spec
 * @see HybridDdSpec ハイブリッド状態Spec
 */
template<typename S, typename T, int AR = 2>
class PodArrayDdSpec: public DdSpecBase<S,AR> {
public:
    /** @brief 配列要素の型 */
    typedef T State;

private:
    typedef std::size_t Word;

    int arraySize;
    int dataWords;

    static State* state(void* p) {
        return static_cast<State*>(p);
    }

    static State const* state(void const* p) {
        return static_cast<State const*>(p);
    }

protected:
    /**
     * @brief 配列サイズを設定する（コンストラクタ内で1回だけ呼ぶこと）
     * @param n 配列の要素数（0以上）
     * @throws std::runtime_error 2回以上呼び出した場合
     */
    void setArraySize(int n) {
        assert(0 <= n);
        if (arraySize >= 0)
            throw std::runtime_error(
                    "Cannot set array size twice; use setArraySize(int) only once in the constructor of DD spec.");
        arraySize = n;
        dataWords = (n * sizeof(State) + sizeof(Word) - 1) / sizeof(Word);
    }

    /**
     * @brief 配列サイズを取得する
     * @return 配列の要素数
     */
    int getArraySize() const {
        return arraySize;
    }

public:
    /**
     * @brief コンストラクタ（配列サイズ未設定状態で初期化）
     */
    PodArrayDdSpec() :
            arraySize(-1), dataWords(-1) {
    }

    /**
     * @brief 状態データのバイトサイズを返す
     * @return ワード単位に切り上げたデータサイズ（バイト）
     * @throws std::runtime_error 配列サイズが未設定の場合
     */
    int datasize() const {
        if (dataWords < 0)
            throw std::runtime_error(
                    "Array size is unknown; please set it by setArraySize(int) in the constructor of DD spec.");
        return dataWords * sizeof(Word);
    }

    /**
     * @brief ルートノードを初期化し、ルートレベルを返す
     * @param p 状態データ領域
     * @return ルートノードのレベル
     */
    int get_root(void* p) {
        return this->entity().getRoot(state(p));
    }

    /**
     * @brief 子ノードのレベルを計算し、状態を更新する
     * @param p 状態データ領域
     * @param level 現在のノードのレベル
     * @param value 分岐の値（0 ~ ARITY-1）
     * @return 子ノードのレベル
     */
    int get_child(void* p, int level, int value) {
        assert(0 <= value && value < S::ARITY);
        return this->entity().getChild(state(p), level, value);
    }

    /**
     * @brief 状態をワード単位でコピーする
     * @param to コピー先の領域
     * @param from コピー元の領域
     */
    void get_copy(void* to, void const* from) {
        Word const* pa = static_cast<Word const*>(from);
        Word const* pz = pa + dataWords;
        Word* qa = static_cast<Word*>(to);
        while (pa != pz) {
            *qa++ = *pa++;
        }
    }

    /**
     * @brief 2つの状態をマージする（デフォルト: 常に成功）
     * @param a1 状態配列1
     * @param a2 状態配列2
     * @return 0: マージ成功
     */
    int mergeStates(T* a1, T* a2) {
        (void)a1;
        (void)a2;
        return 0;
    }

    /**
     * @brief 2つの状態をマージする（内部インターフェース）
     * @param p1 状態1のデータ領域
     * @param p2 状態2のデータ領域
     * @return マージ結果コード
     */
    int merge_states(void* p1, void* p2) {
        return this->entity().mergeStates(state(p1), state(p2));
    }

    /**
     * @brief 状態を破棄する（POD型のため何もしない）
     * @param p 状態データ領域
     */
    void destruct(void* p) {
        (void)p;
    }

    /**
     * @brief 指定レベルのリソースを解放する（デフォルト: 何もしない）
     * @param level レベル番号
     */
    void destructLevel(int level) {
        (void)level;
    }

    /**
     * @brief 状態配列のハッシュ値を計算する
     * @param s 状態配列へのポインタ
     * @return ハッシュ値
     */
    std::size_t hashCode(State const* s) const {
        Word const* pa = reinterpret_cast<Word const*>(s);
        Word const* pz = pa + dataWords;
        std::size_t h = 0;
        while (pa != pz) {
            h += *pa++;
            h *= 314159257;
        }
        return h;
    }

    /**
     * @brief 指定レベルでの状態配列のハッシュ値を計算する
     * @param s 状態配列へのポインタ
     * @param level レベル番号（デフォルト実装では未使用）
     * @return ハッシュ値
     */
    std::size_t hashCodeAtLevel(State const* s, int level) const {
        (void)level;
        return this->entity().hashCode(s);
    }

    /**
     * @brief 状態のハッシュ値を計算する（内部インターフェース）
     * @param p 状態データ領域
     * @param level レベル番号
     * @return ハッシュ値
     */
    std::size_t hash_code(void const* p, int level) const {
        return this->entity().hashCodeAtLevel(state(p), level);
    }

    /**
     * @brief 2つの状態配列が等しいか判定する
     * @param s1 状態配列1へのポインタ
     * @param s2 状態配列2へのポインタ
     * @return 等しければtrue
     */
    bool equalTo(State const* s1, State const* s2) const {
        Word const* pa = reinterpret_cast<Word const*>(s1);
        Word const* qa = reinterpret_cast<Word const*>(s2);
        Word const* pz = pa + dataWords;
        while (pa != pz) {
            if (*pa++ != *qa++) return false;
        }
        return true;
    }

    /**
     * @brief 指定レベルでの2つの状態配列の等価比較を行う
     * @param s1 状態配列1へのポインタ
     * @param s2 状態配列2へのポインタ
     * @param level レベル番号（デフォルト実装では未使用）
     * @return 等しければtrue
     */
    bool equalToAtLevel(State const* s1, State const* s2, int level) const {
        (void)level;
        return this->entity().equalTo(s1, s2);
    }

    /**
     * @brief 2つの状態が等しいか判定する（内部インターフェース）
     * @param p 状態1のデータ領域
     * @param q 状態2のデータ領域
     * @param level レベル番号
     * @return 等しければtrue
     */
    bool equal_to(void const* p, void const* q, int level) const {
        return this->entity().equalToAtLevel(state(p), state(q), level);
    }

    /**
     * @brief 状態配列を出力ストリームに書き出す
     * @param os 出力ストリーム
     * @param a 状態配列へのポインタ
     */
    void printState(std::ostream& os, State const* a) const {
        os << "[";
        for (int i = 0; i < arraySize; ++i) {
            if (i > 0) os << ",";
            os << a[i];
        }
        os << "]";
    }

    /**
     * @brief 指定レベルでの状態配列を出力ストリームに書き出す
     * @param os 出力ストリーム
     * @param a 状態配列へのポインタ
     * @param level レベル番号（デフォルト実装では未使用）
     */
    void printStateAtLevel(std::ostream& os, State const* a, int level) const {
        (void)level;
        this->entity().printState(os, a);
    }

    /**
     * @brief 状態を出力ストリームに書き出す（内部インターフェース）
     * @param os 出力ストリーム
     * @param p 状態データ領域
     * @param level レベル番号
     */
    void print_state(std::ostream& os, void const* p, int level) const {
        this->entity().printStateAtLevel(os, state(p), level);
    }
};

/**
 * @brief スカラー状態とPOD配列状態の両方を持つDD仕様の抽象クラス
 *
 * 各ノードにスカラー型TSとPOD配列型TAの両方の状態を保持するDD仕様を定義する。
 * スカラー部分はオブジェクトとして管理され、配列部分はワード単位のコピーで管理される。
 *
 * 派生クラスは以下のメソッドを実装する必要がある:
 * - int getRoot(S_State& s, A_State* a) : ルートノードの状態を初期化し、レベルを返す
 * - int getChild(S_State& s, A_State* a, int level, int value) : 子ノードの状態を更新し、レベルを返す
 *
 * @tparam S このクラスを実装する派生クラス（CRTP）
 * @tparam TS スカラー状態のデータ型
 * @tparam TA 配列要素のデータ型（POD型であること）
 * @tparam AR ノードの分岐数（デフォルト: 2）
 * @see DdSpec スカラー状態のみのSpec
 * @see PodArrayDdSpec POD配列状態のみのSpec
 */
template<typename S, typename TS, typename TA, int AR = 2>
class HybridDdSpec: public DdSpecBase<S,AR> {
public:
    /** @brief スカラー状態の型 */
    typedef TS S_State;
    /** @brief 配列要素の型 */
    typedef TA A_State;

private:
    typedef std::size_t Word;
    static int const S_WORDS = (sizeof(S_State) + sizeof(Word) - 1)
            / sizeof(Word);

    int arraySize;
    int dataWords;

    static S_State& s_state(void* p) {
        return *static_cast<S_State*>(p);
    }

    static S_State const& s_state(void const* p) {
        return *static_cast<S_State const*>(p);
    }

    static A_State* a_state(void* p) {
        return reinterpret_cast<A_State*>(static_cast<Word*>(p) + S_WORDS);
    }

    static A_State const* a_state(void const* p) {
        return reinterpret_cast<A_State const*>(static_cast<Word const*>(p)
                + S_WORDS);
    }

protected:
    /**
     * @brief 配列サイズを設定する（コンストラクタ内で呼ぶこと）
     * @param n 配列の要素数（0以上）
     */
    void setArraySize(int n) {
        assert(0 <= n);
        arraySize = n;
        dataWords = S_WORDS
                + (n * sizeof(A_State) + sizeof(Word) - 1) / sizeof(Word);
    }

    /**
     * @brief 配列サイズを取得する
     * @return 配列の要素数
     */
    int getArraySize() const {
        return arraySize;
    }

public:
    /**
     * @brief コンストラクタ（配列サイズ未設定状態で初期化）
     */
    HybridDdSpec() :
            arraySize(-1), dataWords(-1) {
    }

    /**
     * @brief 状態データの合計バイトサイズを返す
     * @return スカラー部+配列部のバイトサイズ
     */
    int datasize() const {
        return dataWords * sizeof(Word);
    }

    /**
     * @brief スカラー状態をデフォルトコンストラクタで初期化する
     * @param p 状態データ領域
     */
    void construct(void* p) {
        new (p) S_State();
    }

    /**
     * @brief ルートノードを初期化し、ルートレベルを返す
     * @param p 状態データ領域（スカラー部+配列部が初期化される）
     * @return ルートノードのレベル
     */
    int get_root(void* p) {
        this->entity().construct(p);
        return this->entity().getRoot(s_state(p), a_state(p));
    }

    /**
     * @brief 子ノードのレベルを計算し、状態を更新する
     * @param p 状態データ領域
     * @param level 現在のノードのレベル
     * @param value 分岐の値（0 ~ ARITY-1）
     * @return 子ノードのレベル
     */
    int get_child(void* p, int level, int value) {
        assert(0 <= value && value < S::ARITY);
        return this->entity().getChild(s_state(p), a_state(p), level, value);
    }

    /**
     * @brief スカラー状態をコピーコンストラクタで複製する
     * @param p コピー先の領域
     * @param s コピー元のスカラー状態
     */
    void getCopy(void* p, S_State const& s) {
        new (p) S_State(s);
    }

    /**
     * @brief 状態を複製する（スカラー部はコピーコンストラクタ、配列部はワードコピー）
     * @param to コピー先の領域
     * @param from コピー元の領域
     */
    void get_copy(void* to, void const* from) {
        this->entity().getCopy(to, s_state(from));
        Word const* pa = static_cast<Word const*>(from);
        Word const* pz = pa + dataWords;
        Word* qa = static_cast<Word*>(to);
        pa += S_WORDS;
        qa += S_WORDS;
        while (pa != pz) {
            *qa++ = *pa++;
        }
    }

    /**
     * @brief 2つの状態をマージする（デフォルト: 常に成功）
     * @param s1 スカラー状態1
     * @param a1 配列状態1
     * @param s2 スカラー状態2
     * @param a2 配列状態2
     * @return 0: マージ成功
     */
    int mergeStates(S_State& s1, A_State* a1, S_State& s2, A_State* a2) {
        (void)s1;
        (void)a1;
        (void)s2;
        (void)a2;
        return 0;
    }

    /**
     * @brief 2つの状態をマージする（内部インターフェース）
     * @param p1 状態1のデータ領域
     * @param p2 状態2のデータ領域
     * @return マージ結果コード
     */
    int merge_states(void* p1, void* p2) {
        return this->entity().mergeStates(s_state(p1), a_state(p1), s_state(p2),
                                          a_state(p2));
    }

    /**
     * @brief 状態を破棄する（POD型のため何もしない）
     * @param p 状態データ領域
     */
    void destruct(void* p) {
        (void)p;
    }

    /**
     * @brief 指定レベルのリソースを解放する（デフォルト: 何もしない）
     * @param level レベル番号
     */
    void destructLevel(int level) {
        (void)level;
    }

    /**
     * @brief スカラー状態のハッシュ値を計算する
     * @param s スカラー状態
     * @return ハッシュ値
     */
    std::size_t hashCode(S_State const& s) const {
        return this->rawHashCode(s);
    }

    /**
     * @brief 指定レベルでのスカラー状態のハッシュ値を計算する
     * @param s スカラー状態
     * @param level レベル番号（デフォルト実装では未使用）
     * @return ハッシュ値
     */
    std::size_t hashCodeAtLevel(S_State const& s, int level) const {
        (void)level;
        return this->entity().hashCode(s);
    }

    /**
     * @brief 状態全体（スカラー部+配列部）のハッシュ値を計算する
     * @param p 状態データ領域
     * @param level レベル番号
     * @return ハッシュ値
     */
    std::size_t hash_code(void const* p, int level) const {
        std::size_t h = this->entity().hashCodeAtLevel(s_state(p), level);
        h *= 271828171;
        Word const* pa = static_cast<Word const*>(p);
        Word const* pz = pa + dataWords;
        pa += S_WORDS;
        while (pa != pz) {
            h += *pa++;
            h *= 314159257;
        }
        return h;
    }

    /**
     * @brief 2つのスカラー状態が等しいか判定する
     * @param s1 スカラー状態1
     * @param s2 スカラー状態2
     * @return 等しければtrue
     */
    bool equalTo(S_State const& s1, S_State const& s2) const {
        return this->rawEqualTo(s1, s2);
    }

    /**
     * @brief 指定レベルでの2つのスカラー状態の等価比較を行う
     * @param s1 スカラー状態1
     * @param s2 スカラー状態2
     * @param level レベル番号（デフォルト実装では未使用）
     * @return 等しければtrue
     */
    bool equalToAtLevel(S_State const& s1, S_State const& s2, int level) const {
        (void)level;
        return this->entity().equalTo(s1, s2);
    }

    /**
     * @brief 2つの状態全体が等しいか判定する（スカラー部+配列部）
     * @param p 状態1のデータ領域
     * @param q 状態2のデータ領域
     * @param level レベル番号
     * @return 等しければtrue
     */
    bool equal_to(void const* p, void const* q, int level) const {
        if (!this->entity().equalToAtLevel(s_state(p), s_state(q), level))
            return false;
        Word const* pa = static_cast<Word const*>(p);
        Word const* qa = static_cast<Word const*>(q);
        Word const* pz = pa + dataWords;
        pa += S_WORDS;
        qa += S_WORDS;
        while (pa != pz) {
            if (*pa++ != *qa++) return false;
        }
        return true;
    }

    /**
     * @brief 状態を出力ストリームに書き出す
     * @param os 出力ストリーム
     * @param s スカラー状態
     * @param a 配列状態へのポインタ
     */
    void printState(std::ostream& os,
                    S_State const& s,
                    A_State const* a) const {
        os << "[" << s << ":";
        for (int i = 0; i < arraySize; ++i) {
            if (i > 0) os << ",";
            os << a[i];
        }
        os << "]";
    }

    /**
     * @brief 指定レベルでの状態を出力ストリームに書き出す
     * @param os 出力ストリーム
     * @param s スカラー状態
     * @param a 配列状態へのポインタ
     * @param level レベル番号（デフォルト実装では未使用）
     */
    void printStateAtLevel(std::ostream& os,
                           S_State const& s,
                           A_State const* a,
                           int level) const {
        (void)level;
        this->entity().printState(os, s, a);
    }

    /**
     * @brief 状態を出力ストリームに書き出す（内部インターフェース）
     * @param os 出力ストリーム
     * @param p 状態データ領域
     * @param level レベル番号
     */
    void print_state(std::ostream& os, void const* p, int level) const {
        this->entity().printStateAtLevel(os, s_state(p), a_state(p), level);
    }
};

/**
 * @brief HybridDdSpecの後方互換エイリアス
 *
 * 以前のPodHybridDdSpecという名称との互換性を保つためのクラス。
 *
 * @tparam S このクラスを実装する派生クラス（CRTP）
 * @tparam TS スカラー状態のデータ型
 * @tparam TA 配列要素のデータ型
 * @tparam AR ノードの分岐数
 * @see HybridDdSpec
 */
template<typename S, typename TS, typename TA, int AR>
class PodHybridDdSpec: public HybridDdSpec<S,TS,TA,AR> {
};

} // namespace tdzdd
} // namespace sbdd2
