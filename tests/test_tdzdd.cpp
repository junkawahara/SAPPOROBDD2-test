// TdZdd integration tests
#include <gtest/gtest.h>

#include "sbdd2/dd_manager.hpp"
#include "sbdd2/zdd.hpp"
#include "sbdd2/bdd.hpp"
#include "sbdd2/tdzdd/DdSpec.hpp"
#include "sbdd2/tdzdd/Sbdd2Builder.hpp"
#include "sbdd2/tdzdd/Sbdd2BuilderMP.hpp"
#include "sbdd2/tdzdd/DdSpecOp.hpp"
#include "sbdd2/tdzdd/DdEval.hpp"

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
