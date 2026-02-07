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
    ZDD base = ZDD::single(mgr);

    EXPECT_TRUE(empty.is_zero());
    EXPECT_TRUE(base.is_one());
    EXPECT_EQ(empty.card(), 0.0);
    EXPECT_EQ(base.card(), 1.0);
}

TEST_F(ZDDTest, SingleElement) {
    ZDD s1 = ZDD::singleton(mgr, 1);
    ZDD s2 = ZDD::singleton(mgr, 2);

    EXPECT_EQ(s1.card(), 1.0);
    EXPECT_EQ(s2.card(), 1.0);
    EXPECT_EQ(s1.top(), 1u);
    EXPECT_EQ(s2.top(), 2u);
}

TEST_F(ZDDTest, UnionOperation) {
    ZDD s1 = ZDD::singleton(mgr, 1);
    ZDD s2 = ZDD::singleton(mgr, 2);
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
    ZDD s1 = ZDD::singleton(mgr, 1);
    ZDD s2 = ZDD::singleton(mgr, 2);
    ZDD empty = ZDD::empty(mgr);

    // {1} & {2} = {} (empty)
    ZDD i = s1 & s2;
    EXPECT_TRUE(i.is_zero());

    // s1 & s1 = s1
    EXPECT_EQ(s1 & s1, s1);

    // {} & s1 = {}
    EXPECT_TRUE((empty & s1).is_zero());
}

TEST_F(ZDDTest, DifferenceOperation) {
    ZDD s1 = ZDD::singleton(mgr, 1);
    ZDD s2 = ZDD::singleton(mgr, 2);

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
    ZDD s1 = ZDD::singleton(mgr, 1);
    ZDD s2 = ZDD::singleton(mgr, 2);
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
    ZDD base = ZDD::single(mgr);  // {{}}

    // Toggle 1 on {{}} = {{1}}
    ZDD c = base.change(1);
    EXPECT_EQ(c, ZDD::singleton(mgr, 1));

    // Toggle 1 again = {{}}
    EXPECT_EQ(c.change(1), base);
}

TEST_F(ZDDTest, Product) {
    ZDD s1 = ZDD::singleton(mgr, 1);
    ZDD s2 = ZDD::singleton(mgr, 2);
    ZDD base = ZDD::single(mgr);

    // {1} * base = {1} (cross product with empty set)
    EXPECT_EQ(s1.join(base), s1);
    EXPECT_EQ(s1 * base, s1);  // operator* version

    // base * {1} = {1}
    EXPECT_EQ(base.join(s1), s1);
    EXPECT_EQ(base * s1, s1);  // operator* version

    // {1} join {2} = {{1,2}}
    ZDD p = s1.join(s2);
    EXPECT_EQ(p.card(), 1.0);
    EXPECT_EQ(s1 * s2, p);  // operator* version

    // Enumerate to verify
    auto sets = p.enumerate();
    ASSERT_EQ(sets.size(), 1u);
    EXPECT_EQ(sets[0].size(), 2u);
}

TEST_F(ZDDTest, Enumerate) {
    ZDD s1 = ZDD::singleton(mgr, 1);
    ZDD s2 = ZDD::singleton(mgr, 2);
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
    ZDD s1 = ZDD::singleton(mgr, 1);
    ZDD s2 = ZDD::singleton(mgr, 2);
    ZDD u = s1 + s2;

    auto oneSet = u.one_set();
    ASSERT_EQ(oneSet.size(), 1u);
    EXPECT_TRUE(oneSet[0] == 1 || oneSet[0] == 2);
}

TEST_F(ZDDTest, Card) {
    ZDD s1 = ZDD::singleton(mgr, 1);
    ZDD s2 = ZDD::singleton(mgr, 2);
    ZDD s3 = ZDD::singleton(mgr, 3);

    ZDD u = s1 + s2 + s3;  // {{1}, {2}, {3}}
    EXPECT_EQ(u.card(), 3.0);

    // Product creates combinations
    ZDD p12 = s1.join(s2);  // {{1,2}}
    EXPECT_EQ(p12.card(), 1.0);

    // Union of different sized sets
    ZDD combined = u + p12;  // {{1}, {2}, {3}, {1,2}}
    EXPECT_EQ(combined.card(), 4.0);
}

TEST_F(ZDDTest, Size) {
    ZDD s1 = ZDD::singleton(mgr, 1);
    EXPECT_EQ(s1.size(), 1u);

    ZDD s2 = ZDD::singleton(mgr, 2);
    ZDD u = s1 + s2;
    EXPECT_GE(u.size(), 1u);
}

TEST_F(ZDDTest, Support) {
    ZDD s1 = ZDD::singleton(mgr, 1);
    ZDD s2 = ZDD::singleton(mgr, 2);
    ZDD p = s1.join(s2);  // {{1,2}}

    auto supp = p.support();
    EXPECT_EQ(supp.size(), 2u);
    EXPECT_NE(std::find(supp.begin(), supp.end(), 1), supp.end());
    EXPECT_NE(std::find(supp.begin(), supp.end(), 2), supp.end());
}

TEST_F(ZDDTest, LowHigh) {
    ZDD s1 = ZDD::singleton(mgr, 1);

    // Low should be empty (no sets without element 1)
    EXPECT_TRUE(s1.low().is_zero());

    // High should be base (empty set after removing element 1)
    EXPECT_TRUE(s1.high().is_one());
}

TEST_F(ZDDTest, ComplexFamily) {
    // Create a family representing power set of {1,2}
    // P({1,2}) = {{}, {1}, {2}, {1,2}}

    ZDD base = ZDD::single(mgr);          // {{}}
    ZDD s1 = ZDD::singleton(mgr, 1);       // {{1}}
    ZDD s2 = ZDD::singleton(mgr, 2);       // {{2}}
    ZDD s12 = s1.join(s2);           // {{1,2}}

    ZDD powerset = base + s1 + s2 + s12;
    EXPECT_EQ(powerset.card(), 4.0);

    auto sets = powerset.enumerate();
    EXPECT_EQ(sets.size(), 4u);
}

// ============== New method tests ==============

TEST_F(ZDDTest, Swap) {
    ZDD s12 = ZDD::singleton(mgr, 1).join(ZDD::singleton(mgr, 2));  // {{1,2}}
    ZDD swapped = s12.swap(1, 2);
    EXPECT_EQ(swapped, s12);  // Swapping should give same result for this case

    ZDD s1 = ZDD::singleton(mgr, 1);  // {{1}}
    ZDD s2 = ZDD::singleton(mgr, 2);  // {{2}}
    ZDD u = s1 + s2;  // {{1}, {2}}

    // Swap should work correctly
    ZDD swapped_u = u.swap(1, 2);
    EXPECT_EQ(swapped_u.card(), u.card());
}

TEST_F(ZDDTest, LitLen) {
    ZDD s1 = ZDD::singleton(mgr, 1);  // {{1}}
    ZDD s2 = ZDD::singleton(mgr, 2);  // {{2}}
    ZDD s12 = s1.join(s2);      // {{1,2}}

    EXPECT_EQ(s1.lit(), 1u);   // 1 literal
    EXPECT_EQ(s12.lit(), 2u);  // 2 literals
    EXPECT_EQ(s1.len(), 1u);   // max size 1
    EXPECT_EQ(s12.len(), 2u);  // max size 2

    ZDD combined = s1 + s2 + s12;  // {{1}, {2}, {1,2}}
    EXPECT_EQ(combined.lit(), 4u);  // 1 + 1 + 2 = 4
    EXPECT_EQ(combined.len(), 2u);  // max is 2
}

TEST_F(ZDDTest, Shift) {
    ZDD s1 = ZDD::singleton(mgr, 1);  // {{1}}
    ZDD shifted = s1 << 2;
    EXPECT_EQ(shifted.top(), 3u);  // Variable 1 becomes 3

    ZDD s3 = ZDD::singleton(mgr, 3);
    ZDD rshifted = s3 >> 1;
    EXPECT_EQ(rshifted.top(), 2u);  // Variable 3 becomes 2
}

TEST_F(ZDDTest, PermitSym) {
    // Create a power set
    ZDD base = ZDD::single(mgr);
    ZDD s1 = ZDD::singleton(mgr, 1);
    ZDD s2 = ZDD::singleton(mgr, 2);
    ZDD s12 = s1.join(s2);
    ZDD powerset = base + s1 + s2 + s12;  // {{}, {1}, {2}, {1,2}}

    // permit_sym(1): keep only sets with cardinality <= 1
    ZDD filtered = powerset.permit_sym(1);
    EXPECT_EQ(filtered.card(), 3.0);  // {{}, {1}, {2}}

    // permit_sym(0): only empty set
    ZDD empty_only = powerset.permit_sym(0);
    EXPECT_EQ(empty_only.card(), 1.0);  // {{}}
}

TEST_F(ZDDTest, Always) {
    ZDD s12 = ZDD::singleton(mgr, 1).join(ZDD::singleton(mgr, 2));  // {{1,2}}
    ZDD s13 = ZDD::singleton(mgr, 1).join(ZDD::singleton(mgr, 3));  // {{1,3}}
    ZDD u = s12 + s13;  // {{1,2}, {1,3}}

    // Variable 1 is in all sets
    ZDD always_items = u.always();
    auto items = always_items.one_set();
    EXPECT_EQ(items.size(), 1u);
    EXPECT_EQ(items[0], 1u);
}

TEST_F(ZDDTest, SymChk) {
    ZDD s1 = ZDD::singleton(mgr, 1);
    ZDD s2 = ZDD::singleton(mgr, 2);
    ZDD u = s1 + s2;  // {{1}, {2}}

    // Variables 1 and 2 should be symmetric (can be swapped)
    EXPECT_EQ(u.sym_chk(1, 2), 1);

    // Check non-symmetric case
    ZDD s12 = s1.join(s2);  // {{1,2}}
    ZDD asymm = s1 + s12;  // {{1}, {1,2}}
    EXPECT_EQ(asymm.sym_chk(1, 2), 0);  // Not symmetric
}

TEST_F(ZDDTest, ImplyChk) {
    ZDD s12 = ZDD::singleton(mgr, 1).join(ZDD::singleton(mgr, 2));  // {{1,2}}

    // In {{1,2}}, 1 implies 2 and 2 implies 1
    EXPECT_EQ(s12.imply_chk(1, 2), 1);
    EXPECT_EQ(s12.imply_chk(2, 1), 1);

    ZDD s1 = ZDD::singleton(mgr, 1);
    ZDD combined = s1 + s12;  // {{1}, {1,2}}

    // 1 does not imply 2 (since {1} exists without 2)
    EXPECT_EQ(combined.imply_chk(1, 2), 0);
    // But 2 implies 1 (since every set with 2 also has 1)
    EXPECT_EQ(combined.imply_chk(2, 1), 1);
}

TEST_F(ZDDTest, IsPoly) {
    ZDD s1 = ZDD::singleton(mgr, 1);  // {{1}}
    EXPECT_EQ(s1.is_poly(), 0);     // Single element

    ZDD s2 = ZDD::singleton(mgr, 2);
    ZDD u = s1 + s2;  // {{1}, {2}}
    EXPECT_EQ(u.is_poly(), 1);  // Multiple elements
}

TEST_F(ZDDTest, Meet) {
    ZDD s12 = ZDD::singleton(mgr, 1).join(ZDD::singleton(mgr, 2));  // {{1,2}}
    ZDD s23 = ZDD::singleton(mgr, 2).join(ZDD::singleton(mgr, 3));  // {{2,3}}

    ZDD meet_result = zdd_meet(s12, s23);
    // Meet of {1,2} and {2,3} should give {{2}} (intersection of sets)
    EXPECT_EQ(meet_result.card(), 1.0);
    auto sets = meet_result.enumerate();
    ASSERT_EQ(sets.size(), 1u);
    EXPECT_EQ(sets[0].size(), 1u);
    EXPECT_EQ(sets[0][0], 2u);
}

// ============== Helper function tests ==============

TEST_F(ZDDTest, GetPowerSet) {
    ZDD ps = get_power_set(mgr, 3);  // Power set of {1,2,3}
    EXPECT_EQ(ps.card(), 8.0);       // 2^3 = 8

    std::vector<bddvar> vars = {1, 3};
    ZDD ps13 = get_power_set(mgr, vars);  // Power set of {1,3}
    EXPECT_EQ(ps13.card(), 4.0);          // 2^2 = 4
}

TEST_F(ZDDTest, GetPowerSetWithCard) {
    ZDD ps_k2 = get_power_set_with_card(mgr, 4, 2);  // Subsets of {1,2,3,4} with size 2
    EXPECT_EQ(ps_k2.card(), 6.0);  // C(4,2) = 6
}

TEST_F(ZDDTest, GetSingleSet) {
    std::vector<bddvar> vars = {1, 2, 3};
    ZDD single = get_single_set(mgr, vars);
    EXPECT_EQ(single.card(), 1.0);

    auto sets = single.enumerate();
    ASSERT_EQ(sets.size(), 1u);
    EXPECT_EQ(sets[0].size(), 3u);
}

TEST_F(ZDDTest, MakeDontCare) {
    ZDD s1 = ZDD::singleton(mgr, 1);  // {{1}}
    std::vector<bddvar> dc_vars = {2};
    ZDD dc = make_dont_care(s1, dc_vars);  // {{1}, {1,2}}
    EXPECT_EQ(dc.card(), 2.0);
}

TEST_F(ZDDTest, IsMember) {
    ZDD s12 = ZDD::singleton(mgr, 1).join(ZDD::singleton(mgr, 2));  // {{1,2}}
    ZDD s1 = ZDD::singleton(mgr, 1);
    ZDD combined = s1 + s12;  // {{1}, {1,2}}

    std::vector<bddvar> test1 = {1};
    std::vector<bddvar> test12 = {1, 2};
    std::vector<bddvar> test2 = {2};

    EXPECT_TRUE(is_member(combined, test1));
    EXPECT_TRUE(is_member(combined, test12));
    EXPECT_FALSE(is_member(combined, test2));
}

TEST_F(ZDDTest, WeightFilter) {
    ZDD ps = get_power_set(mgr, 3);  // Power set of {1,2,3}
    std::vector<long long> weights = {1, 2, 3};  // var 1 has weight 1, etc.

    // Filter by weight <= 3
    ZDD filtered = weight_le(ps, 3, weights);
    // Sets with weight <= 3: {}, {1}, {2}, {3}, {1,2}
    EXPECT_EQ(filtered.card(), 5.0);

    // Filter by weight == 3
    ZDD eq3 = weight_eq(ps, 3, weights);
    // Sets with weight == 3: {3}, {1,2}
    EXPECT_EQ(eq3.card(), 2.0);
}

// Test ZDD operations with variable level management
TEST(ZDDLevelTest, OperationsWithDifferentLevels) {
    DDManager mgr;

    // Create variables with default ordering
    bddvar v1 = mgr.new_var();  // var=1, lev=1
    bddvar v2 = mgr.new_var();  // var=2, lev=2

    // Insert variable at level 1, shifting v1 to lev=2, v2 to lev=3
    bddvar v3 = mgr.new_var_of_lev(1);  // var=3, lev=1

    // Now levels: v3 (lev=1) < v1 (lev=2) < v2 (lev=3)
    // SAPPOROBDD convention: higher level = closer to root
    // Create ZDDs using these variables
    ZDD s1 = ZDD::singleton(mgr, v1);  // {{v1}}
    ZDD s2 = ZDD::singleton(mgr, v2);  // {{v2}}
    ZDD s3 = ZDD::singleton(mgr, v3);  // {{v3}}

    // Test union - should respect level ordering
    ZDD u13 = s1 + s3;  // {{v1}, {v3}}
    EXPECT_EQ(u13.card(), 2.0);
    EXPECT_EQ(u13.top(), v1);  // v1 has higher level (lev=2) than v3 (lev=1), so v1 at root

    ZDD u12 = s1 + s2;  // {{v1}, {v2}}
    EXPECT_EQ(u12.card(), 2.0);
    EXPECT_EQ(u12.top(), v2);  // v2 has higher level (lev=3) than v1 (lev=2), so v2 at root

    // Test intersection
    ZDD p13 = s1 & s3;  // Empty (different singleton sets)
    EXPECT_TRUE(p13.is_zero());

    // Test join
    ZDD prod = s3.join(s1);  // {{v3, v1}}
    EXPECT_EQ(prod.card(), 1.0);
    EXPECT_EQ(prod.top(), v1);  // v1 at root (lev=2 > lev=1)
    EXPECT_EQ(s3 * s1, prod);  // operator* version
}

TEST(ZDDLevelTest, MeetWithDifferentLevels) {
    DDManager mgr;

    bddvar v1 = mgr.new_var();  // var=1, lev=1
    bddvar v2 = mgr.new_var();  // var=2, lev=2

    // Insert v3 at level 1
    bddvar v3 = mgr.new_var_of_lev(1);  // var=3, lev=1; now v1 at lev=2, v2 at lev=3

    // Create sets
    ZDD s1 = ZDD::singleton(mgr, v1);  // {{v1}}
    ZDD s3 = ZDD::singleton(mgr, v3);  // {{v3}}
    ZDD base = ZDD::single(mgr);      // {{}}

    // Meet of {{v1}} and {{v3}} should be {{}}
    ZDD m = zdd_meet(s1, s3);
    EXPECT_EQ(m, base);

    // Meet of {{v1}} and {{v1}} should be {{v1}}
    ZDD m2 = zdd_meet(s1, s1);
    EXPECT_EQ(m2, s1);
}

#if defined(SBDD2_HAS_GMP) || defined(SBDD2_HAS_BIGINT)
TEST(ZDDExactCountTest, MatchesCard) {
    DDManager mgr;

    // Empty set
    ZDD empty = ZDD::empty(mgr);
    EXPECT_EQ(empty.exact_count(), "0");

    // Base (single empty set)
    ZDD base = ZDD::single(mgr);
    EXPECT_EQ(base.exact_count(), "1");

    // Single element
    mgr.new_var();
    ZDD s1 = ZDD::singleton(mgr, 1);
    EXPECT_EQ(s1.exact_count(), std::to_string(static_cast<long long>(s1.card())));

    // Union of two singletons
    mgr.new_var();
    ZDD s2 = ZDD::singleton(mgr, 2);
    ZDD u = s1 + s2;
    EXPECT_EQ(u.exact_count(), std::to_string(static_cast<long long>(u.card())));

    // Power set of 3 variables = 2^3 = 8 sets
    mgr.new_var();
    ZDD ps = get_power_set(mgr, 3);
    EXPECT_EQ(ps.exact_count(), std::to_string(static_cast<long long>(ps.card())));
}

TEST(ZDDExactCountTest, LargeCount) {
    DDManager mgr;

    // Create power set with moderate number of variables
    // Power set of 20 variables = 2^20 = 1048576 sets
    for (int i = 0; i < 20; ++i) {
        mgr.new_var();
    }

    ZDD ps = get_power_set(mgr, 20);

    // Verify exact_count returns valid result
    std::string exact_str = ps.exact_count();
    EXPECT_FALSE(exact_str.empty());
    EXPECT_NE(exact_str, "0");

    // Verify consistency (deterministic)
    EXPECT_EQ(ps.exact_count(), exact_str);

    // 2^20 = 1048576
    EXPECT_EQ(exact_str, "1048576");
}
#endif

// ============== ZDD Index Tests ==============

class ZDDIndexTest : public ::testing::Test {
protected:
    DDManager mgr;

    void SetUp() override {
        for (int i = 0; i < 5; ++i) {
            mgr.new_var();
        }
    }
};

TEST_F(ZDDIndexTest, EmptyAndBase) {
    ZDD empty = ZDD::empty(mgr);
    ZDD base = ZDD::single(mgr);

    // Empty set
    EXPECT_FALSE(empty.has_index());
    EXPECT_EQ(empty.indexed_count(), 0.0);
    EXPECT_EQ(empty.index_height(), 0);
    EXPECT_EQ(empty.index_size(), 0u);

    // Base (1-terminal)
    EXPECT_EQ(base.indexed_count(), 1.0);
    EXPECT_EQ(base.index_height(), 0);
    EXPECT_EQ(base.index_size(), 0u);
}

TEST_F(ZDDIndexTest, SingleElement) {
    ZDD s1 = ZDD::singleton(mgr, 1);

    EXPECT_EQ(s1.indexed_count(), 1.0);
    EXPECT_EQ(s1.index_height(), 1);
    EXPECT_EQ(s1.index_size(), 1u);
    EXPECT_EQ(s1.index_size_at_level(1), 1u);
    EXPECT_TRUE(s1.has_index());
}

TEST_F(ZDDIndexTest, TwoElements) {
    ZDD s1 = ZDD::singleton(mgr, 1);
    ZDD s2 = ZDD::singleton(mgr, 2);
    ZDD u = s1 + s2;  // {{1}, {2}}

    EXPECT_EQ(u.card(), 2.0);
    EXPECT_EQ(u.indexed_count(), 2.0);
}

TEST_F(ZDDIndexTest, MultipleElements) {
    ZDD s1 = ZDD::singleton(mgr, 1);
    ZDD s2 = ZDD::singleton(mgr, 2);
    ZDD s3 = ZDD::singleton(mgr, 3);
    ZDD u = s1 + s2 + s3;  // {{1}, {2}, {3}}

    EXPECT_EQ(u.card(), 3.0);
    EXPECT_EQ(u.indexed_count(), 3.0);
    EXPECT_EQ(u.index_height(), 3);
}

TEST_F(ZDDIndexTest, PowerSet) {
    ZDD ps = get_power_set(mgr, 3);  // 2^3 = 8 sets

    EXPECT_EQ(ps.indexed_count(), 8.0);
    EXPECT_EQ(ps.index_height(), 3);
}

TEST_F(ZDDIndexTest, MatchesCard) {
    ZDD s1 = ZDD::singleton(mgr, 1);
    ZDD s2 = ZDD::singleton(mgr, 2);
    ZDD s3 = ZDD::singleton(mgr, 3);

    // Various combinations
    ZDD u12 = s1 + s2;
    ZDD u123 = s1 + s2 + s3;
    ZDD prod = s1.join(s2);
    ZDD complex = (s1 + s2).join(s3) + ZDD::single(mgr);

    EXPECT_EQ(u12.indexed_count(), u12.card());
    EXPECT_EQ(u123.indexed_count(), u123.card());
    EXPECT_EQ(prod.indexed_count(), prod.card());
    EXPECT_EQ(complex.indexed_count(), complex.card());
}

TEST_F(ZDDIndexTest, ClearIndex) {
    ZDD ps = get_power_set(mgr, 3);

    // Build index
    EXPECT_EQ(ps.indexed_count(), 8.0);
    EXPECT_TRUE(ps.has_index());

    // Clear index
    ps.clear_index();
    EXPECT_FALSE(ps.has_index());

    // Rebuild automatically
    EXPECT_EQ(ps.indexed_count(), 8.0);
    EXPECT_TRUE(ps.has_index());
}

TEST_F(ZDDIndexTest, CopyDoesNotShareIndex) {
    ZDD ps = get_power_set(mgr, 3);

    // Build index on original
    ps.build_index();
    EXPECT_TRUE(ps.has_index());

    // Copy should not have index
    ZDD ps_copy = ps;
    EXPECT_FALSE(ps_copy.has_index());

    // But should give same count
    EXPECT_EQ(ps_copy.indexed_count(), ps.indexed_count());
    EXPECT_TRUE(ps_copy.has_index());
}

#if defined(SBDD2_HAS_GMP) || defined(SBDD2_HAS_BIGINT)
TEST_F(ZDDIndexTest, ExactCountMatches) {
    ZDD ps = get_power_set(mgr, 3);

    // Compare indexed_exact_count with exact_count
    EXPECT_EQ(ps.indexed_exact_count(), ps.exact_count());
    EXPECT_TRUE(ps.has_exact_index());

    // Clear and rebuild
    ps.clear_index();
    EXPECT_FALSE(ps.has_exact_index());
    EXPECT_EQ(ps.indexed_exact_count(), "8");
    EXPECT_TRUE(ps.has_exact_index());
}

TEST_F(ZDDIndexTest, ExactCountEmpty) {
    ZDD empty = ZDD::empty(mgr);
    EXPECT_EQ(empty.indexed_exact_count(), "0");

    ZDD base = ZDD::single(mgr);
    EXPECT_EQ(base.indexed_exact_count(), "1");
}
#endif

// ============== Dictionary Order Tests ==============

TEST_F(ZDDIndexTest, OrderOfEmptySet) {
    ZDD base = ZDD::single(mgr);  // {{}}
    std::set<bddvar> empty_set;
    EXPECT_EQ(base.order_of(empty_set), 0);

    std::set<bddvar> non_empty = {1};
    EXPECT_EQ(base.order_of(non_empty), -1);
}

TEST_F(ZDDIndexTest, OrderOfSingleElement) {
    ZDD s1 = ZDD::singleton(mgr, 1);  // {{1}}
    std::set<bddvar> set1 = {1};
    std::set<bddvar> empty_set;

    EXPECT_EQ(s1.order_of(set1), 0);
    EXPECT_EQ(s1.order_of(empty_set), -1);  // Empty set not in {{1}}
}

TEST_F(ZDDIndexTest, OrderOfMultipleSets) {
    ZDD s1 = ZDD::singleton(mgr, 1);
    ZDD s2 = ZDD::singleton(mgr, 2);
    ZDD u = s1 + s2;  // {{1}, {2}}

    std::set<bddvar> set1 = {1};
    std::set<bddvar> set2 = {2};

    // Both should have valid order numbers
    int64_t order1 = u.order_of(set1);
    int64_t order2 = u.order_of(set2);

    EXPECT_GE(order1, 0);
    EXPECT_GE(order2, 0);
    EXPECT_NE(order1, order2);  // Different sets should have different orders
}

TEST_F(ZDDIndexTest, GetSetBasic) {
    ZDD s1 = ZDD::singleton(mgr, 1);  // {{1}}
    std::set<bddvar> result = s1.get_set(0);
    EXPECT_EQ(result.size(), 1u);
    EXPECT_EQ(result.count(1), 1u);
}

TEST_F(ZDDIndexTest, GetSetRoundTrip) {
    ZDD ps = get_power_set(mgr, 3);  // Power set of {1,2,3}

    // For each order, get_set and order_of should be inverses
    double count = ps.indexed_count();
    for (int64_t i = 0; i < static_cast<int64_t>(count); ++i) {
        std::set<bddvar> s = ps.get_set(i);
        int64_t order = ps.order_of(s);
        EXPECT_EQ(order, i) << "Failed for order " << i;
    }
}

TEST_F(ZDDIndexTest, OrderOfAllPowerSetElements) {
    ZDD ps = get_power_set(mgr, 3);  // 8 sets

    // All orders should be unique and in range [0, 7]
    std::set<int64_t> orders;
    auto sets = ps.enumerate();
    for (const auto& vec : sets) {
        std::set<bddvar> s(vec.begin(), vec.end());
        int64_t order = ps.order_of(s);
        EXPECT_GE(order, 0);
        EXPECT_LT(order, 8);
        orders.insert(order);
    }
    EXPECT_EQ(orders.size(), 8u);  // All unique
}

#if defined(SBDD2_HAS_GMP) || defined(SBDD2_HAS_BIGINT)
TEST_F(ZDDIndexTest, ExactOrderOfRoundTrip) {
    ZDD ps = get_power_set(mgr, 3);

    // Get all sets via enumerate
    auto sets = ps.enumerate();
    for (const auto& vec : sets) {
        std::set<bddvar> s(vec.begin(), vec.end());
        std::string order_str = ps.exact_order_of(s);
        EXPECT_NE(order_str, "-1");

        std::set<bddvar> s2 = ps.exact_get_set(order_str);
        EXPECT_EQ(s, s2);
    }
}
#endif

// ============== Weight Optimization Tests ==============

TEST_F(ZDDIndexTest, MaxWeightSimple) {
    ZDD s1 = ZDD::singleton(mgr, 1);
    ZDD s2 = ZDD::singleton(mgr, 2);
    ZDD u = s1 + s2;  // {{1}, {2}}

    std::vector<int64_t> weights = {0, 10, 20};  // weight[1]=10, weight[2]=20
    std::set<bddvar> result;

    int64_t max_w = u.max_weight(weights, result);
    EXPECT_EQ(max_w, 20);
    EXPECT_EQ(result.size(), 1u);
    EXPECT_EQ(result.count(2), 1u);
}

TEST_F(ZDDIndexTest, MinWeightSimple) {
    ZDD s1 = ZDD::singleton(mgr, 1);
    ZDD s2 = ZDD::singleton(mgr, 2);
    ZDD u = s1 + s2;  // {{1}, {2}}

    std::vector<int64_t> weights = {0, 10, 20};
    std::set<bddvar> result;

    int64_t min_w = u.min_weight(weights, result);
    EXPECT_EQ(min_w, 10);
    EXPECT_EQ(result.size(), 1u);
    EXPECT_EQ(result.count(1), 1u);
}

TEST_F(ZDDIndexTest, MaxWeightWithEmptySet) {
    ZDD base = ZDD::single(mgr);  // {{}}
    ZDD s1 = ZDD::singleton(mgr, 1);
    ZDD u = base + s1;  // {{}, {1}}

    std::vector<int64_t> weights = {0, 10};
    std::set<bddvar> result;

    int64_t max_w = u.max_weight(weights, result);
    EXPECT_EQ(max_w, 10);
    EXPECT_EQ(result.size(), 1u);
    EXPECT_EQ(result.count(1), 1u);

    int64_t min_w = u.min_weight(weights, result);
    EXPECT_EQ(min_w, 0);
    EXPECT_EQ(result.size(), 0u);  // Empty set
}

TEST_F(ZDDIndexTest, MaxWeightPowerSet) {
    ZDD ps = get_power_set(mgr, 3);  // Power set of {1,2,3}

    std::vector<int64_t> weights = {0, 1, 2, 3};  // weight[i]=i
    std::set<bddvar> result;

    // Max weight should be {1,2,3} with weight 1+2+3=6
    int64_t max_w = ps.max_weight(weights, result);
    EXPECT_EQ(max_w, 6);
    EXPECT_EQ(result.size(), 3u);
    EXPECT_EQ(result.count(1), 1u);
    EXPECT_EQ(result.count(2), 1u);
    EXPECT_EQ(result.count(3), 1u);

    // Min weight should be {} with weight 0
    int64_t min_w = ps.min_weight(weights, result);
    EXPECT_EQ(min_w, 0);
    EXPECT_EQ(result.size(), 0u);
}

TEST_F(ZDDIndexTest, SumWeight) {
    ZDD s1 = ZDD::singleton(mgr, 1);  // {{1}}
    ZDD s2 = ZDD::singleton(mgr, 2);  // {{2}}
    ZDD u = s1 + s2;  // {{1}, {2}}

    std::vector<int64_t> weights = {0, 10, 20};
    // Sum = weight({1}) + weight({2}) = 10 + 20 = 30
    EXPECT_EQ(u.sum_weight(weights), 30);
}

TEST_F(ZDDIndexTest, SumWeightPowerSet) {
    ZDD ps = get_power_set(mgr, 2);  // {{}, {1}, {2}, {1,2}}

    std::vector<int64_t> weights = {0, 1, 2};
    // Sum = 0 + 1 + 2 + (1+2) = 6
    EXPECT_EQ(ps.sum_weight(weights), 6);
}

TEST_F(ZDDIndexTest, NegativeWeights) {
    ZDD ps = get_power_set(mgr, 2);  // {{}, {1}, {2}, {1,2}}

    std::vector<int64_t> weights = {0, -5, 10};
    std::set<bddvar> result;

    // Max: {2} with weight 10
    int64_t max_w = ps.max_weight(weights, result);
    EXPECT_EQ(max_w, 10);
    EXPECT_EQ(result.size(), 1u);
    EXPECT_EQ(result.count(2), 1u);

    // Min: {1} with weight -5
    int64_t min_w = ps.min_weight(weights, result);
    EXPECT_EQ(min_w, -5);
    EXPECT_EQ(result.size(), 1u);
    EXPECT_EQ(result.count(1), 1u);
}

#if defined(SBDD2_HAS_GMP) || defined(SBDD2_HAS_BIGINT)
TEST_F(ZDDIndexTest, ExactSumWeight) {
    ZDD ps = get_power_set(mgr, 3);

    std::vector<int64_t> weights = {0, 1, 2, 3};
    // Compare with regular sum_weight
    EXPECT_EQ(ps.exact_sum_weight(weights), std::to_string(ps.sum_weight(weights)));
}
#endif

// ============== Random Sampling Tests ==============

TEST_F(ZDDIndexTest, SampleRandomlyEmpty) {
    ZDD empty = ZDD::empty(mgr);

    std::mt19937 rng(42);
    std::set<bddvar> result = empty.sample_randomly(rng);
    EXPECT_TRUE(result.empty());
}

TEST_F(ZDDIndexTest, SampleRandomlyBase) {
    ZDD base = ZDD::single(mgr);

    std::mt19937 rng(42);
    std::set<bddvar> result = base.sample_randomly(rng);
    EXPECT_TRUE(result.empty());  // Base contains only the empty set
}

TEST_F(ZDDIndexTest, SampleRandomlySingle) {
    ZDD s1 = ZDD::singleton(mgr, 1);  // {{1}}

    std::mt19937 rng(42);
    std::set<bddvar> result = s1.sample_randomly(rng);
    EXPECT_EQ(result.size(), 1u);
    EXPECT_EQ(result.count(1), 1u);
}

TEST_F(ZDDIndexTest, SampleRandomlyTwoSets) {
    ZDD s1 = ZDD::singleton(mgr, 1);
    ZDD s2 = ZDD::singleton(mgr, 2);
    ZDD u = s1 + s2;  // {{1}, {2}}

    std::mt19937 rng(42);

    // Sample multiple times and verify we get valid sets
    std::set<std::set<bddvar>> seen;
    for (int i = 0; i < 100; ++i) {
        std::set<bddvar> result = u.sample_randomly(rng);
        EXPECT_EQ(result.size(), 1u);
        EXPECT_TRUE(result.count(1) == 1 || result.count(2) == 1);
        seen.insert(result);
    }
    // With 100 samples from 2 sets, we should see both
    EXPECT_EQ(seen.size(), 2u);
}

TEST_F(ZDDIndexTest, SampleRandomlyPowerSet) {
    ZDD ps = get_power_set(mgr, 3);  // 8 sets

    std::mt19937 rng(42);

    // All sampled sets should be valid members
    auto all_sets = ps.enumerate();
    std::set<std::set<bddvar>> valid_sets;
    for (const auto& vec : all_sets) {
        valid_sets.insert(std::set<bddvar>(vec.begin(), vec.end()));
    }

    for (int i = 0; i < 50; ++i) {
        std::set<bddvar> result = ps.sample_randomly(rng);
        EXPECT_TRUE(valid_sets.count(result) > 0) << "Sampled invalid set";
    }
}

TEST_F(ZDDIndexTest, SampleRandomlyDistribution) {
    ZDD ps = get_power_set(mgr, 2);  // 4 sets: {}, {1}, {2}, {1,2}

    std::mt19937 rng(12345);

    // Sample many times and check distribution
    std::map<std::set<bddvar>, int> counts;
    const int N = 1000;
    for (int i = 0; i < N; ++i) {
        std::set<bddvar> result = ps.sample_randomly(rng);
        counts[result]++;
    }

    // Each set should appear roughly N/4 = 250 times
    // Allow significant deviation due to randomness
    EXPECT_EQ(counts.size(), 4u);  // All 4 sets should appear
    for (const auto& kv : counts) {
        EXPECT_GT(kv.second, N/10);  // At least 10% each
    }
}

#if defined(SBDD2_HAS_GMP) || defined(SBDD2_HAS_BIGINT)
TEST_F(ZDDIndexTest, ExactSampleRandomlyPowerSet) {
    ZDD ps = get_power_set(mgr, 3);  // 8 sets

    std::mt19937 rng(42);

    // All sampled sets should be valid members
    auto all_sets = ps.enumerate();
    std::set<std::set<bddvar>> valid_sets;
    for (const auto& vec : all_sets) {
        valid_sets.insert(std::set<bddvar>(vec.begin(), vec.end()));
    }

    for (int i = 0; i < 50; ++i) {
        std::set<bddvar> result = ps.exact_sample_randomly(rng);
        EXPECT_TRUE(valid_sets.count(result) > 0) << "Sampled invalid set";
    }
}
#endif

// ============== Iterator Tests ==============

TEST_F(ZDDIndexTest, DictIteratorEmpty) {
    ZDD empty = ZDD::empty(mgr);

    int count = 0;
    for (auto it = empty.dict_begin(); it != empty.dict_end(); ++it) {
        ++count;
    }
    EXPECT_EQ(count, 0);
}

TEST_F(ZDDIndexTest, DictIteratorBase) {
    ZDD base = ZDD::single(mgr);  // {{}}

    std::vector<std::set<bddvar>> results;
    for (auto it = base.dict_begin(); it != base.dict_end(); ++it) {
        results.push_back(*it);
    }

    EXPECT_EQ(results.size(), 1u);
    EXPECT_TRUE(results[0].empty());  // Empty set
}

TEST_F(ZDDIndexTest, DictIteratorSingle) {
    ZDD s1 = ZDD::singleton(mgr, 1);  // {{1}}

    std::vector<std::set<bddvar>> results;
    for (auto it = s1.dict_begin(); it != s1.dict_end(); ++it) {
        results.push_back(*it);
    }

    EXPECT_EQ(results.size(), 1u);
    EXPECT_EQ(results[0].size(), 1u);
    EXPECT_EQ(results[0].count(1), 1u);
}

TEST_F(ZDDIndexTest, DictIteratorPowerSet) {
    ZDD ps = get_power_set(mgr, 3);  // 8 sets

    std::vector<std::set<bddvar>> results;
    for (auto it = ps.dict_begin(); it != ps.dict_end(); ++it) {
        results.push_back(*it);
    }

    EXPECT_EQ(results.size(), 8u);

    // Verify all sets are valid and unique
    std::set<std::set<bddvar>> seen(results.begin(), results.end());
    EXPECT_EQ(seen.size(), 8u);

    // Verify order matches get_set
    for (size_t i = 0; i < results.size(); ++i) {
        std::set<bddvar> expected = ps.get_set(static_cast<int64_t>(i));
        EXPECT_EQ(results[i], expected) << "Mismatch at index " << i;
    }
}

TEST_F(ZDDIndexTest, DictReverseIterator) {
    ZDD ps = get_power_set(mgr, 3);  // 8 sets

    std::vector<std::set<bddvar>> results;
    for (auto it = ps.dict_rbegin(); it != ps.dict_rend(); ++it) {
        results.push_back(*it);
    }

    EXPECT_EQ(results.size(), 8u);

    // Verify reverse order
    for (size_t i = 0; i < results.size(); ++i) {
        std::set<bddvar> expected = ps.get_set(static_cast<int64_t>(7 - i));
        EXPECT_EQ(results[i], expected) << "Mismatch at index " << i;
    }
}

TEST_F(ZDDIndexTest, WeightMinIterator) {
    ZDD ps = get_power_set(mgr, 3);  // 8 sets

    std::vector<int64_t> weights = {0, 1, 2, 3};  // weight[i] = i

    std::vector<std::set<bddvar>> results;
    std::vector<int64_t> weight_values;
    for (auto it = ps.weight_min_begin(weights); it != ps.weight_min_end(); ++it) {
        std::set<bddvar> s = *it;
        results.push_back(s);
        int64_t w = 0;
        for (bddvar v : s) {
            w += weights[v];
        }
        weight_values.push_back(w);
    }

    EXPECT_EQ(results.size(), 8u);

    // Verify weights are non-decreasing
    for (size_t i = 1; i < weight_values.size(); ++i) {
        EXPECT_GE(weight_values[i], weight_values[i-1])
            << "Weight decreased at index " << i;
    }

    // First should be empty set (weight 0)
    EXPECT_TRUE(results[0].empty());
    EXPECT_EQ(weight_values[0], 0);
}

TEST_F(ZDDIndexTest, WeightMaxIterator) {
    ZDD ps = get_power_set(mgr, 3);  // 8 sets

    std::vector<int64_t> weights = {0, 1, 2, 3};

    std::vector<std::set<bddvar>> results;
    std::vector<int64_t> weight_values;
    for (auto it = ps.weight_max_begin(weights); it != ps.weight_max_end(); ++it) {
        std::set<bddvar> s = *it;
        results.push_back(s);
        int64_t w = 0;
        for (bddvar v : s) {
            w += weights[v];
        }
        weight_values.push_back(w);
    }

    EXPECT_EQ(results.size(), 8u);

    // Verify weights are non-increasing
    for (size_t i = 1; i < weight_values.size(); ++i) {
        EXPECT_LE(weight_values[i], weight_values[i-1])
            << "Weight increased at index " << i;
    }

    // First should be {1,2,3} (weight 6)
    EXPECT_EQ(results[0].size(), 3u);
    EXPECT_EQ(weight_values[0], 6);
}

TEST_F(ZDDIndexTest, RandomIterator) {
    ZDD ps = get_power_set(mgr, 3);  // 8 sets

    std::mt19937 rng(42);

    std::vector<std::set<bddvar>> results;
    for (auto it = ps.random_begin(rng); it != ps.random_end(); ++it) {
        results.push_back(*it);
    }

    // Should enumerate all 8 sets exactly once
    EXPECT_EQ(results.size(), 8u);

    // All sets should be unique
    std::set<std::set<bddvar>> seen(results.begin(), results.end());
    EXPECT_EQ(seen.size(), 8u);

    // All sets should be valid members of the power set
    auto all_sets = ps.enumerate();
    std::set<std::set<bddvar>> valid_sets;
    for (const auto& vec : all_sets) {
        valid_sets.insert(std::set<bddvar>(vec.begin(), vec.end()));
    }
    for (const auto& s : results) {
        EXPECT_TRUE(valid_sets.count(s) > 0);
    }
}

TEST_F(ZDDIndexTest, WeightIteratorSmall) {
    ZDD s1 = ZDD::singleton(mgr, 1);
    ZDD s2 = ZDD::singleton(mgr, 2);
    ZDD u = s1 + s2;  // {{1}, {2}}

    std::vector<int64_t> weights = {0, 10, 20};

    // Min iterator
    std::vector<std::set<bddvar>> min_results;
    for (auto it = u.weight_min_begin(weights); it != u.weight_min_end(); ++it) {
        min_results.push_back(*it);
    }

    EXPECT_EQ(min_results.size(), 2u);
    // First should be {1} with weight 10
    EXPECT_EQ(min_results[0].size(), 1u);
    EXPECT_EQ(min_results[0].count(1), 1u);

    // Max iterator
    std::vector<std::set<bddvar>> max_results;
    for (auto it = u.weight_max_begin(weights); it != u.weight_max_end(); ++it) {
        max_results.push_back(*it);
    }

    EXPECT_EQ(max_results.size(), 2u);
    // First should be {2} with weight 20
    EXPECT_EQ(max_results[0].size(), 1u);
    EXPECT_EQ(max_results[0].count(2), 1u);
}
