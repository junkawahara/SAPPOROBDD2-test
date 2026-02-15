/**
 * @file test_mvdd_node_ref.cpp
 * @brief Tests for MVDDNodeRef
 */

#include <gtest/gtest.h>
#include "sbdd2/mvbdd.hpp"
#include "sbdd2/mvzdd.hpp"
#include "sbdd2/mvdd_node_ref.hpp"

using namespace sbdd2;

class MVDDNodeRefTest : public ::testing::Test {
protected:
    void SetUp() override {
        mgr.reset(new DDManager());
    }

    std::unique_ptr<DDManager> mgr;
};

// Basic validity tests
TEST_F(MVDDNodeRefTest, DefaultConstructorIsInvalid) {
    MVDDNodeRef ref;
    EXPECT_FALSE(ref.is_valid());
    EXPECT_FALSE(static_cast<bool>(ref));
}

TEST_F(MVDDNodeRefTest, ValidRefFromMVZDD) {
    MVZDD mvzdd = MVZDD::single(*mgr, 4);
    mvzdd.new_var();

    MVDDNodeRef ref = mvzdd.root_ref();
    EXPECT_TRUE(ref.is_valid());
    EXPECT_TRUE(static_cast<bool>(ref));
}

TEST_F(MVDDNodeRefTest, ValidRefFromMVBDD) {
    MVBDD mvbdd = MVBDD::one(*mgr, 4);
    mvbdd.new_var();

    MVDDNodeRef ref = mvbdd.root_ref();
    EXPECT_TRUE(ref.is_valid());
    EXPECT_TRUE(static_cast<bool>(ref));
}

// Terminal tests
TEST_F(MVDDNodeRefTest, TerminalZeroMVZDD) {
    MVZDD mvzdd = MVZDD::empty(*mgr, 4);
    MVDDNodeRef ref = mvzdd.root_ref();

    EXPECT_TRUE(ref.is_terminal());
    EXPECT_TRUE(ref.is_terminal_zero());
    EXPECT_FALSE(ref.is_terminal_one());
    EXPECT_FALSE(ref.terminal_value());
}

TEST_F(MVDDNodeRefTest, TerminalOneMVZDD) {
    MVZDD mvzdd = MVZDD::single(*mgr, 4);
    MVDDNodeRef ref = mvzdd.root_ref();

    EXPECT_TRUE(ref.is_terminal());
    EXPECT_FALSE(ref.is_terminal_zero());
    EXPECT_TRUE(ref.is_terminal_one());
    EXPECT_TRUE(ref.terminal_value());
}

TEST_F(MVDDNodeRefTest, TerminalZeroMVBDD) {
    MVBDD mvbdd = MVBDD::zero(*mgr, 4);
    MVDDNodeRef ref = mvbdd.root_ref();

    EXPECT_TRUE(ref.is_terminal());
    EXPECT_TRUE(ref.is_terminal_zero());
    EXPECT_FALSE(ref.is_terminal_one());
    EXPECT_FALSE(ref.terminal_value());
}

TEST_F(MVDDNodeRefTest, TerminalOneMVBDD) {
    MVBDD mvbdd = MVBDD::one(*mgr, 4);
    MVDDNodeRef ref = mvbdd.root_ref();

    EXPECT_TRUE(ref.is_terminal());
    EXPECT_FALSE(ref.is_terminal_zero());
    EXPECT_TRUE(ref.is_terminal_one());
    EXPECT_TRUE(ref.terminal_value());
}

// k() tests
TEST_F(MVDDNodeRefTest, KValue) {
    MVZDD mvzdd = MVZDD::single(*mgr, 5);
    MVDDNodeRef ref = mvzdd.root_ref();

    EXPECT_EQ(ref.k(), 5);
}

// mvdd_var() tests
TEST_F(MVDDNodeRefTest, TerminalMVDDVar) {
    MVZDD mvzdd = MVZDD::single(*mgr, 4);
    MVDDNodeRef ref = mvzdd.root_ref();

    // Terminal nodes return 0 for mvdd_var
    EXPECT_EQ(ref.mvdd_var(), 0);
}

TEST_F(MVDDNodeRefTest, NonTerminalMVDDVar) {
    MVZDD mvzdd = MVZDD::single(*mgr, 4);
    bddvar mv1 = mvzdd.new_var();
    bddvar mv2 = mvzdd.new_var();

    // Create a non-terminal MVZDD
    MVZDD singleton = MVZDD::singleton(mvzdd, mv1, 2);
    MVDDNodeRef ref = singleton.root_ref();

    EXPECT_FALSE(ref.is_terminal());
    EXPECT_EQ(ref.mvdd_var(), mv1);
}

// child() tests
TEST_F(MVDDNodeRefTest, ChildOfTerminal) {
    MVZDD mvzdd = MVZDD::single(*mgr, 4);
    MVDDNodeRef ref = mvzdd.root_ref();

    // Child of terminal should be the terminal itself
    MVDDNodeRef child0 = ref.child(0);
    MVDDNodeRef child1 = ref.child(1);

    EXPECT_TRUE(child0.is_terminal());
    EXPECT_TRUE(child1.is_terminal());
}

TEST_F(MVDDNodeRefTest, ChildOfSingleton) {
    MVZDD mvzdd = MVZDD::single(*mgr, 4);
    bddvar mv1 = mvzdd.new_var();

    // Create singleton: variable 1 = value 2
    MVZDD singleton = MVZDD::singleton(mvzdd, mv1, 2);
    MVDDNodeRef ref = singleton.root_ref();

    // Child for value 2 should lead to terminal 1
    MVDDNodeRef child2 = ref.child(2);
    EXPECT_TRUE(child2.is_terminal());
    EXPECT_TRUE(child2.terminal_value());

    // Child for value 0 should eventually lead to terminal 1 (empty set is included)
    // Child for value 1 and 3 should lead to terminal 0 (these values are not selected)
}

// dd_node_ref() tests
TEST_F(MVDDNodeRefTest, DDNodeRefAccess) {
    MVZDD mvzdd = MVZDD::single(*mgr, 4);
    MVDDNodeRef ref = mvzdd.root_ref();

    DDNodeRef dd_ref = ref.dd_node_ref();
    EXPECT_TRUE(dd_ref.is_valid());
    EXPECT_TRUE(dd_ref.is_terminal());
    EXPECT_TRUE(dd_ref.is_terminal_one());
}

// Comparison tests
TEST_F(MVDDNodeRefTest, EqualityComparison) {
    MVZDD mvzdd1 = MVZDD::single(*mgr, 4);
    MVZDD mvzdd2 = MVZDD::single(*mgr, 4);

    MVDDNodeRef ref1 = mvzdd1.root_ref();
    MVDDNodeRef ref2 = mvzdd2.root_ref();

    // Same terminal node should be equal
    EXPECT_EQ(ref1, ref2);
}

TEST_F(MVDDNodeRefTest, InequalityComparison) {
    MVZDD mvzdd1 = MVZDD::single(*mgr, 4);
    MVZDD mvzdd2 = MVZDD::empty(*mgr, 4);

    MVDDNodeRef ref1 = mvzdd1.root_ref();
    MVDDNodeRef ref2 = mvzdd2.root_ref();

    EXPECT_NE(ref1, ref2);
}

// to_mvzdd() tests
TEST_F(MVDDNodeRefTest, ToMVZDD) {
    MVZDD mvzdd = MVZDD::single(*mgr, 4);
    bddvar mv1 = mvzdd.new_var();

    MVZDD singleton = MVZDD::singleton(mvzdd, mv1, 1);
    MVDDNodeRef ref = singleton.root_ref();

    MVZDD converted = ref.to_mvzdd();
    EXPECT_TRUE(converted.is_valid());
    EXPECT_EQ(converted.k(), 4);
}

// to_mvbdd() tests
TEST_F(MVDDNodeRefTest, ToMVBDD) {
    MVBDD mvbdd = MVBDD::one(*mgr, 4);
    bddvar mv1 = mvbdd.new_var();

    MVBDD single = MVBDD::single(mvbdd, mv1, 1);
    MVDDNodeRef ref = single.root_ref();

    MVBDD converted = ref.to_mvbdd();
    EXPECT_TRUE(converted.is_valid());
    EXPECT_EQ(converted.k(), 4);
}

// Invalid reference tests
TEST_F(MVDDNodeRefTest, InvalidRefOperations) {
    MVDDNodeRef ref;

    EXPECT_FALSE(ref.is_terminal());
    EXPECT_FALSE(ref.is_terminal_zero());
    EXPECT_FALSE(ref.is_terminal_one());
    EXPECT_EQ(ref.k(), 0);
    EXPECT_EQ(ref.mvdd_var(), 0);

    MVDDNodeRef child = ref.child(0);
    EXPECT_FALSE(child.is_valid());
}

// manager() and var_table() tests
TEST_F(MVDDNodeRefTest, ManagerAndVarTableAccess) {
    MVZDD mvzdd = MVZDD::single(*mgr, 4);
    MVDDNodeRef ref = mvzdd.root_ref();

    EXPECT_EQ(ref.manager(), mgr.get());
    EXPECT_NE(ref.var_table(), nullptr);
    EXPECT_EQ(ref.var_table()->k(), 4);
}

// Traversal test
TEST_F(MVDDNodeRefTest, TraversalWithITE) {
    // Create MVZDD with ITE
    MVZDD base = MVZDD::single(*mgr, 3);  // k=3
    bddvar mv1 = base.new_var();

    MVZDD child0 = MVZDD::single(*mgr, 3);  // value 0 -> {()}
    child0 = MVZDD(base.manager(), base.var_table(), child0.to_zdd());

    MVZDD child1 = MVZDD::empty(*mgr, 3);   // value 1 -> {}
    child1 = MVZDD(base.manager(), base.var_table(), child1.to_zdd());

    MVZDD child2 = MVZDD::single(*mgr, 3);  // value 2 -> {()}
    child2 = MVZDD(base.manager(), base.var_table(), child2.to_zdd());

    MVZDD ite_result = MVZDD::ite(base, mv1, {child0, child1, child2});

    MVDDNodeRef ref = ite_result.root_ref();
    EXPECT_FALSE(ref.is_terminal());
    EXPECT_EQ(ref.mvdd_var(), mv1);
    EXPECT_EQ(ref.k(), 3);
}

// Copy semantics test
TEST_F(MVDDNodeRefTest, CopySemantics) {
    MVZDD mvzdd = MVZDD::single(*mgr, 4);
    MVDDNodeRef ref1 = mvzdd.root_ref();

    // Copy constructor
    MVDDNodeRef ref2(ref1);
    EXPECT_EQ(ref1, ref2);

    // Copy assignment
    MVDDNodeRef ref3;
    ref3 = ref1;
    EXPECT_EQ(ref1, ref3);
}
