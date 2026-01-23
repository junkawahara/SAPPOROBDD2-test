# CLAUDE.md - SAPPOROBDD 2.0 Project Guide

This file provides context for Claude (AI) to understand and work effectively with this codebase.

## Project Overview

SAPPOROBDD 2.0 is a new BDD/ZDD library designed based on SAPPOROBDD++.

- **Language**: C++11 (do not use C++14 or later features)
- **Namespace**: `sbdd2`
- **License**: MIT

## Directory Structure

```
SAPPOROBDD2/
├── include/sbdd2/     # Header files
├── src/               # Source files
├── tests/             # Google Test unit tests
├── docs/              # Sphinx + Breathe documentation
├── build/             # Build directory
└── vendor/            # Reference libraries (gitignored)
    ├── SAPPOROBDD-plus-plus/  # Original library
    └── sbdd_helper/           # Helper library
```

## Main Classes

- `DDManager` - Node management, cache, GC
- `DDNode` - 128-bit node structure
- `BDD` - Reduced BDD
- `ZDD` - Reduced ZDD
- `UnreducedBDD` / `UnreducedZDD` - Non-reduced DD
- `QDD` - Quasi-reduced DD
- `PiDD` - Permutation DD
- `SeqBDD` - Sequence BDD
- `GBase` - Graph base (path/cycle enumeration)
- `BDDCT` - Cost table

## Node Structure (128-bit)

```
Word 0 (64-bit): 0-arc(44) + 1-arc lower(20)
Word 1 (64-bit): 1-arc upper(24) + reduced(1) + refcount(16) + var(20) + reserved(3)
```

- Arc (44-bit): negation(1) + constant(1) + index(42)
- Maximum nodes: ~4.4 trillion

## Build Instructions

```bash
cd build
cmake ..
make -j4
./tests/sbdd2_tests  # Run tests
```

## Coding Conventions

- Class names: PascalCase (`DDManager`, `BDD`)
- Method names: snake_case (`new_var()`, `var_bdd()`)
- Operator overloading: SAPPOROBDD++ compatible
  - BDD: `&`(AND), `|`(OR), `^`(XOR), `~`(NOT)
  - ZDD: `+`(Union), `-`(Diff), `*`(Intersect), `/`(Quotient), `%`(Remainder)

## Git Workflow

- **Commit frequently**: Make small, incremental commits after completing each feature or fix
- Do not batch multiple unrelated changes into a single commit
- **Auto-commit after implementation**: When an implementation task is completed and tests pass, automatically commit the changes without waiting for user instruction

## Design Notes

1. **Negation edges**: BDD uses negation edges (NOT is O(1))
2. **Internal hashing**: Quadratic probing
3. **Reference counting**: 16-bit, saturates at 65535
4. **Thread safety**: Protected by mutex

## Testing

```bash
./tests/sbdd2_tests                           # Run all tests
./tests/sbdd2_tests --gtest_filter=BDDTest.*  # Run BDD tests only
```

## Documentation Generation

```bash
cd docs
doxygen Doxyfile
LC_ALL=C python3 -m sphinx -b html . _build/html
```

## Unimplemented Features

See `next_impl.md` for potential future items:
- ZDDV (ZDD vector) class

## References

- `SAPPOROBDD_2.0_design.md` - Design specification
- `vendor/SAPPOROBDD-plus-plus/` - Original library (API reference)
- `vendor/sbdd_helper/` - Helper library (API reference)
