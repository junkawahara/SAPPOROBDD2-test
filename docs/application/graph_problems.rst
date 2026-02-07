グラフ問題
==========

このガイドでは、SAPPOROBDD 2.0を用いたグラフ問題の解法を
ステップバイステップで解説します。ZDDによるグラフ列挙は
BDD/ZDD研究の代表的な応用分野であり、GBaseクラスやフロンティア法を
活用することで、大規模なグラフに対しても効率的に解を列挙できます。

グラフ問題とZDDの関係
---------------------

グラフ問題におけるZDDの基本的な考え方は、**辺集合をZDDで表現する** ことです。
グラフ :math:`G = (V, E)` の各辺 :math:`e_i \in E` をZDDの変数 :math:`x_i` に
対応させると、辺の部分集合 :math:`S \subseteq E` はZDD上の1つの要素（集合）に対応します。

例えば、3辺のグラフで辺集合 :math:`\{e_1, e_3\}` は、ZDD上では
変数 :math:`x_1` と :math:`x_3` を含む集合 :math:`\{1, 3\}` として表現されます。

ZDDが辺集合の表現に特に優れている理由は以下の通りです。

* **ゼロ抑制規則**: 多くの辺が選ばれない（疎な）部分集合を効率的に圧縮する
* **集合族の共有**: 共通する部分構造を持つ多数の辺集合を、ノード共有により圧縮する
* **集合演算の効率性**: 和集合・積集合・差集合などの演算がZDD上で効率的に実行できる

この表現を用いることで、パス、サイクル、マッチング、独立集合などの
グラフ構造を列挙・数え上げ・最適化することが可能になります。

GBaseによるパス列挙（ステップバイステップ）
-------------------------------------------

GBaseクラスは、グラフ上のパスやサイクルをZDDとして列挙するための
専用クラスです。内部でフロンティア法を使用しており、
大規模なグラフに対しても効率的に動作します。

ステップ1: グラフの作成
~~~~~~~~~~~~~~~~~~~~~~~

まず、DDManagerとGBaseを作成し、グラフを定義します。

.. code-block:: cpp

   #include <sbdd2/sbdd2.hpp>
   using namespace sbdd2;

   int main() {
       DDManager mgr;
       GBase graph(mgr);

       // 3x3グリッドグラフを作成
       // 頂点番号: 1〜9（左上から右下へ）
       //
       //  1 - 2 - 3
       //  |   |   |
       //  4 - 5 - 6
       //  |   |   |
       //  7 - 8 - 9
       //
       graph.set_grid(3, 3);

ステップ2: s-tパスの列挙
~~~~~~~~~~~~~~~~~~~~~~~~~

始点sから終点tへの単純パスを全て列挙します。

.. code-block:: cpp

       // 左上(1)から右下(9)への全単純パスを列挙
       ZDD paths = graph.sim_paths(1, 9);

       // パス数を表示
       std::cout << "パス数: " << paths.card() << std::endl;

       return 0;
   }

``sim_paths(s, t)`` は、頂点sから頂点tへの全ての **単純パス**
（同じ頂点を2度通らないパス）を辺集合のZDDとして返します。

ステップ3: 結果の確認
~~~~~~~~~~~~~~~~~~~~~

列挙されたパスは辺集合のZDDとして表現されています。
個々のパスを確認するには ``enumerate()`` を使用します。

.. code-block:: cpp

   auto all_paths = paths.enumerate();
   for (const auto& path : all_paths) {
       std::cout << "パス: {";
       for (auto edge : path) {
           std::cout << edge << " ";
       }
       std::cout << "}" << std::endl;
   }

.. note::

   3x3グリッドグラフの頂点1から頂点9への単純パス数は12です。
   ``enumerate()`` は全てのパスをメモリに展開するため、
   パス数が膨大な場合はイテレータの使用を検討してください。

ハミルトンパス/サイクル
-----------------------

ハミルトンパスは全ての頂点を1度ずつ通るパス、
ハミルトンサイクルは全ての頂点を1度ずつ通って出発点に戻るサイクルです。

GBaseクラスでは、ハミルトン制約を有効にすることで列挙できます。

ハミルトンパスの列挙
~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   #include <sbdd2/sbdd2.hpp>
   using namespace sbdd2;

   int main() {
       DDManager mgr;
       GBase graph(mgr);
       graph.set_grid(3, 3);

       // ハミルトン制約を有効化
       graph.set_hamilton(true);

       // ハミルトンパスを列挙（全頂点を通るs-tパス）
       ZDD ham_paths = graph.sim_paths(1, 9);
       std::cout << "ハミルトンパス数: " << ham_paths.card() << std::endl;

ハミルトンサイクルの列挙
~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

       // ハミルトンサイクルを列挙
       ZDD ham_cycles = graph.sim_cycles();
       std::cout << "ハミルトンサイクル数: " << ham_cycles.card() << std::endl;

       return 0;
   }

.. note::

   ハミルトン制約を有効にすると、列挙されるパス/サイクルは
   グラフの全ての頂点を1度ずつ通るもののみに制限されます。
   ハミルトンパス/サイクルの存在判定はNP完全問題ですが、
   フロンティア法によるZDD構築は多くの実用的なグラフに対して
   効率的に動作します。

フロンティア法による独立集合列挙（カスタムSpec）
-------------------------------------------------

GBaseが直接サポートしていないグラフ問題については、
TdZdd互換のSpecインターフェースを使ってカスタム列挙を行えます。
ここでは、グラフの **独立集合** （互いに隣接しない頂点の集合）を
フロンティア法で列挙する方法を概念的に示します。

独立集合とは
~~~~~~~~~~~~

グラフ :math:`G = (V, E)` の独立集合とは、頂点部分集合 :math:`S \subseteq V` で、
:math:`S` 内のどの2頂点も辺で結ばれていないものです。

独立集合列挙の場合、各変数は **頂点** に対応します（パス列挙では辺に対応していました）。

HybridDdSpecによる実装概要
~~~~~~~~~~~~~~~~~~~~~~~~~~~

``HybridDdSpec`` を継承してカスタムSpecを作成します。
各頂点を順に処理し、選択（1枝）/非選択（0枝）を判定します。

.. code-block:: cpp

   #include "sbdd2/tdzdd/DdSpec.hpp"
   #include "sbdd2/tdzdd/Sbdd2Builder.hpp"

   using namespace sbdd2;
   using namespace sbdd2::tdzdd;

   class IndependentSetSpec
       : public HybridDdSpec<IndependentSetSpec, int, int, 2> {
       int n_;                              // 頂点数
       std::vector<std::vector<int>> adj_;  // 隣接リスト

   public:
       IndependentSetSpec(int n, const std::vector<std::pair<int,int>>& edges)
           : n_(n) {
           adj_.resize(n);
           for (auto& e : edges) {
               adj_[e.first].push_back(e.second);
               adj_[e.second].push_back(e.first);
           }
           setArraySize(n);  // 各頂点の選択状態を配列で管理
       }

       int getRoot(int& state, int* in_set) {
           state = 0;  // 選択された頂点数
           for (int i = 0; i < n_; ++i) in_set[i] = 0;
           return n_;  // n個の頂点から開始
       }

       int getChild(int& state, int* in_set, int level, int value) {
           int v = n_ - level;  // 現在処理中の頂点

           if (value == 1) {
               // 頂点vを選択する場合：隣接頂点チェック
               for (int u : adj_[v]) {
                   if (u < v && in_set[u]) return 0;  // 隣接頂点が選択済み
               }
               in_set[v] = 1;
               state++;
           }

           return (--level == 0) ? -1 : level;
       }
   };

   int main() {
       DDManager mgr;

       // パスグラフ 0-1-2-3 の独立集合を列挙
       std::vector<std::pair<int,int>> edges = {{0,1}, {1,2}, {2,3}};
       IndependentSetSpec spec(4, edges);

       ZDD indep_sets = build_zdd(mgr, spec);
       std::cout << "独立集合数: " << indep_sets.card() << std::endl;

       return 0;
   }

このようにHybridDdSpecを継承することで、頂点の選択状態を管理しながら
独立集合の制約を満たす集合のみを効率的に列挙できます。

.. seealso::

   フロンティア法の詳細については :doc:`../advanced` の
   「DFS Frontier-based Search によるZDD構築」を参照してください。

マッチング列挙
--------------

グラフの **マッチング** とは、辺の部分集合で、どの2辺も共通の頂点を持たないものです。
言い換えると、マッチングに含まれる辺によって各頂点の次数が1以下になる辺集合です。

マッチング列挙もフロンティア法を用いて効率的に行えます。
辺を順に処理し、各辺の選択/非選択を判定する際に、
端点の次数が1以下であることを制約として課します。

実装概要
~~~~~~~~

.. code-block:: cpp

   class MatchingSpec
       : public HybridDdSpec<MatchingSpec, int, int, 2> {
       int n_;
       std::vector<std::pair<int, int>> edges_;

   public:
       MatchingSpec(int n, const std::vector<std::pair<int, int>>& edges)
           : n_(n), edges_(edges) {
           setArraySize(n);  // 各頂点の次数を管理
       }

       int getRoot(int& state, int* degree) {
           state = 0;
           for (int i = 0; i < n_; ++i) degree[i] = 0;
           return edges_.size();
       }

       int getChild(int& state, int* degree, int level, int value) {
           int idx = edges_.size() - level;
           auto e = edges_[idx];

           if (value == 1) {
               // 辺を含める：端点の次数が既に1ならNG
               if (degree[e.first] >= 1 || degree[e.second] >= 1)
                   return 0;
               degree[e.first]++;
               degree[e.second]++;
               state++;
           }

           return (level == 1) ? -1 : level - 1;
       }
   };

使用例
~~~~~~

.. code-block:: cpp

   DDManager mgr;

   // 三角形グラフ K3
   std::vector<std::pair<int, int>> edges = {{0,1}, {1,2}, {0,2}};
   MatchingSpec spec(3, edges);

   ZDD matchings = build_zdd(mgr, spec);
   std::cout << "マッチング数: " << matchings.card() << std::endl;
   // 出力: マッチング数: 4
   // 空集合{}, {(0,1)}, {(1,2)}, {(0,2)}

マッチング列挙は、ネットワーク設計やスケジューリング問題など、
実用的な応用が多い問題です。

コスト制約付きパス問題（GBase+BDDCT）
--------------------------------------

実際のネットワーク設計問題では、パスの長さやコストに制約がある場合が多くあります。
GBaseで列挙したパスのZDDに対して、BDDCTクラスを使ってコストによるフィルタリングを
行うことで、コスト制約付きのパス問題を解くことができます。

ステップ1: グラフとパスの構築
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   #include <sbdd2/sbdd2.hpp>
   using namespace sbdd2;

   int main() {
       DDManager mgr;
       GBase graph(mgr);

       // グリッドグラフを作成
       graph.set_grid(3, 3);

       // 始点sから終点tへの全パスをZDDとして構築
       int s = 1, t = 9;
       ZDD paths = graph.sim_paths(s, t);
       std::cout << "全パス数: " << paths.card() << std::endl;

ステップ2: 辺コストの設定
~~~~~~~~~~~~~~~~~~~~~~~~~~

各辺にコスト（重み）を設定します。

.. code-block:: cpp

       // 各辺のコスト（例: 距離や所要時間）
       int num_edges = graph.num_edges();
       std::vector<int> costs(num_edges);
       // コストの例（実際の問題に応じて設定）
       for (int i = 0; i < num_edges; ++i) {
           costs[i] = (i % 3) + 1;  // 1, 2, 3 のいずれか
       }

ステップ3: BDDCTによるコストフィルタリング
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

BDDCTクラスを使って、コスト制約を満たすパスだけを抽出します。

.. code-block:: cpp

       // コストテーブルを設定
       BDDCT ct(mgr);
       ct.alloc(num_edges, 0);
       for (int i = 0; i < num_edges; ++i) {
           ct.set_cost(i + 1, costs[i]);  // 変数番号は1から開始
       }

       // コスト制約付きフィルタリング
       int budget = 10;
       ZDD affordable_paths = ct.zdd_cost_le(paths, budget);
       std::cout << "コスト" << budget << "以下のパス数: "
                 << affordable_paths.card() << std::endl;

       // 最小コスト・最大コストの取得
       std::cout << "最小コスト: " << ct.min_cost(paths) << std::endl;
       std::cout << "最大コスト: " << ct.max_cost(paths) << std::endl;

       return 0;
   }

.. seealso::

   BDDCTの詳細なAPIについては :doc:`../api/utility` を参照してください。
   コスト制約付き列挙の一般的なパターンについては
   :doc:`combinatorial_enumeration` も参照してください。

パフォーマンスのコツ（変数順序）
---------------------------------

グラフ問題におけるZDDの性能は、**変数順序** に大きく依存します。
適切な変数順序を選択することで、ZDDのノード数を大幅に削減できます。

基本原則
~~~~~~~~

グラフ問題では、以下の原則が一般的に有効です。

1. **近接する辺を近い変数に割り当てる**: グラフ上で近い辺を
   変数順序でも近くに配置することで、フロンティアのサイズを小さく保てます。

2. **フロンティア幅を最小化する**: 辺の処理順序（変数順序）に沿って
   フロンティア（処理中の頂点集合）の最大幅が小さくなるように順序を選びます。

3. **帯幅の最小化**: グラフの帯幅（bandwidth）が小さくなるような
   頂点番号付けを行い、それに基づいて辺の順序を決定します。

GBaseの自動順序
~~~~~~~~~~~~~~~

GBaseクラスでは、``set_grid()`` などの構築メソッドが内部的に
適切な変数順序を設定します。カスタムグラフを使用する場合は、
辺の追加順序が変数順序に影響するため、注意が必要です。

テーブルサイズの調整
~~~~~~~~~~~~~~~~~~~~

大規模なグラフ問題では、DDManagerのテーブルサイズを
あらかじめ大きく確保しておくことが有効です。

.. code-block:: cpp

   // 大規模問題用に大きなテーブルを確保
   DDManager mgr(1 << 24, 1 << 22);  // 16Mノード、4Mキャッシュ

テーブルサイズが不足すると、頻繁なGCが発生して性能が大幅に低下します。
問題の規模に応じて適切なサイズを見積もることが重要です。

DFS構築の活用
~~~~~~~~~~~~~

グラフ問題では、枝刈りが効く場合が多いため、
DFS構築（``build_zdd_dfs``）が有効なことがあります。

.. code-block:: cpp

   #include "sbdd2/tdzdd/Sbdd2BuilderDFS.hpp"

   // DFS版ビルダーを使用（メモリ効率が向上する場合あり）
   ZDD result = build_zdd_dfs(mgr, spec);

BFSとDFSの使い分けについては :doc:`../advanced` の
「BFSとDFSの比較」を参照してください。
