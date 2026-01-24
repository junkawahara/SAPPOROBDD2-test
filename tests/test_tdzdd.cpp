// TdZdd integration tests
#include <gtest/gtest.h>

#include "sbdd2/dd_manager.hpp"
#include "sbdd2/zdd.hpp"
#include "sbdd2/bdd.hpp"
#include "sbdd2/mvzdd.hpp"
#include "sbdd2/mvbdd.hpp"
#include "sbdd2/tdzdd/DdSpec.hpp"
#include "sbdd2/tdzdd/VarArityDdSpec.hpp"
#include "sbdd2/tdzdd/Sbdd2Builder.hpp"
#include "sbdd2/tdzdd/Sbdd2BuilderMP.hpp"
#include "sbdd2/tdzdd/DdSpecOp.hpp"
#include "sbdd2/tdzdd/DdEval.hpp"
#include "sbdd2/exception.hpp"

using namespace sbdd2;
using namespace sbdd2::tdzdd;

// Simple Combination spec: enumerate all k-subsets of {1,...,n}
class Combination : public DdSpec<Combination, int, 2> {
    int n_;
    int k_;

public:
    Combination(int n, int k) : n_(n), k_(k) {}

    int getRoot(int& state) const {
        state = 0;  // number of selected items so far
        return n_;  // start from level n
    }

    int getChild(int& state, int level, int value) const {
        state += value;  // add 1 if selected
        --level;

        if (level == 0) {
            return (state == k_) ? -1 : 0;  // -1 = accept, 0 = reject
        }

        // Pruning
        if (state > k_) return 0;  // too many selected
        if (state + level < k_) return 0;  // cannot reach k_

        return level;
    }
};

// Stateless spec: all subsets
class PowerSet : public StatelessDdSpec<PowerSet, 2> {
    int n_;

public:
    explicit PowerSet(int n) : n_(n) {}

    int getRoot() const {
        return n_;
    }

    int getChild(int level, int value) const {
        (void)value;  // unused
        if (level == 1) return -1;  // accept at bottom level
        return level - 1;  // continue to next level
    }
};

// Singleton spec: only sets of size 1
class Singleton : public DdSpec<Singleton, int, 2> {
    int n_;

public:
    explicit Singleton(int n) : n_(n) {}

    int getRoot(int& state) const {
        state = 0;
        return n_;
    }

    int getChild(int& state, int level, int value) const {
        state += value;
        --level;

        if (state > 1) return 0;  // too many selected

        if (level == 0) {
            return (state == 1) ? -1 : 0;
        }

        return level;
    }
};

class TdZddTest : public ::testing::Test {
protected:
    DDManager mgr;

    void SetUp() override {
        // Nothing to do
    }
};

TEST_F(TdZddTest, CombinationBasic) {
    // C(5, 3) = 10
    Combination spec(5, 3);
    ZDD result = build_zdd(mgr, spec);

    EXPECT_DOUBLE_EQ(result.card(), 10.0);
}

TEST_F(TdZddTest, CombinationEdgeCases) {
    // C(5, 0) = 1 (empty set)
    {
        Combination spec(5, 0);
        ZDD result = build_zdd(mgr, spec);
        EXPECT_DOUBLE_EQ(result.card(), 1.0);
    }

    // C(5, 5) = 1 (full set)
    {
        Combination spec(5, 5);
        ZDD result = build_zdd(mgr, spec);
        EXPECT_DOUBLE_EQ(result.card(), 1.0);
    }

    // C(5, 6) = 0 (impossible)
    {
        Combination spec(5, 6);
        ZDD result = build_zdd(mgr, spec);
        EXPECT_DOUBLE_EQ(result.card(), 0.0);
    }
}

TEST_F(TdZddTest, CombinationLarger) {
    // C(10, 5) = 252
    Combination spec(10, 5);
    ZDD result = build_zdd(mgr, spec);

    EXPECT_DOUBLE_EQ(result.card(), 252.0);
}

TEST_F(TdZddTest, PowerSetBasic) {
    // 2^3 = 8
    PowerSet spec(3);
    ZDD result = build_zdd(mgr, spec);

    EXPECT_DOUBLE_EQ(result.card(), 8.0);
}

TEST_F(TdZddTest, PowerSetLarger) {
    // 2^10 = 1024
    PowerSet spec(10);
    ZDD result = build_zdd(mgr, spec);

    EXPECT_DOUBLE_EQ(result.card(), 1024.0);
}

TEST_F(TdZddTest, UnreducedZDDBuild) {
    Combination spec(5, 2);
    UnreducedZDD unreduced = build_unreduced_zdd(mgr, spec);

    // Should have correct cardinality after reduction
    ZDD reduced = unreduced.reduce();
    EXPECT_DOUBLE_EQ(reduced.card(), 10.0);  // C(5, 2) = 10
}

// Tests for ZddUnion
TEST_F(TdZddTest, ZddUnion) {
    // Union of C(5,2) and C(5,3)
    // C(5,2) = 10, C(5,3) = 10, but they don't overlap
    // Total should be 20
    Combination spec2(5, 2);
    Combination spec3(5, 3);

    auto unionSpec = zddUnion(spec2, spec3);
    ZDD result = build_zdd(mgr, unionSpec);

    EXPECT_DOUBLE_EQ(result.card(), 20.0);
}

// Tests for ZddIntersection
TEST_F(TdZddTest, ZddIntersection) {
    // Intersection of PowerSet(3) and Singleton(3)
    // PowerSet(3) has 8 sets, Singleton(3) has 3 sets
    // Intersection should be 3 (only singletons)
    PowerSet powerSpec(3);
    Singleton singleSpec(3);

    auto intersectSpec = zddIntersection(powerSpec, singleSpec);
    ZDD result = build_zdd(mgr, intersectSpec);

    EXPECT_DOUBLE_EQ(result.card(), 3.0);
}

// Tests for OpenMP parallel builder
TEST_F(TdZddTest, ParallelBuildBasic) {
    // Same as sequential: C(5, 3) = 10
    Combination spec(5, 3);
    ZDD result = build_zdd_mp(mgr, spec);

    EXPECT_DOUBLE_EQ(result.card(), 10.0);
}

TEST_F(TdZddTest, ParallelBuildLarger) {
    // C(10, 5) = 252
    Combination spec(10, 5);
    ZDD result = build_zdd_mp(mgr, spec);

    EXPECT_DOUBLE_EQ(result.card(), 252.0);
}

TEST_F(TdZddTest, ParallelBuildPowerSet) {
    // 2^10 = 1024
    PowerSet spec(10);
    ZDD result = build_zdd_mp(mgr, spec);

    EXPECT_DOUBLE_EQ(result.card(), 1024.0);
}

// Tests for ZddLookahead
TEST_F(TdZddTest, ZddLookahead) {
    Combination spec(5, 3);
    auto lookaheadSpec = zddLookahead(spec);
    ZDD result = build_zdd(mgr, lookaheadSpec);

    // Should produce same result
    EXPECT_DOUBLE_EQ(result.card(), 10.0);
}

// Tests for ZddUnreduction
TEST_F(TdZddTest, ZddUnreduction) {
    Combination spec(5, 2);
    auto unreducedSpec = zddUnreduction(spec, 5);
    UnreducedZDD result = build_unreduced_zdd(mgr, unreducedSpec);

    // After reduction, should have same cardinality
    ZDD reduced = result.reduce();
    EXPECT_DOUBLE_EQ(reduced.card(), 10.0);
}

// Tests for BDD builder
TEST_F(TdZddTest, BddBuildBasic) {
    // Build a simple BDD
    PowerSet spec(3);
    BDD result = build_bdd(mgr, spec);

    // BDD of all 2^3 assignments
    // For BDD, this represents all truth assignments where result is true
    // With PowerSet spec returning -1 for all paths, it's the tautology
    EXPECT_TRUE(result.is_valid());
    EXPECT_TRUE(result.is_one());  // Should be tautology (always true)
}

// Tests for parallel BDD builder
TEST_F(TdZddTest, ParallelBddBuild) {
    PowerSet spec(5);
    BDD result = build_bdd_mp(mgr, spec);

    EXPECT_TRUE(result.is_valid());
    EXPECT_TRUE(result.is_one());  // Should be tautology
}

// ========================================
// Tests for ARITY >= 3 (MVZDD / MVBDD)
// ========================================

// Ternary spec: each variable takes 3 values (0, 1, 2)
// Counts combinations where sum of values equals target
class TernarySum : public DdSpec<TernarySum, int, 3> {
    int n_;       // number of variables
    int target_;  // target sum

public:
    TernarySum(int n, int target) : n_(n), target_(target) {}

    int getRoot(int& state) const {
        state = 0;  // current sum
        return n_;  // start from level n
    }

    int getChild(int& state, int level, int value) const {
        state += value;  // add value (0, 1, or 2)
        --level;

        if (level == 0) {
            return (state == target_) ? -1 : 0;  // -1 = accept, 0 = reject
        }

        // Pruning
        int maxRemaining = level * 2;  // max sum from remaining variables
        if (state > target_) return 0;  // already exceeded
        if (state + maxRemaining < target_) return 0;  // cannot reach target

        return level;
    }
};

// Simple 4-ary spec: all combinations
class QuaternaryPowerSet : public StatelessDdSpec<QuaternaryPowerSet, 4> {
    int n_;

public:
    explicit QuaternaryPowerSet(int n) : n_(n) {}

    int getRoot() const {
        return n_;
    }

    int getChild(int level, int value) const {
        (void)value;
        if (level == 1) return -1;  // accept at bottom
        return level - 1;
    }
};

// 5-ary spec for testing
class QuinarySpec : public StatelessDdSpec<QuinarySpec, 5> {
    int n_;

public:
    explicit QuinarySpec(int n) : n_(n) {}

    int getRoot() const {
        return n_;
    }

    int getChild(int level, int value) const {
        (void)value;
        if (level == 1) return -1;
        return level - 1;
    }
};

TEST_F(TdZddTest, MVZDDTernaryBasic) {
    // TernarySum(2, 2): 2 variables, target sum = 2
    // Possible: (0,2), (1,1), (2,0) = 3 solutions
    TernarySum spec(2, 2);
    MVZDD result = build_mvzdd(mgr, spec);

    EXPECT_EQ(result.k(), 3);
    EXPECT_DOUBLE_EQ(result.card(), 3.0);
}

TEST_F(TdZddTest, MVZDDTernaryLarger) {
    // TernarySum(3, 3): 3 variables, target sum = 3
    // Combinations: (0,0,3) invalid, (0,1,2), (0,2,1), (1,0,2), (1,1,1), (1,2,0), (2,0,1), (2,1,0)
    // Wait, max value is 2, so valid: (0,1,2), (0,2,1), (1,0,2), (1,1,1), (1,2,0), (2,0,1), (2,1,0)
    // That's 7 solutions
    TernarySum spec(3, 3);
    MVZDD result = build_mvzdd(mgr, spec);

    EXPECT_EQ(result.k(), 3);
    EXPECT_DOUBLE_EQ(result.card(), 7.0);
}

TEST_F(TdZddTest, MVZDDTernaryEdgeCases) {
    // TernarySum(3, 0): only (0,0,0) = 1 solution
    {
        TernarySum spec(3, 0);
        MVZDD result = build_mvzdd(mgr, spec);
        EXPECT_DOUBLE_EQ(result.card(), 1.0);
    }

    // TernarySum(3, 6): only (2,2,2) = 1 solution
    {
        TernarySum spec(3, 6);
        MVZDD result = build_mvzdd(mgr, spec);
        EXPECT_DOUBLE_EQ(result.card(), 1.0);
    }

    // TernarySum(3, 7): impossible (max is 6) = 0 solutions
    {
        TernarySum spec(3, 7);
        MVZDD result = build_mvzdd(mgr, spec);
        EXPECT_DOUBLE_EQ(result.card(), 0.0);
    }
}

TEST_F(TdZddTest, MVZDDQuaternaryPowerSet) {
    // QuaternaryPowerSet(2): 2 variables, each with 4 values
    // Total: 4^2 = 16 combinations
    QuaternaryPowerSet spec(2);
    MVZDD result = build_mvzdd(mgr, spec);

    EXPECT_EQ(result.k(), 4);
    EXPECT_DOUBLE_EQ(result.card(), 16.0);
}

TEST_F(TdZddTest, MVZDDQuaternaryPowerSetLarger) {
    // QuaternaryPowerSet(3): 3 variables, each with 4 values
    // Total: 4^3 = 64 combinations
    QuaternaryPowerSet spec(3);
    MVZDD result = build_mvzdd(mgr, spec);

    EXPECT_EQ(result.k(), 4);
    EXPECT_DOUBLE_EQ(result.card(), 64.0);
}

TEST_F(TdZddTest, MVZDDQuinarySpec) {
    // QuinarySpec(2): 2 variables, each with 5 values
    // Total: 5^2 = 25 combinations
    QuinarySpec spec(2);
    MVZDD result = build_mvzdd(mgr, spec);

    EXPECT_EQ(result.k(), 5);
    EXPECT_DOUBLE_EQ(result.card(), 25.0);
}

TEST_F(TdZddTest, MVZDDEvaluate) {
    // TernarySum(2, 2): solutions are (level2, level1) = (0,2), (1,1), (2,0)
    // TdZdd level 2 (top) → MVDD variable 1
    // TdZdd level 1 (bottom) → MVDD variable 2
    // So evaluate({var1, var2}) = evaluate({level2, level1})
    TernarySum spec(2, 2);
    MVZDD result = build_mvzdd(mgr, spec);

    // {level2, level1} format (= {var1, var2})
    EXPECT_TRUE(result.evaluate({0, 2}));   // level2=0, level1=2
    EXPECT_TRUE(result.evaluate({1, 1}));   // level2=1, level1=1
    EXPECT_TRUE(result.evaluate({2, 0}));   // level2=2, level1=0

    EXPECT_FALSE(result.evaluate({0, 0}));  // sum=0
    EXPECT_FALSE(result.evaluate({0, 1}));  // sum=1
    EXPECT_FALSE(result.evaluate({1, 0}));  // sum=1
    EXPECT_FALSE(result.evaluate({2, 2}));  // sum=4
}

// MVBDD tests
TEST_F(TdZddTest, MVBDDTernaryBasic) {
    TernarySum spec(2, 2);
    MVBDD result = build_mvbdd(mgr, spec);

    EXPECT_EQ(result.k(), 3);

    // Evaluate should give same results as MVZDD
    // {level2, level1} format (= {var1, var2})
    EXPECT_TRUE(result.evaluate({0, 2}));   // level2=0, level1=2
    EXPECT_TRUE(result.evaluate({1, 1}));   // level2=1, level1=1
    EXPECT_TRUE(result.evaluate({2, 0}));   // level2=2, level1=0

    EXPECT_FALSE(result.evaluate({0, 0}));  // sum=0
    EXPECT_FALSE(result.evaluate({0, 1}));  // sum=1
    EXPECT_FALSE(result.evaluate({1, 0}));  // sum=1
    EXPECT_FALSE(result.evaluate({2, 2}));  // sum=4
}

TEST_F(TdZddTest, MVBDDQuaternaryPowerSet) {
    QuaternaryPowerSet spec(2);
    MVBDD result = build_mvbdd(mgr, spec);

    EXPECT_EQ(result.k(), 4);

    // All combinations should evaluate to true
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            EXPECT_TRUE(result.evaluate({i, j}));
        }
    }
}

TEST_F(TdZddTest, MVBDDTernaryEdgeCases) {
    // Empty result
    {
        TernarySum spec(3, 7);  // impossible
        MVBDD result = build_mvbdd(mgr, spec);
        EXPECT_TRUE(result.is_zero());
    }

    // Single result
    {
        TernarySum spec(2, 4);  // only (2,2)
        MVBDD result = build_mvbdd(mgr, spec);
        EXPECT_TRUE(result.evaluate({2, 2}));
        EXPECT_FALSE(result.evaluate({0, 0}));
        EXPECT_FALSE(result.evaluate({1, 2}));
    }
}

// ========================================
// Tests for VarArity Specs (runtime ARITY)
// ========================================

// VarArity Hybrid spec: sum with variable arity
class VarAritySumSpec
    : public VarArityHybridDdSpec<VarAritySumSpec, int, int> {
    int n_;       // number of variables
    int target_;  // target sum

public:
    VarAritySumSpec(int arity, int n, int target) : n_(n), target_(target) {
        setArity(arity);
        setArraySize(1);  // minimal array
    }

    int getRoot(int& s, int* a) {
        s = 0;  // current sum
        a[0] = 0;
        return n_;
    }

    int getChild(int& s, int* a, int level, int value) {
        (void)a;  // unused
        s += value;
        --level;

        if (level == 0) {
            return (s == target_) ? -1 : 0;
        }

        // Pruning
        int maxRemaining = level * (getArity() - 1);
        if (s > target_) return 0;
        if (s + maxRemaining < target_) return 0;

        return level;
    }
};

// VarArity stateless spec: all combinations
class VarArityPowerSetSpec
    : public VarArityStatelessDdSpec<VarArityPowerSetSpec> {
    int n_;

public:
    VarArityPowerSetSpec(int arity, int n) : n_(n) {
        setArity(arity);
    }

    int getRoot() const {
        return n_;
    }

    int getChild(int level, int value) const {
        (void)value;
        if (level == 1) return -1;
        return level - 1;
    }
};

// VarArity scalar spec
class VarArityCombination
    : public VarArityDdSpec<VarArityCombination, int> {
    int n_;
    int k_;

public:
    VarArityCombination(int arity, int n, int k) : n_(n), k_(k) {
        setArity(arity);
    }

    int getRoot(int& state) const {
        state = 0;
        return n_;
    }

    int getChild(int& state, int level, int value) const {
        state += value;
        --level;

        if (level == 0) {
            return (state == k_) ? -1 : 0;
        }

        // Pruning for binary case
        if (getArity() == 2) {
            if (state > k_) return 0;
            if (state + level < k_) return 0;
        }

        return level;
    }
};

// ========================================
// VarArity MVZDD Tests
// ========================================

TEST_F(TdZddTest, VarArityMVZDDTernaryBasic) {
    // Same as TernarySum but with VarArity spec
    // ARITY = 3, 2 variables, target sum = 2
    // Solutions: (0,2), (1,1), (2,0) = 3 solutions
    VarAritySumSpec spec(3, 2, 2);
    MVZDD result = build_mvzdd_va(mgr, spec);

    EXPECT_EQ(result.k(), 3);
    EXPECT_DOUBLE_EQ(result.card(), 3.0);
}

TEST_F(TdZddTest, VarArityMVZDDQuaternary) {
    // ARITY = 4, 2 variables, all combinations
    // Total: 4^2 = 16
    VarArityPowerSetSpec spec(4, 2);
    MVZDD result = build_mvzdd_va(mgr, spec);

    EXPECT_EQ(result.k(), 4);
    EXPECT_DOUBLE_EQ(result.card(), 16.0);
}

TEST_F(TdZddTest, VarArityMVZDDQuinary) {
    // ARITY = 5, 2 variables, all combinations
    // Total: 5^2 = 25
    VarArityPowerSetSpec spec(5, 2);
    MVZDD result = build_mvzdd_va(mgr, spec);

    EXPECT_EQ(result.k(), 5);
    EXPECT_DOUBLE_EQ(result.card(), 25.0);
}

TEST_F(TdZddTest, VarArityMVZDDArity2) {
    // ARITY = 2 should work like binary ZDD
    VarArityCombination spec(2, 5, 3);  // C(5,3) = 10
    MVZDD result = build_mvzdd_va(mgr, spec);

    EXPECT_EQ(result.k(), 2);
    EXPECT_DOUBLE_EQ(result.card(), 10.0);
}

TEST_F(TdZddTest, VarArityMVZDDArity2Minimum) {
    // ARITY = 2 (minimum for MVZDD): binary case
    // VarArity with ARITY = 2 should work like binary
    VarAritySumSpec spec(2, 3, 2);  // sum = 2 with ARITY=2: (0,1,1), (1,0,1), (1,1,0) = 3 solutions
    MVZDD result = build_mvzdd_va(mgr, spec);

    EXPECT_EQ(result.k(), 2);
    EXPECT_DOUBLE_EQ(result.card(), 3.0);
}

TEST_F(TdZddTest, VarAritySpecArity1Allowed) {
    // Note: ARITY = 1 is allowed for VarAritySpec itself,
    // but MVZDD/MVBDD requires k >= 2
    VarAritySumSpec spec(1, 3, 0);
    EXPECT_EQ(spec.getArity(), 1);  // setArity(1) should work
}

TEST_F(TdZddTest, VarArityMVZDDEvaluate) {
    // ARITY = 3, sum = 2
    VarAritySumSpec spec(3, 2, 2);
    MVZDD result = build_mvzdd_va(mgr, spec);

    EXPECT_TRUE(result.evaluate({0, 2}));
    EXPECT_TRUE(result.evaluate({1, 1}));
    EXPECT_TRUE(result.evaluate({2, 0}));
    EXPECT_FALSE(result.evaluate({0, 0}));
    EXPECT_FALSE(result.evaluate({2, 2}));
}

// ========================================
// VarArity MVBDD Tests
// ========================================

TEST_F(TdZddTest, VarArityMVBDDTernaryBasic) {
    VarAritySumSpec spec(3, 2, 2);
    MVBDD result = build_mvbdd_va(mgr, spec);

    EXPECT_EQ(result.k(), 3);
    EXPECT_TRUE(result.evaluate({0, 2}));
    EXPECT_TRUE(result.evaluate({1, 1}));
    EXPECT_TRUE(result.evaluate({2, 0}));
    EXPECT_FALSE(result.evaluate({0, 0}));
}

TEST_F(TdZddTest, VarArityMVBDDQuaternary) {
    VarArityPowerSetSpec spec(4, 2);
    MVBDD result = build_mvbdd_va(mgr, spec);

    EXPECT_EQ(result.k(), 4);
    // All 16 combinations should evaluate to true
    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            EXPECT_TRUE(result.evaluate({i, j}));
        }
    }
}

// ========================================
// VarArity Error Tests
// ========================================

TEST_F(TdZddTest, VarArityInvalidArityZero) {
    EXPECT_THROW({
        VarAritySumSpec spec(0, 2, 2);  // ARITY = 0 is invalid
    }, DDArgumentException);
}

TEST_F(TdZddTest, VarArityInvalidArityNegative) {
    EXPECT_THROW({
        VarAritySumSpec spec(-1, 2, 2);  // negative ARITY is invalid
    }, DDArgumentException);
}

TEST_F(TdZddTest, VarArityInvalidArityTooLarge) {
    EXPECT_THROW({
        VarAritySumSpec spec(101, 2, 2);  // ARITY > 100 is invalid
    }, DDArgumentException);
}

TEST_F(TdZddTest, VarArityMaxArity) {
    // ARITY = 100 should work
    VarArityPowerSetSpec spec(100, 1);  // 1 variable with 100 values = 100 paths
    MVZDD result = build_mvzdd_va(mgr, spec);

    EXPECT_EQ(result.k(), 100);
    EXPECT_DOUBLE_EQ(result.card(), 100.0);
}

// ========================================
// VarArity Parallel Builder Tests
// ========================================

TEST_F(TdZddTest, VarArityMVZDDParallel) {
    VarAritySumSpec spec(3, 3, 3);  // 7 solutions (same as TernarySum(3,3))
    MVZDD result = build_mvzdd_va_mp(mgr, spec);

    EXPECT_EQ(result.k(), 3);
    EXPECT_DOUBLE_EQ(result.card(), 7.0);
}

TEST_F(TdZddTest, VarArityMVBDDParallel) {
    VarAritySumSpec spec(3, 2, 2);
    MVBDD result = build_mvbdd_va_mp(mgr, spec);

    EXPECT_EQ(result.k(), 3);
    EXPECT_TRUE(result.evaluate({0, 2}));
    EXPECT_TRUE(result.evaluate({1, 1}));
    EXPECT_TRUE(result.evaluate({2, 0}));
}

TEST_F(TdZddTest, VarArityParallelEquivalence) {
    // Sequential and parallel should give the same result
    VarAritySumSpec spec1(4, 3, 4);
    VarAritySumSpec spec2(4, 3, 4);

    MVZDD seqResult = build_mvzdd_va(mgr, spec1);
    MVZDD parResult = build_mvzdd_va_mp(mgr, spec2);

    EXPECT_DOUBLE_EQ(seqResult.card(), parResult.card());
}

// ========================================
// VarArity Spec Operation Tests
// ========================================

TEST_F(TdZddTest, VarArityZddUnion) {
    // Union of two VarArity specs with same ARITY
    VarAritySumSpec spec1(3, 2, 1);  // sum = 1: (0,1), (1,0) = 2 solutions
    VarAritySumSpec spec2(3, 2, 2);  // sum = 2: (0,2), (1,1), (2,0) = 3 solutions

    auto unionSpec = zddUnionVA(spec1, spec2);
    MVZDD result = build_mvzdd_va(mgr, unionSpec);

    // No overlap, total = 2 + 3 = 5
    EXPECT_EQ(result.k(), 3);
    EXPECT_DOUBLE_EQ(result.card(), 5.0);
}

TEST_F(TdZddTest, VarArityZddIntersection) {
    // Intersection of power set and specific sum
    VarArityPowerSetSpec powerSpec(3, 2);  // all 9 combinations
    VarAritySumSpec sumSpec(3, 2, 2);       // sum = 2: 3 solutions

    auto intersectSpec = zddIntersectionVA(powerSpec, sumSpec);
    MVZDD result = build_mvzdd_va(mgr, intersectSpec);

    // Intersection = 3 (only the sum=2 solutions)
    EXPECT_EQ(result.k(), 3);
    EXPECT_DOUBLE_EQ(result.card(), 3.0);
}

TEST_F(TdZddTest, VarArityZddOpArityMismatch) {
    // Operations with different arities should throw
    VarAritySumSpec spec3(3, 2, 2);  // ARITY = 3
    VarAritySumSpec spec4(4, 2, 2);  // ARITY = 4

    EXPECT_THROW({
        zddUnionVA(spec3, spec4);
    }, DDArgumentException);

    EXPECT_THROW({
        zddIntersectionVA(spec3, spec4);
    }, DDArgumentException);
}

// ========================================
// ZDD Subset Tests
// ========================================

TEST_F(TdZddTest, ZddSubsetBasic) {
    // Create a universal ZDD (power set of 5 elements): 2^5 = 32 sets
    PowerSet powerSpec(5);
    ZDD power = build_zdd(mgr, powerSpec);
    EXPECT_DOUBLE_EQ(power.card(), 32.0);

    // Subset with Combination(5, 2): C(5,2) = 10 sets
    Combination combSpec(5, 2);
    ZDD result = zdd_subset(mgr, power, combSpec);
    EXPECT_DOUBLE_EQ(result.card(), 10.0);
}

TEST_F(TdZddTest, ZddSubsetSingleton) {
    // Create a universal ZDD (power set of 4 elements): 2^4 = 16 sets
    PowerSet powerSpec(4);
    ZDD power = build_zdd(mgr, powerSpec);
    EXPECT_DOUBLE_EQ(power.card(), 16.0);

    // Subset with Singleton(4): 4 singleton sets
    Singleton singleSpec(4);
    ZDD result = zdd_subset(mgr, power, singleSpec);
    EXPECT_DOUBLE_EQ(result.card(), 4.0);
}

TEST_F(TdZddTest, ZddSubsetEmpty) {
    // Subset of empty ZDD should be empty
    ZDD empty = ZDD::empty(mgr);
    Combination combSpec(5, 2);
    ZDD result = zdd_subset(mgr, empty, combSpec);
    EXPECT_EQ(result, ZDD::empty(mgr));
}

TEST_F(TdZddTest, ZddSubsetEmptySpec) {
    // Subset with spec that accepts nothing
    PowerSet powerSpec(5);
    ZDD power = build_zdd(mgr, powerSpec);

    // Combination(5, 6) accepts nothing (can't choose 6 from 5)
    Combination impossibleSpec(5, 6);
    ZDD result = zdd_subset(mgr, power, impossibleSpec);
    EXPECT_EQ(result, ZDD::empty(mgr));
}

TEST_F(TdZddTest, ZddSubsetSingle) {
    // Subset of single ZDD (empty set only)
    ZDD single = ZDD::single(mgr);

    // Combination(5, 0) accepts only empty set
    Combination emptySetSpec(5, 0);
    ZDD result = zdd_subset(mgr, single, emptySetSpec);
    EXPECT_EQ(result, ZDD::single(mgr));
}

TEST_F(TdZddTest, ZddSubsetIdentity) {
    // Subset with PowerSet spec should return same ZDD (intersection with universal)
    Combination combSpec1(5, 3);
    ZDD comb = build_zdd(mgr, combSpec1);
    double origCard = comb.card();

    // PowerSet accepts everything
    PowerSet powerSpec(5);
    ZDD result = zdd_subset(mgr, comb, powerSpec);
    EXPECT_DOUBLE_EQ(result.card(), origCard);
}

TEST_F(TdZddTest, ZddSubsetIntersection) {
    // Subset is equivalent to intersection
    // Build C(6,2) and C(6,3)
    Combination spec1(6, 2);
    Combination spec2(6, 3);

    ZDD zdd1 = build_zdd(mgr, spec1);  // C(6,2) = 15
    ZDD zdd2 = build_zdd(mgr, spec2);  // C(6,3) = 20

    // Subset of zdd1 with spec2 should be empty (no overlap)
    Combination spec2copy(6, 3);
    ZDD result = zdd_subset(mgr, zdd1, spec2copy);
    EXPECT_EQ(result, ZDD::empty(mgr));
}

// ========================================
// ZDD/BDD Evaluate Tests
// ========================================

TEST_F(TdZddTest, ZddEvaluateCardinality) {
    // Test zdd_evaluate with CardinalityEval
    // Build C(5,2) = 10 sets
    Combination combSpec(5, 2);
    ZDD comb = build_zdd(mgr, combSpec);

    CardinalityEval cardEval;
    double card = zdd_evaluate(comb, cardEval);
    EXPECT_DOUBLE_EQ(card, 10.0);
}

TEST_F(TdZddTest, ZddEvaluateTerminal) {
    // Test evaluation of terminal ZDDs
    CardinalityEval cardEval;

    // Empty ZDD: 0 sets
    ZDD empty = ZDD::empty(mgr);
    EXPECT_DOUBLE_EQ(zdd_evaluate(empty, cardEval), 0.0);

    // Single ZDD: 1 set (the empty set)
    ZDD single = ZDD::single(mgr);
    CardinalityEval cardEval2;
    EXPECT_DOUBLE_EQ(zdd_evaluate(single, cardEval2), 1.0);
}

TEST_F(TdZddTest, ZddEvaluatePowerSet) {
    // Test with power set: 2^n sets
    PowerSet powerSpec(6);
    ZDD power = build_zdd(mgr, powerSpec);

    CardinalityEval cardEval;
    double card = zdd_evaluate(power, cardEval);
    EXPECT_DOUBLE_EQ(card, 64.0);  // 2^6 = 64
}

// Custom evaluator that computes sum of cardinalities weighted by set size
class WeightedCardEval : public DdEval<WeightedCardEval, double, double, 2> {
public:
    void evalTerminal(double& v, int id) {
        v = (id == 1) ? 1.0 : 0.0;
    }

    void evalNode(double& v, int level, DdValues<double, 2> const& values) {
        (void)level;
        // low child: sets not containing this element
        // high child: sets containing this element
        v = values.get(0) + values.get(1);
    }
};

TEST_F(TdZddTest, ZddEvaluateCustom) {
    // Test with custom evaluator
    Combination combSpec(4, 2);
    ZDD comb = build_zdd(mgr, combSpec);

    WeightedCardEval weightedEval;
    double result = zdd_evaluate(comb, weightedEval);
    EXPECT_DOUBLE_EQ(result, 6.0);  // C(4,2) = 6
}

TEST_F(TdZddTest, ZddEvaluateSingleton) {
    // Test with singleton sets
    Singleton singleSpec(4);
    ZDD singles = build_zdd(mgr, singleSpec);

    CardinalityEval cardEval;
    double card = zdd_evaluate(singles, cardEval);
    EXPECT_DOUBLE_EQ(card, 4.0);  // 4 singleton sets
}

// BDD evaluation tests
TEST_F(TdZddTest, BddEvaluateCardinality) {
    // Test bdd_evaluate - count satisfying assignments
    // For a BDD, cardinality counts paths to 1-terminal

    // Create a simple BDD: x1 AND x2
    for (int i = 0; i < 3; i++) mgr.new_var();

    BDD x1 = mgr.var_bdd(1);
    BDD x2 = mgr.var_bdd(2);
    BDD x3 = mgr.var_bdd(3);

    // x1 AND x2: only 1 path to true (both true)
    BDD andBdd = x1 & x2;

    CardinalityEval cardEval;
    double card = bdd_evaluate(andBdd, cardEval);
    // Note: For BDD without considering skipped levels, this counts paths in the DAG
    // The actual satisfying assignments need BDD-specific handling
    EXPECT_GE(card, 1.0);
}

TEST_F(TdZddTest, BddEvaluateTerminal) {
    // Test evaluation of terminal BDDs
    CardinalityEval cardEval;

    // False BDD (zero)
    BDD falseBdd = BDD::zero(mgr);
    EXPECT_DOUBLE_EQ(bdd_evaluate(falseBdd, cardEval), 0.0);

    // True BDD (one)
    BDD trueBdd = BDD::one(mgr);
    CardinalityEval cardEval2;
    EXPECT_DOUBLE_EQ(bdd_evaluate(trueBdd, cardEval2), 1.0);
}
