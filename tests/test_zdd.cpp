// SAPPOROBDD 2.0 - ZDD tests
// MIT License

#include <gtest/gtest.h>
#include "sbdd2/sbdd2.hpp"
#include <algorithm>

using namespace sbdd2;

class ZDDTest : public ::testing::Test {
protected:
    DDManager mgr;

    void SetUp() override {
        // Create some variables
        for (int i = 0; i < 5; ++i) {
            mgr.new_var();
        }
    }
};

TEST_F(ZDDTest, Terminals) {
    ZDD empty = ZDD::empty(mgr);
    ZDD base = ZDD::base(mgr);

    EXPECT_TRUE(empty.is_zero());
    EXPECT_TRUE(base.is_one());
    EXPECT_EQ(empty.card(), 0.0);
    EXPECT_EQ(base.card(), 1.0);
}

TEST_F(ZDDTest, SingleElement) {
    ZDD s1 = ZDD::single(mgr, 1);
    ZDD s2 = ZDD::single(mgr, 2);

    EXPECT_EQ(s1.card(), 1.0);
    EXPECT_EQ(s2.card(), 1.0);
    EXPECT_EQ(s1.top(), 1u);
    EXPECT_EQ(s2.top(), 2u);
}

TEST_F(ZDDTest, UnionOperation) {
    ZDD s1 = ZDD::single(mgr, 1);
    ZDD s2 = ZDD::single(mgr, 2);
    ZDD empty = ZDD::empty(mgr);

    // {1} + {2} = {{1}, {2}}
    ZDD u = s1 + s2;
    EXPECT_EQ(u.card(), 2.0);

    // {} + s1 = s1
    EXPECT_EQ(empty + s1, s1);

    // s1 + s1 = s1
    EXPECT_EQ(s1 + s1, s1);
}

TEST_F(ZDDTest, IntersectionOperation) {
    ZDD s1 = ZDD::single(mgr, 1);
    ZDD s2 = ZDD::single(mgr, 2);
    ZDD empty = ZDD::empty(mgr);

    // {1} * {2} = {} (empty)
    ZDD i = s1 * s2;
    EXPECT_TRUE(i.is_zero());

    // s1 * s1 = s1
    EXPECT_EQ(s1 * s1, s1);

    // {} * s1 = {}
    EXPECT_TRUE((empty * s1).is_zero());
}

TEST_F(ZDDTest, DifferenceOperation) {
    ZDD s1 = ZDD::single(mgr, 1);
    ZDD s2 = ZDD::single(mgr, 2);

    // {1} - {2} = {1}
    EXPECT_EQ(s1 - s2, s1);

    // s1 - s1 = {}
    EXPECT_TRUE((s1 - s1).is_zero());

    // Union then difference
    ZDD u = s1 + s2;  // {{1}, {2}}
    ZDD d = u - s1;   // {{2}}
    EXPECT_EQ(d, s2);
}

TEST_F(ZDDTest, OnsetOffset) {
    ZDD s1 = ZDD::single(mgr, 1);
    ZDD s2 = ZDD::single(mgr, 2);
    ZDD u = s1 + s2;  // {{1}, {2}}

    // onset(1) on {{1}, {2}} = {{}} (base)
    ZDD on1 = u.onset(1);
    EXPECT_TRUE(on1.is_one());

    // offset(1) on {{1}, {2}} = {{2}}
    ZDD off1 = u.offset(1);
    EXPECT_EQ(off1, s2);

    // onset(2) on {{1}, {2}} = {{}}
    ZDD on2 = u.onset(2);
    EXPECT_TRUE(on2.is_one());

    // offset(2) on {{1}, {2}} = {{1}}
    ZDD off2 = u.offset(2);
    EXPECT_EQ(off2, s1);
}

TEST_F(ZDDTest, Change) {
    ZDD base = ZDD::base(mgr);  // {{}}

    // Toggle 1 on {{}} = {{1}}
    ZDD c = base.change(1);
    EXPECT_EQ(c, ZDD::single(mgr, 1));

    // Toggle 1 again = {{}}
    EXPECT_EQ(c.change(1), base);
}

TEST_F(ZDDTest, Product) {
    ZDD s1 = ZDD::single(mgr, 1);
    ZDD s2 = ZDD::single(mgr, 2);
    ZDD base = ZDD::base(mgr);

    // {1} * base = {1} (cross product with empty set)
    EXPECT_EQ(s1.product(base), s1);

    // base * {1} = {1}
    EXPECT_EQ(base.product(s1), s1);

    // {1} product {2} = {{1,2}}
    ZDD p = s1.product(s2);
    EXPECT_EQ(p.card(), 1.0);

    // Enumerate to verify
    auto sets = p.enumerate();
    ASSERT_EQ(sets.size(), 1u);
    EXPECT_EQ(sets[0].size(), 2u);
}

TEST_F(ZDDTest, Enumerate) {
    ZDD s1 = ZDD::single(mgr, 1);
    ZDD s2 = ZDD::single(mgr, 2);
    ZDD u = s1 + s2;  // {{1}, {2}}

    auto sets = u.enumerate();
    ASSERT_EQ(sets.size(), 2u);

    // Check that we have {1} and {2}
    bool found1 = false, found2 = false;
    for (const auto& s : sets) {
        if (s.size() == 1) {
            if (s[0] == 1) found1 = true;
            if (s[0] == 2) found2 = true;
        }
    }
    EXPECT_TRUE(found1);
    EXPECT_TRUE(found2);
}

TEST_F(ZDDTest, OneSet) {
    ZDD s1 = ZDD::single(mgr, 1);
    ZDD s2 = ZDD::single(mgr, 2);
    ZDD u = s1 + s2;

    auto oneSet = u.one_set();
    ASSERT_EQ(oneSet.size(), 1u);
    EXPECT_TRUE(oneSet[0] == 1 || oneSet[0] == 2);
}

TEST_F(ZDDTest, Card) {
    ZDD s1 = ZDD::single(mgr, 1);
    ZDD s2 = ZDD::single(mgr, 2);
    ZDD s3 = ZDD::single(mgr, 3);

    ZDD u = s1 + s2 + s3;  // {{1}, {2}, {3}}
    EXPECT_EQ(u.card(), 3.0);

    // Product creates combinations
    ZDD p12 = s1.product(s2);  // {{1,2}}
    EXPECT_EQ(p12.card(), 1.0);

    // Union of different sized sets
    ZDD combined = u + p12;  // {{1}, {2}, {3}, {1,2}}
    EXPECT_EQ(combined.card(), 4.0);
}

TEST_F(ZDDTest, Size) {
    ZDD s1 = ZDD::single(mgr, 1);
    EXPECT_EQ(s1.size(), 1u);

    ZDD s2 = ZDD::single(mgr, 2);
    ZDD u = s1 + s2;
    EXPECT_GE(u.size(), 1u);
}

TEST_F(ZDDTest, Support) {
    ZDD s1 = ZDD::single(mgr, 1);
    ZDD s2 = ZDD::single(mgr, 2);
    ZDD p = s1.product(s2);  // {{1,2}}

    auto supp = p.support();
    EXPECT_EQ(supp.size(), 2u);
    EXPECT_NE(std::find(supp.begin(), supp.end(), 1), supp.end());
    EXPECT_NE(std::find(supp.begin(), supp.end(), 2), supp.end());
}

TEST_F(ZDDTest, LowHigh) {
    ZDD s1 = ZDD::single(mgr, 1);

    // Low should be empty (no sets without element 1)
    EXPECT_TRUE(s1.low().is_zero());

    // High should be base (empty set after removing element 1)
    EXPECT_TRUE(s1.high().is_one());
}

TEST_F(ZDDTest, ComplexFamily) {
    // Create a family representing power set of {1,2}
    // P({1,2}) = {{}, {1}, {2}, {1,2}}

    ZDD base = ZDD::base(mgr);          // {{}}
    ZDD s1 = ZDD::single(mgr, 1);       // {{1}}
    ZDD s2 = ZDD::single(mgr, 2);       // {{2}}
    ZDD s12 = s1.product(s2);           // {{1,2}}

    ZDD powerset = base + s1 + s2 + s12;
    EXPECT_EQ(powerset.card(), 4.0);

    auto sets = powerset.enumerate();
    EXPECT_EQ(sets.size(), 4u);
}
