/**
 * @file VarArityDdSpec.hpp
 * @brief 実行時可変アリティのDD Spec基底クラス群
 *
 * コンパイル時固定のARITYではなく、実行時にsetArity()で設定可能な
 * DD Specクラスを提供する。ARITY >= 3の多値DD構築に使用する。
 *
 * 提供するクラス：
 * - VarArityHybridDdSpec: スカラ + POD配列の複合状態（可変アリティ）
 * - VarArityDdSpec: スカラ状態のみ（可変アリティ）
 * - VarArityStatelessDdSpec: 状態なし（可変アリティ）
 *
 * @see DdSpec.hpp
 * @see Sbdd2Builder.hpp
 */

#pragma once

#include <cassert>
#include <cstddef>
#include <iostream>
#include <new>

#include "../exception.hpp"

namespace sbdd2 {
namespace tdzdd {

/**
 * @brief スカラ状態とPOD配列状態を組み合わせた可変アリティDD Specの基底クラス
 *
 * HybridDdSpec<S, TS, TA, AR> がコンパイル時にARITYを要求するのに対し、
 * このクラスではsetArity()を使用して実行時にARITYを設定できる。
 * CRTPパターンにより派生クラスSのメソッドを呼び出す。
 *
 * 派生クラスは以下のメソッドを実装する必要がある：
 * - int getRoot(S_State& s, A_State* a) : ルート状態を初期化し、ルートレベルを返す
 * - int getChild(S_State& s, A_State* a, int level, int value) : 子の遷移を計算する
 *
 * 戻り値の規約：
 * - 正の値：非終端ノードのレベル
 * - 0：0終端
 * - 負の値：1終端
 *
 * @tparam S 派生クラス（CRTP）
 * @tparam TS スカラ状態のデータ型
 * @tparam TA 配列状態要素のデータ型
 *
 * @see VarArityDdSpec
 * @see VarArityStatelessDdSpec
 * @see HybridDdSpec
 * @see build_mvzdd_va
 * @see build_mvbdd_va
 */
template<typename S, typename TS, typename TA>
class VarArityHybridDdSpec {
public:
    typedef TS S_State;  ///< スカラ状態の型
    typedef TA A_State;  ///< 配列状態要素の型

private:
    typedef std::size_t Word;
    static int const S_WORDS = (sizeof(S_State) + sizeof(Word) - 1)
            / sizeof(Word);

    int arity_;       ///< アリティ（ノードあたりの子枝数）
    int arraySize_;   ///< 配列状態のサイズ
    int dataWords_;   ///< データのワード数

    /**
     * @brief void*からスカラ状態への参照を取得する
     * @param p 状態データへのポインタ
     * @return スカラ状態への参照
     */
    static S_State& s_state(void* p) {
        return *static_cast<S_State*>(p);
    }

    /**
     * @brief void const*からスカラ状態へのconst参照を取得する
     * @param p 状態データへのポインタ
     * @return スカラ状態へのconst参照
     */
    static S_State const& s_state(void const* p) {
        return *static_cast<S_State const*>(p);
    }

    /**
     * @brief void*から配列状態へのポインタを取得する
     * @param p 状態データへのポインタ
     * @return 配列状態へのポインタ
     */
    static A_State* a_state(void* p) {
        return reinterpret_cast<A_State*>(static_cast<Word*>(p) + S_WORDS);
    }

    /**
     * @brief void const*から配列状態へのconstポインタを取得する
     * @param p 状態データへのポインタ
     * @return 配列状態へのconstポインタ
     */
    static A_State const* a_state(void const* p) {
        return reinterpret_cast<A_State const*>(static_cast<Word const*>(p)
                + S_WORDS);
    }

protected:
    /**
     * @brief アリティ（ノードあたりの子枝数）を設定する
     *
     * 派生クラスのコンストラクタで呼び出す必要がある。
     *
     * @param arity アリティ値（1〜100）
     * @throws DDArgumentException arityが範囲外の場合
     */
    void setArity(int arity) {
        if (arity < 1 || arity > 100) {
            throw DDArgumentException("ARITY must be between 1 and 100");
        }
        arity_ = arity;
    }

    /**
     * @brief 配列状態のサイズを設定する
     *
     * 派生クラスのコンストラクタで呼び出す必要がある。
     *
     * @param n 配列サイズ（非負）
     */
    void setArraySize(int n) {
        assert(0 <= n);
        arraySize_ = n;
        dataWords_ = S_WORDS
                + (n * sizeof(A_State) + sizeof(Word) - 1) / sizeof(Word);
    }

    /**
     * @brief 配列状態のサイズを取得する
     * @return 配列サイズ
     */
    int getArraySize() const {
        return arraySize_;
    }

public:
    /**
     * @brief デフォルトコンストラクタ
     *
     * 派生クラスのコンストラクタでsetArity()とsetArraySize()を呼ぶ必要がある。
     */
    VarArityHybridDdSpec()
            : arity_(0), arraySize_(-1), dataWords_(-1) {
    }

    /**
     * @brief 派生クラスの実体への参照を取得する（CRTP）
     * @return 派生クラスへの参照
     */
    S& entity() {
        return *static_cast<S*>(this);
    }

    /**
     * @brief 派生クラスの実体へのconst参照を取得する（CRTP）
     * @return 派生クラスへのconst参照
     */
    S const& entity() const {
        return *static_cast<S const*>(this);
    }

    /**
     * @brief アリティを取得する
     * @return アリティ値
     */
    int getArity() const {
        return arity_;
    }

    /**
     * @brief 状態データのバイトサイズを取得する
     * @return データサイズ（バイト）
     */
    int datasize() const {
        return dataWords_ * sizeof(Word);
    }

    /**
     * @brief 状態をデフォルトコンストラクタで初期化する
     * @param p 状態データの格納先
     */
    void construct(void* p) {
        new (p) S_State();
    }

    /**
     * @brief ルート状態を初期化しルートレベルを返す
     * @param p 状態データの格納先
     * @return ルートレベル（正:非終端、0:0終端、負:1終端）
     */
    int get_root(void* p) {
        this->entity().construct(p);
        return this->entity().getRoot(s_state(p), a_state(p));
    }

    /**
     * @brief 子ノードへの遷移を計算する
     * @param p 現在の状態データ（遷移後の状態で上書きされる）
     * @param level 現在のレベル
     * @param value 枝の値（0〜arity-1）
     * @return 子のレベル（正:非終端、0:0終端、負:1終端）
     */
    int get_child(void* p, int level, int value) {
        assert(0 <= value && value < arity_);
        return this->entity().getChild(s_state(p), a_state(p), level, value);
    }

    /**
     * @brief スカラ状態をコピーコンストラクタで複製する
     * @param p コピー先
     * @param s コピー元のスカラ状態
     */
    void getCopy(void* p, S_State const& s) {
        new (p) S_State(s);
    }

    /**
     * @brief 状態データ全体を複製する
     * @param to コピー先
     * @param from コピー元
     */
    void get_copy(void* to, void const* from) {
        this->entity().getCopy(to, s_state(from));
        Word const* pa = static_cast<Word const*>(from);
        Word const* pz = pa + dataWords_;
        Word* qa = static_cast<Word*>(to);
        pa += S_WORDS;
        qa += S_WORDS;
        while (pa != pz) {
            *qa++ = *pa++;
        }
    }

    /**
     * @brief 2つの状態をマージする（デフォルトはマージ不可）
     *
     * マージが成功した場合は非ゼロを返す。
     * デフォルト実装は常に0（マージ不可）を返す。
     *
     * @param s1 状態1（マージ先）
     * @param a1 配列状態1
     * @param s2 状態2
     * @param a2 配列状態2
     * @return マージ結果（0:マージ不可、非ゼロ:マージ成功）
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
     * @param p1 状態1
     * @param p2 状態2
     * @return マージ結果
     */
    int merge_states(void* p1, void* p2) {
        return this->entity().mergeStates(s_state(p1), a_state(p1), s_state(p2),
                                          a_state(p2));
    }

    /**
     * @brief 状態を破棄する（デフォルトは何もしない）
     * @param p 破棄する状態データ
     */
    void destruct(void* p) {
        (void)p;
    }

    /**
     * @brief レベル単位の後始末を行う（デフォルトは何もしない）
     * @param level 処理完了したレベル
     */
    void destructLevel(int level) {
        (void)level;
    }

    /**
     * @brief スカラ状態のハッシュコードを計算する
     * @param s スカラ状態
     * @return ハッシュコード
     */
    std::size_t hashCode(S_State const& s) const {
        return rawHashCode(s);
    }

    /**
     * @brief レベルを考慮したスカラ状態のハッシュコードを計算する
     * @param s スカラ状態
     * @param level レベル
     * @return ハッシュコード
     */
    std::size_t hashCodeAtLevel(S_State const& s, int level) const {
        (void)level;
        return this->entity().hashCode(s);
    }

    /**
     * @brief 状態データ全体のハッシュコードを計算する（内部インターフェース）
     * @param p 状態データ
     * @param level レベル
     * @return ハッシュコード
     */
    std::size_t hash_code(void const* p, int level) const {
        std::size_t h = this->entity().hashCodeAtLevel(s_state(p), level);
        h *= 271828171;
        Word const* pa = static_cast<Word const*>(p);
        Word const* pz = pa + dataWords_;
        pa += S_WORDS;
        while (pa != pz) {
            h += *pa++;
            h *= 314159257;
        }
        return h;
    }

    /**
     * @brief 2つのスカラ状態が等しいか判定する
     * @param s1 状態1
     * @param s2 状態2
     * @return 等しければtrue
     */
    bool equalTo(S_State const& s1, S_State const& s2) const {
        return rawEqualTo(s1, s2);
    }

    /**
     * @brief レベルを考慮して2つのスカラ状態が等しいか判定する
     * @param s1 状態1
     * @param s2 状態2
     * @param level レベル
     * @return 等しければtrue
     */
    bool equalToAtLevel(S_State const& s1, S_State const& s2, int level) const {
        (void)level;
        return this->entity().equalTo(s1, s2);
    }

    /**
     * @brief 状態データ全体が等しいか判定する（内部インターフェース）
     * @param p 状態1
     * @param q 状態2
     * @param level レベル
     * @return 等しければtrue
     */
    bool equal_to(void const* p, void const* q, int level) const {
        if (!this->entity().equalToAtLevel(s_state(p), s_state(q), level))
            return false;
        Word const* pa = static_cast<Word const*>(p);
        Word const* qa = static_cast<Word const*>(q);
        Word const* pz = pa + dataWords_;
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
     * @param s スカラ状態
     * @param a 配列状態
     */
    void printState(std::ostream& os,
                    S_State const& s,
                    A_State const* a) const {
        os << "[" << s << ":";
        for (int i = 0; i < arraySize_; ++i) {
            if (i > 0) os << ",";
            os << a[i];
        }
        os << "]";
    }

    /**
     * @brief レベルを考慮して状態を出力ストリームに書き出す
     * @param os 出力ストリーム
     * @param s スカラ状態
     * @param a 配列状態
     * @param level レベル
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
     * @param p 状態データ
     * @param level レベル
     */
    void print_state(std::ostream& os, void const* p, int level) const {
        this->entity().printStateAtLevel(os, s_state(p), a_state(p), level);
    }

    /**
     * @brief レベルを出力ストリームに書き出す
     * @param os 出力ストリーム
     * @param level レベル
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
     * @brief オブジェクトのバイト列からハッシュコードを計算する
     * @tparam T オブジェクトの型
     * @param o ハッシュ対象のオブジェクト
     * @return ハッシュコード
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
     * @brief 2つのオブジェクトのバイト列が等しいか判定する
     * @tparam T オブジェクトの型
     * @param o1 オブジェクト1
     * @param o2 オブジェクト2
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
 * @brief スカラ状態を持つ可変アリティDD Specの基底クラス
 *
 * 単一のスカラ型Tを状態として使用する。
 * CRTPパターンにより派生クラスSのメソッドを呼び出す。
 *
 * 派生クラスは以下のメソッドを実装する必要がある：
 * - int getRoot(T& state) : ルート状態を初期化し、ルートレベルを返す
 * - int getChild(T& state, int level, int value) : 子の遷移を計算する
 *
 * 戻り値の規約：
 * - 正の値：非終端ノードのレベル
 * - 0：0終端
 * - 負の値：1終端
 *
 * @tparam S 派生クラス（CRTP）
 * @tparam T 状態のデータ型
 *
 * @see VarArityHybridDdSpec
 * @see VarArityStatelessDdSpec
 * @see DdSpec
 * @see build_mvzdd_va
 * @see build_mvbdd_va
 */
template<typename S, typename T>
class VarArityDdSpec {
public:
    typedef T State;  ///< 状態の型

private:
    int arity_;  ///< アリティ

    /**
     * @brief void*から状態への参照を取得する
     * @param p 状態データへのポインタ
     * @return 状態への参照
     */
    static State& state(void* p) {
        return *static_cast<State*>(p);
    }

    /**
     * @brief void const*から状態へのconst参照を取得する
     * @param p 状態データへのポインタ
     * @return 状態へのconst参照
     */
    static State const& state(void const* p) {
        return *static_cast<State const*>(p);
    }

protected:
    /**
     * @brief アリティ（ノードあたりの子枝数）を設定する
     *
     * 派生クラスのコンストラクタで呼び出す必要がある。
     *
     * @param arity アリティ値（1〜100）
     * @throws DDArgumentException arityが範囲外の場合
     */
    void setArity(int arity) {
        if (arity < 1 || arity > 100) {
            throw DDArgumentException("ARITY must be between 1 and 100");
        }
        arity_ = arity;
    }

public:
    /**
     * @brief デフォルトコンストラクタ
     *
     * 派生クラスのコンストラクタでsetArity()を呼ぶ必要がある。
     */
    VarArityDdSpec() : arity_(0) {}

    /**
     * @brief 派生クラスの実体への参照を取得する（CRTP）
     * @return 派生クラスへの参照
     */
    S& entity() {
        return *static_cast<S*>(this);
    }

    /**
     * @brief 派生クラスの実体へのconst参照を取得する（CRTP）
     * @return 派生クラスへのconst参照
     */
    S const& entity() const {
        return *static_cast<S const*>(this);
    }

    /**
     * @brief アリティを取得する
     * @return アリティ値
     */
    int getArity() const {
        return arity_;
    }

    /**
     * @brief 状態データのバイトサイズを取得する
     * @return データサイズ（バイト）
     */
    int datasize() const {
        return sizeof(State);
    }

    /**
     * @brief 状態をデフォルトコンストラクタで初期化する
     * @param p 状態データの格納先
     */
    void construct(void* p) {
        new (p) State();
    }

    /**
     * @brief ルート状態を初期化しルートレベルを返す
     * @param p 状態データの格納先
     * @return ルートレベル（正:非終端、0:0終端、負:1終端）
     */
    int get_root(void* p) {
        this->entity().construct(p);
        return this->entity().getRoot(state(p));
    }

    /**
     * @brief 子ノードへの遷移を計算する
     * @param p 現在の状態データ（遷移後の状態で上書きされる）
     * @param level 現在のレベル
     * @param value 枝の値（0〜arity-1）
     * @return 子のレベル（正:非終端、0:0終端、負:1終端）
     */
    int get_child(void* p, int level, int value) {
        assert(0 <= value && value < arity_);
        return this->entity().getChild(state(p), level, value);
    }

    /**
     * @brief 状態をコピーコンストラクタで複製する
     * @param p コピー先
     * @param s コピー元の状態
     */
    void getCopy(void* p, State const& s) {
        new (p) State(s);
    }

    /**
     * @brief 状態データを複製する（内部インターフェース）
     * @param to コピー先
     * @param from コピー元
     */
    void get_copy(void* to, void const* from) {
        this->entity().getCopy(to, state(from));
    }

    /**
     * @brief 2つの状態をマージする（デフォルトはマージ不可）
     * @param s1 状態1
     * @param s2 状態2
     * @return マージ結果（0:マージ不可）
     */
    int mergeStates(State& s1, State& s2) {
        (void)s1;
        (void)s2;
        return 0;
    }

    /**
     * @brief 2つの状態をマージする（内部インターフェース）
     * @param p1 状態1
     * @param p2 状態2
     * @return マージ結果
     */
    int merge_states(void* p1, void* p2) {
        return this->entity().mergeStates(state(p1), state(p2));
    }

    /**
     * @brief 状態をデストラクタで破棄する
     * @param p 破棄する状態データ
     */
    void destruct(void* p) {
        state(p).~State();
    }

    /**
     * @brief レベル単位の後始末を行う（デフォルトは何もしない）
     * @param level 処理完了したレベル
     */
    void destructLevel(int level) {
        (void)level;
    }

    /**
     * @brief 状態のハッシュコードを計算する
     * @param s 状態
     * @return ハッシュコード
     */
    std::size_t hashCode(State const& s) const {
        return rawHashCode(s);
    }

    /**
     * @brief レベルを考慮した状態のハッシュコードを計算する
     * @param s 状態
     * @param level レベル
     * @return ハッシュコード
     */
    std::size_t hashCodeAtLevel(State const& s, int level) const {
        (void)level;
        return this->entity().hashCode(s);
    }

    /**
     * @brief 状態のハッシュコードを計算する（内部インターフェース）
     * @param p 状態データ
     * @param level レベル
     * @return ハッシュコード
     */
    std::size_t hash_code(void const* p, int level) const {
        return this->entity().hashCodeAtLevel(state(p), level);
    }

    /**
     * @brief 2つの状態が等しいか判定する
     * @param s1 状態1
     * @param s2 状態2
     * @return 等しければtrue
     */
    bool equalTo(State const& s1, State const& s2) const {
        return s1 == s2;
    }

    /**
     * @brief レベルを考慮して2つの状態が等しいか判定する
     * @param s1 状態1
     * @param s2 状態2
     * @param level レベル
     * @return 等しければtrue
     */
    bool equalToAtLevel(State const& s1, State const& s2, int level) const {
        (void)level;
        return this->entity().equalTo(s1, s2);
    }

    /**
     * @brief 2つの状態が等しいか判定する（内部インターフェース）
     * @param p 状態1
     * @param q 状態2
     * @param level レベル
     * @return 等しければtrue
     */
    bool equal_to(void const* p, void const* q, int level) const {
        return this->entity().equalToAtLevel(state(p), state(q), level);
    }

    /**
     * @brief 状態を出力ストリームに書き出す
     * @param os 出力ストリーム
     * @param s 状態
     */
    void printState(std::ostream& os, State const& s) const {
        os << s;
    }

    /**
     * @brief レベルを考慮して状態を出力ストリームに書き出す
     * @param os 出力ストリーム
     * @param s 状態
     * @param level レベル
     */
    void printStateAtLevel(std::ostream& os, State const& s, int level) const {
        (void)level;
        this->entity().printState(os, s);
    }

    /**
     * @brief 状態を出力ストリームに書き出す（内部インターフェース）
     * @param os 出力ストリーム
     * @param p 状態データ
     * @param level レベル
     */
    void print_state(std::ostream& os, void const* p, int level) const {
        this->entity().printStateAtLevel(os, state(p), level);
    }

    /**
     * @brief レベルを出力ストリームに書き出す
     * @param os 出力ストリーム
     * @param level レベル
     */
    void printLevel(std::ostream& os, int level) const {
        os << level;
    }

private:
    template<typename U, typename I>
    static std::size_t rawHashCode_(void const* p) {
        std::size_t h = 0;
        I const* a = static_cast<I const*>(p);
        for (std::size_t i = 0; i < sizeof(U) / sizeof(I); ++i) {
            h += a[i];
            h *= 314159257;
        }
        return h;
    }

    template<typename U, typename I>
    static std::size_t rawEqualTo_(void const* p1, void const* p2) {
        I const* a1 = static_cast<I const*>(p1);
        I const* a2 = static_cast<I const*>(p2);
        for (std::size_t i = 0; i < sizeof(U) / sizeof(I); ++i) {
            if (a1[i] != a2[i]) return false;
        }
        return true;
    }

protected:
    /**
     * @brief オブジェクトのバイト列からハッシュコードを計算する
     * @tparam U オブジェクトの型
     * @param o ハッシュ対象のオブジェクト
     * @return ハッシュコード
     */
    template<typename U>
    static std::size_t rawHashCode(U const& o) {
        if (sizeof(U) % sizeof(std::size_t) == 0) {
            return rawHashCode_<U,std::size_t>(&o);
        }
        if (sizeof(U) % sizeof(unsigned int) == 0) {
            return rawHashCode_<U,unsigned int>(&o);
        }
        if (sizeof(U) % sizeof(unsigned short) == 0) {
            return rawHashCode_<U,unsigned short>(&o);
        }
        return rawHashCode_<U,unsigned char>(&o);
    }

    /**
     * @brief 2つのオブジェクトのバイト列が等しいか判定する
     * @tparam U オブジェクトの型
     * @param o1 オブジェクト1
     * @param o2 オブジェクト2
     * @return 等しければtrue
     */
    template<typename U>
    static std::size_t rawEqualTo(U const& o1, U const& o2) {
        if (sizeof(U) % sizeof(std::size_t) == 0) {
            return rawEqualTo_<U,std::size_t>(&o1, &o2);
        }
        if (sizeof(U) % sizeof(unsigned int) == 0) {
            return rawEqualTo_<U,unsigned int>(&o1, &o2);
        }
        if (sizeof(U) % sizeof(unsigned short) == 0) {
            return rawEqualTo_<U,unsigned short>(&o1, &o2);
        }
        return rawEqualTo_<U,unsigned char>(&o1, &o2);
    }
};

/**
 * @brief 状態を持たない可変アリティDD Specの基底クラス
 *
 * 状態を必要としないDD Specに使用する。
 * CRTPパターンにより派生クラスSのメソッドを呼び出す。
 *
 * 派生クラスは以下のメソッドを実装する必要がある：
 * - int getRoot() : ルートレベルを返す
 * - int getChild(int level, int value) : 子の遷移を計算する
 *
 * 戻り値の規約：
 * - 正の値：非終端ノードのレベル
 * - 0：0終端
 * - 負の値：1終端
 *
 * @tparam S 派生クラス（CRTP）
 *
 * @see VarArityDdSpec
 * @see VarArityHybridDdSpec
 * @see StatelessDdSpec
 * @see build_mvzdd_va
 * @see build_mvbdd_va
 */
template<typename S>
class VarArityStatelessDdSpec {
private:
    int arity_;  ///< アリティ

protected:
    /**
     * @brief アリティ（ノードあたりの子枝数）を設定する
     *
     * 派生クラスのコンストラクタで呼び出す必要がある。
     *
     * @param arity アリティ値（1〜100）
     * @throws DDArgumentException arityが範囲外の場合
     */
    void setArity(int arity) {
        if (arity < 1 || arity > 100) {
            throw DDArgumentException("ARITY must be between 1 and 100");
        }
        arity_ = arity;
    }

public:
    /**
     * @brief デフォルトコンストラクタ
     *
     * 派生クラスのコンストラクタでsetArity()を呼ぶ必要がある。
     */
    VarArityStatelessDdSpec() : arity_(0) {}

    /**
     * @brief 派生クラスの実体への参照を取得する（CRTP）
     * @return 派生クラスへの参照
     */
    S& entity() {
        return *static_cast<S*>(this);
    }

    /**
     * @brief 派生クラスの実体へのconst参照を取得する（CRTP）
     * @return 派生クラスへのconst参照
     */
    S const& entity() const {
        return *static_cast<S const*>(this);
    }

    /**
     * @brief アリティを取得する
     * @return アリティ値
     */
    int getArity() const {
        return arity_;
    }

    /**
     * @brief 状態データのバイトサイズを取得する（常に0）
     * @return 0
     */
    int datasize() const {
        return 0;
    }

    /**
     * @brief ルートレベルを返す（状態なし版）
     * @param p 未使用
     * @return ルートレベル
     */
    int get_root(void* p) {
        (void)p;
        return this->entity().getRoot();
    }

    /**
     * @brief 子ノードへの遷移を計算する（状態なし版）
     * @param p 未使用
     * @param level 現在のレベル
     * @param value 枝の値（0〜arity-1）
     * @return 子のレベル
     */
    int get_child(void* p, int level, int value) {
        (void)p;
        assert(0 <= value && value < arity_);
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
     * @brief 2つの状態をマージする（状態なしのため常に0を返す）
     * @param p1 状態1（未使用）
     * @param p2 状態2（未使用）
     * @return 常に0
     */
    int merge_states(void* p1, void* p2) {
        (void)p1;
        (void)p2;
        return 0;
    }

    /**
     * @brief 状態を破棄する（状態なしのため何もしない）
     * @param p 状態データ（未使用）
     */
    void destruct(void* p) {
        (void)p;
    }

    /**
     * @brief レベル単位の後始末を行う（何もしない）
     * @param level 処理完了したレベル（未使用）
     */
    void destructLevel(int level) {
        (void)level;
    }

    /**
     * @brief ハッシュコードを返す（状態なしのため常に0）
     * @param p 状態データ（未使用）
     * @param level レベル（未使用）
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
     * @param level レベル（未使用）
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
     * @param p 状態データ（未使用）
     * @param level レベル（未使用）
     */
    void print_state(std::ostream& os, void const* p, int level) const {
        (void)p;
        (void)level;
        os << "*";
    }

    /**
     * @brief レベルを出力ストリームに書き出す
     * @param os 出力ストリーム
     * @param level レベル
     */
    void printLevel(std::ostream& os, int level) const {
        os << level;
    }
};

} // namespace tdzdd
} // namespace sbdd2
