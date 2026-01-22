TdZdd互換インターフェース
==========================

概要
----

SAPPOROBDD 2.0は、TdZdd（岩下博明氏開発のフロンティア法ライブラリ）と
互換性のあるSpecインターフェースを提供します。
これにより、TdZddのSpecクラスをそのまま使用してBDD/ZDDを構築できます。

ヘッダファイル
~~~~~~~~~~~~~~

.. code-block:: cpp

   #include "sbdd2/tdzdd/DdSpec.hpp"      // Specベースクラス
   #include "sbdd2/tdzdd/Sbdd2Builder.hpp"    // シングルスレッドビルダー
   #include "sbdd2/tdzdd/Sbdd2BuilderMP.hpp"  // 並列ビルダー
   #include "sbdd2/tdzdd/DdSpecOp.hpp"    // Spec演算子
   #include "sbdd2/tdzdd/DdEval.hpp"      // 評価関数

名前空間
~~~~~~~~

.. code-block:: cpp

   using namespace sbdd2;
   using namespace sbdd2::tdzdd;

Specクラスの作成
----------------

DdSpec
~~~~~~

状態を持つSpecクラスの基底クラスです。

.. code-block:: cpp

   // k-部分集合を列挙するSpec
   // C(n, k) = n個から k個を選ぶ組み合わせ
   class Combination : public DdSpec<Combination, int, 2> {
       int n_;  // 全体の要素数
       int k_;  // 選択する要素数

   public:
       Combination(int n, int k) : n_(n), k_(k) {}

       // ルートノードの初期化
       // 戻り値: 開始レベル（n）または終端（-1: accept, 0: reject）
       int getRoot(int& state) const {
           state = 0;  // 選択済み要素数
           return n_;
       }

       // 子ノードへの遷移
       // level: 現在のレベル（n から 1 まで降順）
       // value: 選択値（0: 選ばない, 1: 選ぶ）
       // 戻り値: 次のレベルまたは終端
       int getChild(int& state, int level, int value) const {
           state += value;
           --level;

           if (level == 0) {
               return (state == k_) ? -1 : 0;
           }

           // 枝刈り
           if (state > k_) return 0;           // 選び過ぎ
           if (state + level < k_) return 0;   // 残りで足りない

           return level;
       }
   };

StatelessDdSpec
~~~~~~~~~~~~~~~

状態を持たないSpecクラスの基底クラスです。

.. code-block:: cpp

   // べき集合（全部分集合）を列挙するSpec
   class PowerSet : public StatelessDdSpec<PowerSet, 2> {
       int n_;

   public:
       explicit PowerSet(int n) : n_(n) {}

       int getRoot() const {
           return n_;
       }

       int getChild(int level, int value) const {
           (void)value;
           if (level == 1) return -1;  // 最下層で accept
           return level - 1;
       }
   };

BDD/ZDDの構築
-------------

シングルスレッドビルダー
~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   DDManager mgr;

   // ZDDを構築
   Combination spec(5, 3);
   ZDD result = build_zdd(mgr, spec);
   // result.card() == 10.0 (C(5,3) = 10)

   // BDDを構築
   BDD bdd_result = build_bdd(mgr, spec);

   // UnreducedZDDを構築
   UnreducedZDD unreduced = build_unreduced_zdd(mgr, spec);
   ZDD reduced = unreduced.reduce();

並列ビルダー（OpenMP）
~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   // 並列にZDDを構築
   Combination spec(10, 5);
   ZDD result = build_zdd_mp(mgr, spec);
   // result.card() == 252.0 (C(10,5) = 252)

   // 並列にBDDを構築
   BDD bdd_result = build_bdd_mp(mgr, spec);

.. note::
   並列ビルダーはOpenMPを使用します。
   コンパイル時に ``-fopenmp`` フラグが必要です。

Spec演算子
----------

複数のSpecを組み合わせることができます。

zddUnion
~~~~~~~~

2つのSpecの和集合を計算します。

.. code-block:: cpp

   Combination spec2(5, 2);  // C(5,2) = 10
   Combination spec3(5, 3);  // C(5,3) = 10

   auto unionSpec = zddUnion(spec2, spec3);
   ZDD result = build_zdd(mgr, unionSpec);
   // result.card() == 20.0

zddIntersection
~~~~~~~~~~~~~~~

2つのSpecの積集合を計算します。

.. code-block:: cpp

   PowerSet powerSpec(3);      // 2^3 = 8 sets
   Singleton singleSpec(3);    // 3 singletons

   auto intersectSpec = zddIntersection(powerSpec, singleSpec);
   ZDD result = build_zdd(mgr, intersectSpec);
   // result.card() == 3.0

zddLookahead
~~~~~~~~~~~~

先読み最適化を適用します。

.. code-block:: cpp

   Combination spec(5, 3);
   auto lookaheadSpec = zddLookahead(spec);
   ZDD result = build_zdd(mgr, lookaheadSpec);

zddUnreduction
~~~~~~~~~~~~~~

非縮約形式でのビルドを指定します。

.. code-block:: cpp

   Combination spec(5, 2);
   auto unreducedSpec = zddUnreduction(spec, 5);  // 5 = 最大レベル
   UnreducedZDD result = build_unreduced_zdd(mgr, unreducedSpec);

戻り値の規約
------------

``getRoot()`` と ``getChild()`` の戻り値:

* **0**: 0終端（reject、空集合）
* **-1**: 1終端（accept、集合を含む）
* **正の値**: 次のレベル番号

レベルは n から 1 まで降順で、変数番号に対応します。

使用例
------

ハミルトンパス列挙
~~~~~~~~~~~~~~~~~~

グラフのハミルトンパスを列挙するSpecの例:

.. code-block:: cpp

   class HamiltonPath : public DdSpec<HamiltonPath, State, 2> {
       // ... グラフ構造に依存した実装
   public:
       int getRoot(State& state) const;
       int getChild(State& state, int level, int value) const;
   };

   // 使用例
   HamiltonPath spec(graph);
   ZDD paths = build_zdd(mgr, spec);

参考文献
--------

* TdZdd: https://github.com/kunisura/TdZdd
* ERATO湊プロジェクト: https://www-erato.ist.hokudai.ac.jp/
