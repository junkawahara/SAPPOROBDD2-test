/**
 * @file cnf.cpp
 * @brief CNF SAT Solver using BDD
 * @copyright MIT License
 *
 * This example is based on the BDD Benchmark suite by Steffan Sølvsten
 * (https://github.com/ssoelvsten/bdd-benchmark), which is also MIT licensed.
 *
 * Reads a CNF formula in DIMACS format and converts it to a BDD.
 * Can check satisfiability and count the number of satisfying assignments.
 *
 * DIMACS CNF format:
 * - Comments start with 'c'
 * - Problem line: "p cnf <variables> <clauses>"
 * - Clause lines: space-separated literals ending with 0
 *   - Positive literal n means variable n is true
 *   - Negative literal -n means variable n is false
 */

#include <sbdd2/sbdd2.hpp>

#include <algorithm>
#include <cassert>
#include <chrono>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using namespace sbdd2;

// Command line options
static std::string cnf_file;
static bool do_satcount = false;

////////////////////////////////////////////////////////////////////////////////
// CNF Data Structures
////////////////////////////////////////////////////////////////////////////////

struct Clause {
    std::vector<int> literals;  // Positive = variable, Negative = negated variable
};

struct CNF {
    int num_vars;
    int num_clauses;
    std::vector<Clause> clauses;
};

////////////////////////////////////////////////////////////////////////////////
// DIMACS Parser
////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Parse a DIMACS CNF file
 */
CNF parse_dimacs(const std::string& filename) {
    CNF cnf;
    cnf.num_vars = 0;
    cnf.num_clauses = 0;

    std::ifstream file(filename);
    if (!file.is_open()) {
        throw std::runtime_error("Cannot open file: " + filename);
    }

    std::string line;
    bool header_found = false;

    while (std::getline(file, line)) {
        // Skip empty lines
        if (line.empty()) continue;

        // Skip comments
        if (line[0] == 'c') continue;

        // Parse problem line
        if (line[0] == 'p') {
            std::istringstream iss(line);
            std::string p, cnf_str;
            iss >> p >> cnf_str >> cnf.num_vars >> cnf.num_clauses;
            if (cnf_str != "cnf") {
                throw std::runtime_error("Expected 'cnf' format, got: " + cnf_str);
            }
            header_found = true;
            continue;
        }

        // Parse clause
        if (!header_found) {
            throw std::runtime_error("Clause found before problem line");
        }

        Clause clause;
        std::istringstream iss(line);
        int lit;
        while (iss >> lit) {
            if (lit == 0) break;  // End of clause
            clause.literals.push_back(lit);
        }

        if (!clause.literals.empty()) {
            cnf.clauses.push_back(clause);
        }
    }

    return cnf;
}

////////////////////////////////////////////////////////////////////////////////
// BDD Construction
////////////////////////////////////////////////////////////////////////////////

/**
 * @brief Convert a clause to BDD (disjunction of literals)
 */
BDD clause_to_bdd(DDManager& mgr, const Clause& clause) {
    BDD result = mgr.bdd_zero();

    for (int lit : clause.literals) {
        int var = std::abs(lit);
        BDD var_bdd = mgr.var_bdd(var);

        if (lit > 0) {
            result = result | var_bdd;
        } else {
            result = result | (~var_bdd);
        }
    }

    return result;
}

/**
 * @brief Convert CNF to BDD using balanced tree conjunction
 *
 * Instead of simple left-to-right conjunction, we use a balanced
 * binary tree approach which can be more efficient.
 */
BDD cnf_to_bdd_balanced(DDManager& mgr, const CNF& cnf) {
    if (cnf.clauses.empty()) {
        return mgr.bdd_one();
    }

    // Convert all clauses to BDDs
    std::vector<BDD> clause_bdds;
    clause_bdds.reserve(cnf.clauses.size());

    for (const Clause& clause : cnf.clauses) {
        clause_bdds.push_back(clause_to_bdd(mgr, clause));
    }

    // Balanced tree conjunction
    while (clause_bdds.size() > 1) {
        std::vector<BDD> next_level;
        next_level.reserve((clause_bdds.size() + 1) / 2);

        for (size_t i = 0; i + 1 < clause_bdds.size(); i += 2) {
            next_level.push_back(clause_bdds[i] & clause_bdds[i + 1]);
        }

        // Handle odd element
        if (clause_bdds.size() % 2 == 1) {
            next_level.push_back(clause_bdds.back());
        }

        clause_bdds = std::move(next_level);

        std::cout << "  Tree level done, " << clause_bdds.size()
                  << " BDDs remaining, nodes: " << mgr.alive_count() << std::endl;
    }

    return clause_bdds[0];
}

////////////////////////////////////////////////////////////////////////////////
// Main
////////////////////////////////////////////////////////////////////////////////

void print_usage(const char* prog) {
    std::cout << "Usage: " << prog << " -f <cnf_file> [-c]" << std::endl;
    std::cout << "  -f file   Path to DIMACS CNF file" << std::endl;
    std::cout << "  -c        Count satisfying assignments" << std::endl;
    std::cout << std::endl;
    std::cout << "Solves CNF SAT problems using BDD." << std::endl;
    std::cout << "Based on bdd-benchmark (MIT License) by Steffan Soelvsten." << std::endl;
}

int main(int argc, char** argv) {
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];
        if (arg == "-f" && i + 1 < argc) {
            cnf_file = argv[++i];
        } else if (arg == "-c") {
            do_satcount = true;
        } else if (arg == "-h" || arg == "--help") {
            print_usage(argv[0]);
            return 0;
        }
    }

    if (cnf_file.empty()) {
        std::cerr << "Error: No CNF file specified." << std::endl;
        print_usage(argv[0]);
        return 1;
    }

    std::cout << "========================================" << std::endl;
    std::cout << "CNF SAT Solver using BDD" << std::endl;
    std::cout << "========================================" << std::endl;
    std::cout << std::endl;

    // Parse CNF file
    std::cout << "Parsing: " << cnf_file << std::endl;
    CNF cnf;
    try {
        cnf = parse_dimacs(cnf_file);
    } catch (const std::exception& e) {
        std::cerr << "Error parsing CNF: " << e.what() << std::endl;
        return 1;
    }

    std::cout << "  Variables: " << cnf.num_vars << std::endl;
    std::cout << "  Clauses: " << cnf.clauses.size() << std::endl;
    std::cout << std::endl;

    // Create DD Manager
    DDManager mgr;

    // Create variables
    std::cout << "Creating " << cnf.num_vars << " variables..." << std::endl;
    for (int i = 0; i < cnf.num_vars; i++) {
        mgr.new_var();
    }

    // Build BDD
    std::cout << "Building BDD..." << std::endl;
    auto t1 = std::chrono::high_resolution_clock::now();

    BDD result = cnf_to_bdd_balanced(mgr, cnf);

    auto t2 = std::chrono::high_resolution_clock::now();
    auto build_time = std::chrono::duration_cast<std::chrono::milliseconds>(t2 - t1).count();

    std::cout << "  Build time: " << build_time << " ms" << std::endl;
    std::cout << "  BDD nodes: " << mgr.alive_count() << std::endl;
    std::cout << std::endl;

    // Check satisfiability
    bool is_sat = !result.is_zero();

    std::cout << "========================================" << std::endl;
    std::cout << "Result: " << (is_sat ? "SATISFIABLE" : "UNSATISFIABLE") << std::endl;

    // Count satisfying assignments if requested
    if (do_satcount && is_sat) {
        std::cout << std::endl;
        std::cout << "Counting satisfying assignments..." << std::endl;
        auto t3 = std::chrono::high_resolution_clock::now();

        double count_d = result.count(cnf.num_vars);
        uint64_t count = static_cast<uint64_t>(count_d);

        auto t4 = std::chrono::high_resolution_clock::now();
        auto count_time = std::chrono::duration_cast<std::chrono::milliseconds>(t4 - t3).count();

        std::cout << "  Count time: " << count_time << " ms" << std::endl;
        std::cout << "  Satisfying assignments: " << count << std::endl;
    }

    // If satisfiable, show one solution
    if (is_sat) {
        std::cout << std::endl;
        std::cout << "One satisfying assignment:" << std::endl;
        std::vector<int> solution = result.one_sat();
        std::cout << "  ";
        for (int i = 1; i <= cnf.num_vars && i <= static_cast<int>(solution.size()); i++) {
            if (solution[i] == 1) {
                std::cout << i << " ";
            } else if (solution[i] == 0) {
                std::cout << "-" << i << " ";
            }
            // -1 means don't care
        }
        std::cout << std::endl;
    }

    std::cout << "========================================" << std::endl;
    std::cout << "Total time: " << build_time << " ms" << std::endl;

    return 0;
}
