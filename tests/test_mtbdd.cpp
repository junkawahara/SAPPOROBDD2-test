// SAPPOROBDD 2.0 - MTBDD tests
// MIT License

#include <gtest/gtest.h>
#include "sbdd2/sbdd2.hpp"
#include "sbdd2/mtdd_base.hpp"
#include "sbdd2/mtbdd.hpp"

using namespace sbdd2;

class MTBDDTest : public ::testing::Test {
protected:
    DDManager mgr;

    void SetUp() override {
        // Create some variables
        for (int i = 0; i < 5; ++i) {
            mgr.new_var();
        }
    }
};

// Basic constant creation
TEST_F(MTBDDTest, ConstantInt) {
    MTBDD<int> c0 = MTBDD<int>::constant(mgr, 0);
    MTBDD<int> c42 = MTBDD<int>::constant(mgr, 42);
    MTBDD<int> c_neg = MTBDD<int>::constant(mgr, -10);

    EXPECT_TRUE(c0.is_terminal());
    EXPECT_TRUE(c42.is_terminal());
    EXPECT_TRUE(c_neg.is_terminal());

    EXPECT_EQ(c0.terminal_value(), 0);
    EXPECT_EQ(c42.terminal_value(), 42);
    EXPECT_EQ(c_neg.terminal_value(), -10);
}

TEST_F(MTBDDTest, ConstantDouble) {
    MTBDD<double> c0 = MTBDD<double>::constant(mgr, 0.0);
    MTBDD<double> cpi = MTBDD<double>::constant(mgr, 3.14159);

    EXPECT_TRUE(c0.is_terminal());
    EXPECT_TRUE(cpi.is_terminal());

    EXPECT_DOUBLE_EQ(c0.terminal_value(), 0.0);
    EXPECT_DOUBLE_EQ(cpi.terminal_value(), 3.14159);
}

// Same value shares same index
TEST_F(MTBDDTest, TerminalSharing) {
    MTBDD<int> c1 = MTBDD<int>::constant(mgr, 42);
    MTBDD<int> c2 = MTBDD<int>::constant(mgr, 42);
    MTBDD<int> c3 = MTBDD<int>::constant(mgr, 100);

    EXPECT_EQ(c1, c2);
    EXPECT_NE(c1, c3);
}

// ITE node creation
TEST_F(MTBDDTest, IteCreation) {
    MTBDD<int> hi = MTBDD<int>::constant(mgr, 1);
    MTBDD<int> lo = MTBDD<int>::constant(mgr, 0);

    MTBDD<int> x1 = MTBDD<int>::ite(mgr, 1, hi, lo);

    EXPECT_FALSE(x1.is_terminal());
    EXPECT_EQ(x1.top(), 1u);
    EXPECT_EQ(x1.low(), lo);
    EXPECT_EQ(x1.high(), hi);
}

// BDD reduction rule: if both arcs are same, return child
TEST_F(MTBDDTest, ReductionRule) {
    MTBDD<int> c42 = MTBDD<int>::constant(mgr, 42);

    // Both branches point to same terminal -> should return terminal
    MTBDD<int> x1 = MTBDD<int>::ite(mgr, 1, c42, c42);

    EXPECT_TRUE(x1.is_terminal());
    EXPECT_EQ(x1.terminal_value(), 42);
}

// Binary operations: addition
TEST_F(MTBDDTest, Addition) {
    MTBDD<int> c5 = MTBDD<int>::constant(mgr, 5);
    MTBDD<int> c3 = MTBDD<int>::constant(mgr, 3);

    MTBDD<int> sum = c5 + c3;

    EXPECT_TRUE(sum.is_terminal());
    EXPECT_EQ(sum.terminal_value(), 8);
}

// Binary operations: subtraction
TEST_F(MTBDDTest, Subtraction) {
    MTBDD<int> c10 = MTBDD<int>::constant(mgr, 10);
    MTBDD<int> c4 = MTBDD<int>::constant(mgr, 4);

    MTBDD<int> diff = c10 - c4;

    EXPECT_TRUE(diff.is_terminal());
    EXPECT_EQ(diff.terminal_value(), 6);
}

// Binary operations: multiplication
TEST_F(MTBDDTest, Multiplication) {
    MTBDD<int> c7 = MTBDD<int>::constant(mgr, 7);
    MTBDD<int> c6 = MTBDD<int>::constant(mgr, 6);

    MTBDD<int> prod = c7 * c6;

    EXPECT_TRUE(prod.is_terminal());
    EXPECT_EQ(prod.terminal_value(), 42);
}

// Binary operations: min/max
TEST_F(MTBDDTest, MinMax) {
    MTBDD<int> c5 = MTBDD<int>::constant(mgr, 5);
    MTBDD<int> c3 = MTBDD<int>::constant(mgr, 3);

    MTBDD<int> min_val = MTBDD<int>::min(c5, c3);
    MTBDD<int> max_val = MTBDD<int>::max(c5, c3);

    EXPECT_TRUE(min_val.is_terminal());
    EXPECT_TRUE(max_val.is_terminal());
    EXPECT_EQ(min_val.terminal_value(), 3);
    EXPECT_EQ(max_val.terminal_value(), 5);
}

// Operations with non-terminal MTBDDs
TEST_F(MTBDDTest, AdditionWithVariables) {
    MTBDD<int> hi = MTBDD<int>::constant(mgr, 10);
    MTBDD<int> lo = MTBDD<int>::constant(mgr, 5);
    MTBDD<int> x1 = MTBDD<int>::ite(mgr, 1, hi, lo);

    MTBDD<int> c3 = MTBDD<int>::constant(mgr, 3);
    MTBDD<int> result = x1 + c3;

    // x1=1 -> 10+3=13, x1=0 -> 5+3=8
    std::vector<bool> assign1 = {false, true};  // x1=1
    std::vector<bool> assign0 = {false, false}; // x1=0

    EXPECT_EQ(result.evaluate(assign1), 13);
    EXPECT_EQ(result.evaluate(assign0), 8);
}

// From BDD conversion
TEST_F(MTBDDTest, FromBDD) {
    BDD x1 = mgr.var_bdd(1);
    BDD x2 = mgr.var_bdd(2);
    BDD f = x1 & x2;

    MTBDD<int> m = MTBDD<int>::from_bdd(f, 0, 1);

    // x1=1, x2=1 -> 1
    std::vector<bool> assign_11 = {false, true, true};
    EXPECT_EQ(m.evaluate(assign_11), 1);

    // x1=1, x2=0 -> 0
    std::vector<bool> assign_10 = {false, true, false};
    EXPECT_EQ(m.evaluate(assign_10), 0);

    // x1=0, x2=1 -> 0
    std::vector<bool> assign_01 = {false, false, true};
    EXPECT_EQ(m.evaluate(assign_01), 0);

    // x1=0, x2=0 -> 0
    std::vector<bool> assign_00 = {false, false, false};
    EXPECT_EQ(m.evaluate(assign_00), 0);
}

// From BDD with custom values
TEST_F(MTBDDTest, FromBDDCustomValues) {
    BDD x1 = mgr.var_bdd(1);

    MTBDD<double> m = MTBDD<double>::from_bdd(x1, 0.0, 100.0);

    std::vector<bool> assign1 = {false, true};
    std::vector<bool> assign0 = {false, false};

    EXPECT_DOUBLE_EQ(m.evaluate(assign1), 100.0);
    EXPECT_DOUBLE_EQ(m.evaluate(assign0), 0.0);
}

// ITE operation
TEST_F(MTBDDTest, IteOperation) {
    MTBDD<int> cond_hi = MTBDD<int>::constant(mgr, 1);
    MTBDD<int> cond_lo = MTBDD<int>::constant(mgr, 0);
    MTBDD<int> cond = MTBDD<int>::ite(mgr, 1, cond_hi, cond_lo);

    MTBDD<int> then_val = MTBDD<int>::constant(mgr, 100);
    MTBDD<int> else_val = MTBDD<int>::constant(mgr, 200);

    MTBDD<int> result = cond.ite(then_val, else_val);

    // x1=1 -> cond=1 -> then_val=100
    // x1=0 -> cond=0 -> else_val=200
    std::vector<bool> assign1 = {false, true};
    std::vector<bool> assign0 = {false, false};

    EXPECT_EQ(result.evaluate(assign1), 100);
    EXPECT_EQ(result.evaluate(assign0), 200);
}

// ADD alias
TEST_F(MTBDDTest, ADDAlias) {
    ADD<double> a = ADD<double>::constant(mgr, 1.5);
    ADD<double> b = ADD<double>::constant(mgr, 2.5);
    ADD<double> sum = a + b;

    EXPECT_DOUBLE_EQ(sum.terminal_value(), 4.0);
}

// Size (node count)
TEST_F(MTBDDTest, Size) {
    MTBDD<int> c = MTBDD<int>::constant(mgr, 42);
    EXPECT_EQ(c.size(), 0u);  // Terminal has 0 internal nodes

    MTBDD<int> hi = MTBDD<int>::constant(mgr, 1);
    MTBDD<int> lo = MTBDD<int>::constant(mgr, 0);
    MTBDD<int> x1 = MTBDD<int>::ite(mgr, 1, hi, lo);
    EXPECT_EQ(x1.size(), 1u);  // 1 internal node

    MTBDD<int> x2 = MTBDD<int>::ite(mgr, 2, hi, lo);
    MTBDD<int> f = MTBDD<int>::ite(mgr, 1, x2, lo);
    EXPECT_GE(f.size(), 1u);  // At least 1 internal node
}

// Node sharing in operations
TEST_F(MTBDDTest, NodeSharing) {
    MTBDD<int> hi = MTBDD<int>::constant(mgr, 1);
    MTBDD<int> lo = MTBDD<int>::constant(mgr, 0);
    MTBDD<int> x1 = MTBDD<int>::ite(mgr, 1, hi, lo);
    MTBDD<int> x1_copy = MTBDD<int>::ite(mgr, 1, hi, lo);

    // Same structure should share the same node
    EXPECT_EQ(x1.arc(), x1_copy.arc());
}

// Copy and move semantics
TEST_F(MTBDDTest, CopyMove) {
    MTBDD<int> a = MTBDD<int>::constant(mgr, 42);
    MTBDD<int> b = a;  // copy
    EXPECT_EQ(a, b);

    MTBDD<int> c = std::move(b);  // move
    EXPECT_EQ(a, c);
    EXPECT_FALSE(b.is_valid());  // b should be invalid after move
}
