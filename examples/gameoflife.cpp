/**
 * @file gameoflife.cpp
 * @brief Conway's Game of Life Transition Relation using BDD
 * @copyright MIT License
 *
 * This example is based on the BDD Benchmark suite by Steffan Sølvsten
 * (https://github.com/ssoelvsten/bdd-benchmark), which is also MIT licensed.
 *
 * This simplified version builds the transition relation for Conway's
 * Game of Life on a small grid. The full Garden of Eden counting requires
 * more sophisticated techniques.
 *
 * Conway's Game of Life rules:
 * - A live cell with 2 or 3 live neighbors survives
 * - A dead cell with exactly 3 live neighbors becomes alive
 * - All other cells die or stay dead
 */

#include <sbdd2/sbdd2.hpp>

#include <cassert>
#include <chrono>
#include <cstdlib>
#include <iostream>
#include <string>
#include <vector>

using namespace sbdd2;

// Grid size (default 3x3)
static int N = 3;

////////////////////////////////////////////////////////////////////////////////
// Grid helpers
////////////////////////////////////////////////////////////////////////////////

inline int grid_size() { return N; }
inline int num_cells() { return N * N; }

/**
 * @brief Get variable index for cell (r, c)
 */
inline int cell_var(int r, int c) {
    return r * grid_size() + c + 1;  // 1-indexed
}

/**
 * @brief Get neighbor offsets (including self at index 4)
 */
const int neighbor_offsets[9][2] = {
    {-1, -1}, {-1, 0}, {-1, 1},
    {0, -1},  {0, 0},  {0, 1},
    {1, -1},  {1, 0},  {1, 1}
};

/**
 * @brief Check if (r, c) is valid
 */
inline bool valid_cell(int r, int c) {
    return r >= 0 && r < grid_size() && c >= 0 && c < grid_size();
}

////////////////////////////////////////////////////////////////////////////////
// BDD Construction
////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Build BDD for "exactly k neighbors are alive" around cell (r, c)
 *
 * Neighbors are the 8 cells surrounding (r, c), not including (r, c) itself.
 */
BDD exactly_k_neighbors(DDManager& mgr, int r, int c, int k) {
    // Collect neighbor variables
    std::vector<BDD> nbrs;
    for (int i = 0; i < 9; i++) {
        if (i == 4) continue;  // Skip self
        int nr = r + neighbor_offsets[i][0];
        int nc = c + neighbor_offsets[i][1];
        if (valid_cell(nr, nc)) {
            nbrs.push_back(mgr.var_bdd(cell_var(nr, nc)));
        }
    }

    int n = nbrs.size();
    if (k < 0 || k > n) return mgr.bdd_zero();
    if (n == 0) return (k == 0) ? mgr.bdd_one() : mgr.bdd_zero();

    // Build "exactly k true" using DP
    std::vector<BDD> dp(k + 2, mgr.bdd_zero());
    dp[0] = mgr.bdd_one();

    for (int i = 0; i < n; i++) {
        std::vector<BDD> new_dp(k + 2, mgr.bdd_zero());
        for (int j = 0; j <= std::min(i + 1, k + 1); j++) {
            BDD when_false = dp[j];
            BDD when_true = (j > 0) ? dp[j - 1] : mgr.bdd_zero();
            new_dp[j] = (~nbrs[i] & when_false) | (nbrs[i] & when_true);
        }
        dp = new_dp;
    }

    return dp[k];
}

/**
 * @brief Build BDD for "at least k neighbors are alive"
 */
BDD at_least_k_neighbors(DDManager& mgr, int r, int c, int k) {
    BDD result = mgr.bdd_zero();
    // Count max possible neighbors
    int max_nbrs = 0;
    for (int i = 0; i < 9; i++) {
        if (i == 4) continue;
        int nr = r + neighbor_offsets[i][0];
        int nc = c + neighbor_offsets[i][1];
        if (valid_cell(nr, nc)) max_nbrs++;
    }

    for (int i = k; i <= max_nbrs; i++) {
        result = result | exactly_k_neighbors(mgr, r, c, i);
    }
    return result;
}

/**
 * @brief Build BDD for Game of Life rule at cell (r, c)
 *
 * Returns BDD that is true iff cell survives or is born.
 * - Alive with 2 or 3 neighbors -> survives
 * - Dead with exactly 3 neighbors -> born
 */
BDD life_rule(DDManager& mgr, int r, int c) {
    BDD cell = mgr.var_bdd(cell_var(r, c));

    BDD two_nbrs = exactly_k_neighbors(mgr, r, c, 2);
    BDD three_nbrs = exactly_k_neighbors(mgr, r, c, 3);

    // Survives: alive AND (2 or 3 neighbors)
    BDD survives = cell & (two_nbrs | three_nbrs);

    // Born: dead AND exactly 3 neighbors
    BDD born = (~cell) & three_nbrs;

    // Next state is alive
    return survives | born;
}

////////////////////////////////////////////////////////////////////////////////
// Main
////////////////////////////////////////////////////////////////////////////////

void print_usage(const char* prog) {
    std::cout << "Usage: " << prog << " [-n size]" << std::endl;
    std::cout << "  -n size   Grid size (default: 3)" << std::endl;
    std::cout << std::endl;
    std::cout << "Demonstrates Game of Life transition rules using BDD." << std::endl;
    std::cout << "Based on bdd-benchmark (MIT License) by Steffan Soelvsten." << std::endl;
}

int main(int argc, char** argv) {
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "-n" && i + 1 < argc) {
            N = std::atoi(argv[++i]);
            if (N <= 0 || N > 10) {
                std::cerr << "Error: Grid size must be between 1 and 10." << std::endl;
                return 1;
            }
        } else if (arg == "-h" || arg == "--help") {
            print_usage(argv[0]);
            return 0;
        }
    }

    std::cout << "================================================" << std::endl;
    std::cout << "Game of Life - Transition Rule Demo" << std::endl;
    std::cout << "Grid: " << grid_size() << "x" << grid_size() << std::endl;
    std::cout << "================================================" << std::endl;
    std::cout << std::endl;

    // Create DD Manager
    DDManager mgr;

    // Create variables for each cell
    std::cout << "Creating " << num_cells() << " cell variables..." << std::endl;
    for (int i = 0; i < num_cells(); i++) {
        mgr.new_var();
    }

    // Build life rules for each cell
    std::cout << "Building Game of Life rules..." << std::endl;
    auto t1 = std::chrono::high_resolution_clock::now();

    std::vector<BDD> next_state;
    for (int r = 0; r < grid_size(); r++) {
        for (int c = 0; c < grid_size(); c++) {
            next_state.push_back(life_rule(mgr, r, c));
        }
        std::cout << "  Row " << (r + 1) << "/" << grid_size()
                  << " done, nodes: " << mgr.alive_count() << std::endl;
    }

    auto t2 = std::chrono::high_resolution_clock::now();
    auto build_time = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();

    std::cout << "  Build time: " << build_time << " ms" << std::endl;
    std::cout << std::endl;

    // Count configurations where center cell becomes alive
    if (grid_size() >= 3) {
        int center_r = grid_size() / 2;
        int center_c = grid_size() / 2;

        std::cout << "Analyzing center cell (" << center_r << "," << center_c << ")..." << std::endl;

        BDD center_next = next_state[center_r * grid_size() + center_c];
        double alive_count = center_next.count(num_cells());
        uint64_t total = 1ULL << num_cells();

        std::cout << "  Configurations where center becomes alive: "
                  << static_cast<uint64_t>(alive_count) << " / " << total << std::endl;
    }

    // Count still lifes (configurations that don't change)
    std::cout << std::endl;
    std::cout << "Checking for still lifes..." << std::endl;
    auto t3 = std::chrono::high_resolution_clock::now();

    BDD still_life = mgr.bdd_one();
    for (int r = 0; r < grid_size(); r++) {
        for (int c = 0; c < grid_size(); c++) {
            BDD cell = mgr.var_bdd(cell_var(r, c));
            BDD next = next_state[r * grid_size() + c];
            // Cell stays same: (cell AND next) OR (NOT cell AND NOT next)
            BDD same = (cell & next) | ((~cell) & (~next));
            still_life = still_life & same;
        }
    }

    auto t4 = std::chrono::high_resolution_clock::now();
    auto still_time = std::chrono::duration_cast<std::chrono::milliseconds>(t4 - t3).count();

    double still_count = still_life.count(num_cells());
    std::cout << "  Still lifes found: " << static_cast<uint64_t>(still_count) << std::endl;
    std::cout << "  Time: " << still_time << " ms" << std::endl;

    std::cout << std::endl;
    std::cout << "================================================" << std::endl;
    std::cout << "Total time: " << (build_time + still_time) << " ms" << std::endl;
    std::cout << "Final nodes: " << mgr.alive_count() << std::endl;
    std::cout << "================================================" << std::endl;

    return 0;
}
