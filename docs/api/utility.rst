ユーティリティクラス
====================

.. seealso::

   GBaseの詳細ガイドは :doc:`../guide/gbase_guide` を参照してください。
   グラフ問題への応用は :doc:`../application/graph_problems` を参照してください。
   I/Oの詳細は :doc:`../guide/io_guide` を参照してください。

GBase
-----

グラフベースクラス。グラフ上のパスやサイクルを列挙します。

.. doxygenclass:: sbdd2::GBase
   :members:
   :undoc-members:

GBase型定義
~~~~~~~~~~~

.. cpp:type:: sbdd2::GBVertex

   頂点番号型（uint16_t、最大65534頂点）

.. cpp:type:: sbdd2::GBEdge

   辺番号型（uint16_t、最大65534辺）

GBase::Vertex構造体
~~~~~~~~~~~~~~~~~~~

グラフの頂点情報を保持します。

**メンバ変数**:

* ``tmp`` - 一時変数（内部アルゴリズム用）
* ``degree`` - 頂点の次数

GBase::Edge構造体
~~~~~~~~~~~~~~~~~

グラフの辺情報を保持します。

**メンバ変数**:

* ``endpoints[2]`` - 辺の両端の頂点
* ``cost`` - 辺のコスト（重み）
* ``preset`` - 辺の制約（FIX_NONE, FIX_0, FIX_1）
* ``mate_width`` - mate配列の幅（フロンティア法用）
* ``tmp`` - 一時変数（内部アルゴリズム用）
* ``io[2]`` - 入出力フラグ

辺の制約定数
~~~~~~~~~~~~

.. cpp:var:: constexpr char GBase::FIX_NONE = 0

   制約なし（辺を使っても使わなくてもよい）

.. cpp:var:: constexpr char GBase::FIX_0 = 1

   辺を使わない（パス/サイクルに含めない）

.. cpp:var:: constexpr char GBase::FIX_1 = 2

   辺を必ず使う（パス/サイクルに含める）

BDDCT
-----

コストテーブルクラス。コスト制約付きの列挙を効率的に行います。

.. doxygenclass:: sbdd2::BDDCT
   :members:
   :undoc-members:

BDDCT型定義
~~~~~~~~~~~

**bddcost型**

コスト型（int）

**BDDCOST_NULL定数**

無効なコスト値を表す定数（``0x7FFFFFFF``）

**CostMap型**

コストからZDDへのマッピング型（``std::map<bddcost, ZDD>``）

I/O関数
-------

ファイル形式
~~~~~~~~~~~~

.. doxygenenum:: sbdd2::DDFileFormat

DDFileFormat列挙型はファイル形式を指定します:

* ``AUTO`` - ファイル拡張子から自動検出
* ``BINARY`` - バイナリ形式（コンパクト）
* ``TEXT`` - テキスト形式（人間可読）
* ``DOT`` - GraphViz DOT形式
* ``KNUTH`` - Knuth形式

エクスポートオプション
~~~~~~~~~~~~~~~~~~~~~~

.. doxygenstruct:: sbdd2::ExportOptions
   :members:

**メンバ変数**:

* ``format`` - ファイル形式（デフォルト: ``DDFileFormat::BINARY``）
* ``include_header`` - ヘッダを含めるか（デフォルト: ``true``）
* ``compress`` - 圧縮するか（デフォルト: ``false``）
* ``precision`` - 浮動小数点精度（デフォルト: 6）

インポートオプション
~~~~~~~~~~~~~~~~~~~~

.. doxygenstruct:: sbdd2::ImportOptions
   :members:

**メンバ変数**:

* ``format`` - ファイル形式（デフォルト: ``DDFileFormat::AUTO``）

Export/Import
~~~~~~~~~~~~~

.. doxygenfunction:: sbdd2::export_bdd(const BDD&, const std::string&, const ExportOptions&)
.. doxygenfunction:: sbdd2::export_bdd(const BDD&, std::ostream&, const ExportOptions&)
.. doxygenfunction:: sbdd2::import_bdd(DDManager&, const std::string&, const ImportOptions&)
.. doxygenfunction:: sbdd2::import_bdd(DDManager&, std::istream&, const ImportOptions&)

.. doxygenfunction:: sbdd2::export_zdd(const ZDD&, const std::string&, const ExportOptions&)
.. doxygenfunction:: sbdd2::export_zdd(const ZDD&, std::ostream&, const ExportOptions&)
.. doxygenfunction:: sbdd2::import_zdd(DDManager&, const std::string&, const ImportOptions&)
.. doxygenfunction:: sbdd2::import_zdd(DDManager&, std::istream&, const ImportOptions&)

DOT形式
~~~~~~~

.. doxygenfunction:: sbdd2::to_dot(const BDD&, const std::string&)
.. doxygenfunction:: sbdd2::to_dot(const ZDD&, const std::string&)

Graphillion形式
~~~~~~~~~~~~~~~

Graphillionライブラリと互換性のある形式でZDDを入出力します。

.. doxygenfunction:: sbdd2::import_zdd_as_graphillion(DDManager&, std::istream&, int)
.. doxygenfunction:: sbdd2::import_zdd_as_graphillion(DDManager&, const std::string&, int)
.. doxygenfunction:: sbdd2::export_zdd_as_graphillion(const ZDD&, std::ostream&, int)
.. doxygenfunction:: sbdd2::export_zdd_as_graphillion(const ZDD&, const std::string&, int)

Knuth形式
~~~~~~~~~

Knuthの形式でZDDを入出力します。

.. doxygenfunction:: sbdd2::import_zdd_as_knuth(DDManager&, std::istream&, bool, int)
.. doxygenfunction:: sbdd2::import_zdd_as_knuth(DDManager&, const std::string&, bool, int)
.. doxygenfunction:: sbdd2::export_zdd_as_knuth(const ZDD&, std::ostream&, bool)
.. doxygenfunction:: sbdd2::export_zdd_as_knuth(const ZDD&, const std::string&, bool)

lib_bdd形式
~~~~~~~~~~~

Rustのlib-bddライブラリと互換性のあるバイナリ形式でBDD/ZDDを入出力します。

**形式仕様**:

* ノードあたり10バイト（リトルエンディアン）
* var_type (uint16_t): 変数レベル（0xFFFF = 終端）
* ptr_type (uint32_t): 0枝の子ポインタ
* ptr_type (uint32_t): 1枝の子ポインタ
* インデックス0 = false終端、インデックス1 = true終端

.. doxygenfunction:: sbdd2::import_bdd_as_libbdd(DDManager&, std::istream&)
.. doxygenfunction:: sbdd2::import_bdd_as_libbdd(DDManager&, const std::string&)
.. doxygenfunction:: sbdd2::import_zdd_as_libbdd(DDManager&, std::istream&)
.. doxygenfunction:: sbdd2::import_zdd_as_libbdd(DDManager&, const std::string&)
.. doxygenfunction:: sbdd2::export_bdd_as_libbdd(const BDD&, std::ostream&)
.. doxygenfunction:: sbdd2::export_bdd_as_libbdd(const BDD&, const std::string&)
.. doxygenfunction:: sbdd2::export_zdd_as_libbdd(const ZDD&, std::ostream&)
.. doxygenfunction:: sbdd2::export_zdd_as_libbdd(const ZDD&, const std::string&)

SVG形式
~~~~~~~

ZDDをSVG画像として出力します。

.. doxygenstruct:: sbdd2::SvgExportOptions
   :members:

**SvgExportOptionsメンバ変数**:

* ``width`` / ``height`` - SVG画像サイズ（デフォルト: 800x600）
* ``node_fill_color`` - ノード塗りつぶし色（デフォルト: "#ffffff"）
* ``node_stroke_color`` - ノード輪郭色（デフォルト: "#000000"）
* ``edge_0_color`` - 0枝の色（点線、デフォルト: "#0000ff"）
* ``edge_1_color`` - 1枝の色（実線、デフォルト: "#000000"）
* ``terminal_0_color`` - 0終端の色（デフォルト: "#ff6666"）
* ``terminal_1_color`` - 1終端の色（デフォルト: "#66ff66"）
* ``font_family`` - フォントファミリー（デフォルト: "sans-serif"）
* ``font_size`` - フォントサイズ（デフォルト: 12）
* ``show_terminal_labels`` - 終端ラベル表示（デフォルト: true）
* ``show_variable_labels`` - 変数ラベル表示（デフォルト: true）
* ``node_radius`` - ノード半径（デフォルト: 20）
* ``level_gap`` - レベル間隔（デフォルト: 60）
* ``horizontal_gap`` - 水平間隔（デフォルト: 40）

.. doxygenfunction:: sbdd2::export_zdd_as_svg(const ZDD&, std::ostream&, const SvgExportOptions&)
.. doxygenfunction:: sbdd2::export_zdd_as_svg(const ZDD&, const std::string&, const SvgExportOptions&)

検証
~~~~

.. doxygenfunction:: sbdd2::validate_bdd
.. doxygenfunction:: sbdd2::validate_zdd

フォーマット検出
~~~~~~~~~~~~~~~~

.. doxygenfunction:: sbdd2::detect_format

バイナリ形式定数
~~~~~~~~~~~~~~~~

.. cpp:var:: constexpr char sbdd2::DD_MAGIC[4] = {'S', 'B', 'D', 'D'}

   バイナリ形式のマジックナンバー

.. cpp:var:: constexpr std::uint16_t sbdd2::DD_VERSION = 1

   バイナリ形式のバージョン

.. cpp:var:: constexpr std::uint8_t sbdd2::DD_TYPE_BDD = 0

   BDD型コード

.. cpp:var:: constexpr std::uint8_t sbdd2::DD_TYPE_ZDD = 1

   ZDD型コード

使用例
------

GBaseによるパス列挙
~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   DDManager mgr;
   GBase graph(mgr);

   // 3x3グリッドグラフを作成
   graph.set_grid(3, 3);

   // 頂点1から頂点9へのパスを列挙
   ZDD paths = graph.sim_paths(1, 9);

   std::cout << "パス数: " << paths.card() << std::endl;

   // ハミルトンパス（全頂点を通るパス）
   graph.set_hamilton(true);
   ZDD hamilton_paths = graph.sim_paths(1, 9);

GBaseによるサイクル列挙
~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   DDManager mgr;
   GBase graph(mgr);

   // カスタムグラフを作成（5頂点、7辺）
   graph.init(5, 7);
   graph.set_edge(0, 0, 1);  // 辺0: 頂点0-1
   graph.set_edge(1, 1, 2);  // 辺1: 頂点1-2
   graph.set_edge(2, 2, 3);  // 辺2: 頂点2-3
   graph.set_edge(3, 3, 4);  // 辺3: 頂点3-4
   graph.set_edge(4, 4, 0);  // 辺4: 頂点4-0
   graph.set_edge(5, 0, 2);  // 辺5: 頂点0-2
   graph.set_edge(6, 1, 3);  // 辺6: 頂点1-3

   // 全サイクルを列挙
   ZDD cycles = graph.sim_cycles();
   std::cout << "サイクル数: " << cycles.card() << std::endl;

辺の制約を使ったパス列挙
~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   DDManager mgr;
   GBase graph(mgr);
   graph.set_grid(3, 3);

   // 特定の辺を必ず使う
   graph.fix_edge(0, GBase::FIX_1);

   // 特定の辺を使わない
   graph.fix_edge(5, GBase::FIX_0);

   ZDD constrained_paths = graph.sim_paths(1, 9);

BDDCTによるコスト制約付き列挙
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   DDManager mgr;
   for (int i = 1; i <= 5; ++i) mgr.new_var();

   BDDCT ct(mgr);
   ct.alloc(5, 0);

   // コストを設定
   ct.set_cost(1, 10);
   ct.set_cost(2, 20);
   ct.set_cost(3, 15);
   ct.set_cost(4, 25);
   ct.set_cost(5, 30);

   // ラベルを設定（オプション）
   ct.set_label(1, "item_A");
   ct.set_label(2, "item_B");
   ct.set_label(3, "item_C");
   ct.set_label(4, "item_D");
   ct.set_label(5, "item_E");

   // 全部分集合を作成
   ZDD all = ZDD::single(mgr);
   for (int i = 1; i <= 5; ++i) {
       all = all + all * ZDD::singleton(mgr, i);
   }

   // コスト50以下の集合を抽出
   ZDD affordable = ct.zdd_cost_le(all, 50);

   // 最小/最大コストを計算
   bddcost min_c = ct.min_cost(all);
   bddcost max_c = ct.max_cost(all);

   // コスト表を出力
   ct.export_table();

BDD/ZDDのエクスポート
~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   DDManager mgr;
   mgr.new_var();
   mgr.new_var();

   BDD f = mgr.var_bdd(1) & mgr.var_bdd(2);

   // バイナリ形式でエクスポート
   ExportOptions opts;
   opts.format = DDFileFormat::BINARY;

   std::ofstream ofs("bdd.bin", std::ios::binary);
   export_bdd(f, ofs, opts);

   // テキスト形式でエクスポート
   opts.format = DDFileFormat::TEXT;
   export_bdd(f, "bdd.txt", opts);

   // DOT形式で出力
   std::string dot = to_dot(f, "my_bdd");
   std::ofstream dot_file("bdd.dot");
   dot_file << dot;

BDD/ZDDのインポート
~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   DDManager mgr;

   // バイナリ形式からインポート
   BDD f1 = import_bdd(mgr, "bdd.bin");

   // テキスト形式からインポート
   ImportOptions opts;
   opts.format = DDFileFormat::TEXT;
   BDD f2 = import_bdd(mgr, "bdd.txt", opts);

   // 自動検出（拡張子から判断）
   BDD f3 = import_bdd(mgr, "bdd.bin");  // .bin -> BINARY

lib_bdd形式
~~~~~~~~~~~

.. code-block:: cpp

   DDManager mgr;
   mgr.new_var();
   mgr.new_var();

   BDD f = mgr.var_bdd(1) | mgr.var_bdd(2);

   // lib_bdd形式でエクスポート（Rustライブラリ互換）
   export_bdd_as_libbdd(f, "bdd.libbdd");

   // lib_bdd形式からインポート
   BDD imported = import_bdd_as_libbdd(mgr, "bdd.libbdd");

   // ZDDでも同様
   ZDD z = ZDD::singleton(mgr, 1) + ZDD::singleton(mgr, 2);
   export_zdd_as_libbdd(z, "zdd.libbdd");
   ZDD z_imported = import_zdd_as_libbdd(mgr, "zdd.libbdd");

Graphillion形式
~~~~~~~~~~~~~~~

.. code-block:: cpp

   DDManager mgr;
   for (int i = 1; i <= 5; ++i) mgr.new_var();

   ZDD family = ZDD::singleton(mgr, 1) + ZDD::singleton(mgr, 2);

   // Graphillion形式でエクスポート
   std::ofstream ofs("family.graphillion");
   export_zdd_as_graphillion(family, ofs);

   // Graphillion形式からインポート
   std::ifstream ifs("family.graphillion");
   ZDD imported = import_zdd_as_graphillion(mgr, ifs);

Knuth形式
~~~~~~~~~

.. code-block:: cpp

   DDManager mgr;
   for (int i = 1; i <= 5; ++i) mgr.new_var();

   ZDD family = ZDD::singleton(mgr, 1) + ZDD::singleton(mgr, 2);

   // Knuth形式でエクスポート
   std::ofstream ofs("family.knuth");
   export_zdd_as_knuth(family, ofs);

   // 16進数形式
   export_zdd_as_knuth(family, "family_hex.knuth", true);

   // Knuth形式からインポート
   ZDD imported = import_zdd_as_knuth(mgr, "family.knuth");

SVG出力
~~~~~~~

.. code-block:: cpp

   DDManager mgr;
   for (int i = 1; i <= 3; ++i) mgr.new_var();

   ZDD family = ZDD::singleton(mgr, 1) + ZDD::singleton(mgr, 2) +
                ZDD::singleton(mgr, 1) * ZDD::singleton(mgr, 2);

   // デフォルトオプションでSVG出力
   export_zdd_as_svg(family, "family.svg");

   // カスタムオプション
   SvgExportOptions opts;
   opts.width = 1024;
   opts.height = 768;
   opts.node_fill_color = "#f0f0f0";
   opts.edge_0_color = "#ff0000";  // 0枝を赤に
   opts.edge_1_color = "#0000ff";  // 1枝を青に
   opts.font_size = 14;

   export_zdd_as_svg(family, "family_custom.svg", opts);
