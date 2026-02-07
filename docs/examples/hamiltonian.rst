ハミルトン閉路問題
==================

このサンプルでは、BDDを用いてグリッドグラフ上のハミルトン閉路の数を求めます。
ソースコード: ``examples/hamiltonian.cpp``

問題の定義
----------

ハミルトン閉路とは、グラフの全ての頂点をちょうど1回ずつ通り、
出発点に戻る閉路のことです。

この例では、N x M のグリッドグラフにおけるハミルトン閉路の数を数えます。
グリッドグラフでは、隣接する上下左右のセルが辺で接続されています。

.. code-block:: text

   4x4 グリッドグラフの例:

   (0,0)--(0,1)--(0,2)--(0,3)
     |      |      |      |
   (1,0)--(1,1)--(1,2)--(1,3)
     |      |      |      |
   (2,0)--(2,1)--(2,2)--(2,3)
     |      |      |      |
   (3,0)--(3,1)--(3,2)--(3,3)

ハミルトン閉路が存在するための必要条件：

* :math:`N \times M` が偶数であること（チェス盤着色による議論）
* N = 1 または M = 1 の場合は閉路が存在しない（2x2 の場合を除く）

BDD/ZDDによるモデリング
-----------------------

辺ベースの符号化
~~~~~~~~~~~~~~~~

各辺にブール変数を割り当てます。変数が真ならその辺がハミルトン閉路に含まれ、
偽なら含まれません。

N x M グリッドグラフの辺数は以下の通りです：

* 水平辺: :math:`N \times (M - 1)` 本
* 垂直辺: :math:`(N - 1) \times M` 本
* 合計: :math:`N \times (M - 1) + (N - 1) \times M` 本

辺の番号付け
~~~~~~~~~~~~

辺は以下の順序で番号を割り当てます：

1. **水平辺**: 行 r の列 c と列 c+1 を結ぶ辺

   .. code-block:: text

      index = r * (M - 1) + c

2. **垂直辺**: 列 c の行 r と行 r+1 を結ぶ辺

   .. code-block:: text

      index = N * (M - 1) + r * M + c

次数制約
~~~~~~~~

ハミルトン閉路では、全ての頂点の次数がちょうど 2 でなければなりません。
すなわち、各頂点に接続する辺のうち、ちょうど 2 本が選択されます。

この制約を各頂点について BDD で表現し、全頂点の制約の AND を取ります。

.. note::

   次数制約だけではハミルトン閉路を完全に特徴づけることはできません。
   複数の短い閉路の組合せも次数制約を満たしますが、ハミルトン閉路ではありません。
   ただし、小さなグリッドグラフでは結果が一致することが多く、
   この例はBDDの使い方の説明を主目的としています。

対称性の除去
~~~~~~~~~~~~

セル (0,0) に隣接する辺（(0,0)-(1,0) と (0,0)-(0,1)）を固定することで、
対称な解の重複を排除します。

コードのウォークスルー
----------------------

データ構造
~~~~~~~~~~

グリッドグラフを表現するために、``Cell`` 構造体と ``Edge`` 構造体を定義しています。

.. code-block:: cpp

   struct Cell {
       int row, col;

       int index() const {
           return row * cols() + col;
       }

       std::vector<Cell> neighbors() const {
           // 上下左右の隣接セルを返す
           std::vector<Cell> result;
           int moves[4][2] = {{-1, 0}, {0, -1}, {0, 1}, {1, 0}};
           for (int i = 0; i < 4; i++) {
               Cell next(row + moves[i][0], col + moves[i][1]);
               if (!next.out_of_range()) {
                   result.push_back(next);
               }
           }
           return result;
       }
   };

``Edge`` 構造体は辺の2端点を保持し、正規化（小さいインデックスの頂点を先に）と
辺番号の計算を行います。

.. code-block:: cpp

   struct Edge {
       Cell u, v;

       int index() const {
           if (u.row == v.row) {
               // 水平辺
               int minCol = std::min(u.col, v.col);
               return u.row * (cols() - 1) + minCol;
           } else {
               // 垂直辺
               int minRow = std::min(u.row, v.row);
               return rows() * (cols() - 1) + minRow * cols() + u.col;
           }
       }
   };

degree_constraint: 次数制約の構築
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

``degree_constraint(mgr, c)`` は、セル c に接続する辺のうち
ちょうど 2 本が選択される制約BDDを構築します。

.. code-block:: cpp

   BDD degree_constraint(DDManager& mgr, const Cell& c) {
       std::vector<int> edges = edges_of_cell(c);
       int n = edges.size();

       if (n < 2) {
           return mgr.bdd_zero();  // 次数2は不可能
       }

       // 辺変数を取得
       std::vector<BDD> vars;
       for (int e : edges) {
           vars.push_back(mgr.var_bdd(e + 1));  // 1-indexed
       }

       // ちょうど2本を選ぶ全ての組合せのOR
       BDD result = mgr.bdd_zero();
       for (int i = 0; i < n; i++) {
           for (int j = i + 1; j < n; j++) {
               BDD term = vars[i] & vars[j];
               for (int k = 0; k < n; k++) {
                   if (k != i && k != j) {
                       term = term & (~vars[k]);
                   }
               }
               result = result | term;
           }
       }
       return result;
   }

この関数は組合せ的な構築を行います：

1. セルに接続する辺を列挙
2. そのうち2本を選ぶ全ての :math:`\binom{n}{2}` 通りについて
3. 選んだ2本は真、残りは偽とするBDDを構築
4. 全ての選び方のORを取る

corner_constraint: コーナー制約
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

``corner_constraint(mgr)`` は、セル (0,0) に接続する2辺を固定する制約です。

.. code-block:: cpp

   BDD corner_constraint(DDManager& mgr) {
       Cell c00(0, 0), c10(1, 0), c01(0, 1);

       Edge e1(c00, c10);
       Edge e2(c00, c01);
       e1.canonicalize();
       e2.canonicalize();

       BDD v1 = mgr.var_bdd(e1.index() + 1);
       BDD v2 = mgr.var_bdd(e2.index() + 1);

       return v1 & v2;
   }

セル (0,0) は隅にあるため隣接辺はちょうど 2 本です。
ハミルトン閉路では次数 2 が必要なので、両方の辺を選択する必要があります。
この制約を明示的に追加することで、探索空間を削減します。

メイン処理
~~~~~~~~~~

メイン関数では、以下の順序で処理を行います。

.. code-block:: cpp

   // DDManager を作成し、辺変数を登録
   DDManager mgr;
   for (int i = 0; i < num_edges; i++) {
       mgr.new_var();
   }

   // 制約を構築
   BDD result = mgr.bdd_one();

   // コーナー制約
   result = result & corner_constraint(mgr);

   // 全セルの次数制約
   for (int r = 0; r < rows(); r++) {
       for (int c = 0; c < cols(); c++) {
           Cell cell(r, c);
           result = result & degree_constraint(mgr, cell);
       }
   }

   // 解の数え上げ
   double solutions_d = result.count(num_edges);

実行例と結果
------------

.. code-block:: bash

   # 4x4 グリッド（デフォルト）
   ./hamiltonian

   # 任意のサイズを指定（正方形）
   ./hamiltonian -n 6

   # 長方形グリッド
   ./hamiltonian -n 4 -n 6

代表的な結果を以下に示します（正方形グリッド、OEIS A003763）：

.. list-table:: グリッドグラフのハミルトン閉路数
   :header-rows: 1
   :widths: 15 25

   * - N x N
     - ハミルトン閉路数
   * - 2 x 2
     - 1
   * - 3 x 3
     - 0（奇数頂点）
   * - 4 x 4
     - 6
   * - 5 x 5
     - 0（奇数頂点）
   * - 6 x 6
     - 1,072
   * - 7 x 7
     - 0（奇数頂点）
   * - 8 x 8
     - 4,638,576

.. note::

   N x N グリッドで N が奇数の場合、頂点数 :math:`N^2` が奇数になるため
   ハミルトン閉路は存在しません（チェス盤着色の議論による）。

改善の可能性
------------

GBaseの活用
~~~~~~~~~~~

SAPPOROBDD 2.0の ``GBase`` クラスを使用すると、グラフ上のパスや閉路の列挙を
より効率的に行えます。GBaseはフロンティア法を内部的に使用しており、
連結性の制約も正しく扱います。

.. code-block:: cpp

   DDManager mgr;
   GBase graph(mgr);
   graph.set_grid(N, M);

   // ハミルトン閉路の列挙
   ZDD cycles = graph.ham_cycles();
   std::cout << "閉路数: " << cycles.card() << std::endl;

ZDDベースのアプローチ
~~~~~~~~~~~~~~~~~~~~~

辺の集合をZDDで表現するアプローチも有効です。
ZDDは疎な集合族の表現に適しているため、
選択される辺の数が全辺数に比べて少ない場合に効率的です。

TdZdd Specによる構築
~~~~~~~~~~~~~~~~~~~~

フロンティア法をTdZdd互換のSpecとして実装し、``build_zdd()`` または
``build_zdd_dfs()`` で構築することで、連結性制約を正しく扱いつつ
効率的なZDD構築が可能です。
詳しくは :doc:`../advanced` を参照してください。

.. seealso::

   * :doc:`../tutorial` -- GBaseによるパス列挙の例
   * :doc:`../guide/index` -- GBase詳細ガイド
   * :doc:`../advanced` -- DFS構築とフロンティア法
