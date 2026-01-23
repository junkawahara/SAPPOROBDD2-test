# Tests for SAPPOROBDD 2.0

# Find GTest
find_package(GTest REQUIRED)

# Include directories
include_directories(${GTEST_INCLUDE_DIRS})

# Test executable
add_executable(sbdd2_tests
    test_basic.cpp
    test_bdd.cpp
    test_zdd.cpp
    test_extended.cpp
    test_tdzdd.cpp
    test_tdzdd_dfs.cpp
    test_mtbdd.cpp
    test_mtzdd.cpp
    test_mvzdd.cpp
    test_mvbdd.cpp
)

# Link against our library and GTest
target_link_libraries(sbdd2_tests
    sbdd2_static
    GTest::GTest
    GTest::Main
    Threads::Threads
)

# Add tests
add_test(NAME sbdd2_tests COMMAND sbdd2_tests)
