// SAPPOROBDD 2.0 - BDD tests
// MIT License

#include <gtest/gtest.h>
#include <algorithm>
#include "sbdd2/sbdd2.hpp"

using namespace sbdd2;

class BDDTest : public ::testing::Test {
protected:
    DDManager mgr;

    void SetUp() override {
        // Create some variables
        for (int i = 0; i < 5; ++i) {
            mgr.new_var();
        }
    }
};

TEST_F(BDDTest, VarBDD) {
    BDD x1 = mgr.var_bdd(1);
    BDD x2 = mgr.var_bdd(2);

    EXPECT_FALSE(x1.is_terminal());
    EXPECT_FALSE(x2.is_terminal());
    EXPECT_EQ(x1.top(), 1u);
    EXPECT_EQ(x2.top(), 2u);
}

TEST_F(BDDTest, Negation) {
    BDD x1 = mgr.var_bdd(1);
    BDD not_x1 = ~x1;

    EXPECT_NE(x1, not_x1);
    EXPECT_EQ(x1, ~~x1);
}

TEST_F(BDDTest, AndOperation) {
    BDD x1 = mgr.var_bdd(1);
    BDD x2 = mgr.var_bdd(2);

    // x1 & x1 = x1
    EXPECT_EQ(x1 & x1, x1);

    // x1 & ~x1 = 0
    BDD zero = x1 & ~x1;
    EXPECT_TRUE(zero.is_zero());

    // x1 & 1 = x1
    EXPECT_EQ(x1 & mgr.bdd_one(), x1);

    // x1 & 0 = 0
    EXPECT_TRUE((x1 & mgr.bdd_zero()).is_zero());
}

TEST_F(BDDTest, OrOperation) {
    BDD x1 = mgr.var_bdd(1);
    BDD x2 = mgr.var_bdd(2);

    // x1 | x1 = x1
    EXPECT_EQ(x1 | x1, x1);

    // x1 | ~x1 = 1
    BDD one = x1 | ~x1;
    EXPECT_TRUE(one.is_one());

    // x1 | 0 = x1
    EXPECT_EQ(x1 | mgr.bdd_zero(), x1);

    // x1 | 1 = 1
    EXPECT_TRUE((x1 | mgr.bdd_one()).is_one());
}

TEST_F(BDDTest, XorOperation) {
    BDD x1 = mgr.var_bdd(1);

    // x1 ^ x1 = 0
    EXPECT_TRUE((x1 ^ x1).is_zero());

    // x1 ^ ~x1 = 1
    EXPECT_TRUE((x1 ^ ~x1).is_one());

    // x1 ^ 0 = x1
    EXPECT_EQ(x1 ^ mgr.bdd_zero(), x1);

    // x1 ^ 1 = ~x1
    EXPECT_EQ(x1 ^ mgr.bdd_one(), ~x1);
}

TEST_F(BDDTest, DiffOperation) {
    BDD x1 = mgr.var_bdd(1);
    BDD x2 = mgr.var_bdd(2);

    // x1 - x1 = 0
    EXPECT_TRUE((x1 - x1).is_zero());

    // x1 - 0 = x1
    EXPECT_EQ(x1 - mgr.bdd_zero(), x1);

    // x1 - 1 = 0
    EXPECT_TRUE((x1 - mgr.bdd_one()).is_zero());
}

TEST_F(BDDTest, ITE) {
    BDD x1 = mgr.var_bdd(1);
    BDD x2 = mgr.var_bdd(2);
    BDD one = mgr.bdd_one();
    BDD zero = mgr.bdd_zero();

    // ITE(x1, 1, 0) = x1
    EXPECT_EQ(x1.ite(one, zero), x1);

    // ITE(x1, 0, 1) = ~x1
    EXPECT_EQ(x1.ite(zero, one), ~x1);

    // ITE(1, x1, x2) = x1
    EXPECT_EQ(one.ite(x1, x2), x1);

    // ITE(0, x1, x2) = x2
    EXPECT_EQ(zero.ite(x1, x2), x2);
}

TEST_F(BDDTest, Cofactors) {
    BDD x1 = mgr.var_bdd(1);
    BDD x2 = mgr.var_bdd(2);
    BDD f = x1 & x2;  // x1 AND x2

    // f|_{x1=0} = 0
    EXPECT_TRUE(f.at0(1).is_zero());

    // f|_{x1=1} = x2
    EXPECT_EQ(f.at1(1), x2);

    // f|_{x2=0} = 0
    EXPECT_TRUE(f.at0(2).is_zero());

    // f|_{x2=1} = x1
    EXPECT_EQ(f.at1(2), x1);
}

TEST_F(BDDTest, Quantification) {
    BDD x1 = mgr.var_bdd(1);
    BDD x2 = mgr.var_bdd(2);
    BDD f = x1 & x2;

    // Exist(x1, f) = x2
    EXPECT_EQ(f.exist(1), x2);

    // Forall(x1, f) = 0
    EXPECT_TRUE(f.forall(1).is_zero());
}

TEST_F(BDDTest, Size) {
    BDD x1 = mgr.var_bdd(1);
    BDD x2 = mgr.var_bdd(2);

    EXPECT_EQ(x1.size(), 1u);
    EXPECT_EQ(x2.size(), 1u);

    BDD f = x1 & x2;
    EXPECT_GE(f.size(), 1u);
}

TEST_F(BDDTest, Support) {
    BDD x1 = mgr.var_bdd(1);
    BDD x2 = mgr.var_bdd(2);
    BDD x3 = mgr.var_bdd(3);

    BDD f = (x1 & x2) | x3;
    std::vector<bddvar> supp = f.support();

    EXPECT_EQ(supp.size(), 3u);
    EXPECT_NE(std::find(supp.begin(), supp.end(), 1), supp.end());
    EXPECT_NE(std::find(supp.begin(), supp.end(), 2), supp.end());
    EXPECT_NE(std::find(supp.begin(), supp.end(), 3), supp.end());
}

TEST_F(BDDTest, Counting) {
    BDD x1 = mgr.var_bdd(1);

    // x1 has 2^(n-1) satisfying assignments (where n=5 vars)
    // For x1 alone: when x1=1, all other 4 vars can be anything = 16 assignments
    double c = x1.count(5);
    EXPECT_DOUBLE_EQ(c, 16.0);

    // x1 & x2 has 2^(n-2) = 8 satisfying assignments
    BDD x2 = mgr.var_bdd(2);
    BDD f = x1 & x2;
    EXPECT_DOUBLE_EQ(f.count(5), 8.0);
}

TEST_F(BDDTest, OneSat) {
    BDD x1 = mgr.var_bdd(1);
    BDD x2 = mgr.var_bdd(2);
    BDD f = x1 & x2;

    std::vector<int> sat = f.one_sat();
    ASSERT_FALSE(sat.empty());
    EXPECT_EQ(sat[1], 1);  // x1 must be 1
    EXPECT_EQ(sat[2], 1);  // x2 must be 1
}

TEST_F(BDDTest, LowHigh) {
    BDD x1 = mgr.var_bdd(1);

    // For variable BDD: low = 0, high = 1
    EXPECT_TRUE(x1.low().is_zero());
    EXPECT_TRUE(x1.high().is_one());

    // For negated variable: low = 1, high = 0
    BDD not_x1 = ~x1;
    EXPECT_TRUE(not_x1.low().is_one());
    EXPECT_TRUE(not_x1.high().is_zero());
}

#if defined(SBDD2_HAS_GMP) || defined(SBDD2_HAS_BIGINT)
TEST(BDDExactCountTest, MatchesCard) {
    DDManager mgr;

    // Create 10 variables
    for (int i = 0; i < 10; ++i) {
        mgr.new_var();
    }

    // Test terminals
    BDD zero = mgr.bdd_zero();
    BDD one = mgr.bdd_one();
    EXPECT_EQ(zero.exact_count(), std::to_string(static_cast<long long>(zero.card())));
    EXPECT_EQ(one.exact_count(), std::to_string(static_cast<long long>(one.card())));

    // Test variables
    BDD x1 = mgr.var_bdd(1);
    BDD x2 = mgr.var_bdd(2);
    EXPECT_EQ(x1.exact_count(), std::to_string(static_cast<long long>(x1.card())));
    EXPECT_EQ(x2.exact_count(), std::to_string(static_cast<long long>(x2.card())));

    // Test operations
    BDD f_and = x1 & x2;
    BDD f_or = x1 | x2;
    BDD f_xor = x1 ^ x2;
    EXPECT_EQ(f_and.exact_count(), std::to_string(static_cast<long long>(f_and.card())));
    EXPECT_EQ(f_or.exact_count(), std::to_string(static_cast<long long>(f_or.card())));
    EXPECT_EQ(f_xor.exact_count(), std::to_string(static_cast<long long>(f_xor.card())));
}

TEST(BDDExactCountTest, LargeCount) {
    DDManager mgr;

    // Create 60 variables
    for (int i = 0; i < 60; ++i) {
        mgr.new_var();
    }

    // x1 with 60 variables
    BDD x1 = mgr.var_bdd(1);

    // exact_count() should match card() for all values
    // For large values (> 2^53), double precision loses accuracy
    // but exact_count() remains precise
    std::string exact_str = x1.exact_count();

    // The value is large enough to exceed 2^53, demonstrating
    // the usefulness of exact_count() over card()
    // (exact_count uses GMP for arbitrary precision)
    EXPECT_FALSE(exact_str.empty());
    EXPECT_NE(exact_str, "0");

    // Verify the value is consistent (deterministic)
    EXPECT_EQ(x1.exact_count(), exact_str);
}
#endif
