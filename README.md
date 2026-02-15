# SAPPOROBDD 2.0

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

[日本語版 README はこちら](README_ja.md)

SAPPOROBDD 2.0 is a high-performance BDD (Binary Decision Diagram) and ZDD (Zero-suppressed Decision Diagram) library written in C++11. It is a complete redesign based on [SAPPOROBDD++](https://github.com/junkawahara/SAPPOROBDD), featuring a modern architecture with 128-bit node structure, instance-based design, and thread safety.

## Features

### Core DD Types
- **BDD** / **ZDD** -- Reduced decision diagrams with full operator overloading
- **UnreducedBDD** / **UnreducedZDD** -- For top-down construction, convertible to reduced form via `reduce()`
- **QDD** -- Quasi-reduced decision diagrams

### Extended DD Types
- **MTBDD\<T\>** / **MTZDD\<T\>** -- Multi-terminal DDs with arbitrary terminal values
- **MVBDD** / **MVZDD** -- Multi-valued DDs where each variable takes k values
- **PiDD** -- Permutation decision diagrams
- **SeqBDD** -- Sequence BDDs

### Graph and Combinatorics
- **GBase** -- Graph path/cycle enumeration using the frontier-based method
- **BDDCT** -- Cost-constrained enumeration

### TdZdd Integration
- Compatible Spec interface for top-down DD construction
- BFS, DFS, and parallel (OpenMP) builders

### ZDD Extensions
- Iterators: dictionary-order, weight-order, random-order
- Index structure for ranking/unranking
- Exact counting via GMP or BigInt fallback
- Helper functions: power set, weight filtering, random generation

### I/O Formats
- Binary, Text, DOT (Graphviz), Knuth, Graphillion, lib\_bdd, SVG

### Architecture
- 128-bit node structure (44-bit arcs, 16-bit refcount, 20-bit variable)
- Negation edges for O(1) NOT operations
- Internal hashing with quadratic probing
- Thread-safe operations with mutex protection

## Requirements

- CMake 3.1+
- C++11 compiler (GCC 4.8+, Clang 3.4+, MSVC 2015+)
- (Optional) Google Test -- for running tests
- (Optional) GMP -- for exact counting with arbitrary precision

## Build & Install

```bash
git clone <REPOSITORY_URL>
cd SAPPOROBDD2

mkdir build && cd build
cmake ..
make -j4

# Run tests (optional)
./tests/sbdd2_tests

# Install (optional)
sudo make install
```

### CMake Integration

To use SAPPOROBDD 2.0 in your CMake project:

```cmake
add_subdirectory(path/to/SAPPOROBDD2)
target_link_libraries(your_target sbdd2_static)
```

## Quick Start

```cpp
#include <sbdd2/sbdd2.hpp>
using namespace sbdd2;

int main() {
    DDManager mgr;

    // Create variables
    for (int i = 0; i < 5; ++i) mgr.new_var();

    // BDD operations
    BDD x1 = mgr.var_bdd(1);
    BDD x2 = mgr.var_bdd(2);
    BDD f = (x1 & x2) | (~x1 & x2);  // x2
    std::cout << "Satisfying assignments: " << f.count(5) << std::endl;

    // ZDD operations
    ZDD s1 = ZDD::singleton(mgr, 1);  // {{1}}
    ZDD s2 = ZDD::singleton(mgr, 2);  // {{2}}
    ZDD family = s1 + s2;             // {{1}, {2}}
    ZDD joined = s1 * s2;             // {{1, 2}}
    std::cout << "Set count: " << family.card() << std::endl;

    return 0;
}
```

## Documentation

Full documentation is available in the `docs/` directory, built with Sphinx + Breathe:

```bash
cd docs
doxygen Doxyfile
LC_ALL=C python3 -m sphinx -b html . _build/html
```

Documentation includes:
- [Quick Start](docs/quickstart.rst) -- Getting started guide
- [Tutorial](docs/tutorial.rst) -- Hands-on examples
- [API Reference](docs/api/index.rst) -- Complete API documentation
- [Migration Guide](docs/migration.rst) -- Migrating from SAPPOROBDD++
- [Examples](docs/examples/index.rst) -- Sample programs (N-Queens, Hamiltonian, CNF SAT, etc.)

## License

This project is licensed under the MIT License. See [LICENSE](LICENSE) for details.

Copyright (c) <AUTHOR_NAME>
