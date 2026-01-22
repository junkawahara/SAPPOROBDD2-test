// SAPPOROBDD 2.0 - Basic tests
// MIT License

#include <gtest/gtest.h>
#include "sbdd2/sbdd2.hpp"

using namespace sbdd2;

// Test Arc structure
TEST(ArcTest, TerminalCreation) {
    Arc term0 = Arc::terminal(false);
    Arc term1 = Arc::terminal(true);

    EXPECT_TRUE(term0.is_constant());
    EXPECT_TRUE(term1.is_constant());
    EXPECT_FALSE(term0.terminal_value());
    EXPECT_TRUE(term1.terminal_value());
    EXPECT_FALSE(term0.is_negated());
    EXPECT_FALSE(term1.is_negated());
}

TEST(ArcTest, NodeCreation) {
    Arc node = Arc::node(42, false);
    Arc node_neg = Arc::node(42, true);

    EXPECT_FALSE(node.is_constant());
    EXPECT_FALSE(node_neg.is_constant());
    EXPECT_EQ(node.index(), 42u);
    EXPECT_EQ(node_neg.index(), 42u);
    EXPECT_FALSE(node.is_negated());
    EXPECT_TRUE(node_neg.is_negated());
}

TEST(ArcTest, Negation) {
    Arc node = Arc::node(100, false);
    Arc negated = node.negated();

    EXPECT_TRUE(negated.is_negated());
    EXPECT_EQ(node.index(), negated.index());
    EXPECT_EQ(node, negated.negated());
}

// Test DDNode structure
TEST(DDNodeTest, Size) {
    EXPECT_EQ(sizeof(DDNode), 16u);  // 128 bits
}

TEST(DDNodeTest, BasicOperations) {
    DDNode node(Arc::terminal(false), Arc::terminal(true), 5, true, 1);

    EXPECT_EQ(node.var(), 5u);
    EXPECT_TRUE(node.is_reduced());
    EXPECT_EQ(node.refcount(), 1u);
    EXPECT_EQ(node.arc0(), Arc::terminal(false));
    EXPECT_EQ(node.arc1(), Arc::terminal(true));
}

TEST(DDNodeTest, RefCount) {
    DDNode node;
    node.set_refcount_raw(0);
    EXPECT_EQ(node.refcount(), 0u);

    node.inc_refcount();
    EXPECT_EQ(node.refcount(), 1u);

    node.inc_refcount();
    EXPECT_EQ(node.refcount(), 2u);

    EXPECT_FALSE(node.dec_refcount());
    EXPECT_EQ(node.refcount(), 1u);

    EXPECT_TRUE(node.dec_refcount());
    EXPECT_EQ(node.refcount(), 0u);
}

TEST(DDNodeTest, RefCountSaturation) {
    DDNode node;
    node.set_refcount_raw(BDDREFCOUNT_MAX);
    EXPECT_EQ(node.refcount(), BDDREFCOUNT_MAX);

    // Should not increase beyond max
    node.inc_refcount();
    EXPECT_EQ(node.refcount(), BDDREFCOUNT_MAX);

    // Should not decrease from max
    EXPECT_FALSE(node.dec_refcount());
    EXPECT_EQ(node.refcount(), BDDREFCOUNT_MAX);
}

// Test DDManager
TEST(DDManagerTest, Construction) {
    DDManager mgr(1024, 256);

    EXPECT_EQ(mgr.var_count(), 0u);
    EXPECT_EQ(mgr.node_count(), 0u);
}

TEST(DDManagerTest, NewVar) {
    DDManager mgr;

    bddvar v1 = mgr.new_var();
    bddvar v2 = mgr.new_var();
    bddvar v3 = mgr.new_var();

    EXPECT_EQ(v1, 1u);
    EXPECT_EQ(v2, 2u);
    EXPECT_EQ(v3, 3u);
    EXPECT_EQ(mgr.var_count(), 3u);
}

TEST(DDManagerTest, Terminals) {
    DDManager mgr;

    BDD zero = mgr.bdd_zero();
    BDD one = mgr.bdd_one();

    EXPECT_TRUE(zero.is_zero());
    EXPECT_TRUE(one.is_one());
    EXPECT_TRUE(zero.is_terminal());
    EXPECT_TRUE(one.is_terminal());
}

// Test variable level management
TEST(DDManagerTest, VariableLevelBasic) {
    DDManager mgr;

    bddvar v1 = mgr.new_var();
    bddvar v2 = mgr.new_var();
    bddvar v3 = mgr.new_var();

    // Default: variable number == level
    EXPECT_EQ(mgr.lev_of_var(v1), 1u);
    EXPECT_EQ(mgr.lev_of_var(v2), 2u);
    EXPECT_EQ(mgr.lev_of_var(v3), 3u);

    EXPECT_EQ(mgr.var_of_lev(1), v1);
    EXPECT_EQ(mgr.var_of_lev(2), v2);
    EXPECT_EQ(mgr.var_of_lev(3), v3);

    EXPECT_EQ(mgr.top_lev(), 3u);
}

TEST(DDManagerTest, NewVarOfLev) {
    DDManager mgr;

    bddvar v1 = mgr.new_var();  // var=1, lev=1
    bddvar v2 = mgr.new_var();  // var=2, lev=2

    // Insert new variable at level 1, shifting existing ones down
    bddvar v3 = mgr.new_var_of_lev(1);  // var=3, lev=1

    EXPECT_EQ(mgr.lev_of_var(v3), 1u);  // v3 is now at level 1
    EXPECT_EQ(mgr.lev_of_var(v1), 2u);  // v1 shifted to level 2
    EXPECT_EQ(mgr.lev_of_var(v2), 3u);  // v2 shifted to level 3

    EXPECT_EQ(mgr.var_of_lev(1), v3);
    EXPECT_EQ(mgr.var_of_lev(2), v1);
    EXPECT_EQ(mgr.var_of_lev(3), v2);

    EXPECT_EQ(mgr.top_lev(), 3u);
}

TEST(DDManagerTest, VarOfMinLev) {
    DDManager mgr;

    bddvar v1 = mgr.new_var();  // var=1, lev=1
    bddvar v2 = mgr.new_var();  // var=2, lev=2
    mgr.new_var_of_lev(1);      // var=3, lev=1 (now v1 is at lev=2, v2 at lev=3)

    // var=3 is at level 1, var=1 is at level 2
    bddvar min_lev_var = mgr.var_of_min_lev(1, 3);
    EXPECT_EQ(min_lev_var, 3u);  // v3 has smaller level

    // Test with BDDVAR_MAX
    bddvar result = mgr.var_of_min_lev(1, BDDVAR_MAX);
    EXPECT_EQ(result, 1u);
}

TEST(DDManagerTest, VarIsAboveBelow) {
    DDManager mgr;

    bddvar v1 = mgr.new_var();  // var=1, lev=1
    bddvar v2 = mgr.new_var();  // var=2, lev=2
    bddvar v3 = mgr.new_var_of_lev(1);  // var=3, lev=1 (shifts v1 to lev=2, v2 to lev=3)

    // v3 is at lev=1, v1 is at lev=2, v2 is at lev=3
    EXPECT_TRUE(mgr.var_is_above_or_equal(v3, v1));   // lev(v3)=1 <= lev(v1)=2
    EXPECT_TRUE(mgr.var_is_above_or_equal(v3, v2));   // lev(v3)=1 <= lev(v2)=3
    EXPECT_TRUE(mgr.var_is_above_or_equal(v1, v1));   // equal
    EXPECT_FALSE(mgr.var_is_above_or_equal(v2, v1));  // lev(v2)=3 > lev(v1)=2

    EXPECT_TRUE(mgr.var_is_below(v2, v1));   // lev(v2)=3 > lev(v1)=2
    EXPECT_FALSE(mgr.var_is_below(v3, v1));  // lev(v3)=1 < lev(v1)=2
}
