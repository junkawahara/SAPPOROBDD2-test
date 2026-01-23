/**
 * @file relprod.cpp
 * @brief Relational Product Benchmark using lib_bdd format
 * @copyright MIT License
 *
 * This example is based on the BDD Benchmark suite by Steffan Sølvsten
 * (https://github.com/ssoelvsten/bdd-benchmark), which is also MIT licensed.
 *
 * Computes the relational product (image computation) for state transition
 * systems. Given:
 * - States: BDD representing current states
 * - Relation: BDD representing transition relation (current -> next state)
 *
 * The relational product computes the set of successor (or predecessor) states.
 *
 * RelNext: ∃curr. (States(curr) ∧ Relation(curr, next))
 * RelPrev: ∃next. (States(next) ∧ Relation(curr, next))
 *
 * lib_bdd format: Binary format from Rust's lib-bdd library
 */

#include <sbdd2/sbdd2.hpp>

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

using namespace sbdd2;

////////////////////////////////////////////////////////////////////////////////
// Command Line Options
////////////////////////////////////////////////////////////////////////////////

static std::string relation_path;
static std::string states_path;
static bool compute_prev = false;  // Default is NEXT

void print_usage(const char* prog) {
    std::cout << "Usage: " << prog << " -r <relation.bdd> -s <states.bdd> [-p]" << std::endl;
    std::cout << "  -r FILE   Path to relation BDD (lib_bdd format)" << std::endl;
    std::cout << "  -s FILE   Path to states BDD (lib_bdd format)" << std::endl;
    std::cout << "  -p        Compute predecessor (RelPrev) instead of successor (RelNext)" << std::endl;
    std::cout << std::endl;
    std::cout << "Computes relational product for state transition analysis." << std::endl;
    std::cout << "Based on bdd-benchmark (MIT License) by Steffan Soelvsten." << std::endl;
    std::cout << std::endl;
    std::cout << "Variable encoding assumption:" << std::endl;
    std::cout << "  - Even variables (2,4,6,...): current state bits" << std::endl;
    std::cout << "  - Odd variables (1,3,5,...): next state bits" << std::endl;
}

////////////////////////////////////////////////////////////////////////////////
// Relational Product Implementation
////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Compute RelNext: successor states
 *
 * RelNext(States, Relation) = ∃curr. (States(curr) ∧ Relation(curr, next))
 *
 * Assuming interleaved encoding where:
 * - Even variables represent current state
 * - Odd variables represent next state
 *
 * After quantifying out current state variables, rename next->current
 */
BDD rel_next(const BDD& states, const BDD& relation, DDManager& mgr) {
    int num_vars = mgr.var_count();
    if (num_vars == 0) return mgr.bdd_zero();

    // Step 1: AND states with relation
    BDD conjunction = states & relation;

    // Step 2: Existentially quantify out current state variables (even)
    // Collect even variables to quantify
    std::vector<bddvar> current_vars;
    for (int v = 2; v <= num_vars; v += 2) {
        current_vars.push_back(v);
    }

    BDD result = conjunction;
    for (bddvar v : current_vars) {
        result = result.exist(v);
    }

    return result;
}

/**
 * @brief Compute RelPrev: predecessor states
 *
 * RelPrev(States, Relation) = ∃next. (States(next) ∧ Relation(curr, next))
 */
BDD rel_prev(const BDD& states, const BDD& relation, DDManager& mgr) {
    int num_vars = mgr.var_count();
    if (num_vars == 0) return mgr.bdd_zero();

    // Step 1: AND states with relation
    BDD conjunction = states & relation;

    // Step 2: Existentially quantify out next state variables (odd)
    std::vector<bddvar> next_vars;
    for (int v = 1; v <= num_vars; v += 2) {
        next_vars.push_back(v);
    }

    BDD result = conjunction;
    for (bddvar v : next_vars) {
        result = result.exist(v);
    }

    return result;
}

////////////////////////////////////////////////////////////////////////////////
// Main
////////////////////////////////////////////////////////////////////////////////

int main(int argc, char** argv) {
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "-r" && i + 1 < argc) {
            relation_path = argv[++i];
        } else if (arg == "-s" && i + 1 < argc) {
            states_path = argv[++i];
        } else if (arg == "-p") {
            compute_prev = true;
        } else if (arg == "-h" || arg == "--help") {
            print_usage(argv[0]);
            return 0;
        }
    }

    if (relation_path.empty()) {
        std::cerr << "Error: Relation file not specified." << std::endl;
        print_usage(argv[0]);
        return 1;
    }

    if (states_path.empty()) {
        std::cerr << "Error: States file not specified." << std::endl;
        print_usage(argv[0]);
        return 1;
    }

    // Verify files exist
    {
        std::ifstream ifs(relation_path, std::ios::binary);
        if (!ifs.good()) {
            std::cerr << "Error: Cannot open relation file: " << relation_path << std::endl;
            return 1;
        }
    }
    {
        std::ifstream ifs(states_path, std::ios::binary);
        if (!ifs.good()) {
            std::cerr << "Error: Cannot open states file: " << states_path << std::endl;
            return 1;
        }
    }

    std::cout << "================================================" << std::endl;
    std::cout << "Relational Product Benchmark (lib_bdd format)" << std::endl;
    std::cout << "Operation: " << (compute_prev ? "RelPrev (predecessor)" : "RelNext (successor)") << std::endl;
    std::cout << "================================================" << std::endl;
    std::cout << std::endl;

    DDManager mgr;
    size_t total_time = 0;

    // Load relation BDD
    std::cout << "Loading relation BDD..." << std::endl;
    auto t1 = std::chrono::high_resolution_clock::now();

    BDD relation = import_bdd_as_libbdd(mgr, relation_path);

    auto t2 = std::chrono::high_resolution_clock::now();
    auto relation_load_time = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();
    total_time += relation_load_time;

    std::cout << "  Path: " << relation_path << std::endl;
    std::cout << "  Size: " << relation.size() << " nodes" << std::endl;
    std::cout << "  Terminal: " << (relation.is_terminal() ? "yes" : "no") << std::endl;
    std::cout << "  Load time: " << relation_load_time << " ms" << std::endl;
    std::cout << std::endl;

    // Load states BDD
    std::cout << "Loading states BDD..." << std::endl;
    auto t3 = std::chrono::high_resolution_clock::now();

    BDD states = import_bdd_as_libbdd(mgr, states_path);

    auto t4 = std::chrono::high_resolution_clock::now();
    auto states_load_time = std::chrono::duration_cast<std::chrono::milliseconds>(t4 - t3).count();
    total_time += states_load_time;

    std::cout << "  Path: " << states_path << std::endl;
    std::cout << "  Size: " << states.size() << " nodes" << std::endl;
    std::cout << "  Terminal: " << (states.is_terminal() ? "yes" : "no") << std::endl;
    std::cout << "  Load time: " << states_load_time << " ms" << std::endl;
    std::cout << std::endl;

    int num_vars = mgr.var_count();
    std::cout << "Variables: " << num_vars << std::endl;
    std::cout << "State bits: " << (num_vars / 2) << std::endl;
    std::cout << "Total nodes: " << mgr.alive_count() << std::endl;
    std::cout << std::endl;

    // Compute relational product
    std::cout << "Computing " << (compute_prev ? "RelPrev" : "RelNext") << "..." << std::endl;
    auto t5 = std::chrono::high_resolution_clock::now();

    BDD result;
    if (compute_prev) {
        result = rel_prev(states, relation, mgr);
    } else {
        result = rel_next(states, relation, mgr);
    }

    auto t6 = std::chrono::high_resolution_clock::now();
    auto relprod_time = std::chrono::duration_cast<std::chrono::milliseconds>(t6 - t5).count();
    total_time += relprod_time;

    std::cout << "  RelProd time: " << relprod_time << " ms" << std::endl;
    std::cout << std::endl;

    // Count satisfying assignments
    std::cout << "Counting results..." << std::endl;
    auto t7 = std::chrono::high_resolution_clock::now();

    // Result has half the variables (only current or next state bits)
    int result_vars = num_vars / 2;
    double sat_count = result.count(num_vars);  // Use full var count for proper counting

    auto t8 = std::chrono::high_resolution_clock::now();
    auto count_time = std::chrono::duration_cast<std::chrono::milliseconds>(t8 - t7).count();
    total_time += count_time;

    // Output results
    std::cout << std::endl;
    std::cout << "================================================" << std::endl;
    std::cout << "Results:" << std::endl;
    std::cout << "  Operation: " << (compute_prev ? "RelPrev" : "RelNext") << std::endl;
    std::cout << "  Input variables: " << num_vars << std::endl;
    std::cout << "  State bits: " << (num_vars / 2) << std::endl;
    std::cout << "  Relation size: " << relation.size() << " nodes" << std::endl;
    std::cout << "  States size: " << states.size() << " nodes" << std::endl;
    std::cout << "  Result size: " << result.size() << " nodes" << std::endl;
    std::cout << "  Result states: " << std::scientific << sat_count << std::endl;
    std::cout << "  Load time: " << (relation_load_time + states_load_time) << " ms" << std::endl;
    std::cout << "  RelProd time: " << relprod_time << " ms" << std::endl;
    std::cout << "  Count time: " << count_time << " ms" << std::endl;
    std::cout << "  Total time: " << total_time << " ms" << std::endl;
    std::cout << "================================================" << std::endl;

    return 0;
}
