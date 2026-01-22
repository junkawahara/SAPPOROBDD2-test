// SAPPOROBDD 2.0 - Import/Export functions
// MIT License

#ifndef SBDD2_IO_HPP
#define SBDD2_IO_HPP

#include "bdd.hpp"
#include "zdd.hpp"
#include <string>
#include <iostream>
#include <fstream>
#include <vector>

namespace sbdd2 {

// File format types
enum class DDFileFormat {
    AUTO,       // Auto-detect from extension
    BINARY,     // Binary format (compact)
    TEXT,       // Text format (human-readable)
    DOT,        // GraphViz DOT format
    KNUTH       // Knuth's format
};

// Export options
struct ExportOptions {
    DDFileFormat format = DDFileFormat::BINARY;
    bool include_header = true;
    bool compress = false;
    int precision = 6;  // For floating point in text format
};

// Import options
struct ImportOptions {
    DDFileFormat format = DDFileFormat::AUTO;
};

// ============== BDD Import/Export ==============

// Export BDD to file
bool export_bdd(const BDD& bdd, const std::string& filename,
                const ExportOptions& options = ExportOptions());
bool export_bdd(const BDD& bdd, std::ostream& os,
                const ExportOptions& options = ExportOptions());

// Import BDD from file
BDD import_bdd(DDManager& mgr, const std::string& filename,
               const ImportOptions& options = ImportOptions());
BDD import_bdd(DDManager& mgr, std::istream& is,
               const ImportOptions& options = ImportOptions());

// ============== ZDD Import/Export ==============

// Export ZDD to file
bool export_zdd(const ZDD& zdd, const std::string& filename,
                const ExportOptions& options = ExportOptions());
bool export_zdd(const ZDD& zdd, std::ostream& os,
                const ExportOptions& options = ExportOptions());

// Import ZDD from file
ZDD import_zdd(DDManager& mgr, const std::string& filename,
               const ImportOptions& options = ImportOptions());
ZDD import_zdd(DDManager& mgr, std::istream& is,
               const ImportOptions& options = ImportOptions());

// ============== DOT Export ==============

// Export to DOT format for visualization
std::string to_dot(const BDD& bdd, const std::string& name = "bdd");
std::string to_dot(const ZDD& zdd, const std::string& name = "zdd");

// ============== Binary Format ==============
// Format specification based on:
// https://raw.githubusercontent.com/junkawahara/dd_documents/refs/heads/main/formats/bdd_binary_format.md

/*
Binary format structure:
- Header (16 bytes):
  - Magic: "SBDD" (4 bytes)
  - Version: uint16_t (2 bytes)
  - Type: uint8_t (1 byte: 0=BDD, 1=ZDD)
  - Flags: uint8_t (1 byte)
  - Node count: uint64_t (8 bytes)
- Node table:
  - For each node:
    - Variable: uint32_t (4 bytes)
    - Low: uint64_t (8 bytes, arc format)
    - High: uint64_t (8 bytes, arc format)
- Root arc: uint64_t (8 bytes)
*/

// Magic number
constexpr char DD_MAGIC[4] = {'S', 'B', 'D', 'D'};
constexpr std::uint16_t DD_VERSION = 1;

// Type codes
constexpr std::uint8_t DD_TYPE_BDD = 0;
constexpr std::uint8_t DD_TYPE_ZDD = 1;

// ============== Text Format ==============

/*
Text format structure:
Line 1: "BDD" or "ZDD"
Line 2: Number of nodes
Line 3+: Node definitions (one per line)
  Format: node_id var_number low_id high_id
  - Terminal 0: "T0"
  - Terminal 1: "T1"
  - Negated arc: prefix with "~"
Last line: Root reference
*/

// ============== Utility Functions ==============

// Detect format from filename extension
DDFileFormat detect_format(const std::string& filename);

// Validation
bool validate_bdd(const BDD& bdd);
bool validate_zdd(const ZDD& zdd);

} // namespace sbdd2

#endif // SBDD2_IO_HPP
