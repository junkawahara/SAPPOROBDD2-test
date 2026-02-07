/**
 * @file io.hpp
 * @brief SAPPOROBDD 2.0 - BDD/ZDDの入出力関数
 * @author SAPPOROBDD Team
 * @copyright MIT License
 *
 * BDD/ZDDの各種フォーマットでのインポート・エクスポート機能を提供する。
 * 対応フォーマット: バイナリ、テキスト、DOT、Knuth、Graphillion、lib_bdd、SVG
 */

#ifndef SBDD2_IO_HPP
#define SBDD2_IO_HPP

#include "bdd.hpp"
#include "zdd.hpp"
#include <string>
#include <iostream>
#include <fstream>
#include <vector>

namespace sbdd2 {

/**
 * @brief DDファイルフォーマットの種類
 *
 * インポート・エクスポート時に使用するファイル形式を指定する列挙型。
 */
enum class DDFileFormat {
    AUTO,       ///< 拡張子から自動検出
    BINARY,     ///< バイナリ形式（コンパクト）
    TEXT,       ///< テキスト形式（可読性あり）
    DOT,        ///< GraphViz DOT形式（可視化用）
    KNUTH       ///< Knuth形式
};

/**
 * @brief エクスポートオプション
 *
 * BDD/ZDDをファイルに書き出す際の設定を格納する構造体。
 */
struct ExportOptions {
    DDFileFormat format = DDFileFormat::BINARY;  ///< 出力フォーマット
    bool include_header = true;                  ///< ヘッダを含めるかどうか
    bool compress = false;                       ///< 圧縮を有効にするかどうか
    int precision = 6;                           ///< テキスト形式での浮動小数点精度
};

/**
 * @brief インポートオプション
 *
 * BDD/ZDDをファイルから読み込む際の設定を格納する構造体。
 */
struct ImportOptions {
    DDFileFormat format = DDFileFormat::AUTO;  ///< 入力フォーマット（AUTOで自動検出）
};

// ============== BDD Import/Export ==============

/**
 * @brief BDDをファイルにエクスポートする
 * @param bdd エクスポートするBDD
 * @param filename 出力ファイルパス
 * @param options エクスポートオプション
 * @return エクスポート成功時にtrue、失敗時にfalse
 * @see import_bdd, ExportOptions
 */
bool export_bdd(const BDD& bdd, const std::string& filename,
                const ExportOptions& options = ExportOptions());

/**
 * @brief BDDを出力ストリームにエクスポートする
 * @param bdd エクスポートするBDD
 * @param os 出力ストリーム
 * @param options エクスポートオプション
 * @return エクスポート成功時にtrue、失敗時にfalse
 * @see import_bdd, ExportOptions
 */
bool export_bdd(const BDD& bdd, std::ostream& os,
                const ExportOptions& options = ExportOptions());

/**
 * @brief ファイルからBDDをインポートする
 * @param mgr DDマネージャー
 * @param filename 入力ファイルパス
 * @param options インポートオプション
 * @return インポートされたBDD
 * @see export_bdd, ImportOptions
 */
BDD import_bdd(DDManager& mgr, const std::string& filename,
               const ImportOptions& options = ImportOptions());

/**
 * @brief 入力ストリームからBDDをインポートする
 * @param mgr DDマネージャー
 * @param is 入力ストリーム
 * @param options インポートオプション
 * @return インポートされたBDD
 * @see export_bdd, ImportOptions
 */
BDD import_bdd(DDManager& mgr, std::istream& is,
               const ImportOptions& options = ImportOptions());

// ============== ZDD Import/Export ==============

/**
 * @brief ZDDをファイルにエクスポートする
 * @param zdd エクスポートするZDD
 * @param filename 出力ファイルパス
 * @param options エクスポートオプション
 * @return エクスポート成功時にtrue、失敗時にfalse
 * @see import_zdd, ExportOptions
 */
bool export_zdd(const ZDD& zdd, const std::string& filename,
                const ExportOptions& options = ExportOptions());

/**
 * @brief ZDDを出力ストリームにエクスポートする
 * @param zdd エクスポートするZDD
 * @param os 出力ストリーム
 * @param options エクスポートオプション
 * @return エクスポート成功時にtrue、失敗時にfalse
 * @see import_zdd, ExportOptions
 */
bool export_zdd(const ZDD& zdd, std::ostream& os,
                const ExportOptions& options = ExportOptions());

/**
 * @brief ファイルからZDDをインポートする
 * @param mgr DDマネージャー
 * @param filename 入力ファイルパス
 * @param options インポートオプション
 * @return インポートされたZDD
 * @see export_zdd, ImportOptions
 */
ZDD import_zdd(DDManager& mgr, const std::string& filename,
               const ImportOptions& options = ImportOptions());

/**
 * @brief 入力ストリームからZDDをインポートする
 * @param mgr DDマネージャー
 * @param is 入力ストリーム
 * @param options インポートオプション
 * @return インポートされたZDD
 * @see export_zdd, ImportOptions
 */
ZDD import_zdd(DDManager& mgr, std::istream& is,
               const ImportOptions& options = ImportOptions());

// ============== DOT Export ==============

/**
 * @brief BDDをGraphViz DOT形式の文字列に変換する
 * @param bdd 変換するBDD
 * @param name DOTグラフの名前
 * @return DOT形式の文字列
 * @see export_bdd
 */
std::string to_dot(const BDD& bdd, const std::string& name = "bdd");

/**
 * @brief ZDDをGraphViz DOT形式の文字列に変換する
 * @param zdd 変換するZDD
 * @param name DOTグラフの名前
 * @return DOT形式の文字列
 * @see export_zdd
 */
std::string to_dot(const ZDD& zdd, const std::string& name = "zdd");

// ============== Binary Format ==============

/**
 * @brief バイナリ形式の仕様
 *
 * フォーマット仕様:
 * https://raw.githubusercontent.com/junkawahara/dd_documents/refs/heads/main/formats/bdd_binary_format.md
 *
 * バイナリ形式の構造:
 * - ヘッダ (16バイト):
 *   - マジックナンバー: "SBDD" (4バイト)
 *   - バージョン: uint16_t (2バイト)
 *   - 種別: uint8_t (1バイト: 0=BDD, 1=ZDD)
 *   - フラグ: uint8_t (1バイト)
 *   - ノード数: uint64_t (8バイト)
 * - ノードテーブル:
 *   - 各ノードについて:
 *     - 変数番号: uint32_t (4バイト)
 *     - 0枝: uint64_t (8バイト, アーク形式)
 *     - 1枝: uint64_t (8バイト, アーク形式)
 * - ルートアーク: uint64_t (8バイト)
 */

constexpr char DD_MAGIC[4] = {'S', 'B', 'D', 'D'};  ///< マジックナンバー
constexpr std::uint16_t DD_VERSION = 1;               ///< フォーマットバージョン

constexpr std::uint8_t DD_TYPE_BDD = 0;  ///< BDD種別コード
constexpr std::uint8_t DD_TYPE_ZDD = 1;  ///< ZDD種別コード

// ============== Text Format ==============

/**
 * @brief テキスト形式の仕様
 *
 * テキスト形式の構造:
 * - 1行目: "BDD" または "ZDD"
 * - 2行目: ノード数
 * - 3行目以降: ノード定義（1行に1ノード）
 *   - 形式: ノードID 変数番号 0枝ID 1枝ID
 *   - 終端0: "T0"
 *   - 終端1: "T1"
 *   - 否定枝: "~" プレフィックス付き
 * - 最終行: ルート参照
 */

// ============== Utility Functions ==============

/**
 * @brief ファイル名の拡張子からフォーマットを自動検出する
 * @param filename ファイル名
 * @return 検出されたフォーマット種別
 * @see DDFileFormat
 */
DDFileFormat detect_format(const std::string& filename);

/**
 * @brief BDDの整合性を検証する
 * @param bdd 検証するBDD
 * @return 整合性が正しければtrue、不正な場合はfalse
 * @see validate_zdd
 */
bool validate_bdd(const BDD& bdd);

/**
 * @brief ZDDの整合性を検証する
 * @param zdd 検証するZDD
 * @return 整合性が正しければtrue、不正な場合はfalse
 * @see validate_bdd
 */
bool validate_zdd(const ZDD& zdd);

// ============== Graphillion Format ==============
// Graphillionライブラリとの互換フォーマット

/**
 * @brief Graphillion形式のストリームからZDDをインポートする
 * @param mgr DDマネージャー
 * @param is 入力ストリーム
 * @param root_level ルートレベル（-1で自動検出）
 * @return インポートされたZDD
 * @see export_zdd_as_graphillion
 */
ZDD import_zdd_as_graphillion(DDManager& mgr, std::istream& is, int root_level = -1);

/**
 * @brief Graphillion形式のファイルからZDDをインポートする
 * @param mgr DDマネージャー
 * @param filename 入力ファイルパス
 * @param root_level ルートレベル（-1で自動検出）
 * @return インポートされたZDD
 * @see export_zdd_as_graphillion
 */
ZDD import_zdd_as_graphillion(DDManager& mgr, const std::string& filename, int root_level = -1);

/**
 * @brief ZDDをGraphillion形式でストリームにエクスポートする
 * @param zdd エクスポートするZDD
 * @param os 出力ストリーム
 * @param root_level ルートレベル（-1で自動設定）
 * @see import_zdd_as_graphillion
 */
void export_zdd_as_graphillion(const ZDD& zdd, std::ostream& os, int root_level = -1);

/**
 * @brief ZDDをGraphillion形式でファイルにエクスポートする
 * @param zdd エクスポートするZDD
 * @param filename 出力ファイルパス
 * @param root_level ルートレベル（-1で自動設定）
 * @see import_zdd_as_graphillion
 */
void export_zdd_as_graphillion(const ZDD& zdd, const std::string& filename, int root_level = -1);

// ============== Knuth Format ==============
// Knuth形式のBDD/ZDDフォーマットとの互換

/**
 * @brief Knuth形式のストリームからZDDをインポートする
 * @param mgr DDマネージャー
 * @param is 入力ストリーム
 * @param is_hex trueの場合、16進数形式として解釈する
 * @param root_level ルートレベル（-1で自動検出）
 * @return インポートされたZDD
 * @see export_zdd_as_knuth
 */
ZDD import_zdd_as_knuth(DDManager& mgr, std::istream& is, bool is_hex = false, int root_level = -1);

/**
 * @brief Knuth形式のファイルからZDDをインポートする
 * @param mgr DDマネージャー
 * @param filename 入力ファイルパス
 * @param is_hex trueの場合、16進数形式として解釈する
 * @param root_level ルートレベル（-1で自動検出）
 * @return インポートされたZDD
 * @see export_zdd_as_knuth
 */
ZDD import_zdd_as_knuth(DDManager& mgr, const std::string& filename, bool is_hex = false, int root_level = -1);

/**
 * @brief ZDDをKnuth形式でストリームにエクスポートする
 * @param zdd エクスポートするZDD
 * @param os 出力ストリーム
 * @param is_hex trueの場合、16進数形式で出力する
 * @see import_zdd_as_knuth
 */
void export_zdd_as_knuth(const ZDD& zdd, std::ostream& os, bool is_hex = false);

/**
 * @brief ZDDをKnuth形式でファイルにエクスポートする
 * @param zdd エクスポートするZDD
 * @param filename 出力ファイルパス
 * @param is_hex trueの場合、16進数形式で出力する
 * @see import_zdd_as_knuth
 */
void export_zdd_as_knuth(const ZDD& zdd, const std::string& filename, bool is_hex = false);

// ============== lib_bdd Format ==============

/**
 * @brief lib_bddバイナリ形式
 *
 * Rustのlib-bddライブラリ (https://github.com/sybila/biodivine-lib-bdd) との
 * 互換バイナリ形式。
 *
 * 形式: 1ノードあたり10バイト（リトルエンディアン）
 * - var_type (uint16_t): 変数レベル（0xFFFF = 終端）
 * - ptr_type (uint32_t): 0枝の子ノードポインタ
 * - ptr_type (uint32_t): 1枝の子ノードポインタ
 * - インデックス0 = 偽終端、インデックス1 = 真終端
 */

/**
 * @brief lib_bdd形式のストリームからBDDをインポートする
 * @param mgr DDマネージャー
 * @param is 入力ストリーム
 * @return インポートされたBDD
 * @see export_bdd_as_libbdd
 */
BDD import_bdd_as_libbdd(DDManager& mgr, std::istream& is);

/**
 * @brief lib_bdd形式のファイルからBDDをインポートする
 * @param mgr DDマネージャー
 * @param filename 入力ファイルパス
 * @return インポートされたBDD
 * @see export_bdd_as_libbdd
 */
BDD import_bdd_as_libbdd(DDManager& mgr, const std::string& filename);

/**
 * @brief lib_bdd形式のストリームからZDDをインポートする
 * @param mgr DDマネージャー
 * @param is 入力ストリーム
 * @return インポートされたZDD
 * @see export_zdd_as_libbdd
 */
ZDD import_zdd_as_libbdd(DDManager& mgr, std::istream& is);

/**
 * @brief lib_bdd形式のファイルからZDDをインポートする
 * @param mgr DDマネージャー
 * @param filename 入力ファイルパス
 * @return インポートされたZDD
 * @see export_zdd_as_libbdd
 */
ZDD import_zdd_as_libbdd(DDManager& mgr, const std::string& filename);

/**
 * @brief BDDをlib_bdd形式でストリームにエクスポートする
 * @param bdd エクスポートするBDD
 * @param os 出力ストリーム
 * @see import_bdd_as_libbdd
 */
void export_bdd_as_libbdd(const BDD& bdd, std::ostream& os);

/**
 * @brief BDDをlib_bdd形式でファイルにエクスポートする
 * @param bdd エクスポートするBDD
 * @param filename 出力ファイルパス
 * @see import_bdd_as_libbdd
 */
void export_bdd_as_libbdd(const BDD& bdd, const std::string& filename);

/**
 * @brief ZDDをlib_bdd形式でストリームにエクスポートする
 * @param zdd エクスポートするZDD
 * @param os 出力ストリーム
 * @see import_zdd_as_libbdd
 */
void export_zdd_as_libbdd(const ZDD& zdd, std::ostream& os);

/**
 * @brief ZDDをlib_bdd形式でファイルにエクスポートする
 * @param zdd エクスポートするZDD
 * @param filename 出力ファイルパス
 * @see import_zdd_as_libbdd
 */
void export_zdd_as_libbdd(const ZDD& zdd, const std::string& filename);

// ============== SVG Format ==============

/**
 * @brief SVGエクスポートオプション
 *
 * ZDDをSVG画像として出力する際の描画設定を格納する構造体。
 * ノードの色、サイズ、フォントなど、SVG出力の各種パラメータを制御する。
 *
 * @see export_zdd_as_svg
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
 * @brief ZDDをSVG形式でストリームにエクスポートする
 * @param zdd エクスポートするZDD
 * @param os 出力ストリーム
 * @param options SVGエクスポートオプション
 * @see SvgExportOptions
 */
void export_zdd_as_svg(const ZDD& zdd, std::ostream& os,
                       const SvgExportOptions& options = SvgExportOptions());

/**
 * @brief ZDDをSVG形式でファイルにエクスポートする
 * @param zdd エクスポートするZDD
 * @param filename 出力ファイルパス
 * @param options SVGエクスポートオプション
 * @see SvgExportOptions
 */
void export_zdd_as_svg(const ZDD& zdd, const std::string& filename,
                       const SvgExportOptions& options = SvgExportOptions());

} // namespace sbdd2

#endif // SBDD2_IO_HPP
