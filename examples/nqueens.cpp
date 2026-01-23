/**
 * @file nqueens.cpp
 * @brief N-Queens Problem Solver using BDD
 * @copyright MIT License
 *
 * This example is based on the BDD Benchmark suite by Steffan Sølvsten
 * (https://github.com/ssoelvsten/bdd-benchmark), which is also MIT licensed.
 *
 * The N-Queens problem asks: in how many ways can N queens be placed on an
 * N×N chess board such that no two queens threaten each other?
 *
 * This implementation constructs a BDD representing all valid placements
 * by encoding constraints row by row.
 */

#include <sbdd2/sbdd2.hpp>

#include <cassert>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

using namespace sbdd2;

// Board size (default 8x8)
static int N = 8;

////////////////////////////////////////////////////////////////////////////////
// Helper functions
////////////////////////////////////////////////////////////////////////////////

inline int rows() { return N; }
inline int cols() { return N; }
inline int MAX_ROW() { return N - 1; }
inline int MAX_COL() { return N - 1; }

/**
 * @brief Convert board position (row, col) to BDD variable label
 *
 * Variables are numbered from 1 to N*N, row by row.
 * Position (0,0) -> var 1, (0,1) -> var 2, ..., (N-1,N-1) -> var N*N
 */
inline int label_of_position(int r, int c) {
    assert(r >= 0 && c >= 0);
    return (rows() * r) + c + 1;  // +1 because SAPPOROBDD2 uses 1-indexed variables
}

inline std::string row_to_string(int r) {
    return std::to_string(r + 1);
}

inline std::string col_to_string(int c) {
    return std::string(1, static_cast<char>('A' + c));
}

inline std::string pos_to_string(int r, int c) {
    return col_to_string(c) + row_to_string(r);
}

////////////////////////////////////////////////////////////////////////////////
// BDD Construction
////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Build BDD for placing a queen at position (i, j)
 *
 * Creates a BDD that is true iff:
 * - A queen is placed at position (i, j)
 * - No queen is placed on any conflicting position (same row, column, or diagonal)
 */
BDD queens_S(DDManager& mgr, int i, int j) {
    // Start with true (all assignments valid)
    BDD result = mgr.bdd_one();

    for (int row = MAX_ROW(); row >= 0; row--) {
        for (int col = MAX_COL(); col >= 0; col--) {
            int var = label_of_position(row, col);
            BDD x = mgr.var_bdd(var);

            // Queen must be placed here
            if (row == i && col == j) {
                // x must be true
                result = result & x;
                continue;
            }

            // Check for conflicts
            int row_diff = std::abs(row - i);
            int col_diff = std::abs(col - j);

            bool same_row = (row == i);
            bool same_col = (col == j);
            bool same_diag = (row_diff == col_diff);

            if (same_row || same_col || same_diag) {
                // Conflict: x must be false
                result = result & (~x);
            }
            // else: no constraint on this position
        }
    }

    return result;
}

/**
 * @brief Build BDD for row r: exactly one queen in this row
 *
 * The BDD represents the disjunction of all valid single-queen placements
 * in row r: at least one queen must be placed somewhere in row r.
 */
BDD queens_R(DDManager& mgr, int r) {
    // Start with the BDD for placing queen at (r, 0)
    BDD result = queens_S(mgr, r, 0);

    // OR with BDDs for placing queen at other columns
    for (int c = 1; c < cols(); c++) {
        result = result | queens_S(mgr, r, c);
    }

    return result;
}

/**
 * @brief Build BDD for the entire board: valid N-Queens configuration
 *
 * The BDD represents the conjunction of all row constraints:
 * each row must have exactly one queen, with no conflicts.
 */
BDD queens_B(DDManager& mgr) {
    if (rows() == 1 && cols() == 1) {
        return queens_S(mgr, 0, 0);
    }

    // Start with row 0
    BDD result = queens_R(mgr, 0);

    // AND with remaining rows
    for (int r = 1; r < rows(); r++) {
        result = result & queens_R(mgr, r);

        std::cout << "  Row " << row_to_string(r) << " done, "
                  << "nodes: " << mgr.alive_count() << std::endl;
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////
// Expected solutions (from Wikipedia: Eight queens puzzle)
////////////////////////////////////////////////////////////////////////////////

const uint64_t expected[28] = {
    0,                   //  0x0
    1,                   //  1x1
    0,                   //  2x2
    0,                   //  3x3
    2,                   //  4x4
    10,                  //  5x5
    4,                   //  6x6
    40,                  //  7x7
    92,                  //  8x8
    352,                 //  9x9
    724,                 // 10x10
    2680,                // 11x11
    14200,               // 12x12
    73712,               // 13x13
    365596,              // 14x14
    2279184,             // 15x15
    14772512,            // 16x16
    95815104,            // 17x17
    666090624,           // 18x18
    4968057848,          // 19x19
    39029188884,         // 20x20
    314666222712,        // 21x21
    2691008701644,       // 22x22
    24233937684440,      // 23x23
    227514171973736,     // 24x24
    2207893435808352,    // 25x25
    22317699616364044,   // 26x26
    234907967154122528   // 27x27
};

////////////////////////////////////////////////////////////////////////////////
// Main
////////////////////////////////////////////////////////////////////////////////

void print_usage(const char* prog) {
    std::cout << "Usage: " << prog << " [-n size]" << std::endl;
    std::cout << "  -n size   Board size (default: 8)" << std::endl;
    std::cout << std::endl;
    std::cout << "Solves the N-Queens problem using BDD." << std::endl;
    std::cout << "Based on bdd-benchmark (MIT License) by Steffan Soelvsten." << std::endl;
}

int main(int argc, char** argv) {
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "-n" && i + 1 < argc) {
            N = std::atoi(argv[++i]);
            if (N <= 0) {
                std::cerr << "Error: Board size must be positive." << std::endl;
                return 1;
            }
        } else if (arg == "-h" || arg == "--help") {
            print_usage(argv[0]);
            return 0;
        }
    }

    std::cout << "====================================" << std::endl;
    std::cout << "N-Queens Problem (N = " << N << ")" << std::endl;
    std::cout << "====================================" << std::endl;
    std::cout << std::endl;

    // Create DD Manager
    DDManager mgr;

    // Create variables (N*N variables for the board)
    std::cout << "Creating " << N * N << " variables..." << std::endl;
    for (int i = 0; i < N * N; i++) {
        mgr.new_var();
    }

    // Build the BDD
    std::cout << "Building BDD..." << std::endl;
    auto t1 = std::chrono::high_resolution_clock::now();

    BDD result = queens_B(mgr);

    auto t2 = std::chrono::high_resolution_clock::now();
    auto construction_time = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();

    std::cout << std::endl;
    std::cout << "BDD construction completed." << std::endl;
    std::cout << "  Time: " << construction_time << " ms" << std::endl;
    std::cout << "  Nodes: " << mgr.alive_count() << std::endl;
    std::cout << std::endl;

    // Count solutions
    std::cout << "Counting solutions..." << std::endl;
    auto t3 = std::chrono::high_resolution_clock::now();

    double solutions_d = result.count(N * N);
    uint64_t solutions = static_cast<uint64_t>(solutions_d);

    auto t4 = std::chrono::high_resolution_clock::now();
    auto counting_time = std::chrono::duration_cast<std::chrono::milliseconds>(t4 - t3).count();

    std::cout << "  Time: " << counting_time << " ms" << std::endl;
    std::cout << std::endl;

    // Output results
    std::cout << "====================================" << std::endl;
    std::cout << "Results:" << std::endl;
    std::cout << "  Board size: " << N << "x" << N << std::endl;
    std::cout << "  Solutions: " << solutions << std::endl;
    std::cout << "  Total time: " << (construction_time + counting_time) << " ms" << std::endl;
    std::cout << "====================================" << std::endl;

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
