/*
 * DdSpec.hpp - TdZdd-compatible Spec Interface for SAPPOROBDD2
 *
 * Based on TdZdd by Hiroaki Iwashita
 * Copyright (c) 2014 ERATO MINATO Project
 * Ported to SAPPOROBDD2 namespace
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
 * Base class of DD specs.
 *
 * Every implementation must have the following functions:
 * - int datasize() const
 * - int get_root(void* p)
 * - int get_child(void* p, int level, int value)
 * - void get_copy(void* to, void const* from)
 * - int merge_states(void* p1, void* p2)
 * - void destruct(void* p)
 * - void destructLevel(int level)
 * - size_t hash_code(void const* p, int level) const
 * - bool equal_to(void const* p, void const* q, int level) const
 * - void print_state(std::ostream& os, void const* p, int level) const
 *
 * A return code of get_root(void*) or get_child(void*, int, int) is:
 * 0 when the node is the 0-terminal, -1 when it is the 1-terminal, or
 * the node level when it is a non-terminal.
 * A return code of merge_states(void*, void*) is: 0 when the states are
 * merged into the first one, 1 when they cannot be merged and the first
 * one should be forwarded to the 0-terminal, 2 when they cannot be merged
 * and the second one should be forwarded to the 0-terminal.
 *
 * @tparam S the class implementing this class.
 * @tparam AR arity of the nodes.
 */
template<typename S, int AR>
class DdSpecBase {
public:
    static int const ARITY = AR;

    S& entity() {
        return *static_cast<S*>(this);
    }

    S const& entity() const {
        return *static_cast<S const*>(this);
    }

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
 * Abstract class of DD specifications without states.
 *
 * Every implementation must have the following functions:
 * - int getRoot()
 * - int getChild(int level, int value)
 *
 * @tparam S the class implementing this class.
 * @tparam AR arity of the nodes.
 */
template<typename S, int AR = 2>
class StatelessDdSpec: public DdSpecBase<S,AR> {
public:
    int datasize() const {
        return 0;
    }

    int get_root(void* p) {
        (void)p;
        return this->entity().getRoot();
    }

    int get_child(void* p, int level, int value) {
        (void)p;
        assert(0 <= value && value < S::ARITY);
        return this->entity().getChild(level, value);
    }

    void get_copy(void* to, void const* from) {
        (void)to;
        (void)from;
    }

    int merge_states(void* p1, void* p2) {
        (void)p1;
        (void)p2;
        return 0;
    }

    void destruct(void* p) {
        (void)p;
    }

    void destructLevel(int level) {
        (void)level;
    }

    std::size_t hash_code(void const* p, int level) const {
        (void)p;
        (void)level;
        return 0;
    }

    bool equal_to(void const* p, void const* q, int level) const {
        (void)p;
        (void)q;
        (void)level;
        return true;
    }

    void print_state(std::ostream& os, void const* p, int level) const {
        (void)p;
        (void)level;
        os << "*";
    }
};

/**
 * Abstract class of DD specifications using scalar states.
 *
 * Every implementation must have the following functions:
 * - int getRoot(T& state)
 * - int getChild(T& state, int level, int value)
 *
 * @tparam S the class implementing this class.
 * @tparam T data type.
 * @tparam AR arity of the nodes.
 */
template<typename S, typename T, int AR = 2>
class DdSpec: public DdSpecBase<S,AR> {
public:
    typedef T State;

private:
    static State& state(void* p) {
        return *static_cast<State*>(p);
    }

    static State const& state(void const* p) {
        return *static_cast<State const*>(p);
    }

public:
    int datasize() const {
        return sizeof(State);
    }

    void construct(void* p) {
        new (p) State();
    }

    int get_root(void* p) {
        this->entity().construct(p);
        return this->entity().getRoot(state(p));
    }

    int get_child(void* p, int level, int value) {
        assert(0 <= value && value < S::ARITY);
        return this->entity().getChild(state(p), level, value);
    }

    void getCopy(void* p, State const& s) {
        new (p) State(s);
    }

    void get_copy(void* to, void const* from) {
        this->entity().getCopy(to, state(from));
    }

    int mergeStates(State& s1, State& s2) {
        (void)s1;
        (void)s2;
        return 0;
    }

    int merge_states(void* p1, void* p2) {
        return this->entity().mergeStates(state(p1), state(p2));
    }

    void destruct(void* p) {
        state(p).~State();
    }

    void destructLevel(int level) {
        (void)level;
    }

    std::size_t hashCode(State const& s) const {
        return this->rawHashCode(s);
    }

    std::size_t hashCodeAtLevel(State const& s, int level) const {
        (void)level;
        return this->entity().hashCode(s);
    }

    std::size_t hash_code(void const* p, int level) const {
        return this->entity().hashCodeAtLevel(state(p), level);
    }

    bool equalTo(State const& s1, State const& s2) const {
        return s1 == s2;
    }

    bool equalToAtLevel(State const& s1, State const& s2, int level) const {
        (void)level;
        return this->entity().equalTo(s1, s2);
    }

    bool equal_to(void const* p, void const* q, int level) const {
        return this->entity().equalToAtLevel(state(p), state(q), level);
    }

    void printState(std::ostream& os, State const& s) const {
        os << s;
    }

    void printStateAtLevel(std::ostream& os, State const& s, int level) const {
        (void)level;
        this->entity().printState(os, s);
    }

    void print_state(std::ostream& os, void const* p, int level) const {
        this->entity().printStateAtLevel(os, state(p), level);
    }
};

/**
 * Abstract class of DD specifications using POD array states.
 *
 * @tparam S the class implementing this class.
 * @tparam T data type of array elements.
 * @tparam AR arity of the nodes.
 */
template<typename S, typename T, int AR = 2>
class PodArrayDdSpec: public DdSpecBase<S,AR> {
public:
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
    void setArraySize(int n) {
        assert(0 <= n);
        if (arraySize >= 0)
            throw std::runtime_error(
                    "Cannot set array size twice; use setArraySize(int) only once in the constructor of DD spec.");
        arraySize = n;
        dataWords = (n * sizeof(State) + sizeof(Word) - 1) / sizeof(Word);
    }

    int getArraySize() const {
        return arraySize;
    }

public:
    PodArrayDdSpec() :
            arraySize(-1), dataWords(-1) {
    }

    int datasize() const {
        if (dataWords < 0)
            throw std::runtime_error(
                    "Array size is unknown; please set it by setArraySize(int) in the constructor of DD spec.");
        return dataWords * sizeof(Word);
    }

    int get_root(void* p) {
        return this->entity().getRoot(state(p));
    }

    int get_child(void* p, int level, int value) {
        assert(0 <= value && value < S::ARITY);
        return this->entity().getChild(state(p), level, value);
    }

    void get_copy(void* to, void const* from) {
        Word const* pa = static_cast<Word const*>(from);
        Word const* pz = pa + dataWords;
        Word* qa = static_cast<Word*>(to);
        while (pa != pz) {
            *qa++ = *pa++;
        }
    }

    int mergeStates(T* a1, T* a2) {
        (void)a1;
        (void)a2;
        return 0;
    }

    int merge_states(void* p1, void* p2) {
        return this->entity().mergeStates(state(p1), state(p2));
    }

    void destruct(void* p) {
        (void)p;
    }

    void destructLevel(int level) {
        (void)level;
    }

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

    std::size_t hashCodeAtLevel(State const* s, int level) const {
        (void)level;
        return this->entity().hashCode(s);
    }

    std::size_t hash_code(void const* p, int level) const {
        return this->entity().hashCodeAtLevel(state(p), level);
    }

    bool equalTo(State const* s1, State const* s2) const {
        Word const* pa = reinterpret_cast<Word const*>(s1);
        Word const* qa = reinterpret_cast<Word const*>(s2);
        Word const* pz = pa + dataWords;
        while (pa != pz) {
            if (*pa++ != *qa++) return false;
        }
        return true;
    }

    bool equalToAtLevel(State const* s1, State const* s2, int level) const {
        (void)level;
        return this->entity().equalTo(s1, s2);
    }

    bool equal_to(void const* p, void const* q, int level) const {
        return this->entity().equalToAtLevel(state(p), state(q), level);
    }

    void printState(std::ostream& os, State const* a) const {
        os << "[";
        for (int i = 0; i < arraySize; ++i) {
            if (i > 0) os << ",";
            os << a[i];
        }
        os << "]";
    }

    void printStateAtLevel(std::ostream& os, State const* a, int level) const {
        (void)level;
        this->entity().printState(os, a);
    }

    void print_state(std::ostream& os, void const* p, int level) const {
        this->entity().printStateAtLevel(os, state(p), level);
    }
};

/**
 * Abstract class of DD specifications using both scalar and POD array states.
 *
 * @tparam S the class implementing this class.
 * @tparam TS data type of scalar.
 * @tparam TA data type of array elements.
 * @tparam AR arity of the nodes.
 */
template<typename S, typename TS, typename TA, int AR = 2>
class HybridDdSpec: public DdSpecBase<S,AR> {
public:
    typedef TS S_State;
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
    void setArraySize(int n) {
        assert(0 <= n);
        arraySize = n;
        dataWords = S_WORDS
                + (n * sizeof(A_State) + sizeof(Word) - 1) / sizeof(Word);
    }

    int getArraySize() const {
        return arraySize;
    }

public:
    HybridDdSpec() :
            arraySize(-1), dataWords(-1) {
    }

    int datasize() const {
        return dataWords * sizeof(Word);
    }

    void construct(void* p) {
        new (p) S_State();
    }

    int get_root(void* p) {
        this->entity().construct(p);
        return this->entity().getRoot(s_state(p), a_state(p));
    }

    int get_child(void* p, int level, int value) {
        assert(0 <= value && value < S::ARITY);
        return this->entity().getChild(s_state(p), a_state(p), level, value);
    }

    void getCopy(void* p, S_State const& s) {
        new (p) S_State(s);
    }

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

    int mergeStates(S_State& s1, A_State* a1, S_State& s2, A_State* a2) {
        (void)s1;
        (void)a1;
        (void)s2;
        (void)a2;
        return 0;
    }

    int merge_states(void* p1, void* p2) {
        return this->entity().mergeStates(s_state(p1), a_state(p1), s_state(p2),
                                          a_state(p2));
    }

    void destruct(void* p) {
        (void)p;
    }

    void destructLevel(int level) {
        (void)level;
    }

    std::size_t hashCode(S_State const& s) const {
        return this->rawHashCode(s);
    }

    std::size_t hashCodeAtLevel(S_State const& s, int level) const {
        (void)level;
        return this->entity().hashCode(s);
    }

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

    bool equalTo(S_State const& s1, S_State const& s2) const {
        return this->rawEqualTo(s1, s2);
    }

    bool equalToAtLevel(S_State const& s1, S_State const& s2, int level) const {
        (void)level;
        return this->entity().equalTo(s1, s2);
    }

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

    void printStateAtLevel(std::ostream& os,
                           S_State const& s,
                           A_State const* a,
                           int level) const {
        (void)level;
        this->entity().printState(os, s, a);
    }

    void print_state(std::ostream& os, void const* p, int level) const {
        this->entity().printStateAtLevel(os, s_state(p), a_state(p), level);
    }
};

// for backward compatibility
template<typename S, typename TS, typename TA, int AR>
class PodHybridDdSpec: public HybridDdSpec<S,TS,TA,AR> {
};

} // namespace tdzdd
} // namespace sbdd2
