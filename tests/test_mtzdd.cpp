// SAPPOROBDD 2.0 - MTZDD tests
// MIT License

#include <gtest/gtest.h>
#include "sbdd2/sbdd2.hpp"
#include "sbdd2/mtdd_base.hpp"
#include "sbdd2/mtzdd.hpp"

using namespace sbdd2;

class MTZDDTest : public ::testing::Test {
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
TEST_F(MTZDDTest, ConstantInt) {
    MTZDD<int> c0 = MTZDD<int>::constant(mgr, 0);
    MTZDD<int> c42 = MTZDD<int>::constant(mgr, 42);

    EXPECT_TRUE(c0.is_terminal());
    EXPECT_TRUE(c42.is_terminal());

    EXPECT_EQ(c0.terminal_value(), 0);
    EXPECT_EQ(c42.terminal_value(), 42);
}

TEST_F(MTZDDTest, ConstantDouble) {
    MTZDD<double> c0 = MTZDD<double>::constant(mgr, 0.0);
    MTZDD<double> cpi = MTZDD<double>::constant(mgr, 3.14159);

    EXPECT_TRUE(c0.is_terminal());
    EXPECT_TRUE(cpi.is_terminal());

    EXPECT_DOUBLE_EQ(c0.terminal_value(), 0.0);
    EXPECT_DOUBLE_EQ(cpi.terminal_value(), 3.14159);
}

// Same value shares same index
TEST_F(MTZDDTest, TerminalSharing) {
    MTZDD<int> c1 = MTZDD<int>::constant(mgr, 42);
    MTZDD<int> c2 = MTZDD<int>::constant(mgr, 42);
    MTZDD<int> c3 = MTZDD<int>::constant(mgr, 100);

    EXPECT_EQ(c1, c2);
    EXPECT_NE(c1, c3);
}

// ITE node creation
TEST_F(MTZDDTest, IteCreation) {
    MTZDD<int> hi = MTZDD<int>::constant(mgr, 1);
    MTZDD<int> lo = MTZDD<int>::constant(mgr, 5);

    MTZDD<int> x1 = MTZDD<int>::ite(mgr, 1, hi, lo);

    EXPECT_FALSE(x1.is_terminal());
    EXPECT_EQ(x1.top(), 1u);
    EXPECT_EQ(x1.low(), lo);
    EXPECT_EQ(x1.high(), hi);
}

// ZDD reduction rule: if 1-arc points to zero terminal, return 0-arc
TEST_F(MTZDDTest, ZDDReductionRule) {
    MTZDD<int> zero = MTZDD<int>::constant(mgr, 0);  // Zero terminal
    MTZDD<int> lo = MTZDD<int>::constant(mgr, 42);

    // 1-arc points to zero terminal -> should return 0-arc (lo)
    MTZDD<int> x1 = MTZDD<int>::ite(mgr, 1, zero, lo);

    EXPECT_TRUE(x1.is_terminal());
    EXPECT_EQ(x1.terminal_value(), 42);  // Should be lo's value
}

// Both arcs same -> return child (common reduction)
TEST_F(MTZDDTest, CommonReductionRule) {
    MTZDD<int> c42 = MTZDD<int>::constant(mgr, 42);

    // Both branches point to same terminal -> should return terminal
    MTZDD<int> x1 = MTZDD<int>::ite(mgr, 1, c42, c42);

    EXPECT_TRUE(x1.is_terminal());
    EXPECT_EQ(x1.terminal_value(), 42);
}

// Binary operations: addition
TEST_F(MTZDDTest, Addition) {
    MTZDD<int> c5 = MTZDD<int>::constant(mgr, 5);
    MTZDD<int> c3 = MTZDD<int>::constant(mgr, 3);

    MTZDD<int> sum = c5 + c3;

    EXPECT_TRUE(sum.is_terminal());
    EXPECT_EQ(sum.terminal_value(), 8);
}

// Binary operations: subtraction
TEST_F(MTZDDTest, Subtraction) {
    MTZDD<int> c10 = MTZDD<int>::constant(mgr, 10);
    MTZDD<int> c4 = MTZDD<int>::constant(mgr, 4);

    MTZDD<int> diff = c10 - c4;

    EXPECT_TRUE(diff.is_terminal());
    EXPECT_EQ(diff.terminal_value(), 6);
}

// Binary operations: multiplication
TEST_F(MTZDDTest, Multiplication) {
    MTZDD<int> c7 = MTZDD<int>::constant(mgr, 7);
    MTZDD<int> c6 = MTZDD<int>::constant(mgr, 6);

    MTZDD<int> prod = c7 * c6;

    EXPECT_TRUE(prod.is_terminal());
    EXPECT_EQ(prod.terminal_value(), 42);
}

// Binary operations: min/max
TEST_F(MTZDDTest, MinMax) {
    MTZDD<int> c5 = MTZDD<int>::constant(mgr, 5);
    MTZDD<int> c3 = MTZDD<int>::constant(mgr, 3);

    MTZDD<int> min_val = MTZDD<int>::min(c5, c3);
    MTZDD<int> max_val = MTZDD<int>::max(c5, c3);

    EXPECT_TRUE(min_val.is_terminal());
    EXPECT_TRUE(max_val.is_terminal());
    EXPECT_EQ(min_val.terminal_value(), 3);
    EXPECT_EQ(max_val.terminal_value(), 5);
}

// Operations with non-terminal MTZDDs
TEST_F(MTZDDTest, AdditionWithVariables) {
    MTZDD<int> hi = MTZDD<int>::constant(mgr, 10);
    MTZDD<int> lo = MTZDD<int>::constant(mgr, 5);
    MTZDD<int> x1 = MTZDD<int>::ite(mgr, 1, hi, lo);

    MTZDD<int> c3 = MTZDD<int>::constant(mgr, 3);
    MTZDD<int> result = x1 + c3;

    // x1=1 -> 10+3=13, x1=0 -> 5+3=8
    std::vector<bool> assign1 = {false, true};  // x1=1
    std::vector<bool> assign0 = {false, false}; // x1=0

    EXPECT_EQ(result.evaluate(assign1), 13);
    EXPECT_EQ(result.evaluate(assign0), 8);
}

// From ZDD conversion
TEST_F(MTZDDTest, FromZDD) {
    ZDD s1 = ZDD::singleton(mgr, 1);
    ZDD s2 = ZDD::singleton(mgr, 2);
    ZDD f = s1 + s2;  // {{1}, {2}}

    MTZDD<int> m = MTZDD<int>::from_zdd(f, 0, 1);

    // For a ZDD representing {{1}, {2}}:
    // x1=1, x2=0 -> 1 (contains {1})
    // x1=0, x2=1 -> 1 (contains {2})
    // x1=0, x2=0 -> 0 (empty)
    std::vector<bool> assign_10 = {false, true, false};
    std::vector<bool> assign_01 = {false, false, true};
    std::vector<bool> assign_00 = {false, false, false};

    EXPECT_EQ(m.evaluate(assign_10), 1);
    EXPECT_EQ(m.evaluate(assign_01), 1);
    EXPECT_EQ(m.evaluate(assign_00), 0);
}

// ZDD reduction in operations
TEST_F(MTZDDTest, ZDDReductionInOperations) {
    // Create two MTZDDs that, when added, result in zero for 1-arc
    MTZDD<int> hi1 = MTZDD<int>::constant(mgr, 5);
    MTZDD<int> lo1 = MTZDD<int>::constant(mgr, 10);
    MTZDD<int> z1 = MTZDD<int>::ite(mgr, 1, hi1, lo1);

    MTZDD<int> hi2 = MTZDD<int>::constant(mgr, -5);
    MTZDD<int> lo2 = MTZDD<int>::constant(mgr, -10);
    MTZDD<int> z2 = MTZDD<int>::ite(mgr, 1, hi2, lo2);

    // z1 + z2: hi=5+(-5)=0, lo=10+(-10)=0
    MTZDD<int> sum = z1 + z2;

    // Both should be 0, and with ZDD reduction, the node should collapse
    EXPECT_TRUE(sum.is_terminal());
    EXPECT_EQ(sum.terminal_value(), 0);
}

// ITE operation
TEST_F(MTZDDTest, IteOperation) {
    MTZDD<int> cond_hi = MTZDD<int>::constant(mgr, 1);
    MTZDD<int> cond_lo = MTZDD<int>::constant(mgr, 0);
    MTZDD<int> cond = MTZDD<int>::ite(mgr, 1, cond_hi, cond_lo);

    MTZDD<int> then_val = MTZDD<int>::constant(mgr, 100);
    MTZDD<int> else_val = MTZDD<int>::constant(mgr, 200);

    MTZDD<int> result = cond.ite(then_val, else_val);

    // x1=1 -> cond=1 -> then_val=100
    // x1=0 -> cond=0 -> else_val=200
    std::vector<bool> assign1 = {false, true};
    std::vector<bool> assign0 = {false, false};

    EXPECT_EQ(result.evaluate(assign1), 100);
    EXPECT_EQ(result.evaluate(assign0), 200);
}

// Size (node count)
TEST_F(MTZDDTest, Size) {
    MTZDD<int> c = MTZDD<int>::constant(mgr, 42);
    EXPECT_EQ(c.size(), 0u);  // Terminal has 0 internal nodes

    MTZDD<int> hi = MTZDD<int>::constant(mgr, 1);
    MTZDD<int> lo = MTZDD<int>::constant(mgr, 2);  // Non-zero, so node will exist
    MTZDD<int> x1 = MTZDD<int>::ite(mgr, 1, hi, lo);
    EXPECT_EQ(x1.size(), 1u);  // 1 internal node
}

// ZDD reduction should reduce node count
TEST_F(MTZDDTest, ZDDReductionReducesNodes) {
    MTZDD<int> zero = MTZDD<int>::constant(mgr, 0);
    MTZDD<int> nonzero = MTZDD<int>::constant(mgr, 5);

    // Without ZDD reduction, this would create a node
    // With ZDD reduction, hi=0 means return lo
    MTZDD<int> x1 = MTZDD<int>::ite(mgr, 1, zero, nonzero);
    EXPECT_EQ(x1.size(), 0u);  // Should be reduced to terminal
}

// Copy and move semantics
TEST_F(MTZDDTest, CopyMove) {
    MTZDD<int> a = MTZDD<int>::constant(mgr, 42);
    MTZDD<int> b = a;  // copy
    EXPECT_EQ(a, b);

    MTZDD<int> c = std::move(b);  // move
    EXPECT_EQ(a, c);
    EXPECT_FALSE(b.is_valid());  // b should be invalid after move
}

// Shared terminal table with MTBDD
TEST_F(MTZDDTest, SharedTerminalTable) {
    // Create MTBDD with value 42
    MTBDDTerminalTable<int>& table = mgr.get_or_create_terminal_table<int>();
    bddindex idx1 = table.get_or_insert(42);

    // Create MTZDD with same value - should use same index
    MTZDD<int> z = MTZDD<int>::constant(mgr, 42);

    // They should use the same terminal table
    EXPECT_EQ(z.terminal_value(), 42);
}
