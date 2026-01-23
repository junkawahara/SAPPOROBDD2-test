/**
 * @file tictactoe.cpp
 * @brief 4x4x4 Tic-Tac-Toe Draw Counter using BDD
 * @copyright MIT License
 *
 * This example is based on the BDD Benchmark suite by Steffan Sølvsten
 * (https://github.com/ssoelvsten/bdd-benchmark), which is also MIT licensed.
 *
 * The problem: Count the number of draw positions in 4x4x4 Tic-Tac-Toe
 * where exactly N crosses (X) are placed and no player has won.
 *
 * A 4x4x4 cube has 64 positions and 76 winning lines. We count configurations
 * where:
 * - Exactly N positions have X
 * - No winning line is completely filled with X or completely empty of X
 */

#include <sbdd2/sbdd2.hpp>

#include <array>
#include <cassert>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace sbdd2;

// Number of crosses (default 20)
static int N = 20;

////////////////////////////////////////////////////////////////////////////////
// Helper functions
////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Convert 3D position (i,j,k) to BDD variable label
 *
 * 4x4x4 cube: positions are numbered 1 to 64
 */
inline int label_of_position(int i, int j, int k) {
    assert(i >= 0 && i < 4);
    assert(j >= 0 && j < 4);
    assert(k >= 0 && k < 4);
    return (4 * 4 * i) + (4 * j) + k + 1;  // +1 for 1-indexed variables
}

////////////////////////////////////////////////////////////////////////////////
// Constraint lines (winning lines in 4x4x4 Tic-Tac-Toe)
////////////////////////////////////////////////////////////////////////////////

std::vector<std::array<int, 4>> lines;

/**
 * @brief Construct all 76 winning lines in a 4x4x4 cube
 */
void construct_lines() {
    // 4 planes and the rows in these (16 lines)
    for (int i = 0; i < 4; i++) {
        for (int j = 0; j < 4; j++) {
            lines.push_back({
                label_of_position(i, j, 0),
                label_of_position(i, j, 1),
                label_of_position(i, j, 2),
                label_of_position(i, j, 3)
            });
        }
    }

    // 4 planes and a diagonal within (4 lines)
    for (int i = 0; i < 4; i++) {
        lines.push_back({
            label_of_position(i, 0, 3),
            label_of_position(i, 1, 2),
            label_of_position(i, 2, 1),
            label_of_position(i, 3, 0)
        });
    }

    // 4 planes, columns (16 lines)
    for (int i = 0; i < 4; i++) {
        for (int k = 0; k < 4; k++) {
            lines.push_back({
                label_of_position(i, 0, k),
                label_of_position(i, 1, k),
                label_of_position(i, 2, k),
                label_of_position(i, 3, k)
            });
        }
    }

    // 4 planes and the other diagonal within (4 lines)
    for (int i = 0; i < 4; i++) {
        lines.push_back({
            label_of_position(i, 0, 0),
            label_of_position(i, 1, 1),
            label_of_position(i, 2, 2),
            label_of_position(i, 3, 3)
        });
    }

    // Diagonal of the entire cube (2 lines)
    lines.push_back({
        label_of_position(0, 3, 3),
        label_of_position(1, 2, 2),
        label_of_position(2, 1, 1),
        label_of_position(3, 0, 0)
    });
    lines.push_back({
        label_of_position(0, 3, 0),
        label_of_position(1, 2, 1),
        label_of_position(2, 1, 2),
        label_of_position(3, 0, 3)
    });

    // Diagonals in vertical planes (8 lines)
    for (int j = 0; j < 4; j++) {
        lines.push_back({
            label_of_position(0, j, 3),
            label_of_position(1, j, 2),
            label_of_position(2, j, 1),
            label_of_position(3, j, 0)
        });
    }

    // 16 vertical lines
    for (int j = 0; j < 4; j++) {
        for (int k = 0; k < 4; k++) {
            lines.push_back({
                label_of_position(0, j, k),
                label_of_position(1, j, k),
                label_of_position(2, j, k),
                label_of_position(3, j, k)
            });
        }
    }

    // More diagonals in vertical planes (4 lines)
    for (int j = 0; j < 4; j++) {
        lines.push_back({
            label_of_position(0, j, 0),
            label_of_position(1, j, 1),
            label_of_position(2, j, 2),
            label_of_position(3, j, 3)
        });
    }

    // Diagonals through the cube (8 lines)
    for (int k = 0; k < 4; k++) {
        lines.push_back({
            label_of_position(0, 3, k),
            label_of_position(1, 2, k),
            label_of_position(2, 1, k),
            label_of_position(3, 0, k)
        });
    }
    for (int k = 0; k < 4; k++) {
        lines.push_back({
            label_of_position(0, 0, k),
            label_of_position(1, 1, k),
            label_of_position(2, 2, k),
            label_of_position(3, 3, k)
        });
    }

    // The 4 space diagonals (2 more lines)
    lines.push_back({
        label_of_position(0, 0, 3),
        label_of_position(1, 1, 2),
        label_of_position(2, 2, 1),
        label_of_position(3, 3, 0)
    });
    lines.push_back({
        label_of_position(0, 0, 0),
        label_of_position(1, 1, 1),
        label_of_position(2, 2, 2),
        label_of_position(3, 3, 3)
    });
}

////////////////////////////////////////////////////////////////////////////////
// BDD Construction
////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Build BDD for "exactly N variables are true" among variables 1..64
 *
 * Uses a recursive approach building BDD bottom-up.
 */
BDD construct_exactly_n(DDManager& mgr, int target_n) {
    const int num_vars = 64;

    // dp[count] = BDD representing "exactly count variables are true among remaining vars"
    // Start from the bottom (var 64) and work up to var 1

    // Initialize: at the "bottom" (after all variables), we need exactly 0 more
    std::vector<BDD> dp(target_n + 2, mgr.bdd_zero());
    dp[0] = mgr.bdd_one();  // Need 0 more true variables = success

    // Process variables from 64 down to 1
    for (int v = num_vars; v >= 1; v--) {
        BDD var = mgr.var_bdd(v);
        std::vector<BDD> new_dp(target_n + 2, mgr.bdd_zero());

        // After processing variable v, we have (v-1) variables remaining above
        // To reach exactly target_n total, at this point we need k true among vars 1..v
        // where k can range from 0 to min(target_n, num_vars - v + 1)

        for (int k = 0; k <= target_n + 1 && k < static_cast<int>(dp.size()); k++) {
            // new_dp[k] = BDD for "need exactly k true among vars 1..v"
            // If var v is false: need k true among vars 1..v-1 -> dp[k]
            // If var v is true: need k-1 true among vars 1..v-1 -> dp[k-1]

            BDD when_false = dp[k];
            BDD when_true = (k > 0) ? dp[k - 1] : mgr.bdd_zero();

            new_dp[k] = (~var & when_false) | (var & when_true);
        }

        dp = new_dp;
    }

    // dp[target_n] = BDD for "exactly target_n variables are true among all 64"
    return dp[target_n];
}

/**
 * @brief Build BDD for "line is not a winning configuration"
 *
 * A line is winning if all 4 positions have X (all true) or
 * all 4 positions have O (all false). We want to exclude both.
 */
BDD construct_not_winning_line(DDManager& mgr, const std::array<int, 4>& line) {
    BDD v0 = mgr.var_bdd(line[0]);
    BDD v1 = mgr.var_bdd(line[1]);
    BDD v2 = mgr.var_bdd(line[2]);
    BDD v3 = mgr.var_bdd(line[3]);

    // All X (all true) - forbidden
    BDD all_x = v0 & v1 & v2 & v3;

    // All O (all false) - forbidden
    BDD all_o = (~v0) & (~v1) & (~v2) & (~v3);

    // Neither all X nor all O
    return (~all_x) & (~all_o);
}

////////////////////////////////////////////////////////////////////////////////
// Expected results
////////////////////////////////////////////////////////////////////////////////

const uint64_t expected[30] = {
    0,           //  0
    0,           //  1
    0,           //  2
    0,           //  3
    0,           //  4
    0,           //  5
    0,           //  6
    0,           //  7
    0,           //  8
    0,           //  9
    0,           // 10
    0,           // 11
    0,           // 12
    0,           // 13
    0,           // 14
    0,           // 15
    0,           // 16
    0,           // 17
    0,           // 18
    0,           // 19
    304,         // 20
    136288,      // 21
    9734400,     // 22
    296106640,   // 23
    5000129244,  // 24
    52676341760,    // 25
    370421947296,   // 26
    1819169272400,  // 27
    6444883392304,  // 28
    16864508850272  // 29
};

////////////////////////////////////////////////////////////////////////////////
// Main
////////////////////////////////////////////////////////////////////////////////

void print_usage(const char* prog) {
    std::cout << "Usage: " << prog << " [-n crosses]" << std::endl;
    std::cout << "  -n crosses   Number of X marks (default: 20)" << std::endl;
    std::cout << std::endl;
    std::cout << "Counts draw positions in 4x4x4 Tic-Tac-Toe using BDD." << std::endl;
    std::cout << "Based on bdd-benchmark (MIT License) by Steffan Soelvsten." << std::endl;
}

int main(int argc, char** argv) {
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "-n" && i + 1 < argc) {
            N = std::atoi(argv[++i]);
            if (N < 0 || N > 64) {
                std::cerr << "Error: Number of crosses must be between 0 and 64." << std::endl;
                return 1;
            }
        } else if (arg == "-h" || arg == "--help") {
            print_usage(argv[0]);
            return 0;
        }
    }

    std::cout << "============================================" << std::endl;
    std::cout << "4x4x4 Tic-Tac-Toe Draw Counter (N = " << N << ")" << std::endl;
    std::cout << "============================================" << std::endl;
    std::cout << std::endl;

    // Construct winning lines
    construct_lines();
    std::cout << "Winning lines: " << lines.size() << std::endl;

    // Create DD Manager
    DDManager mgr;

    // Create 64 variables
    std::cout << "Creating 64 variables..." << std::endl;
    for (int i = 0; i < 64; i++) {
        mgr.new_var();
    }

    // Build initial constraint: exactly N crosses
    std::cout << "Building 'exactly " << N << " crosses' constraint..." << std::endl;
    auto t1 = std::chrono::high_resolution_clock::now();

    BDD result = construct_exactly_n(mgr, N);

    auto t2 = std::chrono::high_resolution_clock::now();
    auto initial_time = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();

    std::cout << "  Time: " << initial_time << " ms" << std::endl;
    std::cout << "  Nodes: " << mgr.alive_count() << std::endl;
    std::cout << std::endl;

    // Apply line constraints
    std::cout << "Applying " << lines.size() << " line constraints..." << std::endl;
    auto t3 = std::chrono::high_resolution_clock::now();

    int line_count = 0;
    for (const auto& line : lines) {
        result = result & construct_not_winning_line(mgr, line);
        line_count++;
        if (line_count % 20 == 0) {
            std::cout << "  Processed " << line_count << "/" << lines.size()
                      << " lines, nodes: " << mgr.alive_count() << std::endl;
        }
    }

    auto t4 = std::chrono::high_resolution_clock::now();
    auto constraint_time = std::chrono::duration_cast<std::chrono::milliseconds>(t4 - t3).count();

    std::cout << "  Time: " << constraint_time << " ms" << std::endl;
    std::cout << "  Final nodes: " << mgr.alive_count() << std::endl;
    std::cout << std::endl;

    // Count solutions
    std::cout << "Counting solutions..." << std::endl;
    auto t5 = std::chrono::high_resolution_clock::now();

    double solutions_d = result.count(64);
    uint64_t solutions = static_cast<uint64_t>(solutions_d);

    auto t6 = std::chrono::high_resolution_clock::now();
    auto counting_time = std::chrono::duration_cast<std::chrono::milliseconds>(t6 - t5).count();

    std::cout << "  Time: " << counting_time << " ms" << std::endl;
    std::cout << std::endl;

    // Output results
    std::cout << "============================================" << std::endl;
    std::cout << "Results:" << std::endl;
    std::cout << "  Crosses (N): " << N << std::endl;
    std::cout << "  Draw positions: " << solutions << std::endl;
    std::cout << "  Total time: " << (initial_time + constraint_time + counting_time) << " ms" << std::endl;
    std::cout << "============================================" << std::endl;

    // Verify result
    if (static_cast<size_t>(N) < sizeof(expected) / sizeof(expected[0])) {
        if (solutions == expected[N]) {
            std::cout << "Verification: PASSED" << std::endl;
        } else {
            std::cout << "Verification: FAILED (expected " << expected[N] << ")" << std::endl;
            return 1;
        }
    }

    return 0;
}
