// TdZdd integration tests
#include <gtest/gtest.h>

#include "sbdd2/dd_manager.hpp"
#include "sbdd2/zdd.hpp"
#include "sbdd2/bdd.hpp"
#include "sbdd2/tdzdd/DdSpec.hpp"
#include "sbdd2/tdzdd/Sbdd2Builder.hpp"

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
