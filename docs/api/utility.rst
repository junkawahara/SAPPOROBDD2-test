ユーティリティクラス
====================

GBase
-----

グラフベースクラス。グラフ上のパスやサイクルを列挙します。

.. doxygenclass:: sbdd2::GBase
   :members:
   :undoc-members:

BDDCT
-----

コストテーブルクラス。コスト制約付きの列挙を効率的に行います。

.. doxygenclass:: sbdd2::BDDCT
   :members:
   :undoc-members:

I/O関数
-------

Export/Import
~~~~~~~~~~~~~

.. doxygenfunction:: sbdd2::export_bdd
.. doxygenfunction:: sbdd2::import_bdd
.. doxygenfunction:: sbdd2::export_zdd
.. doxygenfunction:: sbdd2::import_zdd

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

SVG形式
~~~~~~~

ZDDをSVG画像として出力します。

.. doxygenstruct:: sbdd2::SvgExportOptions
   :members:

.. doxygenfunction:: sbdd2::export_zdd_as_svg(const ZDD&, std::ostream&, const SvgExportOptions&)
.. doxygenfunction:: sbdd2::export_zdd_as_svg(const ZDD&, const std::string&, const SvgExportOptions&)

検証
~~~~

.. doxygenfunction:: sbdd2::validate_bdd
.. doxygenfunction:: sbdd2::validate_zdd

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

   // 全部分集合を作成
   ZDD all = ZDD::base(mgr);
   for (int i = 1; i <= 5; ++i) {
       all = all + all.product(ZDD::single(mgr, i));
   }

   // コスト50以下の集合を抽出
   ZDD affordable = ct.zdd_cost_le(all, 50);

   // 最小/最大コストを計算
   bddcost min_c = ct.min_cost(all);
   bddcost max_c = ct.max_cost(all);

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

   // DOT形式で出力
   std::string dot = to_dot(f, "my_bdd");
   std::ofstream dot_file("bdd.dot");
   dot_file << dot;

Graphillion形式
~~~~~~~~~~~~~~~

.. code-block:: cpp

   DDManager mgr;
   for (int i = 1; i <= 5; ++i) mgr.new_var();

   ZDD family = ZDD::single(mgr, 1) + ZDD::single(mgr, 2);

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

   ZDD family = ZDD::single(mgr, 1) + ZDD::single(mgr, 2);

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

   ZDD family = ZDD::single(mgr, 1) + ZDD::single(mgr, 2) +
                ZDD::single(mgr, 1).product(ZDD::single(mgr, 2));

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
