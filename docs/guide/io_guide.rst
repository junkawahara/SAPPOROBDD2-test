.. _io_guide:

I/Oガイド
=========

この章では、SAPPOROBDD 2.0のBDD/ZDDの入出力機能を解説します。
バイナリ形式、テキスト形式、GraphViz DOT形式、Knuth形式、Graphillion形式、
lib_bdd形式、SVG形式など、さまざまなフォーマットに対応しています。

.. contents:: 目次
   :local:
   :depth: 2

フォーマット概要
----------------

SAPPOROBDD 2.0がサポートする入出力フォーマットの一覧です。

.. list-table:: サポートされるフォーマット
   :header-rows: 1
   :widths: 15 15 15 55

   * - フォーマット
     - インポート
     - エクスポート
     - 説明
   * - BINARY
     - BDD/ZDD
     - BDD/ZDD
     - 独自バイナリ形式。コンパクトで高速
   * - TEXT
     - BDD/ZDD
     - BDD/ZDD
     - 人間が読めるテキスト形式
   * - DOT
     - \-
     - BDD/ZDD
     - GraphViz DOT形式（可視化用）
   * - Knuth
     - ZDD
     - ZDD
     - Knuth先生のBDD形式（10進/16進）
   * - Graphillion
     - ZDD
     - ZDD
     - Graphillionライブラリ互換形式
   * - lib_bdd
     - BDD/ZDD
     - BDD/ZDD
     - Rust lib-bdd ライブラリ互換形式（10バイト/ノード）
   * - SVG
     - \-
     - ZDD
     - SVG画像形式（可視化用）

.. code-block:: cpp

   #include <sbdd2/sbdd2.hpp>
   using namespace sbdd2;

汎用エクスポート/インポート
---------------------------

BDDのエクスポート/インポート
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   DDManager mgr;
   mgr.new_var();
   mgr.new_var();
   BDD f = mgr.var_bdd(1) & mgr.var_bdd(2);

   // バイナリ形式でエクスポート
   ExportOptions bin_opts;
   bin_opts.format = DDFileFormat::BINARY;
   export_bdd(f, "bdd.bin", bin_opts);

   // テキスト形式でエクスポート
   ExportOptions txt_opts;
   txt_opts.format = DDFileFormat::TEXT;
   export_bdd(f, "bdd.txt", txt_opts);

   // インポート（フォーマット自動検出）
   DDManager mgr2;
   BDD f2 = import_bdd(mgr2, "bdd.bin");

ZDDのエクスポート/インポート
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   DDManager mgr;
   mgr.new_var();
   mgr.new_var();
   ZDD s1 = ZDD::singleton(mgr, 1);
   ZDD s2 = ZDD::singleton(mgr, 2);
   ZDD family = s1 + s2;  // {{1}, {2}}

   // エクスポート
   ExportOptions opts;
   opts.format = DDFileFormat::BINARY;
   export_zdd(family, "zdd.bin", opts);

   // インポート
   DDManager mgr2;
   ZDD family2 = import_zdd(mgr2, "zdd.bin");

ストリームからの入出力
~~~~~~~~~~~~~~~~~~~~~~

ファイル名の代わりに ``std::ostream`` / ``std::istream`` を使うこともできます。

.. code-block:: cpp

   #include <fstream>
   #include <sstream>

   // ストリームへエクスポート
   std::ofstream ofs("output.bin", std::ios::binary);
   export_zdd(family, ofs);
   ofs.close();

   // ストリームからインポート
   std::ifstream ifs("output.bin", std::ios::binary);
   ZDD imported = import_zdd(mgr, ifs);
   ifs.close();

   // メモリ上のバッファにエクスポート
   std::ostringstream oss;
   export_zdd(family, oss);
   std::string data = oss.str();

ExportOptions
~~~~~~~~~~~~~

``ExportOptions`` 構造体でエクスポートの挙動を制御できます。

.. code-block:: cpp

   ExportOptions opts;
   opts.format = DDFileFormat::BINARY;   // フォーマット
   opts.include_header = true;           // ヘッダーを含める
   opts.compress = false;                // 圧縮しない
   opts.precision = 6;                   // テキスト形式の浮動小数点精度

バイナリ形式
------------

バイナリ形式は、BDD/ZDDをコンパクトかつ高速に保存・読み込みするための形式です。

フォーマット構造
~~~~~~~~~~~~~~~~

.. code-block:: text

   ヘッダー（16バイト）:
   - マジック: "SBDD" (4バイト)
   - バージョン: uint16_t (2バイト)
   - タイプ: uint8_t (1バイト: 0=BDD, 1=ZDD)
   - フラグ: uint8_t (1バイト)
   - ノード数: uint64_t (8バイト)

   ノードテーブル:
   - 各ノード:
     - 変数番号: uint32_t (4バイト)
     - Low枝: uint64_t (8バイト、アーク形式)
     - High枝: uint64_t (8バイト、アーク形式)

   ルートアーク: uint64_t (8バイト)

使用例
~~~~~~

.. code-block:: cpp

   // バイナリ形式でエクスポート
   ExportOptions opts;
   opts.format = DDFileFormat::BINARY;
   export_bdd(bdd, "data.sbdd", opts);

   // インポート
   BDD imported = import_bdd(mgr, "data.sbdd");

テキスト形式
------------

テキスト形式は、人間が読める形式でBDD/ZDDを保存します。
デバッグや小規模なデータの確認に便利です。

フォーマット構造
~~~~~~~~~~~~~~~~

.. code-block:: text

   行1: "BDD" または "ZDD"
   行2: ノード数
   行3以降: ノード定義（1行1ノード）
     形式: ノードID 変数番号 Low子ID High子ID
     - 終端0: "T0"
     - 終端1: "T1"
     - 否定枝: "~" プレフィックス
   最終行: ルート参照

使用例
~~~~~~

.. code-block:: cpp

   // テキスト形式でエクスポート
   ExportOptions opts;
   opts.format = DDFileFormat::TEXT;
   export_zdd(zdd, "data.txt", opts);

   // テキスト形式でインポート
   ImportOptions imp_opts;
   imp_opts.format = DDFileFormat::TEXT;
   ZDD imported = import_zdd(mgr, "data.txt", imp_opts);

DOT形式
-------

GraphViz DOT形式は、BDD/ZDDをグラフとして可視化するための形式です。
``dot`` コマンドを使ってPDFやPNG画像に変換できます。

.. code-block:: cpp

   DDManager mgr;
   mgr.new_var();
   mgr.new_var();
   mgr.new_var();

   BDD f = (mgr.var_bdd(1) & mgr.var_bdd(2)) | mgr.var_bdd(3);

   // DOT文字列を取得
   std::string dot = to_dot(f, "my_bdd");
   std::cout << dot << std::endl;

   // ファイルに書き出し
   ExportOptions opts;
   opts.format = DDFileFormat::DOT;
   export_bdd(f, "bdd.dot", opts);

   // ZDDのDOT出力
   ZDD z = ZDD::singleton(mgr, 1) + ZDD::singleton(mgr, 2);
   std::string zdd_dot = to_dot(z, "my_zdd");

DOTファイルからの画像生成（コマンドライン）:

.. code-block:: bash

   # PDF出力
   dot -Tpdf bdd.dot -o bdd.pdf

   # PNG出力
   dot -Tpng bdd.dot -o bdd.png

   # SVG出力
   dot -Tsvg bdd.dot -o bdd.svg

DOT形式では以下の規約でBDD/ZDDが表示されます:

- 点線: 0枝（Low枝）
- 実線: 1枝（High枝）
- 四角形: 終端ノード（0終端、1終端）
- 円形: 内部ノード（変数番号がラベル）

Knuth形式
---------

Knuth先生が著書で使用しているBDD/ZDDフォーマットです。
10進数形式と16進数形式の2種類があります。

.. code-block:: cpp

   DDManager mgr;
   mgr.new_var();
   mgr.new_var();
   ZDD z = ZDD::singleton(mgr, 1) + ZDD::singleton(mgr, 2);

   // 10進数形式でエクスポート
   export_zdd_as_knuth(z, "zdd_knuth.txt");

   // 16進数形式でエクスポート
   export_zdd_as_knuth(z, "zdd_knuth_hex.txt", true);

   // ストリームへ出力
   export_zdd_as_knuth(z, std::cout);

   // インポート（10進数形式）
   DDManager mgr2;
   ZDD z2 = import_zdd_as_knuth(mgr2, "zdd_knuth.txt");

   // インポート（16進数形式）
   ZDD z3 = import_zdd_as_knuth(mgr2, "zdd_knuth_hex.txt", true);

   // ルートレベルを指定してインポート
   ZDD z4 = import_zdd_as_knuth(mgr2, "zdd_knuth.txt", false, 5);

Graphillion形式
---------------

Graphillionライブラリとの相互運用のためのフォーマットです。

.. code-block:: cpp

   DDManager mgr;
   mgr.new_var();
   mgr.new_var();
   mgr.new_var();

   ZDD z = ZDD::singleton(mgr, 1) + ZDD::singleton(mgr, 2);

   // Graphillion形式でエクスポート
   export_zdd_as_graphillion(z, "zdd_graphillion.txt");

   // ストリームへエクスポート
   export_zdd_as_graphillion(z, std::cout);

   // ルートレベルを指定してエクスポート
   export_zdd_as_graphillion(z, "zdd_graphillion.txt", 3);

   // インポート
   DDManager mgr2;
   ZDD z2 = import_zdd_as_graphillion(mgr2, "zdd_graphillion.txt");

   // ルートレベルを指定してインポート
   ZDD z3 = import_zdd_as_graphillion(mgr2, "zdd_graphillion.txt", 3);

lib_bdd形式
-----------

Rustの `biodivine-lib-bdd <https://github.com/sybila/biodivine-lib-bdd>`_
ライブラリと互換性のあるバイナリ形式です。
1ノードあたり10バイトのコンパクトな形式で、BDDとZDDの両方に対応しています。

フォーマット構造
~~~~~~~~~~~~~~~~

.. code-block:: text

   各ノード（10バイト、リトルエンディアン）:
   - var_type: uint16_t (変数レベル、0xFFFF = 終端)
   - low_ptr: uint32_t (0枝の子ポインタ)
   - high_ptr: uint32_t (1枝の子ポインタ)

   インデックス 0 = 偽終端
   インデックス 1 = 真終端

使用例
~~~~~~

.. code-block:: cpp

   DDManager mgr;
   mgr.new_var();
   mgr.new_var();
   BDD f = mgr.var_bdd(1) & mgr.var_bdd(2);

   // BDDをlib_bdd形式でエクスポート
   export_bdd_as_libbdd(f, "bdd.libbdd");

   // インポート
   DDManager mgr2;
   BDD f2 = import_bdd_as_libbdd(mgr2, "bdd.libbdd");

   // ZDDにも対応
   ZDD z = ZDD::singleton(mgr, 1);
   export_zdd_as_libbdd(z, "zdd.libbdd");
   ZDD z2 = import_zdd_as_libbdd(mgr2, "zdd.libbdd");

   // ストリーム版
   std::ofstream ofs("output.libbdd", std::ios::binary);
   export_bdd_as_libbdd(f, ofs);
   ofs.close();

   std::ifstream ifs("output.libbdd", std::ios::binary);
   BDD f3 = import_bdd_as_libbdd(mgr, ifs);
   ifs.close();

SVG形式
-------

ZDDをSVG画像として直接出力できます。
外部ツールなしでBDD/ZDDの可視化が可能です。

基本的な使い方
~~~~~~~~~~~~~~

.. code-block:: cpp

   DDManager mgr;
   mgr.new_var();
   mgr.new_var();
   mgr.new_var();

   ZDD z = ZDD::singleton(mgr, 1) + ZDD::singleton(mgr, 2);

   // デフォルト設定でSVGエクスポート
   export_zdd_as_svg(z, "zdd.svg");

   // ストリームに出力
   export_zdd_as_svg(z, std::cout);

カスタマイズ
~~~~~~~~~~~~

``SvgExportOptions`` 構造体で外観を詳細にカスタマイズできます。

.. code-block:: cpp

   SvgExportOptions svg_opts;
   svg_opts.width = 1200;                           // SVG幅
   svg_opts.height = 800;                            // SVG高さ
   svg_opts.node_fill_color = "#e0e0ff";             // ノード塗りつぶし色
   svg_opts.node_stroke_color = "#333333";           // ノード輪郭色
   svg_opts.edge_0_color = "#0000ff";                // 0枝の色（点線）
   svg_opts.edge_1_color = "#ff0000";                // 1枝の色（実線）
   svg_opts.terminal_0_color = "#ff6666";            // 終端0の色
   svg_opts.terminal_1_color = "#66ff66";            // 終端1の色
   svg_opts.font_family = "monospace";               // フォントファミリー
   svg_opts.font_size = 14;                          // フォントサイズ
   svg_opts.show_terminal_labels = true;             // 終端ラベルを表示
   svg_opts.show_variable_labels = true;             // 変数ラベルを表示
   svg_opts.node_radius = 25;                        // ノード半径
   svg_opts.level_gap = 80;                          // レベル間の間隔
   svg_opts.horizontal_gap = 50;                     // 水平間隔

   export_zdd_as_svg(z, "zdd_custom.svg", svg_opts);

フォーマット自動検出
--------------------

``detect_format()`` 関数は、ファイル名の拡張子からフォーマットを自動検出します。

.. code-block:: cpp

   DDFileFormat fmt;

   fmt = detect_format("data.sbdd");    // DDFileFormat::BINARY
   fmt = detect_format("data.bin");     // DDFileFormat::BINARY
   fmt = detect_format("data.txt");     // DDFileFormat::TEXT
   fmt = detect_format("data.dot");     // DDFileFormat::DOT
   fmt = detect_format("data.knuth");   // DDFileFormat::KNUTH

``ImportOptions`` でフォーマットを ``DDFileFormat::AUTO`` に設定すると、
この自動検出が使用されます（デフォルト）。

.. code-block:: cpp

   // 自動検出でインポート
   ImportOptions opts;
   opts.format = DDFileFormat::AUTO;  // デフォルト
   BDD f = import_bdd(mgr, "data.sbdd", opts);

バリデーション
--------------

``validate_bdd()`` と ``validate_zdd()`` は、
BDD/ZDDの内部構造が正しいかどうかを検証します。

.. code-block:: cpp

   DDManager mgr;
   mgr.new_var();
   mgr.new_var();

   BDD f = mgr.var_bdd(1) & mgr.var_bdd(2);
   ZDD z = ZDD::singleton(mgr, 1);

   // BDDの検証
   bool bdd_ok = validate_bdd(f);
   std::cout << "BDDは有効: " << bdd_ok << std::endl;

   // ZDDの検証
   bool zdd_ok = validate_zdd(z);
   std::cout << "ZDDは有効: " << zdd_ok << std::endl;

バリデーションでは以下の項目がチェックされます:

- ノード構造の整合性
- 変数順序の正しさ（親のレベル > 子のレベル）
- 終端ノードの正しさ
- 参照の有効性

.. note::

   バリデーションはデバッグ目的で使用してください。
   大規模なBDD/ZDDに対しては処理時間がかかる場合があります。

実践例: ZDDの保存と復元
------------------------

.. code-block:: cpp

   #include <sbdd2/sbdd2.hpp>
   using namespace sbdd2;

   void save_zdd() {
       DDManager mgr;
       for (int i = 0; i < 10; ++i) {
           mgr.new_var();
       }

       // 複雑なZDDを構築
       ZDD z = ZDD::single(mgr);
       for (int i = 1; i <= 10; ++i) {
           z = z + ZDD::singleton(mgr, i);
       }

       // バイナリ形式で保存
       ExportOptions opts;
       opts.format = DDFileFormat::BINARY;
       export_zdd(z, "complex_zdd.sbdd", opts);

       std::cout << "保存: " << z.card() << " 集合" << std::endl;
   }

   void load_zdd() {
       DDManager mgr;

       // バイナリ形式で読み込み
       ZDD z = import_zdd(mgr, "complex_zdd.sbdd");

       std::cout << "読み込み: " << z.card() << " 集合" << std::endl;
       std::cout << "ノード数: " << z.size() << std::endl;

       // 検証
       if (validate_zdd(z)) {
           std::cout << "ZDDは有効です" << std::endl;
       }
   }

   int main() {
       save_zdd();
       load_zdd();
       return 0;
   }

.. seealso::

   - :doc:`bdd_guide` - BDDの基本操作
   - :doc:`zdd_guide` - ZDDの基本操作
