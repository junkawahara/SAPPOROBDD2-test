組合せ列挙
==========

このガイドでは、SAPPOROBDD 2.0を用いた組合せ列挙の
基本パターンと応用テクニックを解説します。ZDDは集合族を
コンパクトに表現するデータ構造であり、制約付き部分集合の
列挙・数え上げ・サンプリング・ランキングなどを効率的に行えます。

ZDDによる組合せ列挙の基本パターン
----------------------------------

べき集合の構築
~~~~~~~~~~~~~~

n要素のべき集合（全ての部分集合からなる集合族）をZDDで構築する
基本パターンを示します。

.. code-block:: cpp

   #include <sbdd2/sbdd2.hpp>
   using namespace sbdd2;

   int main() {
       DDManager mgr;
       int n = 5;
       for (int i = 1; i <= n; ++i) mgr.new_var();

       // べき集合を構築
       // 初期値: {∅}（空集合のみを含む集合族）
       ZDD all = ZDD::single(mgr);
       for (int i = 1; i <= n; ++i) {
           // 各要素iについて「含む」「含まない」の選択を追加
           all = all + all * ZDD::singleton(mgr, i);
       }

       std::cout << "部分集合の数: " << all.card() << std::endl;
       // 出力: 部分集合の数: 32 (= 2^5)

       return 0;
   }

この構築は、空集合 :math:`\{\emptyset\}` から始めて、各要素 :math:`i` について
「:math:`i` を含まない集合」と「:math:`i` を含む集合」の和を取ることで、
段階的にべき集合を構築しています。

濃度制約付き部分集合
~~~~~~~~~~~~~~~~~~~~

特定の濃度（要素数）の部分集合のみを列挙するには、
べき集合を構築した後でフィルタリングするか、
最初から濃度を制御して構築します。

.. code-block:: cpp

   // 方法1: べき集合から濃度kの部分集合を抽出
   // onset/offsetを組み合わせて実現

   // 方法2: ヘルパー関数を使用
   ZDD k_subsets = get_power_set_with_card(mgr, n, k);
   std::cout << "C(" << n << "," << k << ") = "
             << k_subsets.card() << std::endl;

.. seealso::

   ``get_power_set`` および ``get_power_set_with_card`` の詳細については
   :doc:`../api/zdd_helper` を参照してください。

制約付き集合族の構築
---------------------

ZDDの演算を組み合わせることで、様々な制約を満たす集合族を構築できます。

onset/offsetによるフィルタリング
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

``onset`` と ``offset`` を使うと、特定の要素の有無で集合族をフィルタリングできます。

.. code-block:: cpp

   DDManager mgr;
   int n = 5;
   for (int i = 1; i <= n; ++i) mgr.new_var();

   // べき集合
   ZDD all = ZDD::single(mgr);
   for (int i = 1; i <= n; ++i) {
       all = all + all * ZDD::singleton(mgr, i);
   }

   // 要素1を必ず含む集合のみ
   ZDD with_1 = all.onset(1);
   std::cout << "要素1を含む集合数: " << with_1.card() << std::endl;
   // 出力: 16 (= 2^4)

   // 要素2を含まない集合のみ
   ZDD without_2 = all.offset(2);
   std::cout << "要素2を含まない集合数: " << without_2.card() << std::endl;
   // 出力: 16 (= 2^4)

   // 要素1を含み、要素2を含まない集合
   ZDD filtered = all.onset(1).offset(2);
   std::cout << "要素1を含み要素2を含まない集合数: "
             << filtered.card() << std::endl;
   // 出力: 8 (= 2^3)

restrictとpermitによる制約
~~~~~~~~~~~~~~~~~~~~~~~~~~~

``restrict`` と ``permit`` を使うと、より複雑な制約を表現できます。

.. code-block:: cpp

   // restrict(p): pに含まれる要素のみ使用可能な部分集合に制限
   ZDD allowed_elements = ZDD::singleton(mgr, 1)
                          * ZDD::singleton(mgr, 3)
                          * ZDD::singleton(mgr, 5);
   ZDD restricted = all.restrict(allowed_elements);
   // {1,3,5}の部分集合のみ: {∅}, {1}, {3}, {5}, {1,3}, {1,5}, {3,5}, {1,3,5}

   // permit(p): pの部分集合のみを許可
   ZDD permitted = all.permit(allowed_elements);

集合演算による制約の組み合わせ
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

複数の制約を集合演算で組み合わせることで、複雑な条件を表現できます。

.. code-block:: cpp

   // 制約1: 要素1または要素2を含む
   ZDD c1 = all.onset(1) + all.onset(2);

   // 制約2: 要素4と要素5を同時に含まない
   ZDD forbidden = all.onset(4).onset(5);  // {4,5}を両方含む集合
   ZDD c2 = all - forbidden;

   // 両方の制約を満たす集合族
   ZDD feasible = c1 & c2;  // 積集合
   std::cout << "実行可能な集合数: " << feasible.card() << std::endl;

ランダムサンプリング
---------------------

ZDDが表現する集合族から、一様ランダムに要素（集合）をサンプリングできます。
大規模な集合族の性質を統計的に調べる際に有用です。

.. code-block:: cpp

   #include <sbdd2/sbdd2.hpp>
   #include <random>
   using namespace sbdd2;

   int main() {
       DDManager mgr;
       int n = 10;
       for (int i = 1; i <= n; ++i) mgr.new_var();

       // 集合族を構築
       ZDD family = ZDD::single(mgr);
       for (int i = 1; i <= n; ++i) {
           family = family + family * ZDD::singleton(mgr, i);
       }

       // ランダムサンプリング
       std::mt19937 rng(42);  // シード値42で初期化
       auto sampler = family.random_iterator(rng);

       // 10個のサンプルを取得
       for (int i = 0; i < 10; ++i) {
           auto sample = sampler.next();
           std::cout << "サンプル" << i << ": {";
           for (auto elem : sample) {
               std::cout << elem << " ";
           }
           std::cout << "}" << std::endl;
       }

       return 0;
   }

ランダムサンプリングは、ZDDのノードに事前計算された
カウント情報を用いて、根から葉へのランダムウォークで実現されます。
サンプリング1回あたりの計算量はZDDの深さ（変数数）に比例します。

.. note::

   ランダムサンプリングは一様分布に基づきます。
   つまり、ZDDが表現する集合族の各要素が等確率で選ばれます。

重み順列挙とランキング
-----------------------

ZDDが表現する集合族に対して、インデックス構造を構築することで
ランキング（集合から順位への変換）とアンランキング（順位から集合への変換）を
効率的に行えます。

インデックスの構築
~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   #include <sbdd2/sbdd2.hpp>
   using namespace sbdd2;

   int main() {
       DDManager mgr;
       int n = 5;
       for (int i = 1; i <= n; ++i) mgr.new_var();

       // 集合族を構築
       ZDD family = ZDD::single(mgr);
       for (int i = 1; i <= n; ++i) {
           family = family + family * ZDD::singleton(mgr, i);
       }

       // インデックス構造を構築
       auto idx = family.build_index();

ランキング（集合 -> 順位）
~~~~~~~~~~~~~~~~~~~~~~~~~~

与えられた集合が集合族の中で何番目かを求めます。

.. code-block:: cpp

       // 集合{1, 3, 5}の順位を取得
       std::vector<int> some_set = {1, 3, 5};
       auto rank = idx.rank(some_set);
       std::cout << "{1, 3, 5}の順位: " << rank << std::endl;

アンランキング（順位 -> 集合）
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

指定した順位に対応する集合を求めます。

.. code-block:: cpp

       // 順位42に対応する集合を取得
       auto set = idx.unrank(42);
       std::cout << "順位42の集合: {";
       for (auto elem : set) {
           std::cout << elem << " ";
       }
       std::cout << "}" << std::endl;

       return 0;
   }

ランキングとアンランキングは、辞書順に基づいて行われます。
計算量はいずれもZDDの深さに比例し、集合族のサイズには依存しません。

応用例
~~~~~~

ランキング/アンランキングの代表的な応用は以下の通りです。

* **圧縮**: 集合を整数（順位）で表現することで効率的に格納
* **ランダムアクセス**: k番目の要素を直接取得
* **分散処理**: 順位の範囲を分割して並列処理

コスト制約付き列挙（BDDCT）
----------------------------

BDDCTクラスを使うと、各要素にコスト（重み）を割り当て、
コストに基づいたフィルタリングを効率的に行えます。
ナップサック問題やスケジューリング問題などに応用できます。

基本的な使い方
~~~~~~~~~~~~~~

.. code-block:: cpp

   #include <sbdd2/sbdd2.hpp>
   using namespace sbdd2;

   int main() {
       DDManager mgr;
       int n = 6;
       for (int i = 1; i <= n; ++i) mgr.new_var();

       // 各アイテムの重み
       std::vector<int> weights = {0, 10, 20, 15, 25, 30, 5};
       // weights[0]はダミー（変数番号は1から開始）

       // コストテーブルの設定
       BDDCT ct(mgr);
       ct.alloc(n, 0);
       for (int i = 1; i <= n; ++i) {
           ct.set_cost(i, weights[i]);
       }

       // 全部分集合を構築
       ZDD all = ZDD::single(mgr);
       for (int i = 1; i <= n; ++i) {
           all = all + all * ZDD::singleton(mgr, i);
       }

       // コスト制約によるフィルタリング
       int budget = 50;
       ZDD filtered = ct.zdd_cost_le(all, budget);
       std::cout << "コスト" << budget << "以下の組合せ数: "
                 << filtered.card() << std::endl;

       // 最小コスト・最大コストの取得
       std::cout << "最小コスト: " << ct.min_cost(all) << std::endl;
       std::cout << "最大コスト: " << ct.max_cost(all) << std::endl;

       return 0;
   }

BDDCTの主要操作
~~~~~~~~~~~~~~~~

BDDCTでは以下の操作が利用できます。

.. list-table::
   :header-rows: 1
   :widths: 30 70

   * - メソッド
     - 説明
   * - ``zdd_cost_le(zdd, c)``
     - コストがc以下の集合を抽出
   * - ``zdd_cost_ge(zdd, c)``
     - コストがc以上の集合を抽出
   * - ``zdd_cost_eq(zdd, c)``
     - コストがちょうどcの集合を抽出
   * - ``min_cost(zdd)``
     - 最小コストを取得
   * - ``max_cost(zdd)``
     - 最大コストを取得

ナップサック問題の例
~~~~~~~~~~~~~~~~~~~~

0-1ナップサック問題をBDDCTで解く例を示します。

.. code-block:: cpp

   #include <sbdd2/sbdd2.hpp>
   using namespace sbdd2;

   int main() {
       DDManager mgr;

       // アイテム: (重さ, 価値)
       std::vector<std::pair<int, int>> items = {
           {10, 60}, {20, 100}, {30, 120}, {15, 75}, {25, 90}
       };
       int n = items.size();
       int capacity = 50;

       for (int i = 1; i <= n; ++i) mgr.new_var();

       // 全部分集合を構築
       ZDD all = ZDD::single(mgr);
       for (int i = 1; i <= n; ++i) {
           all = all + all * ZDD::singleton(mgr, i);
       }

       // 重さの制約: 合計重量がcapacity以下
       BDDCT weight_ct(mgr);
       weight_ct.alloc(n, 0);
       for (int i = 0; i < n; ++i) {
           weight_ct.set_cost(i + 1, items[i].first);
       }
       ZDD feasible = weight_ct.zdd_cost_le(all, capacity);
       std::cout << "容量制約を満たす組合せ数: "
                 << feasible.card() << std::endl;

       // 価値の最大化: 実行可能集合の中で最大価値を求める
       BDDCT value_ct(mgr);
       value_ct.alloc(n, 0);
       for (int i = 0; i < n; ++i) {
           value_ct.set_cost(i + 1, items[i].second);
       }
       std::cout << "最大価値: " << value_ct.max_cost(feasible) << std::endl;

       return 0;
   }

グラフ問題との組み合わせ
~~~~~~~~~~~~~~~~~~~~~~~~

BDDCTは :doc:`graph_problems` で紹介したGBaseとの組み合わせにも有効です。
例えば、重み付きグラフの最短パス問題や、
コスト制約付きのネットワーク設計問題に応用できます。

.. code-block:: cpp

   // GBaseでパスを列挙した後、BDDCTでコスト制約を適用
   GBase graph(mgr);
   graph.set_grid(3, 3);
   ZDD paths = graph.sim_paths(1, 9);

   // 辺のコストを設定してフィルタリング
   BDDCT ct(mgr);
   ct.alloc(graph.num_edges(), 0);
   for (int i = 0; i < graph.num_edges(); ++i) {
       ct.set_cost(i + 1, edge_costs[i]);
   }
   ZDD short_paths = ct.zdd_cost_le(paths, max_cost);

.. seealso::

   BDDCTの完全なAPIリファレンスについては :doc:`../api/utility` を
   参照してください。GBaseとの組み合わせの詳細については
   :doc:`graph_problems` の「コスト制約付きパス問題」を参照してください。
