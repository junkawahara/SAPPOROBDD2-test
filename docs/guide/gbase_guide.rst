.. _gbase_guide:

GBaseガイド
===========

この章では、SAPPOROBDD 2.0のGBase（グラフベース）クラスとBDDCT（コストテーブル）クラスの使い方を解説します。
GBaseはグラフ上のパスやサイクルをZDDで列挙するためのクラスであり、
フロンティア法に基づく効率的なアルゴリズムを実装しています。

.. contents:: 目次
   :local:
   :depth: 2

GBaseクラスの概要
-----------------

GBaseは、グラフ上のパス（s-tパス）やサイクル（ハミルトン閉路を含む）を
ZDD（集合族）として列挙するクラスです。
辺の集合を要素とする集合族をZDDで表現し、
グラフの構造を利用した効率的な列挙を行います。

主な特徴:

- **フロンティア法**: Simpath アルゴリズムによるトップダウン列挙
- **辺制約**: 特定の辺を必ず使う（FIX_1）、使わない（FIX_0）制約を指定可能
- **ハミルトンモード**: 全頂点を訪問するパス・サイクルの列挙
- **コスト付きグラフ**: BDDCTクラスと組み合わせたコスト制約付き列挙

.. code-block:: cpp

   #include <sbdd2/sbdd2.hpp>
   using namespace sbdd2;

グラフの構築
------------

グラフの初期化
~~~~~~~~~~~~~~

GBaseでグラフを構築するには、まず頂点数と辺数を指定して初期化します。

.. code-block:: cpp

   DDManager mgr;
   GBase gb(mgr);

   // 頂点4つ、辺5つのグラフを初期化
   gb.init(4, 5);

   // 辺を設定（辺番号, 頂点1, 頂点2, コスト）
   gb.set_edge(1, 1, 2);        // 辺1: 頂点1-2（コスト1）
   gb.set_edge(2, 1, 3);        // 辺2: 頂点1-3
   gb.set_edge(3, 2, 3, 2);     // 辺3: 頂点2-3（コスト2）
   gb.set_edge(4, 2, 4);        // 辺4: 頂点2-4
   gb.set_edge(5, 3, 4);        // 辺5: 頂点3-4

.. note::

   頂点番号と辺番号はともに1から始まります。

グラフの読み込み
~~~~~~~~~~~~~~~~

ファイルからグラフを読み込むこともできます。

.. code-block:: cpp

   GBase gb(mgr);
   gb.import("graph.txt");

グリッドグラフ
~~~~~~~~~~~~~~

格子グラフは ``set_grid()`` で簡単に作成できます。

.. code-block:: cpp

   GBase gb(mgr);
   gb.set_grid(3, 3);  // 3x3の格子グラフ

   std::cout << "頂点数: " << gb.vertex_count() << std::endl;  // 9
   std::cout << "辺数: " << gb.edge_count() << std::endl;       // 12

パス列挙
--------

``sim_paths(start, end)`` は、始点 ``start`` から終点 ``end`` への
全ての単純パスをZDDとして列挙します。

.. code-block:: cpp

   DDManager mgr;
   GBase gb(mgr);

   // 4頂点のグラフ
   gb.init(4, 5);
   gb.set_edge(1, 1, 2);
   gb.set_edge(2, 1, 3);
   gb.set_edge(3, 2, 3);
   gb.set_edge(4, 2, 4);
   gb.set_edge(5, 3, 4);
   gb.pack();  // 不要な頂点を除去

   // 頂点1から頂点4への全単純パスを列挙
   ZDD paths = gb.sim_paths(1, 4);

   double count = paths.card();
   std::cout << "パスの数: " << count << std::endl;

   // 各パスを表示
   auto all_paths = paths.enumerate();
   for (const auto& path : all_paths) {
       std::cout << "パス: ";
       for (bddvar e : path) {
           std::cout << "辺" << e << " ";
       }
       std::cout << std::endl;
   }

``pack()`` を呼び出すことで、使用していない頂点が除去され、
効率的な列挙が可能になります。

サイクル列挙
------------

``sim_cycles()`` は、グラフ上の全ての単純サイクルをZDDとして列挙します。

.. code-block:: cpp

   DDManager mgr;
   GBase gb(mgr);

   gb.init(4, 6);
   gb.set_edge(1, 1, 2);
   gb.set_edge(2, 1, 3);
   gb.set_edge(3, 1, 4);
   gb.set_edge(4, 2, 3);
   gb.set_edge(5, 2, 4);
   gb.set_edge(6, 3, 4);
   gb.pack();

   // 全ての単純サイクルを列挙
   ZDD cycles = gb.sim_cycles();
   std::cout << "サイクルの数: " << cycles.card() << std::endl;

ハミルトンモード
----------------

``set_hamilton(true)`` を設定すると、列挙されるパスやサイクルが
全ての頂点を訪問するもの（ハミルトンパス・ハミルトンサイクル）に限定されます。

.. code-block:: cpp

   DDManager mgr;
   GBase gb(mgr);
   gb.set_grid(3, 3);
   gb.pack();

   // ハミルトンモードを有効にする
   gb.set_hamilton(true);

   // ハミルトンサイクルを列挙
   ZDD hamilton_cycles = gb.sim_cycles();
   std::cout << "ハミルトンサイクル数: " << hamilton_cycles.card() << std::endl;

   // ハミルトンパスを列挙（左上から右下）
   ZDD hamilton_paths = gb.sim_paths(1, gb.vertex_count());
   std::cout << "ハミルトンパス数: " << hamilton_paths.card() << std::endl;

辺制約
------

``fix_edge()`` を使って、特定の辺に対する制約を設定できます。

.. code-block:: cpp

   DDManager mgr;
   GBase gb(mgr);
   gb.set_grid(3, 3);

   // 辺1を必ず使用する
   gb.fix_edge(1, GBase::FIX_1);

   // 辺2を使用しない
   gb.fix_edge(2, GBase::FIX_0);

   // 制約付きでパスを列挙
   gb.pack();
   ZDD paths = gb.sim_paths(1, gb.vertex_count());

.. list-table:: 辺制約の種類
   :header-rows: 1
   :widths: 20 30 50

   * - 制約
     - 定数
     - 説明
   * - 制約なし
     - ``GBase::FIX_NONE``
     - 辺を使用してもしなくてもよい（デフォルト）
   * - 必ず使用
     - ``GBase::FIX_1``
     - この辺を必ずパス・サイクルに含める
   * - 使用しない
     - ``GBase::FIX_0``
     - この辺をパス・サイクルから除外する

コスト付きグラフとBDDCT
-----------------------

BDDCTクラスの概要
~~~~~~~~~~~~~~~~~

``BDDCT`` クラスは、ZDDで表現された集合族に対して
コスト（重み）制約付きの操作を提供します。
各変数（辺）にコストを割り当て、コストの上限以下の集合だけを抽出できます。

.. code-block:: cpp

   DDManager mgr;
   GBase gb(mgr);
   gb.set_grid(3, 3);
   gb.pack();

   // 全パスを列挙
   ZDD paths = gb.sim_paths(1, gb.vertex_count());

コストテーブルの作成
~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   // コストテーブルを作成
   BDDCT ct(mgr);
   ct.alloc(mgr.var_count(), 1);  // 全変数、デフォルトコスト1

   // 特定の辺にコストを設定
   int edge1_var = gb.bdd_var_of_edge(1);
   ct.set_cost(edge1_var, 3);   // 辺1のコスト: 3

   int edge3_var = gb.bdd_var_of_edge(3);
   ct.set_cost(edge3_var, 5);   // 辺3のコスト: 5

コスト制約付きフィルタリング
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   // コスト10以下のパスだけを抽出
   ZDD filtered = ct.zdd_cost_le(paths, 10);
   std::cout << "コスト10以下のパス数: " << filtered.card() << std::endl;

   // 最小コスト・最大コストの計算
   bddcost min = ct.min_cost(paths);
   bddcost max = ct.max_cost(paths);
   std::cout << "最小コスト: " << min << std::endl;
   std::cout << "最大コスト: " << max << std::endl;

変数マッピング
~~~~~~~~~~~~~~

GBaseでは辺番号とZDDの変数番号が異なる場合があります。
``bdd_var_of_edge()`` と ``edge_of_bdd_var()`` で相互変換できます。

.. code-block:: cpp

   // 辺番号 → ZDD変数番号
   int var = gb.bdd_var_of_edge(3);
   std::cout << "辺3のZDD変数: " << var << std::endl;

   // ZDD変数番号 → 辺番号
   GBEdge edge = gb.edge_of_bdd_var(var);
   std::cout << "変数" << var << "の辺: " << edge << std::endl;

グリッドグラフの実践例
----------------------

3x3格子グラフのs-tパスとハミルトンパスを列挙する実践的な例です。

.. code-block:: cpp

   #include <sbdd2/sbdd2.hpp>
   using namespace sbdd2;

   int main() {
       DDManager mgr;
       GBase gb(mgr);

       // 3x3格子グラフを作成
       gb.set_grid(3, 3);
       gb.pack();

       std::cout << "=== 3x3格子グラフ ===" << std::endl;
       std::cout << "頂点数: " << gb.vertex_count() << std::endl;
       std::cout << "辺数: " << gb.edge_count() << std::endl;

       // 左上(1)から右下(9)への全パス
       ZDD paths = gb.sim_paths(1, gb.vertex_count());
       std::cout << "全パス数: " << paths.card() << std::endl;
       std::cout << "ZDDノード数: " << paths.size() << std::endl;

       // 全サイクル
       ZDD cycles = gb.sim_cycles();
       std::cout << "全サイクル数: " << cycles.card() << std::endl;

       // ハミルトンパス
       GBase gb_ham(mgr);
       gb_ham.set_grid(3, 3);
       gb_ham.set_hamilton(true);
       gb_ham.pack();

       ZDD ham_paths = gb_ham.sim_paths(1, gb_ham.vertex_count());
       std::cout << "ハミルトンパス数: " << ham_paths.card() << std::endl;

       // コスト付き列挙
       BDDCT ct(mgr);
       ct.alloc(mgr.var_count(), 1);

       bddcost min_c = ct.min_cost(paths);
       bddcost max_c = ct.max_cost(paths);
       std::cout << "最短パス長: " << min_c << std::endl;
       std::cout << "最長パス長: " << max_c << std::endl;

       // 最短パスのみ抽出
       ZDD shortest = ct.zdd_cost_le(paths, min_c);
       std::cout << "最短パス数: " << shortest.card() << std::endl;

       return 0;
   }

フロンティア法の概要
--------------------

GBaseが内部で使用しているフロンティア法（Simpath アルゴリズム）の概要を説明します。

アルゴリズムの基本
~~~~~~~~~~~~~~~~~~

フロンティア法は、グラフの辺を1本ずつ処理しながら、
各辺を「使う（1）」か「使わない（0）」かの選択を行い、
制約を満たすかどうかをチェックします。

**フロンティア** とは、現在処理中の辺に隣接する頂点の集合です。
各頂点の「接続状態（mate）」を管理することで、
途中経過が最終的に有効なパスやサイクルになるかどうかを判定できます。

.. code-block:: text

   辺の処理順序:
   Level N:   辺N (使う/使わない)
   Level N-1: 辺N-1 (使う/使わない)
   ...
   Level 1:   辺1 (使う/使わない)

   各レベルで:
   1. フロンティア上の頂点の接続状態を更新
   2. 制約違反がないかチェック
   3. フロンティアから離脱する頂点の最終チェック

状態の管理
~~~~~~~~~~

各フロンティア頂点には **mate 配列** が関連付けられます。
mate[v] は頂点 v が現在接続されている相手の頂点を示します。

- **mate[v] = v**: 頂点 v はまだどの辺にも接続されていない
- **mate[v] = w**: 頂点 v は頂点 w とパスで接続されている
- **mate[v] = 0**: 頂点 v は既にフロンティアから離脱した（度数制約を満たした）

この状態管理により、等価な部分問題を効率的に共有でき、
ZDDのノード数を大幅に削減できます。

TdZddとの関係
~~~~~~~~~~~~~~

GBaseのフロンティア法は、内部的にはTdZdd互換のSpecインターフェースとしても
実装可能です。より柔軟なカスタマイズが必要な場合は、
:doc:`tdzdd_guide` で解説するSpecインターフェースを使って
独自のフロンティア法Specを実装することもできます。

.. seealso::

   - :doc:`zdd_guide` - ZDDの基本操作
   - :doc:`tdzdd_guide` - TdZdd互換Specインターフェース
