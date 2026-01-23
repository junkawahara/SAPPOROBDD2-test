/**
 * @file apply.cpp
 * @brief BDD Apply Benchmark using lib_bdd format
 * @copyright MIT License
 *
 * This example is based on the BDD Benchmark suite by Steffan Sølvsten
 * (https://github.com/ssoelvsten/bdd-benchmark), which is also MIT licensed.
 *
 * Loads multiple BDDs from lib_bdd format files and applies binary operations
 * (AND/OR) to combine them. This benchmark measures the performance of BDD
 * operations on real-world decision diagrams.
 *
 * lib_bdd format: Binary format from Rust's lib-bdd library
 * - 10 bytes per node (little-endian)
 * - uint16_t level, uint32_t low, uint32_t high
 * - Index 0 = false terminal, Index 1 = true terminal
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

static std::vector<std::string> input_files;
static bool use_or = false;  // Default is AND

void print_usage(const char* prog) {
    std::cout << "Usage: " << prog << " -f <file1.bdd> -f <file2.bdd> ... [-o]" << std::endl;
    std::cout << "  -f FILE   Path to lib_bdd format BDD file (2+ required)" << std::endl;
    std::cout << "  -o        Use OR operation (default: AND)" << std::endl;
    std::cout << std::endl;
    std::cout << "Applies binary operations to multiple BDDs loaded from lib_bdd files." << std::endl;
    std::cout << "Based on bdd-benchmark (MIT License) by Steffan Soelvsten." << std::endl;
}

////////////////////////////////////////////////////////////////////////////////
// Main
////////////////////////////////////////////////////////////////////////////////

int main(int argc, char** argv) {
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "-f" && i + 1 < argc) {
            input_files.push_back(argv[++i]);
        } else if (arg == "-o") {
            use_or = true;
        } else if (arg == "-h" || arg == "--help") {
            print_usage(argv[0]);
            return 0;
        }
    }

    if (input_files.size() < 2) {
        std::cerr << "Error: At least 2 BDD files required for binary operation." << std::endl;
        print_usage(argv[0]);
        return 1;
    }

    // Verify files exist
    for (const auto& path : input_files) {
        std::ifstream ifs(path, std::ios::binary);
        if (!ifs.good()) {
            std::cerr << "Error: Cannot open file: " << path << std::endl;
            return 1;
        }
    }

    std::cout << "================================================" << std::endl;
    std::cout << "BDD Apply Benchmark (lib_bdd format)" << std::endl;
    std::cout << "Operation: " << (use_or ? "OR" : "AND") << std::endl;
    std::cout << "Input files: " << input_files.size() << std::endl;
    std::cout << "================================================" << std::endl;
    std::cout << std::endl;

    DDManager mgr;
    std::vector<BDD> bdds;
    bdds.reserve(input_files.size());

    // Load all BDD files
    std::cout << "Loading BDD files..." << std::endl;
    auto t_load_start = std::chrono::high_resolution_clock::now();

    for (size_t i = 0; i < input_files.size(); ++i) {
        auto t1 = std::chrono::high_resolution_clock::now();

        BDD bdd = import_bdd_as_libbdd(mgr, input_files[i]);

        auto t2 = std::chrono::high_resolution_clock::now();
        auto load_time = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();

        std::cout << "  [" << (i + 1) << "] " << input_files[i] << std::endl;
        std::cout << "      Size: " << bdd.size() << " nodes" << std::endl;
        std::cout << "      Terminal: " << (bdd.is_terminal() ? "yes" : "no") << std::endl;
        std::cout << "      Load time: " << load_time << " ms" << std::endl;

        bdds.push_back(bdd);
    }

    auto t_load_end = std::chrono::high_resolution_clock::now();
    auto total_load_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        t_load_end - t_load_start).count();

    std::cout << std::endl;
    std::cout << "Total load time: " << total_load_time << " ms" << std::endl;
    std::cout << "Variables: " << mgr.var_count() << std::endl;
    std::cout << "Total nodes: " << mgr.alive_count() << std::endl;
    std::cout << std::endl;

    // Apply operations
    std::cout << "Applying " << (use_or ? "OR" : "AND") << " operations..." << std::endl;
    auto t_apply_start = std::chrono::high_resolution_clock::now();

    BDD result = bdds[0];
    for (size_t i = 1; i < bdds.size(); ++i) {
        auto t1 = std::chrono::high_resolution_clock::now();

        if (use_or) {
            result = result | bdds[i];
        } else {
            result = result & bdds[i];
        }

        auto t2 = std::chrono::high_resolution_clock::now();
        auto op_time = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();

        std::cout << "  Operation " << i << "/" << (bdds.size() - 1)
                  << ": result size = " << result.size()
                  << " nodes, time = " << op_time << " ms" << std::endl;
    }

    auto t_apply_end = std::chrono::high_resolution_clock::now();
    auto total_apply_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        t_apply_end - t_apply_start).count();

    std::cout << std::endl;

    // Count satisfying assignments
    std::cout << "Counting satisfying assignments..." << std::endl;
    auto t_count_start = std::chrono::high_resolution_clock::now();

    int num_vars = mgr.var_count();
    double sat_count = result.count(num_vars);

    auto t_count_end = std::chrono::high_resolution_clock::now();
    auto count_time = std::chrono::duration_cast<std::chrono::milliseconds>(
        t_count_end - t_count_start).count();

    // Output results
    std::cout << std::endl;
    std::cout << "================================================" << std::endl;
    std::cout << "Results:" << std::endl;
    std::cout << "  Operation: " << (use_or ? "OR" : "AND") << std::endl;
    std::cout << "  Input BDDs: " << bdds.size() << std::endl;
    std::cout << "  Variables: " << num_vars << std::endl;
    std::cout << "  Result size: " << result.size() << " nodes" << std::endl;
    std::cout << "  Satisfying assignments: " << std::scientific << sat_count << std::endl;
    std::cout << "  Load time: " << total_load_time << " ms" << std::endl;
    std::cout << "  Apply time: " << total_apply_time << " ms" << std::endl;
    std::cout << "  Count time: " << count_time << " ms" << std::endl;
    std::cout << "  Total time: " << (total_load_time + total_apply_time + count_time) << " ms" << std::endl;
    std::cout << "================================================" << std::endl;

    return 0;
}
