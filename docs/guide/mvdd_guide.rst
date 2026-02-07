.. _mvdd_guide:

MVDDガイド
==========

この章では、SAPPOROBDD 2.0の多値変数・多終端DDクラスの使い方を解説します。
MTBDD（Multi-Terminal BDD）、MVZDD（Multi-Valued ZDD）、MVBDD（Multi-Valued BDD）の
3つのクラスが提供されています。

.. contents:: 目次
   :local:
   :depth: 2

概要
----

SAPPOROBDD 2.0では、通常の2値BDD/ZDDに加えて、
多値・多終端の決定図を扱うクラスが提供されています。

.. list-table:: 多値・多終端DDクラス
   :header-rows: 1
   :widths: 15 30 55

   * - クラス
     - 正式名称
     - 説明
   * - ``MTBDD<T>``
     - Multi-Terminal BDD
     - 終端が任意の型Tの値を持つBDD（ADDとも呼ばれる）
   * - ``MVZDD``
     - Multi-Valued ZDD
     - 各変数がk個の値を取れるZDD
   * - ``MVBDD``
     - Multi-Valued BDD
     - 各変数がk個の値を取れるBDD

.. code-block:: cpp

   #include <sbdd2/sbdd2.hpp>
   using namespace sbdd2;

MTBDD / MTZDD
--------------

MTBDD（Multi-Terminal BDD）
~~~~~~~~~~~~~~~~~~~~~~~~~~~

MTBDDは、終端が0/1だけでなく任意の型Tの値を持てるBDDです。
代数的決定図（ADD: Algebraic Decision Diagram）とも呼ばれます。

縮約規則はBDDと同じで、0枝と1枝が同じノードを指す場合にそのノードが除去されます。
ただし、否定枝表現は使用しません。

.. code-block:: cpp

   DDManager mgr;
   mgr.new_var();
   mgr.new_var();

   // 定数MTBDD
   MTBDD<double> c1 = MTBDD<double>::constant(mgr, 3.14);
   MTBDD<double> c2 = MTBDD<double>::constant(mgr, 2.71);

   // 変数で分岐するMTBDD（ITE形式）
   // 変数1が真ならc1、偽ならc2
   MTBDD<double> x1 = MTBDD<double>::ite(mgr, 1, c1, c2);

   // 変数2が真なら1.0、偽なら0.0
   MTBDD<double> hi = MTBDD<double>::constant(mgr, 1.0);
   MTBDD<double> lo = MTBDD<double>::constant(mgr, 0.0);
   MTBDD<double> x2 = MTBDD<double>::ite(mgr, 2, hi, lo);

算術演算
~~~~~~~~

MTBDDでは終端値に対する算術演算がサポートされています。

.. code-block:: cpp

   MTBDD<double> a = MTBDD<double>::constant(mgr, 10.0);
   MTBDD<double> b = MTBDD<double>::constant(mgr, 3.0);

   // 加算
   MTBDD<double> sum = a + b;

   // 減算
   MTBDD<double> diff = a - b;

   // 乗算
   MTBDD<double> prod = a * b;

   // 最小値
   MTBDD<double> min_val = MTBDD<double>::min(a, b);

   // 最大値
   MTBDD<double> max_val = MTBDD<double>::max(a, b);

汎用apply演算
~~~~~~~~~~~~~

``apply()`` メソッドで任意の二項演算をMTBDDに適用できます。

.. code-block:: cpp

   MTBDD<double> a = MTBDD<double>::ite(mgr, 1,
       MTBDD<double>::constant(mgr, 5.0),
       MTBDD<double>::constant(mgr, 3.0));

   MTBDD<double> b = MTBDD<double>::ite(mgr, 2,
       MTBDD<double>::constant(mgr, 2.0),
       MTBDD<double>::constant(mgr, 4.0));

   // カスタム演算: 平均を計算
   MTBDD<double> avg = a.apply(b,
       [](const double& x, const double& y) {
           return (x + y) / 2.0;
       });

BDDからの変換
~~~~~~~~~~~~~

既存のBDDをMTBDDに変換できます。BDDの0終端と1終端にそれぞれ値を割り当てます。

.. code-block:: cpp

   BDD f = mgr.var_bdd(1) & mgr.var_bdd(2);

   // BDD -> MTBDD: 0終端=0.0, 1終端=1.0
   MTBDD<double> mtbdd = MTBDD<double>::from_bdd(f, 0.0, 1.0);

   // 整数型のMTBDD
   MTBDD<int> mtbdd_int = MTBDD<int>::from_bdd(f, 0, 100);

評価
~~~~

変数への値の割り当てに対して、MTBDDの終端値を評価できます。

.. code-block:: cpp

   MTBDD<double> mtbdd = MTBDD<double>::ite(mgr, 1,
       MTBDD<double>::constant(mgr, 10.0),
       MTBDD<double>::constant(mgr, 20.0));

   // x1=true, x2=false
   std::vector<bool> assignment = {false, true, false};  // index 0は未使用
   double value = mtbdd.evaluate(assignment);
   std::cout << "値: " << value << std::endl;  // 10.0

ADDエイリアス
~~~~~~~~~~~~~

``ADD<T>`` は ``MTBDD<T>`` のエイリアスです。

.. code-block:: cpp

   ADD<double> add = ADD<double>::constant(mgr, 42.0);

子ノードアクセス
~~~~~~~~~~~~~~~~

.. code-block:: cpp

   MTBDD<double> mtbdd = MTBDD<double>::ite(mgr, 1,
       MTBDD<double>::constant(mgr, 5.0),
       MTBDD<double>::constant(mgr, 3.0));

   MTBDD<double> lo = mtbdd.low();   // 0枝の子
   MTBDD<double> hi = mtbdd.high();  // 1枝の子

MVZDD / MVBDD
--------------

MVZDD（Multi-Valued ZDD）とMVBDD（Multi-Valued BDD）は、
各変数が :math:`k` 個の値 :math:`\{0, 1, \ldots, k-1\}` を取れる多値DDです。

内部的には通常のBDD/ZDDを使ったOne-Hotエンコーディングで実装されています。

One-Hotエンコーディング
~~~~~~~~~~~~~~~~~~~~~~~

多値変数 :math:`x` が :math:`k` 個の値を取る場合、
内部的には :math:`k-1` 個の2値変数 :math:`x_1, x_2, \ldots, x_{k-1}` を使って
エンコードします。

:math:`k=4` の場合の例:

.. list-table:: One-Hotエンコーディング（k=4）
   :header-rows: 1
   :widths: 20 20 20 20 20

   * - 多値変数の値
     - :math:`x_1`
     - :math:`x_2`
     - :math:`x_3`
     - 説明
   * - :math:`x = 0`
     - 0
     - 0
     - 0
     - 全て0
   * - :math:`x = 1`
     - 1
     - \-
     - \-
     - :math:`x_1` の1枝を選択
   * - :math:`x = 2`
     - 0
     - 1
     - \-
     - :math:`x_1` の0枝、:math:`x_2` の1枝を選択
   * - :math:`x = 3`
     - 0
     - 0
     - 1
     - :math:`x_1, x_2` の0枝、:math:`x_3` の1枝を選択

MVZDDの基本操作
~~~~~~~~~~~~~~~

.. code-block:: cpp

   DDManager mgr;

   // k=4（各変数が0,1,2,3の値を取る）のMVZDDを作成
   MVZDD base = MVZDD::empty(mgr, 4);

   // MVDD変数を追加
   base.new_var();  // MVDD変数1
   base.new_var();  // MVDD変数2

   // 単一要素: 変数1が値2を取る
   MVZDD s1 = MVZDD::singleton(base, 1, 2);

   // 単一要素: 変数2が値3を取る
   MVZDD s2 = MVZDD::singleton(base, 2, 3);

   // 和集合
   MVZDD family = s1 + s2;
   std::cout << "集合数: " << family.card() << std::endl;

ITE構築
~~~~~~~

``MVZDD::ite()`` を使って、各値に対応する子ノードを指定してMVZDDを構築します。

.. code-block:: cpp

   DDManager mgr;
   MVZDD base = MVZDD::empty(mgr, 3);  // k=3
   base.new_var();  // MVDD変数1
   base.new_var();  // MVDD変数2

   // 終端
   MVZDD empty = MVZDD::empty(mgr, 3);
   MVZDD single = MVZDD::single(mgr, 3);

   // 変数1の値ごとに子を指定
   // children[0] = 変数1が値0の場合 -> empty
   // children[1] = 変数1が値1の場合 -> single
   // children[2] = 変数1が値2の場合 -> single
   MVZDD node = MVZDD::ite(base, 1, {empty, single, single});

MVZDD集合族演算
~~~~~~~~~~~~~~~

MVZDDは通常のZDDと同様の集合族演算をサポートします。

.. code-block:: cpp

   MVZDD a = MVZDD::singleton(base, 1, 1);
   MVZDD b = MVZDD::singleton(base, 1, 2);

   // 和集合（Union）
   MVZDD union_result = a + b;

   // 差集合（Difference）
   MVZDD diff_result = a - b;

   // 積集合（Intersection）
   MVZDD inter_result = a & b;

MVBDDの基本操作
~~~~~~~~~~~~~~~

MVBDDは、多値変数を扱うBDDです。ブール関数を多値変数で表現します。

.. code-block:: cpp

   DDManager mgr;

   // k=4（各変数が0,1,2,3の値を取る）のMVBDDを作成
   MVBDD base_zero = MVBDD::zero(mgr, 4);   // 定数0（偽）
   MVBDD base_one = MVBDD::one(mgr, 4);     // 定数1（真）

   // MVDD変数を追加
   base_one.new_var();  // MVDD変数1
   base_one.new_var();  // MVDD変数2

   // 単一リテラル: 変数1 = 2
   MVBDD lit = MVBDD::single(base_one, 1, 2);

MVBDD ITE構築
~~~~~~~~~~~~~

.. code-block:: cpp

   DDManager mgr;
   MVBDD base = MVBDD::one(mgr, 3);  // k=3
   base.new_var();
   base.new_var();

   MVBDD zero = MVBDD::zero(mgr, 3);
   MVBDD one = MVBDD::one(mgr, 3);

   // 変数1の値ごとに子を指定
   MVBDD node = MVBDD::ite(base, 1, {zero, one, one});
   // 変数1が1または2なら真、0なら偽

MVBDDブール演算
~~~~~~~~~~~~~~~

MVBDDは通常のBDDと同様のブール演算をサポートします。

.. code-block:: cpp

   MVBDD a = MVBDD::single(base, 1, 1);  // 変数1 = 1
   MVBDD b = MVBDD::single(base, 2, 2);  // 変数2 = 2

   // 論理積（AND）
   MVBDD and_result = a & b;

   // 論理和（OR）
   MVBDD or_result = a | b;

   // 排他的論理和（XOR）
   MVBDD xor_result = a ^ b;

   // 否定（NOT）- O(1)
   MVBDD not_result = ~a;

子ノードアクセスと評価
~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   MVZDD mvzdd = MVZDD::ite(base_mvzdd, 1, {child0, child1, child2});

   // トップMVDD変数の取得
   bddvar top = mvzdd.top_var();
   std::cout << "トップMVDD変数: " << top << std::endl;

   // 値に対応する子の取得
   MVZDD c0 = mvzdd.child(0);  // 値0に対応する子
   MVZDD c1 = mvzdd.child(1);  // 値1に対応する子
   MVZDD c2 = mvzdd.child(2);  // 値2に対応する子

   // 評価: MVDD変数への割り当てが集合族に含まれるか
   std::vector<int> assignment = {2, 1};  // 変数1=2, 変数2=1
   bool result = mvzdd.evaluate(assignment);

   // 全ての充足割り当てを列挙（MVZDDのみ）
   auto all = mvzdd.all_sat();
   for (const auto& sat : all) {
       for (size_t i = 0; i < sat.size(); ++i) {
           std::cout << "x" << (i+1) << "=" << sat[i] << " ";
       }
       std::cout << std::endl;
   }

変換
----

ZDD/BDDとの相互変換
~~~~~~~~~~~~~~~~~~~~

MVZDD/MVBDDは内部的にZDD/BDDで実装されているため、
相互に変換できます。

.. code-block:: cpp

   // MVZDD -> ZDD
   MVZDD mvzdd = MVZDD::singleton(base_mvzdd, 1, 2);
   const ZDD& internal_zdd = mvzdd.to_zdd();
   std::cout << "内部ZDDノード数: " << internal_zdd.size() << std::endl;

   // ZDD -> MVZDD
   MVZDD restored = MVZDD::from_zdd(base_mvzdd, internal_zdd);

   // MVBDD -> BDD
   MVBDD mvbdd = MVBDD::single(base_mvbdd, 1, 2);
   const BDD& internal_bdd = mvbdd.to_bdd();

   // BDD -> MVBDD
   MVBDD restored_bdd = MVBDD::from_bdd(base_mvbdd, internal_bdd);

ノード数
~~~~~~~~

MVZDD/MVBDDでは、内部DDのノード数とMVDDとしてのノード数の2種類があります。

.. code-block:: cpp

   // 内部ZDDのノード数
   std::size_t zdd_nodes = mvzdd.size();

   // MVDDとしてのノード数（MVDD変数単位でカウント）
   std::size_t mvzdd_nodes = mvzdd.mvzdd_node_count();

   std::cout << "内部ZDDノード数: " << zdd_nodes << std::endl;
   std::cout << "MVZDDノード数: " << mvzdd_nodes << std::endl;

VarArity Specとの連携
---------------------

TdZdd互換のVarArity Specを使って、多値DDをトップダウンで構築できます。
詳細は :doc:`tdzdd_guide` のVarArity Specの項を参照してください。

コンパイル時アリティ
~~~~~~~~~~~~~~~~~~~~

テンプレートパラメータ ``AR`` でアリティを指定したSpecは、
``build_mvzdd()`` / ``build_mvbdd()`` でビルドします。

.. code-block:: cpp

   #include <sbdd2/tdzdd/DdSpec.hpp>
   #include <sbdd2/tdzdd/Sbdd2Builder.hpp>
   using namespace sbdd2::tdzdd;

   // アリティ3のSpec
   class TernarySpec : public DdSpec<TernarySpec, int, 3> {
       int n_;
   public:
       explicit TernarySpec(int n) : n_(n) {}

       int getRoot(int& state) {
           state = 0;
           return n_;
       }

       int getChild(int& state, int level, int value) {
           state += value;
           if (level == 1) return -1;
           return level - 1;
       }
   };

   DDManager mgr;
   TernarySpec spec(4);
   MVZDD mvzdd = build_mvzdd(mgr, spec);

ランタイムアリティ
~~~~~~~~~~~~~~~~~~

``VarArityDdSpec`` や ``VarArityHybridDdSpec`` を使うと、
アリティをランタイムに設定できます。
``build_mvzdd_va()`` / ``build_mvbdd_va()`` でビルドします。

.. code-block:: cpp

   #include <sbdd2/tdzdd/VarArityDdSpec.hpp>
   #include <sbdd2/tdzdd/Sbdd2Builder.hpp>
   using namespace sbdd2::tdzdd;

   class DynamicAritySpec : public VarArityDdSpec<DynamicAritySpec, int> {
       int n_;
   public:
       DynamicAritySpec(int n, int arity) : n_(n) {
           setArity(arity);
       }

       int getRoot(int& state) {
           state = 0;
           return n_;
       }

       int getChild(int& state, int level, int value) {
           state = state * getArity() + value;
           if (level == 1) return -1;
           return level - 1;
       }
   };

   DDManager mgr;
   int arity = 5;  // ランタイムに決定
   DynamicAritySpec spec(3, arity);
   MVZDD mvzdd = build_mvzdd_va(mgr, spec);

   std::cout << "集合数: " << mvzdd.card() << std::endl;

実践例: カラーリング問題
~~~~~~~~~~~~~~~~~~~~~~~~

多値DDを使ったグラフ彩色問題の例です。
各頂点に :math:`k` 色のうち1色を割り当て、
隣接する頂点が異なる色になる割り当てを列挙します。

.. code-block:: cpp

   #include <sbdd2/sbdd2.hpp>
   using namespace sbdd2;
   using namespace sbdd2::tdzdd;

   // k色グラフ彩色のVarAritySpec
   class GraphColoringSpec : public VarArityDdSpec<GraphColoringSpec, int> {
       int n_;                          // 頂点数
       std::vector<std::pair<int,int>> edges_;  // 辺リスト

   public:
       GraphColoringSpec(int n, int k,
                         const std::vector<std::pair<int,int>>& edges)
           : n_(n), edges_(edges) {
           setArity(k);
       }

       int getRoot(int& state) {
           state = 0;  // まだ色が決まった辺はない
           return n_;
       }

       int getChild(int& state, int level, int value) {
           // level番目の頂点にvalue色を割り当て
           int vertex = n_ - level + 1;

           // 隣接する頂点で既に同じ色が割り当てられていないかチェック
           // （簡略化のため、stateにエンコード）
           // 実際にはより複雑な状態管理が必要

           if (level == 1) return -1;
           return level - 1;
       }
   };

   int main() {
       DDManager mgr;

       // 三角形グラフ（3頂点、3辺）を3色で彩色
       std::vector<std::pair<int,int>> edges = {{1,2}, {2,3}, {1,3}};
       GraphColoringSpec spec(3, 3, edges);

       MVZDD coloring = build_mvzdd_va(mgr, spec);
       std::cout << "彩色パターン数: " << coloring.card() << std::endl;

       return 0;
   }

.. seealso::

   - :doc:`bdd_guide` - BDDの基本操作
   - :doc:`zdd_guide` - ZDDの基本操作
   - :doc:`tdzdd_guide` - TdZdd互換Specインターフェースの詳細
