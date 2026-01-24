// SAPPOROBDD 2.0 - Extended class tests
// MIT License

#include <gtest/gtest.h>
#include <algorithm>
#include <sstream>
#include "sbdd2/sbdd2.hpp"

using namespace sbdd2;

// ============== UnreducedBDD Tests ==============

class UnreducedBDDTest : public ::testing::Test {
protected:
    DDManager mgr;

    void SetUp() override {
        for (int i = 0; i < 5; ++i) {
            mgr.new_var();
        }
    }
};

TEST_F(UnreducedBDDTest, Terminals) {
    UnreducedBDD zero = UnreducedBDD::zero(mgr);
    UnreducedBDD one = UnreducedBDD::one(mgr);

    EXPECT_TRUE(zero.is_zero());
    EXPECT_TRUE(one.is_one());
}

TEST_F(UnreducedBDDTest, CreateNode) {
    UnreducedBDD zero = UnreducedBDD::zero(mgr);
    UnreducedBDD one = UnreducedBDD::one(mgr);

    // Create a node with var 1
    UnreducedBDD node = UnreducedBDD::node(mgr, 1, zero, one);
    EXPECT_FALSE(node.is_terminal());
    EXPECT_EQ(node.top(), 1u);
}

TEST_F(UnreducedBDDTest, Reduce) {
    UnreducedBDD zero = UnreducedBDD::zero(mgr);
    UnreducedBDD one = UnreducedBDD::one(mgr);

    // Create x1
    UnreducedBDD x1 = UnreducedBDD::node(mgr, 1, zero, one);

    // Reduce should give same result as var_bdd
    BDD reduced = x1.reduce();
    BDD x1_bdd = mgr.var_bdd(1);

    EXPECT_EQ(reduced, x1_bdd);
}

TEST_F(UnreducedBDDTest, RedundantNode) {
    UnreducedBDD one = UnreducedBDD::one(mgr);

    // Create redundant node: both children are the same
    UnreducedBDD redundant = UnreducedBDD::node(mgr, 1, one, one);

    // Reduce should eliminate redundant node
    BDD reduced = redundant.reduce();
    EXPECT_TRUE(reduced.is_one());
}

// ============== UnreducedZDD Tests ==============

class UnreducedZDDTest : public ::testing::Test {
protected:
    DDManager mgr;

    void SetUp() override {
        for (int i = 0; i < 5; ++i) {
            mgr.new_var();
        }
    }
};

TEST_F(UnreducedZDDTest, Terminals) {
    UnreducedZDD empty = UnreducedZDD::empty(mgr);
    UnreducedZDD base = UnreducedZDD::single(mgr);

    EXPECT_TRUE(empty.is_zero());
    EXPECT_TRUE(base.is_one());
}

TEST_F(UnreducedZDDTest, CreateNode) {
    UnreducedZDD empty = UnreducedZDD::empty(mgr);
    UnreducedZDD base = UnreducedZDD::single(mgr);

    UnreducedZDD node = UnreducedZDD::node(mgr, 1, empty, base);
    EXPECT_FALSE(node.is_terminal());
    EXPECT_EQ(node.top(), 1u);
}

TEST_F(UnreducedZDDTest, Reduce) {
    UnreducedZDD empty = UnreducedZDD::empty(mgr);
    UnreducedZDD base = UnreducedZDD::single(mgr);

    // Create {1}
    UnreducedZDD s1 = UnreducedZDD::node(mgr, 1, empty, base);

    ZDD reduced = s1.reduce();
    ZDD expected = ZDD::singleton(mgr, 1);

    EXPECT_EQ(reduced, expected);
}

TEST_F(UnreducedZDDTest, ZDDReduction) {
    UnreducedZDD empty = UnreducedZDD::empty(mgr);
    UnreducedZDD base = UnreducedZDD::single(mgr);

    // Create redundant ZDD node: high = empty (should be reduced away)
    UnreducedZDD redundant = UnreducedZDD::node(mgr, 1, base, empty);

    ZDD reduced = redundant.reduce();
    EXPECT_TRUE(reduced.is_one());  // Should be just base
}

// ============== QDD Tests ==============

class QDDTest : public ::testing::Test {
protected:
    DDManager mgr;

    void SetUp() override {
        for (int i = 0; i < 5; ++i) {
            mgr.new_var();
        }
    }
};

TEST_F(QDDTest, Terminals) {
    QDD zero = QDD::zero(mgr);
    QDD one = QDD::one(mgr);

    EXPECT_TRUE(zero.is_zero());
    EXPECT_TRUE(one.is_one());
}

TEST_F(QDDTest, CreateNode) {
    QDD zero = QDD::zero(mgr);
    QDD one = QDD::one(mgr);

    QDD node = QDD::node(mgr, 1, zero, one);
    EXPECT_EQ(node.top(), 1u);
}

TEST_F(QDDTest, ToBDD) {
    QDD zero = QDD::zero(mgr);
    QDD one = QDD::one(mgr);

    QDD qdd = QDD::node(mgr, 1, zero, one);
    BDD bdd = qdd.to_bdd();

    EXPECT_EQ(bdd, mgr.var_bdd(1));
}

TEST_F(QDDTest, ToZDD) {
    QDD zero = QDD::zero(mgr);
    QDD one = QDD::one(mgr);

    // This creates a node equivalent to ZDD single(1)
    QDD qdd = QDD::node(mgr, 1, zero, one);
    ZDD zdd = qdd.to_zdd();

    EXPECT_EQ(zdd, ZDD::singleton(mgr, 1));
}

// ============== SeqBDD Tests ==============

class SeqBDDTest : public ::testing::Test {
protected:
    DDManager mgr;

    void SetUp() override {
        for (int i = 0; i < 5; ++i) {
            mgr.new_var();
        }
    }
};

TEST_F(SeqBDDTest, Terminals) {
    SeqBDD empty = SeqBDD::empty(mgr);
    SeqBDD base = SeqBDD::single(mgr);

    EXPECT_EQ(empty.card(), 0.0);
    EXPECT_EQ(base.card(), 1.0);
}

TEST_F(SeqBDDTest, Single) {
    SeqBDD s1 = SeqBDD::singleton(mgr, 1);
    EXPECT_EQ(s1.card(), 1.0);
}

TEST_F(SeqBDDTest, Union) {
    SeqBDD s1 = SeqBDD::singleton(mgr, 1);
    SeqBDD s2 = SeqBDD::singleton(mgr, 2);

    SeqBDD u = s1 + s2;
    EXPECT_EQ(u.card(), 2.0);
}

TEST_F(SeqBDDTest, Push) {
    SeqBDD base = SeqBDD::single(mgr);
    SeqBDD s1 = base.push(1);

    EXPECT_EQ(s1.card(), 1.0);
    EXPECT_EQ(s1.top(), 1u);
}

// ============== BDDCT Tests ==============

class BDDCTTest : public ::testing::Test {
protected:
    DDManager mgr;

    void SetUp() override {
        for (int i = 0; i < 5; ++i) {
            mgr.new_var();
        }
    }
};

TEST_F(BDDCTTest, Alloc) {
    BDDCT ct(mgr);
    EXPECT_TRUE(ct.alloc(5, 1));
    EXPECT_EQ(ct.size(), 5);
}

TEST_F(BDDCTTest, SetCost) {
    BDDCT ct(mgr);
    ct.alloc(5, 1);

    ct.set_cost(1, 10);
    ct.set_cost(2, 20);

    EXPECT_EQ(ct.cost(1), 10);
    EXPECT_EQ(ct.cost(2), 20);
}

TEST_F(BDDCTTest, MinCost) {
    BDDCT ct(mgr);
    ct.alloc(3, 1);
    ct.set_cost(1, 5);
    ct.set_cost(2, 3);
    ct.set_cost(3, 7);

    // Create ZDD: {{1}, {2}, {3}}
    ZDD z1 = ZDD::singleton(mgr, 1);
    ZDD z2 = ZDD::singleton(mgr, 2);
    ZDD z3 = ZDD::singleton(mgr, 3);
    ZDD family = z1 + z2 + z3;

    bddcost min = ct.min_cost(family);
    EXPECT_EQ(min, 3);  // {2} has minimum cost
}

TEST_F(BDDCTTest, MaxCost) {
    BDDCT ct(mgr);
    ct.alloc(3, 1);
    ct.set_cost(1, 5);
    ct.set_cost(2, 3);
    ct.set_cost(3, 7);

    ZDD z1 = ZDD::singleton(mgr, 1);
    ZDD z2 = ZDD::singleton(mgr, 2);
    ZDD z3 = ZDD::singleton(mgr, 3);
    ZDD family = z1 + z2 + z3;

    bddcost max = ct.max_cost(family);
    EXPECT_EQ(max, 7);  // {3} has maximum cost
}

// ============== IO Tests ==============

class IOTest : public ::testing::Test {
protected:
    DDManager mgr;

    void SetUp() override {
        for (int i = 0; i < 5; ++i) {
            mgr.new_var();
        }
    }
};

TEST_F(IOTest, DetectFormat) {
    EXPECT_EQ(detect_format("test.bdd"), DDFileFormat::BINARY);
    EXPECT_EQ(detect_format("test.txt"), DDFileFormat::TEXT);
    EXPECT_EQ(detect_format("test.dot"), DDFileFormat::DOT);
}

TEST_F(IOTest, ToDotBDD) {
    BDD x1 = mgr.var_bdd(1);
    std::string dot = to_dot(x1, "test");

    EXPECT_FALSE(dot.empty());
    EXPECT_NE(dot.find("digraph"), std::string::npos);
    EXPECT_NE(dot.find("test"), std::string::npos);
}

TEST_F(IOTest, ToDotZDD) {
    ZDD s1 = ZDD::singleton(mgr, 1);
    std::string dot = to_dot(s1, "test");

    EXPECT_FALSE(dot.empty());
    EXPECT_NE(dot.find("digraph"), std::string::npos);
}

TEST_F(IOTest, ExportImportBDD) {
    BDD x1 = mgr.var_bdd(1);
    BDD x2 = mgr.var_bdd(2);
    BDD f = x1 & x2;

    std::stringstream ss;
    ExportOptions opts;
    opts.format = DDFileFormat::BINARY;

    EXPECT_TRUE(export_bdd(f, ss, opts));

    DDManager mgr2;
    mgr2.new_var();
    mgr2.new_var();

    BDD imported = import_bdd(mgr2, ss);
    EXPECT_TRUE(imported.is_valid());
}

TEST_F(IOTest, ExportImportZDD) {
    ZDD s1 = ZDD::singleton(mgr, 1);
    ZDD s2 = ZDD::singleton(mgr, 2);
    ZDD f = s1 + s2;

    std::stringstream ss;
    ExportOptions opts;
    opts.format = DDFileFormat::BINARY;

    EXPECT_TRUE(export_zdd(f, ss, opts));

    DDManager mgr2;
    mgr2.new_var();
    mgr2.new_var();

    ZDD imported = import_zdd(mgr2, ss);
    EXPECT_TRUE(imported.is_valid());
}

TEST_F(IOTest, Validation) {
    BDD x1 = mgr.var_bdd(1);
    ZDD s1 = ZDD::singleton(mgr, 1);

    EXPECT_TRUE(validate_bdd(x1));
    EXPECT_TRUE(validate_zdd(s1));

    BDD invalid;
    ZDD invalid_z;
    EXPECT_FALSE(validate_bdd(invalid));
    EXPECT_FALSE(validate_zdd(invalid_z));
}
