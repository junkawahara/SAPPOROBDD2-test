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

// ============== Graphillion Format ==============
// Compatible with Graphillion library format

/**
 * @brief Import ZDD from Graphillion format
 * @param mgr DDマネージャー
 * @param is 入力ストリーム
 * @param root_level ルートレベル（-1で自動検出）
 * @return インポートされたZDD
 */
ZDD import_zdd_as_graphillion(DDManager& mgr, std::istream& is, int root_level = -1);

/**
 * @brief Import ZDD from Graphillion format file
 * @param mgr DDマネージャー
 * @param filename ファイル名
 * @param root_level ルートレベル
 * @return インポートされたZDD
 */
ZDD import_zdd_as_graphillion(DDManager& mgr, const std::string& filename, int root_level = -1);

/**
 * @brief Export ZDD to Graphillion format
 * @param zdd エクスポートするZDD
 * @param os 出力ストリーム
 * @param root_level ルートレベル（-1で自動）
 */
void export_zdd_as_graphillion(const ZDD& zdd, std::ostream& os, int root_level = -1);

/**
 * @brief Export ZDD to Graphillion format file
 * @param zdd エクスポートするZDD
 * @param filename ファイル名
 * @param root_level ルートレベル
 */
void export_zdd_as_graphillion(const ZDD& zdd, const std::string& filename, int root_level = -1);

// ============== Knuth Format ==============
// Compatible with Knuth's BDD format

/**
 * @brief Import ZDD from Knuth format
 * @param mgr DDマネージャー
 * @param is 入力ストリーム
 * @param is_hex 16進数形式かどうか
 * @param root_level ルートレベル
 * @return インポートされたZDD
 */
ZDD import_zdd_as_knuth(DDManager& mgr, std::istream& is, bool is_hex = false, int root_level = -1);

/**
 * @brief Import ZDD from Knuth format file
 * @param mgr DDマネージャー
 * @param filename ファイル名
 * @param is_hex 16進数形式かどうか
 * @param root_level ルートレベル
 * @return インポートされたZDD
 */
ZDD import_zdd_as_knuth(DDManager& mgr, const std::string& filename, bool is_hex = false, int root_level = -1);

/**
 * @brief Export ZDD to Knuth format
 * @param zdd エクスポートするZDD
 * @param os 出力ストリーム
 * @param is_hex 16進数形式で出力するか
 */
void export_zdd_as_knuth(const ZDD& zdd, std::ostream& os, bool is_hex = false);

/**
 * @brief Export ZDD to Knuth format file
 * @param zdd エクスポートするZDD
 * @param filename ファイル名
 * @param is_hex 16進数形式で出力するか
 */
void export_zdd_as_knuth(const ZDD& zdd, const std::string& filename, bool is_hex = false);

// ============== lib_bdd Format ==============
// Compatible with Rust's lib-bdd library binary format
// https://github.com/sybila/biodivine-lib-bdd
//
// Format: 10 bytes per node (little-endian)
// - var_type (uint16_t): variable level (0xFFFF = terminal)
// - ptr_type (uint32_t): low child pointer
// - ptr_type (uint32_t): high child pointer
// Index 0 = false terminal, Index 1 = true terminal

/**
 * @brief Import BDD from lib_bdd format
 * @param mgr DDManager
 * @param is Input stream
 * @return Imported BDD
 */
BDD import_bdd_as_libbdd(DDManager& mgr, std::istream& is);

/**
 * @brief Import BDD from lib_bdd format file
 * @param mgr DDManager
 * @param filename File path
 * @return Imported BDD
 */
BDD import_bdd_as_libbdd(DDManager& mgr, const std::string& filename);

/**
 * @brief Import ZDD from lib_bdd format
 * @param mgr DDManager
 * @param is Input stream
 * @return Imported ZDD
 */
ZDD import_zdd_as_libbdd(DDManager& mgr, std::istream& is);

/**
 * @brief Import ZDD from lib_bdd format file
 * @param mgr DDManager
 * @param filename File path
 * @return Imported ZDD
 */
ZDD import_zdd_as_libbdd(DDManager& mgr, const std::string& filename);

/**
 * @brief Export BDD to lib_bdd format
 * @param bdd BDD to export
 * @param os Output stream
 */
void export_bdd_as_libbdd(const BDD& bdd, std::ostream& os);

/**
 * @brief Export BDD to lib_bdd format file
 * @param bdd BDD to export
 * @param filename File path
 */
void export_bdd_as_libbdd(const BDD& bdd, const std::string& filename);

/**
 * @brief Export ZDD to lib_bdd format
 * @param zdd ZDD to export
 * @param os Output stream
 */
void export_zdd_as_libbdd(const ZDD& zdd, std::ostream& os);

/**
 * @brief Export ZDD to lib_bdd format file
 * @param zdd ZDD to export
 * @param filename File path
 */
void export_zdd_as_libbdd(const ZDD& zdd, const std::string& filename);

// ============== SVG Format ==============
// Export ZDD as SVG visualization

/**
 * @brief SVGエクスポートオプション
 */
struct SvgExportOptions {
    int width = 800;                          ///< SVG幅
    int height = 600;                         ///< SVG高さ
    std::string node_fill_color = "#ffffff";  ///< ノード塗りつぶし色
    std::string node_stroke_color = "#000000"; ///< ノード輪郭色
    std::string edge_0_color = "#0000ff";     ///< 0枝の色（点線）
    std::string edge_1_color = "#000000";     ///< 1枝の色（実線）
    std::string terminal_0_color = "#ff6666"; ///< 終端0の色
    std::string terminal_1_color = "#66ff66"; ///< 終端1の色
    std::string font_family = "sans-serif";   ///< フォントファミリー
    int font_size = 12;                       ///< フォントサイズ
    bool show_terminal_labels = true;         ///< 終端ラベルを表示
    bool show_variable_labels = true;         ///< 変数ラベルを表示
    int node_radius = 20;                     ///< ノード半径
    int level_gap = 60;                       ///< レベル間の間隔
    int horizontal_gap = 40;                  ///< 水平間隔
};

/**
 * @brief Export ZDD as SVG
 * @param zdd エクスポートするZDD
 * @param os 出力ストリーム
 * @param options SVGオプション
 */
void export_zdd_as_svg(const ZDD& zdd, std::ostream& os,
                       const SvgExportOptions& options = SvgExportOptions());

/**
 * @brief Export ZDD as SVG file
 * @param zdd エクスポートするZDD
 * @param filename ファイル名
 * @param options SVGオプション
 */
void export_zdd_as_svg(const ZDD& zdd, const std::string& filename,
                       const SvgExportOptions& options = SvgExportOptions());

} // namespace sbdd2

#endif // SBDD2_IO_HPP
