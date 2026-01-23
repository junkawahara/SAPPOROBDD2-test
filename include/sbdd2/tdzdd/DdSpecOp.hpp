/*
 * DdSpecOp.hpp - TdZdd-compatible Spec Operations for SAPPOROBDD2
 *
 * Based on TdZdd by Hiroaki Iwashita
 * Copyright (c) 2014 ERATO MINATO Project
 * Ported to SAPPOROBDD2 namespace
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
 * Base class for binary operations on specs.
 */
template<typename S, typename S1, typename S2>
class BinaryOperation: public PodArrayDdSpec<S, std::size_t, 2> {
protected:
    typedef S1 Spec1;
    typedef S2 Spec2;
    typedef std::size_t Word;

    static std::size_t const levelWords = (sizeof(int[2]) + sizeof(Word) - 1)
            / sizeof(Word);

    Spec1 spec1;
    Spec2 spec2;
    int const stateWords1;
    int const stateWords2;

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
    BinaryOperation(S1 const& s1, S2 const& s2) :
                    spec1(s1),
                    spec2(s2),
                    stateWords1(wordSize(spec1.datasize())),
                    stateWords2(wordSize(spec2.datasize())) {
        BinaryOperation::setArraySize(levelWords + stateWords1 + stateWords2);
    }

    void get_copy(void* to, void const* from) {
        setLevel1(to, level1(from));
        setLevel2(to, level2(from));
        spec1.get_copy(state1(to), state1(from));
        spec2.get_copy(state2(to), state2(from));
    }

    int merge_states(void* p1, void* p2) {
        return spec1.merge_states(state1(p1), state1(p2))
                | spec2.merge_states(state2(p1), state2(p2));
    }

    void destruct(void* p) {
        spec1.destruct(state1(p));
        spec2.destruct(state2(p));
    }

    void destructLevel(int level) {
        spec1.destructLevel(level);
        spec2.destructLevel(level);
    }

    std::size_t hash_code(void const* p, int level) const {
        std::size_t h = std::size_t(level1(p)) * 314159257
                + std::size_t(level2(p)) * 271828171;
        if (level1(p) > 0)
            h += spec1.hash_code(state1(p), level1(p)) * 171828143;
        if (level2(p) > 0)
            h += spec2.hash_code(state2(p), level2(p)) * 141421333;
        return h;
    }

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
 * BDD AND operation.
 */
template<typename S1, typename S2>
struct BddAnd: public BinaryOperation<BddAnd<S1,S2>, S1, S2> {
    typedef BinaryOperation<BddAnd<S1,S2>, S1, S2> base;
    typedef typename base::Word Word;

    BddAnd(S1 const& s1, S2 const& s2) : base(s1, s2) {}

    int getRoot(Word* p) {
        int i1 = base::spec1.get_root(base::state1(p));
        if (i1 == 0) return 0;
        int i2 = base::spec2.get_root(base::state2(p));
        if (i2 == 0) return 0;
        base::setLevel1(p, i1);
        base::setLevel2(p, i2);
        return std::max(base::level1(p), base::level2(p));
    }

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
 * BDD OR operation.
 */
template<typename S1, typename S2>
struct BddOr: public BinaryOperation<BddOr<S1,S2>, S1, S2> {
    typedef BinaryOperation<BddOr<S1,S2>, S1, S2> base;
    typedef typename base::Word Word;

    BddOr(S1 const& s1, S2 const& s2) : base(s1, s2) {}

    int getRoot(Word* p) {
        int i1 = base::spec1.get_root(base::state1(p));
        if (i1 < 0) return -1;
        int i2 = base::spec2.get_root(base::state2(p));
        if (i2 < 0) return -1;
        base::setLevel1(p, i1);
        base::setLevel2(p, i2);
        return std::max(base::level1(p), base::level2(p));
    }

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
 * ZDD Intersection operation.
 */
template<typename S1, typename S2>
struct ZddIntersection: public PodArrayDdSpec<ZddIntersection<S1,S2>, std::size_t, 2> {
    typedef S1 Spec1;
    typedef S2 Spec2;
    typedef std::size_t Word;

    Spec1 spec1;
    Spec2 spec2;
    int const stateWords1;
    int const stateWords2;

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
    ZddIntersection(S1 const& s1, S2 const& s2) :
                    spec1(s1),
                    spec2(s2),
                    stateWords1(wordSize(spec1.datasize())),
                    stateWords2(wordSize(spec2.datasize())) {
        ZddIntersection::setArraySize(stateWords1 + stateWords2);
    }

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

    void get_copy(void* to, void const* from) {
        spec1.get_copy(state1(to), state1(from));
        spec2.get_copy(state2(to), state2(from));
    }

    int merge_states(void* p1, void* p2) {
        return spec1.merge_states(state1(p1), state1(p2))
                | spec2.merge_states(state2(p1), state2(p2));
    }

    void destruct(void* p) {
        spec1.destruct(state1(p));
        spec2.destruct(state2(p));
    }

    void destructLevel(int level) {
        spec1.destructLevel(level);
        spec2.destructLevel(level);
    }

    std::size_t hash_code(void const* p, int level) const {
        return spec1.hash_code(state1(p), level) * 314159257
                + spec2.hash_code(state2(p), level) * 271828171;
    }

    bool equal_to(void const* p, void const* q, int level) const {
        return spec1.equal_to(state1(p), state1(q), level)
                && spec2.equal_to(state2(p), state2(q), level);
    }

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
 * ZDD Union operation.
 */
template<typename S1, typename S2>
struct ZddUnion: public BinaryOperation<ZddUnion<S1,S2>, S1, S2> {
    typedef BinaryOperation<ZddUnion<S1,S2>, S1, S2> base;
    typedef typename base::Word Word;

    ZddUnion(S1 const& s1, S2 const& s2) : base(s1, s2) {}

    int getRoot(Word* p) {
        int i1 = base::spec1.get_root(base::state1(p));
        int i2 = base::spec2.get_root(base::state2(p));
        if (i1 == 0 && i2 == 0) return 0;
        if (i1 <= 0 && i2 <= 0) return -1;
        base::setLevel1(p, i1);
        base::setLevel2(p, i2);
        return std::max(base::level1(p), base::level2(p));
    }

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
 * BDD Lookahead optimization.
 * Optimizes a BDD specification in terms of the BDD node deletion rule.
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
    BddLookahead(S const& s)
            : spec(s), work0(spec.datasize()), work1(spec.datasize()) {
    }

    int datasize() const {
        return spec.datasize();
    }

    int get_root(void* p) {
        return lookahead(p, spec.get_root(p));
    }

    int get_child(void* p, int level, int b) {
        return lookahead(p, spec.get_child(p, level, b));
    }

    void get_copy(void* to, void const* from) {
        spec.get_copy(to, from);
    }

    int merge_states(void* p1, void* p2) {
        return spec.merge_states(p1, p2);
    }

    void destruct(void* p) {
        spec.destruct(p);
    }

    void destructLevel(int level) {
        spec.destructLevel(level);
    }

    std::size_t hash_code(void const* p, int level) const {
        return spec.hash_code(p, level);
    }

    bool equal_to(void const* p, void const* q, int level) const {
        return spec.equal_to(p, q, level);
    }

    void print_state(std::ostream& os, void const* p, int level) const {
        spec.print_state(os, p, level);
    }
};

/**
 * ZDD Lookahead optimization.
 * Optimizes a ZDD specification in terms of the ZDD node deletion rule.
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
    ZddLookahead(S const& s)
            : spec(s), work(spec.datasize()) {
    }

    int datasize() const {
        return spec.datasize();
    }

    int get_root(void* p) {
        return lookahead(p, spec.get_root(p));
    }

    int get_child(void* p, int level, int b) {
        return lookahead(p, spec.get_child(p, level, b));
    }

    void get_copy(void* to, void const* from) {
        spec.get_copy(to, from);
    }

    int merge_states(void* p1, void* p2) {
        return spec.merge_states(p1, p2);
    }

    void destruct(void* p) {
        spec.destruct(p);
    }

    void destructLevel(int level) {
        spec.destructLevel(level);
    }

    std::size_t hash_code(void const* p, int level) const {
        return spec.hash_code(p, level);
    }

    bool equal_to(void const* p, void const* q, int level) const {
        return spec.equal_to(p, q, level);
    }

    void print_state(std::ostream& os, void const* p, int level) const {
        spec.print_state(os, p, level);
    }
};

/**
 * BDD Unreduction.
 * Creates a QDD specification from a BDD specification by complementing
 * skipped nodes in terms of the BDD node deletion rule.
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
    BddUnreduction(S const& s, int numVars_)
            : spec(s), stateWords(wordSize(spec.datasize())), numVars(numVars_) {
        BddUnreduction::setArraySize(levelWords + stateWords);
    }

    int getRoot(Word* p) {
        level(p) = spec.get_root(state(p));
        if (level(p) == 0) return 0;
        if (level(p) >= numVars) numVars = level(p);
        return (numVars > 0) ? numVars : -1;
    }

    int getChild(Word* p, int i, int value) {
        if (level(p) == i) {
            level(p) = spec.get_child(state(p), i, value);
            if (level(p) == 0) return 0;
        }

        --i;
        assert(level(p) <= i);
        return (i > 0) ? i : level(p);
    }

    void get_copy(void* to, void const* from) {
        level(to) = level(from);
        spec.get_copy(state(to), state(from));
    }

    void destruct(void* p) {
        spec.destruct(state(p));
    }

    void destructLevel(int lvl) {
        spec.destructLevel(lvl);
    }

    int merge_states(void* p1, void* p2) {
        return spec.merge_states(state(p1), state(p2));
    }

    std::size_t hash_code(void const* p, int i) const {
        (void)i;
        std::size_t h = std::size_t(level(p)) * 314159257;
        if (level(p) > 0) h += spec.hash_code(state(p), level(p)) * 271828171;
        return h;
    }

    bool equal_to(void const* p, void const* q, int i) const {
        (void)i;
        if (level(p) != level(q)) return false;
        if (level(p) > 0 && !spec.equal_to(state(p), state(q), level(p))) return false;
        return true;
    }

    void print_state(std::ostream& os, void const* p, int l) const {
        Word const* q = static_cast<Word const*>(p);
        os << "<" << level(q) << ",";
        spec.print_state(os, state(q), l);
        os << ">";
    }
};

/**
 * ZDD Unreduction.
 * Creates a QDD specification from a ZDD specification by complementing
 * skipped nodes in terms of the ZDD node deletion rule.
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
    ZddUnreduction(S const& s, int numVars_)
            : spec(s), stateWords(wordSize(spec.datasize())), numVars(numVars_) {
        ZddUnreduction::setArraySize(levelWords + stateWords);
    }

    int getRoot(Word* p) {
        level(p) = spec.get_root(state(p));
        if (level(p) == 0) return 0;
        if (level(p) >= numVars) numVars = level(p);
        return (numVars > 0) ? numVars : -1;
    }

    int getChild(Word* p, int i, int value) {
        if (level(p) == i) {
            level(p) = spec.get_child(state(p), i, value);
            if (level(p) == 0) return 0;
        }
        else if (value) {
            return 0;  // ZDD: 1-edge to skipped level goes to 0-terminal
        }

        --i;
        assert(level(p) <= i);
        return (i > 0) ? i : level(p);
    }

    void get_copy(void* to, void const* from) {
        level(to) = level(from);
        spec.get_copy(state(to), state(from));
    }

    void destruct(void* p) {
        spec.destruct(state(p));
    }

    void destructLevel(int lvl) {
        spec.destructLevel(lvl);
    }

    int merge_states(void* p1, void* p2) {
        return spec.merge_states(state(p1), state(p2));
    }

    std::size_t hash_code(void const* p, int i) const {
        (void)i;
        std::size_t h = std::size_t(level(p)) * 314159257;
        if (level(p) > 0) h += spec.hash_code(state(p), level(p)) * 271828171;
        return h;
    }

    bool equal_to(void const* p, void const* q, int i) const {
        (void)i;
        if (level(p) != level(q)) return false;
        if (level(p) > 0 && !spec.equal_to(state(p), state(q), level(p))) return false;
        return true;
    }

    void print_state(std::ostream& os, void const* p, int l) const {
        Word const* q = static_cast<Word const*>(p);
        os << "<" << level(q) << ",";
        spec.print_state(os, state(q), l);
        os << ">";
    }
};

// Convenience functions

/**
 * Returns a BDD specification for logical AND of two BDD specifications.
 */
template<typename S1, typename S2>
BddAnd<S1,S2> bddAnd(S1 const& spec1, S2 const& spec2) {
    return BddAnd<S1,S2>(spec1, spec2);
}

/**
 * Returns a BDD specification for logical OR of two BDD specifications.
 */
template<typename S1, typename S2>
BddOr<S1,S2> bddOr(S1 const& spec1, S2 const& spec2) {
    return BddOr<S1,S2>(spec1, spec2);
}

/**
 * Returns a ZDD specification for set intersection of two ZDD specifications.
 */
template<typename S1, typename S2>
ZddIntersection<S1,S2> zddIntersection(S1 const& spec1, S2 const& spec2) {
    return ZddIntersection<S1,S2>(spec1, spec2);
}

/**
 * Returns a ZDD specification for set union of two ZDD specifications.
 */
template<typename S1, typename S2>
ZddUnion<S1,S2> zddUnion(S1 const& spec1, S2 const& spec2) {
    return ZddUnion<S1,S2>(spec1, spec2);
}

/**
 * Optimizes a BDD specification in terms of the BDD node deletion rule.
 */
template<typename S>
BddLookahead<S> bddLookahead(S const& spec) {
    return BddLookahead<S>(spec);
}

/**
 * Optimizes a ZDD specification in terms of the ZDD node deletion rule.
 */
template<typename S>
ZddLookahead<S> zddLookahead(S const& spec) {
    return ZddLookahead<S>(spec);
}

/**
 * Creates a QDD specification from a BDD specification.
 */
template<typename S>
BddUnreduction<S> bddUnreduction(S const& spec, int numVars) {
    return BddUnreduction<S>(spec, numVars);
}

/**
 * Creates a QDD specification from a ZDD specification.
 */
template<typename S>
ZddUnreduction<S> zddUnreduction(S const& spec, int numVars) {
    return ZddUnreduction<S>(spec, numVars);
}

// ============================================================
// Variable Arity Spec Operations
// ============================================================

/**
 * Base class for binary operations on VarArity specs.
 */
template<typename S, typename S1, typename S2>
class VarArityBinaryOperation {
protected:
    typedef S1 Spec1;
    typedef S2 Spec2;
    typedef std::size_t Word;

    static std::size_t const levelWords = (sizeof(int[2]) + sizeof(Word) - 1)
            / sizeof(Word);

    Spec1 spec1;
    Spec2 spec2;
    int const stateWords1;
    int const stateWords2;
    int arity_;
    int arraySize_;

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

    S& entity() {
        return *static_cast<S*>(this);
    }

    S const& entity() const {
        return *static_cast<S const*>(this);
    }

    int getArity() const {
        return arity_;
    }

    int datasize() const {
        return arraySize_ * sizeof(Word);
    }

    void get_copy(void* to, void const* from) {
        setLevel1(to, level1(from));
        setLevel2(to, level2(from));
        spec1.get_copy(state1(to), state1(from));
        spec2.get_copy(state2(to), state2(from));
    }

    int merge_states(void* p1, void* p2) {
        return spec1.merge_states(state1(p1), state1(p2))
                | spec2.merge_states(state2(p1), state2(p2));
    }

    void destruct(void* p) {
        spec1.destruct(state1(p));
        spec2.destruct(state2(p));
    }

    void destructLevel(int level) {
        spec1.destructLevel(level);
        spec2.destructLevel(level);
    }

    std::size_t hash_code(void const* p, int level) const {
        std::size_t h = std::size_t(level1(p)) * 314159257
                + std::size_t(level2(p)) * 271828171;
        if (level1(p) > 0)
            h += spec1.hash_code(state1(p), level1(p)) * 171828143;
        if (level2(p) > 0)
            h += spec2.hash_code(state2(p), level2(p)) * 141421333;
        return h;
    }

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

    void printLevel(std::ostream& os, int level) const {
        os << level;
    }
};

/**
 * VarArity ZDD Union operation.
 */
template<typename S1, typename S2>
struct VarArityZddUnion
        : public VarArityBinaryOperation<VarArityZddUnion<S1,S2>, S1, S2> {
    typedef VarArityBinaryOperation<VarArityZddUnion<S1,S2>, S1, S2> base;
    typedef typename base::Word Word;

    VarArityZddUnion(S1 const& s1, S2 const& s2) : base(s1, s2) {}

    int getRoot(Word* p) {
        int i1 = base::spec1.get_root(base::state1(p));
        int i2 = base::spec2.get_root(base::state2(p));
        if (i1 == 0 && i2 == 0) return 0;
        if (i1 <= 0 && i2 <= 0) return -1;
        base::setLevel1(p, i1);
        base::setLevel2(p, i2);
        return std::max(base::level1(p), base::level2(p));
    }

    int get_root(void* p) {
        return getRoot(static_cast<Word*>(p));
    }

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

    int get_child(void* p, int level, int value) {
        return getChild(static_cast<Word*>(p), level, value);
    }

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
 * VarArity ZDD Intersection operation.
 */
template<typename S1, typename S2>
struct VarArityZddIntersection {
    typedef S1 Spec1;
    typedef S2 Spec2;
    typedef std::size_t Word;

    Spec1 spec1;
    Spec2 spec2;
    int const stateWords1;
    int const stateWords2;
    int arity_;
    int arraySize_;

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

    int getArity() const {
        return arity_;
    }

    int datasize() const {
        return arraySize_ * sizeof(Word);
    }

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

    int get_root(void* p) {
        return getRoot(static_cast<Word*>(p));
    }

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

    int get_child(void* p, int level, int value) {
        return getChild(static_cast<Word*>(p), level, value);
    }

    void get_copy(void* to, void const* from) {
        spec1.get_copy(state1(to), state1(from));
        spec2.get_copy(state2(to), state2(from));
    }

    int merge_states(void* p1, void* p2) {
        return spec1.merge_states(state1(p1), state1(p2))
                | spec2.merge_states(state2(p1), state2(p2));
    }

    void destruct(void* p) {
        spec1.destruct(state1(p));
        spec2.destruct(state2(p));
    }

    void destructLevel(int level) {
        spec1.destructLevel(level);
        spec2.destructLevel(level);
    }

    std::size_t hash_code(void const* p, int level) const {
        return spec1.hash_code(state1(p), level) * 314159257
                + spec2.hash_code(state2(p), level) * 271828171;
    }

    bool equal_to(void const* p, void const* q, int level) const {
        return spec1.equal_to(state1(p), state1(q), level)
                && spec2.equal_to(state2(p), state2(q), level);
    }

    void print_state(std::ostream& os, void const* p, int level) const {
        Word const* q = static_cast<Word const*>(p);
        os << "<";
        spec1.print_state(os, state1(q), level);
        os << "> INTERSECT <";
        spec2.print_state(os, state2(q), level);
        os << ">";
    }

    void printLevel(std::ostream& os, int level) const {
        os << level;
    }
};

/**
 * Returns a VarArity ZDD specification for set union of two VarArity ZDD specifications.
 * Both specs must have the same ARITY.
 *
 * @throws DDArgumentException if specs have different arities
 */
template<typename S1, typename S2>
VarArityZddUnion<S1,S2> zddUnionVA(S1 const& spec1, S2 const& spec2) {
    return VarArityZddUnion<S1,S2>(spec1, spec2);
}

/**
 * Returns a VarArity ZDD specification for set intersection of two VarArity ZDD specifications.
 * Both specs must have the same ARITY.
 *
 * @throws DDArgumentException if specs have different arities
 */
template<typename S1, typename S2>
VarArityZddIntersection<S1,S2> zddIntersectionVA(S1 const& spec1, S2 const& spec2) {
    return VarArityZddIntersection<S1,S2>(spec1, spec2);
}

} // namespace tdzdd
} // namespace sbdd2
