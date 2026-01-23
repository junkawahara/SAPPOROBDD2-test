// SAPPOROBDD 2.0 - MVBDD tests
// MIT License

#include <gtest/gtest.h>
#include "sbdd2/sbdd2.hpp"
#include "sbdd2/mvdd_base.hpp"
#include "sbdd2/mvbdd.hpp"

using namespace sbdd2;

class MVBDDTest : public ::testing::Test {
protected:
    DDManager mgr;
    int k = 4;  // Test with k=4
};

// --- Basic Factory Tests ---

TEST_F(MVBDDTest, Zero) {
    MVBDD f = MVBDD::zero(mgr, k);
    EXPECT_TRUE(f.is_zero());
    EXPECT_EQ(f.k(), k);
}

TEST_F(MVBDDTest, One) {
    MVBDD f = MVBDD::one(mgr, k);
    EXPECT_TRUE(f.is_one());
}

// --- Variable Management Tests ---

TEST_F(MVBDDTest, NewVar) {
    MVBDD f = MVBDD::zero(mgr, k);
    bddvar mv1 = f.new_var();
    bddvar mv2 = f.new_var();

    EXPECT_EQ(mv1, 1u);
    EXPECT_EQ(mv2, 2u);
    EXPECT_EQ(f.mvdd_var_count(), 2u);

    // Check that k-1 internal DD variables are created per MVDD variable
    EXPECT_EQ(f.dd_vars_of(1).size(), static_cast<size_t>(k - 1));
    EXPECT_EQ(f.dd_vars_of(2).size(), static_cast<size_t>(k - 1));
}

// --- Single Literal Tests ---

TEST_F(MVBDDTest, SingleValue0) {
    MVBDD f = MVBDD::zero(mgr, k);
    f.new_var();

    MVBDD s = MVBDD::single(f, 1, 0);

    EXPECT_TRUE(s.evaluate({0}));
    EXPECT_FALSE(s.evaluate({1}));
    EXPECT_FALSE(s.evaluate({2}));
    EXPECT_FALSE(s.evaluate({3}));
}

TEST_F(MVBDDTest, SingleValue1) {
    MVBDD f = MVBDD::zero(mgr, k);
    f.new_var();

    MVBDD s = MVBDD::single(f, 1, 1);

    EXPECT_FALSE(s.evaluate({0}));
    EXPECT_TRUE(s.evaluate({1}));
    EXPECT_FALSE(s.evaluate({2}));
    EXPECT_FALSE(s.evaluate({3}));
}

TEST_F(MVBDDTest, SingleValue2) {
    MVBDD f = MVBDD::zero(mgr, k);
    f.new_var();

    MVBDD s = MVBDD::single(f, 1, 2);

    EXPECT_FALSE(s.evaluate({0}));
    EXPECT_FALSE(s.evaluate({1}));
    EXPECT_TRUE(s.evaluate({2}));
    EXPECT_FALSE(s.evaluate({3}));
}

TEST_F(MVBDDTest, SingleValueK1) {
    MVBDD f = MVBDD::zero(mgr, k);
    f.new_var();

    MVBDD s = MVBDD::single(f, 1, k - 1);

    for (int i = 0; i < k; ++i) {
        EXPECT_EQ(s.evaluate({i}), i == k - 1);
    }
}

// --- AND Tests ---

TEST_F(MVBDDTest, AndSame) {
    MVBDD f = MVBDD::zero(mgr, k);
    f.new_var();

    MVBDD s1 = MVBDD::single(f, 1, 1);
    MVBDD s2 = MVBDD::single(f, 1, 1);

    MVBDD a = s1 & s2;
    EXPECT_EQ(a, s1);
}

TEST_F(MVBDDTest, AndDifferent) {
    MVBDD f = MVBDD::zero(mgr, k);
    f.new_var();

    MVBDD s1 = MVBDD::single(f, 1, 1);
    MVBDD s2 = MVBDD::single(f, 1, 2);

    // Same variable can only have one value, so AND should be zero
    MVBDD a = s1 & s2;
    EXPECT_TRUE(a.is_zero());
}

TEST_F(MVBDDTest, AndWithOne) {
    MVBDD f = MVBDD::zero(mgr, k);
    f.new_var();

    MVBDD s = MVBDD::single(f, 1, 1);
    MVBDD one = MVBDD(f.manager(), f.var_table(), BDD::one(mgr));

    MVBDD a = s & one;
    EXPECT_EQ(a, s);
}

TEST_F(MVBDDTest, AndWithZero) {
    MVBDD f = MVBDD::zero(mgr, k);
    f.new_var();

    MVBDD s = MVBDD::single(f, 1, 1);
    MVBDD zero = MVBDD(f.manager(), f.var_table(), BDD::zero(mgr));

    MVBDD a = s & zero;
    EXPECT_TRUE(a.is_zero());
}

// --- OR Tests ---

TEST_F(MVBDDTest, Or) {
    MVBDD f = MVBDD::zero(mgr, k);
    f.new_var();

    MVBDD s1 = MVBDD::single(f, 1, 1);
    MVBDD s2 = MVBDD::single(f, 1, 2);

    MVBDD o = s1 | s2;

    EXPECT_FALSE(o.evaluate({0}));
    EXPECT_TRUE(o.evaluate({1}));
    EXPECT_TRUE(o.evaluate({2}));
    EXPECT_FALSE(o.evaluate({3}));
}

TEST_F(MVBDDTest, OrWithOne) {
    MVBDD f = MVBDD::zero(mgr, k);
    f.new_var();

    MVBDD s = MVBDD::single(f, 1, 1);
    MVBDD one = MVBDD(f.manager(), f.var_table(), BDD::one(mgr));

    MVBDD o = s | one;
    EXPECT_TRUE(o.is_one());
}

TEST_F(MVBDDTest, OrWithZero) {
    MVBDD f = MVBDD::zero(mgr, k);
    f.new_var();

    MVBDD s = MVBDD::single(f, 1, 1);
    MVBDD zero = MVBDD(f.manager(), f.var_table(), BDD::zero(mgr));

    MVBDD o = s | zero;
    EXPECT_EQ(o, s);
}

// --- XOR Tests ---

TEST_F(MVBDDTest, Xor) {
    MVBDD f = MVBDD::zero(mgr, k);
    f.new_var();

    MVBDD s1 = MVBDD::single(f, 1, 1);
    MVBDD s2 = MVBDD::single(f, 1, 2);

    MVBDD x = s1 ^ s2;

    // XOR: one or the other but not both (since they're mutually exclusive)
    EXPECT_FALSE(x.evaluate({0}));
    EXPECT_TRUE(x.evaluate({1}));
    EXPECT_TRUE(x.evaluate({2}));
    EXPECT_FALSE(x.evaluate({3}));
}

TEST_F(MVBDDTest, XorSame) {
    MVBDD f = MVBDD::zero(mgr, k);
    f.new_var();

    MVBDD s = MVBDD::single(f, 1, 1);

    MVBDD x = s ^ s;
    EXPECT_TRUE(x.is_zero());
}

// --- NOT Tests ---

TEST_F(MVBDDTest, Not) {
    MVBDD f = MVBDD::zero(mgr, k);
    f.new_var();

    MVBDD s = MVBDD::single(f, 1, 1);
    MVBDD n = ~s;

    EXPECT_TRUE(n.evaluate({0}));
    EXPECT_FALSE(n.evaluate({1}));
    EXPECT_TRUE(n.evaluate({2}));
    EXPECT_TRUE(n.evaluate({3}));
}

TEST_F(MVBDDTest, DoubleNot) {
    MVBDD f = MVBDD::zero(mgr, k);
    f.new_var();

    MVBDD s = MVBDD::single(f, 1, 1);
    MVBDD nn = ~~s;

    EXPECT_EQ(nn, s);
}

// --- ITE Construction Tests ---

TEST_F(MVBDDTest, IteConstruction) {
    MVBDD f = MVBDD::zero(mgr, k);
    f.new_var();

    MVBDD one = MVBDD(f.manager(), f.var_table(), BDD::one(mgr));
    MVBDD zero = MVBDD(f.manager(), f.var_table(), BDD::zero(mgr));

    // value=1 should be true only
    std::vector<MVBDD> children = {zero, one, zero, zero};
    MVBDD result = MVBDD::ite(f, 1, children);

    EXPECT_FALSE(result.evaluate({0}));
    EXPECT_TRUE(result.evaluate({1}));
    EXPECT_FALSE(result.evaluate({2}));
    EXPECT_FALSE(result.evaluate({3}));
}

TEST_F(MVBDDTest, IteAllOne) {
    MVBDD f = MVBDD::zero(mgr, k);
    f.new_var();

    MVBDD one = MVBDD(f.manager(), f.var_table(), BDD::one(mgr));

    // All children are one -> should be one
    std::vector<MVBDD> children = {one, one, one, one};
    MVBDD result = MVBDD::ite(f, 1, children);

    EXPECT_TRUE(result.is_one());
}

// --- Child Access Tests ---

TEST_F(MVBDDTest, ChildAccess) {
    MVBDD f = MVBDD::zero(mgr, k);
    f.new_var();

    MVBDD one = MVBDD(f.manager(), f.var_table(), BDD::one(mgr));
    MVBDD zero = MVBDD(f.manager(), f.var_table(), BDD::zero(mgr));

    std::vector<MVBDD> children = {zero, one, zero, one};
    MVBDD result = MVBDD::ite(f, 1, children);

    EXPECT_TRUE(result.child(0).is_zero());
    EXPECT_TRUE(result.child(1).is_one());
    EXPECT_TRUE(result.child(2).is_zero());
    EXPECT_TRUE(result.child(3).is_one());
}

// --- Multiple Variables Tests ---

TEST_F(MVBDDTest, MultipleVariablesAnd) {
    MVBDD f = MVBDD::zero(mgr, k);
    f.new_var();
    f.new_var();

    // (x1=1) AND (x2=2)
    MVBDD s1 = MVBDD::single(f, 1, 1);
    MVBDD s2 = MVBDD::single(f, 2, 2);
    MVBDD a = s1 & s2;

    EXPECT_FALSE(a.evaluate({1, 1}));
    EXPECT_TRUE(a.evaluate({1, 2}));
    EXPECT_FALSE(a.evaluate({2, 2}));
    EXPECT_FALSE(a.evaluate({0, 0}));
}

TEST_F(MVBDDTest, MultipleVariablesOr) {
    MVBDD f = MVBDD::zero(mgr, k);
    f.new_var();
    f.new_var();

    // (x1=1) OR (x2=2)
    MVBDD s1 = MVBDD::single(f, 1, 1);
    MVBDD s2 = MVBDD::single(f, 2, 2);
    MVBDD o = s1 | s2;

    EXPECT_TRUE(o.evaluate({1, 0}));   // x1=1
    EXPECT_TRUE(o.evaluate({1, 2}));   // both
    EXPECT_TRUE(o.evaluate({0, 2}));   // x2=2
    EXPECT_FALSE(o.evaluate({0, 0}));  // neither
    EXPECT_FALSE(o.evaluate({2, 1}));  // neither
}

// --- Conversion Tests ---

TEST_F(MVBDDTest, ToFromBDD) {
    MVBDD f = MVBDD::zero(mgr, k);
    f.new_var();

    MVBDD s = MVBDD::single(f, 1, 2);
    BDD b = s.to_bdd();

    MVBDD s2 = MVBDD::from_bdd(f, b);
    EXPECT_EQ(s, s2);
}

// --- Node Count Tests ---

TEST_F(MVBDDTest, NodeCount) {
    MVBDD f = MVBDD::zero(mgr, k);
    f.new_var();

    MVBDD s = MVBDD::single(f, 1, 2);

    // Internal BDD node count
    EXPECT_GE(s.size(), 1u);

    // MVBDD node count
    EXPECT_GE(s.mvbdd_node_count(), 1u);
}

// --- Error Handling Tests ---

TEST_F(MVBDDTest, InvalidValue) {
    MVBDD f = MVBDD::zero(mgr, k);
    f.new_var();

    EXPECT_THROW(MVBDD::single(f, 1, -1), DDArgumentException);
    EXPECT_THROW(MVBDD::single(f, 1, k), DDArgumentException);
}

TEST_F(MVBDDTest, InvalidVariable) {
    MVBDD f = MVBDD::zero(mgr, k);
    f.new_var();

    EXPECT_THROW(MVBDD::single(f, 0, 1), DDArgumentException);
    EXPECT_THROW(MVBDD::single(f, 2, 1), DDArgumentException);  // Only 1 var created
}

// --- Different k values ---

TEST_F(MVBDDTest, DifferentK) {
    MVBDD f2 = MVBDD::zero(mgr, 2);  // Binary
    MVBDD f3 = MVBDD::zero(mgr, 3);  // Ternary
    MVBDD f5 = MVBDD::zero(mgr, 5);  // 5-valued

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

TEST_F(MVBDDTest, CompoundAssignment) {
    MVBDD f = MVBDD::zero(mgr, k);
    f.new_var();

    MVBDD s1 = MVBDD::single(f, 1, 1);
    MVBDD s2 = MVBDD::single(f, 1, 2);

    MVBDD o = s1;
    o |= s2;

    EXPECT_FALSE(o.evaluate({0}));
    EXPECT_TRUE(o.evaluate({1}));
    EXPECT_TRUE(o.evaluate({2}));
    EXPECT_FALSE(o.evaluate({3}));
}
