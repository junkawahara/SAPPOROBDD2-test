チュートリアル
==============

このチュートリアルでは、SAPPOROBDD 2.0を使った実践的な例を紹介します。

BDDによるブール関数の表現
-------------------------

.. seealso::

   BDDの詳細は :doc:`guide/bdd_guide` を参照してください。

例: 多数決関数
~~~~~~~~~~~~~~

3変数の多数決関数（2つ以上が真なら真）を表現します。

.. code-block:: cpp

   #include <sbdd2/sbdd2.hpp>
   using namespace sbdd2;

   int main() {
       DDManager mgr;

       // 3変数を作成
       mgr.new_var();
       mgr.new_var();
       mgr.new_var();

       BDD x1 = mgr.var_bdd(1);
       BDD x2 = mgr.var_bdd(2);
       BDD x3 = mgr.var_bdd(3);

       // 多数決関数: (x1 ∧ x2) ∨ (x2 ∧ x3) ∨ (x1 ∧ x3)
       BDD majority = (x1 & x2) | (x2 & x3) | (x1 & x3);

       // ノードサイズを確認
       std::cout << "ノード数: " << majority.size() << std::endl;

       // 充足割当数（8通り中4通り）
       std::cout << "充足割当数: " << majority.count(3) << std::endl;

       return 0;
   }

ZDDによる組合せ列挙
-------------------

.. seealso::

   ZDDの詳細は :doc:`guide/zdd_guide`、組合せ列挙パターンは :doc:`application/combinatorial_enumeration` を参照してください。

例: グラフの独立集合
~~~~~~~~~~~~~~~~~~~~

グラフの独立集合（互いに隣接しない頂点の集合）を列挙します。

.. code-block:: cpp

   #include <sbdd2/sbdd2.hpp>
   using namespace sbdd2;

   int main() {
       DDManager mgr;

       // 4頂点のグラフ（パス: 1-2-3-4）
       // 変数iは「頂点iを選ぶ」を表す
       for (int i = 1; i <= 4; ++i) {
           mgr.new_var();
       }

       // 全ての部分集合から開始
       ZDD all = ZDD::single(mgr);  // {∅}
       for (int i = 1; i <= 4; ++i) {
           ZDD with_i = all * ZDD::singleton(mgr, i);
           all = all + with_i;  // iを含む/含まないを選択
       }

       // 隣接する頂点を同時に選ばない制約
       // エッジ (1,2), (2,3), (3,4) を除外
       ZDD exclude_12 = ZDD::singleton(mgr, 1) * ZDD::singleton(mgr, 2);
       ZDD exclude_23 = ZDD::singleton(mgr, 2) * ZDD::singleton(mgr, 3);
       ZDD exclude_34 = ZDD::singleton(mgr, 3) * ZDD::singleton(mgr, 4);

       // 禁止パターンを含む集合を除外
       ZDD invalid = all * exclude_12 + all * exclude_23 + all * exclude_34;
       ZDD independent = all - invalid;

       std::cout << "独立集合の数: " << independent.card() << std::endl;

       // 列挙
       auto sets = independent.enumerate();
       for (const auto& s : sets) {
           std::cout << "{";
           for (auto v : s) std::cout << v << " ";
           std::cout << "}" << std::endl;
       }

       return 0;
   }

GBaseによるパス列挙
-------------------

.. seealso::

   GBaseの詳細は :doc:`guide/gbase_guide`、グラフ問題の応用は :doc:`application/graph_problems` を参照してください。

GBaseクラスを使ってグラフ上のパスを列挙します。

.. code-block:: cpp

   #include <sbdd2/sbdd2.hpp>
   using namespace sbdd2;

   int main() {
       DDManager mgr;
       GBase graph(mgr);

       // 3x3グリッドグラフを作成
       graph.set_grid(3, 3);

       // 左上(1)から右下(9)へのパスを列挙
       ZDD paths = graph.sim_paths(1, 9);

       std::cout << "パス数: " << paths.card() << std::endl;

       return 0;
   }

BDDCTによるコスト制約付き列挙
-----------------------------

.. seealso::

   コスト制約付き列挙の詳細は :doc:`application/combinatorial_enumeration` を参照してください。

コスト制約のある組合せを効率的に列挙します。

.. code-block:: cpp

   #include <sbdd2/sbdd2.hpp>
   using namespace sbdd2;

   int main() {
       DDManager mgr;

       // 5つのアイテム
       for (int i = 1; i <= 5; ++i) {
           mgr.new_var();
       }

       // コストテーブルを設定
       BDDCT ct(mgr);
       ct.alloc(5, 0);
       ct.set_cost(1, 10);  // アイテム1のコスト: 10
       ct.set_cost(2, 20);  // アイテム2のコスト: 20
       ct.set_cost(3, 15);  // アイテム3のコスト: 15
       ct.set_cost(4, 25);  // アイテム4のコスト: 25
       ct.set_cost(5, 30);  // アイテム5のコスト: 30

       // 全ての部分集合を作成
       ZDD all = ZDD::single(mgr);
       for (int i = 1; i <= 5; ++i) {
           all = all + all * ZDD::singleton(mgr, i);
       }

       // コスト50以下の集合を抽出
       ZDD affordable = ct.zdd_cost_le(all, 50);

       std::cout << "コスト50以下の組合せ数: " << affordable.card() << std::endl;
       std::cout << "最小コスト: " << ct.min_cost(all) << std::endl;
       std::cout << "最大コスト: " << ct.max_cost(all) << std::endl;

       return 0;
   }

UnreducedBDD/ZDDの使用
----------------------

.. seealso::

   TdZddによるトップダウン構築の詳細は :doc:`guide/tdzdd_guide` を参照してください。

トップダウン構築法などで非縮約DDを使用し、最後に縮約する例です。

.. code-block:: cpp

   #include <sbdd2/sbdd2.hpp>
   using namespace sbdd2;

   int main() {
       DDManager mgr;
       mgr.new_var();
       mgr.new_var();

       // 非縮約BDDでノードを手動構築
       UnreducedBDD zero = UnreducedBDD::zero(mgr);
       UnreducedBDD one = UnreducedBDD::one(mgr);

       // x1のノードを作成
       UnreducedBDD x1_node = UnreducedBDD::node(mgr, 1, zero, one);

       // x2のノードを作成
       UnreducedBDD x2_node = UnreducedBDD::node(mgr, 2, zero, one);

       // 組み合わせ（x1 AND x2）を手動構築
       UnreducedBDD and_node = UnreducedBDD::node(mgr, 1, zero, x2_node);

       // 縮約してBDDに変換
       BDD result = and_node.reduce();

       // 検証
       BDD expected = mgr.var_bdd(1) & mgr.var_bdd(2);
       if (result == expected) {
           std::cout << "正しく構築されました" << std::endl;
       }

       return 0;
   }
