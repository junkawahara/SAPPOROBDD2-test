// SAPPOROBDD 2.0 - MVZDD tests
// MIT License

#include <gtest/gtest.h>
#include "sbdd2/sbdd2.hpp"
#include "sbdd2/mvdd_base.hpp"
#include "sbdd2/mvzdd.hpp"

using namespace sbdd2;

class MVZDDTest : public ::testing::Test {
protected:
    DDManager mgr;
    int k = 4;  // Test with k=4
};

// --- Basic Factory Tests ---

TEST_F(MVZDDTest, Empty) {
    MVZDD f = MVZDD::empty(mgr, k);
    EXPECT_TRUE(f.is_empty());
    EXPECT_EQ(f.card(), 0.0);
    EXPECT_EQ(f.k(), k);
}

TEST_F(MVZDDTest, Base) {
    MVZDD f = MVZDD::single(mgr, k);
    EXPECT_TRUE(f.is_base());
    EXPECT_EQ(f.card(), 1.0);
}

// --- Variable Management Tests ---

TEST_F(MVZDDTest, NewVar) {
    MVZDD f = MVZDD::empty(mgr, k);
    bddvar mv1 = f.new_var();
    bddvar mv2 = f.new_var();

    EXPECT_EQ(mv1, 1u);
    EXPECT_EQ(mv2, 2u);
    EXPECT_EQ(f.mvdd_var_count(), 2u);

    // Check that k-1 internal DD variables are created per MVDD variable
    EXPECT_EQ(f.dd_vars_of(1).size(), static_cast<size_t>(k - 1));
    EXPECT_EQ(f.dd_vars_of(2).size(), static_cast<size_t>(k - 1));
}

// --- Single Element Tests ---

TEST_F(MVZDDTest, SingleValue0) {
    MVZDD f = MVZDD::empty(mgr, k);
    f.new_var();

    MVZDD s = MVZDD::single(f, 1, 0);
    EXPECT_EQ(s.card(), 1.0);

    // Evaluation tests
    EXPECT_TRUE(s.evaluate({0}));
    EXPECT_FALSE(s.evaluate({1}));
    EXPECT_FALSE(s.evaluate({2}));
    EXPECT_FALSE(s.evaluate({3}));
}

TEST_F(MVZDDTest, SingleValue1) {
    MVZDD f = MVZDD::empty(mgr, k);
    f.new_var();

    MVZDD s = MVZDD::single(f, 1, 1);
    EXPECT_EQ(s.card(), 1.0);

    EXPECT_FALSE(s.evaluate({0}));
    EXPECT_TRUE(s.evaluate({1}));
    EXPECT_FALSE(s.evaluate({2}));
    EXPECT_FALSE(s.evaluate({3}));
}

TEST_F(MVZDDTest, SingleValue2) {
    MVZDD f = MVZDD::empty(mgr, k);
    f.new_var();

    MVZDD s = MVZDD::single(f, 1, 2);
    EXPECT_EQ(s.card(), 1.0);

    EXPECT_FALSE(s.evaluate({0}));
    EXPECT_FALSE(s.evaluate({1}));
    EXPECT_TRUE(s.evaluate({2}));
    EXPECT_FALSE(s.evaluate({3}));
}

TEST_F(MVZDDTest, SingleValueK1) {
    MVZDD f = MVZDD::empty(mgr, k);
    f.new_var();

    MVZDD s = MVZDD::single(f, 1, k - 1);
    EXPECT_EQ(s.card(), 1.0);

    for (int i = 0; i < k; ++i) {
        EXPECT_EQ(s.evaluate({i}), i == k - 1);
    }
}

// --- ITE Construction Tests ---

TEST_F(MVZDDTest, IteAllBase) {
    MVZDD f = MVZDD::empty(mgr, k);
    f.new_var();

    MVZDD base = MVZDD::single(mgr, k);
    base = MVZDD(base.manager(), f.var_table(), base.to_zdd());

    // All children are base -> all assignments should be true
    std::vector<MVZDD> children = {base, base, base, base};
    MVZDD result = MVZDD::ite(f, 1, children);

    for (int i = 0; i < k; ++i) {
        EXPECT_TRUE(result.evaluate({i}));
    }
}

TEST_F(MVZDDTest, IteSelectiveChildren) {
    MVZDD f = MVZDD::empty(mgr, k);
    f.new_var();

    MVZDD base = MVZDD::single(mgr, k);
    base = MVZDD(base.manager(), f.var_table(), base.to_zdd());
    MVZDD empty = MVZDD(f.manager(), f.var_table(), ZDD::empty(mgr));

    // Only value=1 should be in the family
    std::vector<MVZDD> children = {empty, base, empty, empty};
    MVZDD result = MVZDD::ite(f, 1, children);

    EXPECT_FALSE(result.evaluate({0}));
    EXPECT_TRUE(result.evaluate({1}));
    EXPECT_FALSE(result.evaluate({2}));
    EXPECT_FALSE(result.evaluate({3}));
}

// --- Union Tests ---

TEST_F(MVZDDTest, Union) {
    MVZDD f = MVZDD::empty(mgr, k);
    f.new_var();

    MVZDD s1 = MVZDD::single(f, 1, 1);
    MVZDD s2 = MVZDD::single(f, 1, 2);

    MVZDD u = s1 + s2;
    EXPECT_EQ(u.card(), 2.0);

    EXPECT_FALSE(u.evaluate({0}));
    EXPECT_TRUE(u.evaluate({1}));
    EXPECT_TRUE(u.evaluate({2}));
    EXPECT_FALSE(u.evaluate({3}));
}

TEST_F(MVZDDTest, UnionWithEmpty) {
    MVZDD f = MVZDD::empty(mgr, k);
    f.new_var();

    MVZDD s = MVZDD::single(f, 1, 1);
    MVZDD empty = MVZDD(f.manager(), f.var_table(), ZDD::empty(mgr));

    MVZDD u = s + empty;
    EXPECT_EQ(u, s);
}

// --- Intersection Tests ---

TEST_F(MVZDDTest, IntersectionDisjoint) {
    MVZDD f = MVZDD::empty(mgr, k);
    f.new_var();

    MVZDD s1 = MVZDD::single(f, 1, 1);
    MVZDD s2 = MVZDD::single(f, 1, 2);

    // Intersection of disjoint sets is empty
    MVZDD i = s1 * s2;
    EXPECT_TRUE(i.is_empty());
}

TEST_F(MVZDDTest, IntersectionSame) {
    MVZDD f = MVZDD::empty(mgr, k);
    f.new_var();

    MVZDD s1 = MVZDD::single(f, 1, 1);
    MVZDD s2 = MVZDD::single(f, 1, 1);

    MVZDD i = s1 * s2;
    EXPECT_EQ(i, s1);
}

// --- Difference Tests ---

TEST_F(MVZDDTest, Difference) {
    MVZDD f = MVZDD::empty(mgr, k);
    f.new_var();

    MVZDD s1 = MVZDD::single(f, 1, 1);
    MVZDD s2 = MVZDD::single(f, 1, 2);
    MVZDD u = s1 + s2;

    MVZDD d = u - s1;
    EXPECT_EQ(d, s2);
}

// --- Child Access Tests ---

TEST_F(MVZDDTest, ChildAccess) {
    MVZDD f = MVZDD::empty(mgr, k);
    f.new_var();

    MVZDD base = MVZDD::single(mgr, k);
    base = MVZDD(base.manager(), f.var_table(), base.to_zdd());
    MVZDD empty = MVZDD(f.manager(), f.var_table(), ZDD::empty(mgr));

    std::vector<MVZDD> children = {empty, base, empty, base};
    MVZDD result = MVZDD::ite(f, 1, children);

    EXPECT_TRUE(result.child(0).is_empty());
    EXPECT_TRUE(result.child(1).is_base());
    EXPECT_TRUE(result.child(2).is_empty());
    EXPECT_TRUE(result.child(3).is_base());
}

// --- all_sat Tests ---

TEST_F(MVZDDTest, AllSat) {
    MVZDD f = MVZDD::empty(mgr, k);
    f.new_var();

    MVZDD s1 = MVZDD::single(f, 1, 1);
    MVZDD s2 = MVZDD::single(f, 1, 2);
    MVZDD u = s1 + s2;

    auto sats = u.all_sat();
    EXPECT_EQ(sats.size(), 2u);

    // Check that both {1} and {2} are present
    bool has_1 = false, has_2 = false;
    for (const auto& sat : sats) {
        EXPECT_EQ(sat.size(), 1u);
        if (sat[0] == 1) has_1 = true;
        if (sat[0] == 2) has_2 = true;
    }
    EXPECT_TRUE(has_1);
    EXPECT_TRUE(has_2);
}

// --- Multiple Variables Tests ---

TEST_F(MVZDDTest, MultipleVariablesUnion) {
    MVZDD f = MVZDD::empty(mgr, k);
    f.new_var();
    f.new_var();

    MVZDD s1 = MVZDD::single(f, 1, 1);
    MVZDD s2 = MVZDD::single(f, 2, 2);

    // Union of single elements for different variables
    MVZDD u = s1 + s2;
    EXPECT_EQ(u.card(), 2.0);
}

// --- Conversion Tests ---

TEST_F(MVZDDTest, ToFromZDD) {
    MVZDD f = MVZDD::empty(mgr, k);
    f.new_var();

    MVZDD s = MVZDD::single(f, 1, 2);
    ZDD z = s.to_zdd();

    MVZDD s2 = MVZDD::from_zdd(f, z);
    EXPECT_EQ(s, s2);
}

// --- Node Count Tests ---

TEST_F(MVZDDTest, NodeCount) {
    MVZDD f = MVZDD::empty(mgr, k);
    f.new_var();

    MVZDD s = MVZDD::single(f, 1, 2);

    // Internal ZDD node count
    EXPECT_GE(s.size(), 1u);

    // MVZDD node count
    EXPECT_GE(s.mvzdd_node_count(), 1u);
}

// --- Error Handling Tests ---

TEST_F(MVZDDTest, InvalidValue) {
    MVZDD f = MVZDD::empty(mgr, k);
    f.new_var();

    EXPECT_THROW(MVZDD::single(f, 1, -1), DDArgumentException);
    EXPECT_THROW(MVZDD::single(f, 1, k), DDArgumentException);
}

TEST_F(MVZDDTest, InvalidVariable) {
    MVZDD f = MVZDD::empty(mgr, k);
    f.new_var();

    EXPECT_THROW(MVZDD::single(f, 0, 1), DDArgumentException);
    EXPECT_THROW(MVZDD::single(f, 2, 1), DDArgumentException);  // Only 1 var created
}

// --- Different k values ---

TEST_F(MVZDDTest, DifferentK) {
    MVZDD f2 = MVZDD::empty(mgr, 2);  // Binary
    MVZDD f3 = MVZDD::empty(mgr, 3);  // Ternary
    MVZDD f5 = MVZDD::empty(mgr, 5);  // 5-valued

    EXPECT_EQ(f2.k(), 2);
    EXPECT_EQ(f3.k(), 3);
    EXPECT_EQ(f5.k(), 5);

    f2.new_var();
    f3.new_var();
    f5.new_var();

    EXPECT_EQ(f2.dd_vars_of(1).size(), 1u);  // k-1 = 1
    EXPECT_EQ(f3.dd_vars_of(1).size(), 2u);  // k-1 = 2
    EXPECT_EQ(f5.dd_vars_of(1).size(), 4u);  // k-1 = 4
}

// --- Compound Assignment Tests ---

TEST_F(MVZDDTest, CompoundAssignment) {
    MVZDD f = MVZDD::empty(mgr, k);
    f.new_var();

    MVZDD s1 = MVZDD::single(f, 1, 1);
    MVZDD s2 = MVZDD::single(f, 1, 2);

    MVZDD u = s1;
    u += s2;
    EXPECT_EQ(u.card(), 2.0);

    MVZDD d = u;
    d -= s1;
    EXPECT_EQ(d, s2);
}
