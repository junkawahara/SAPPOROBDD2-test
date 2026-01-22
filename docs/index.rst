SAPPOROBDD 2.0 ドキュメント
============================

SAPPOROBDD 2.0は、高性能なBDD（二分決定図）およびZDD（ゼロ抑制二分決定図）ライブラリです。

.. toctree::
   :maxdepth: 2
   :caption: 目次

   introduction
   quickstart
   tutorial
   api/index
   advanced

特徴
----

* **高性能**: 128ビットノード構造、内部ハッシュ法による高速なノード管理
* **C++11対応**: モダンなC++で記述、移植性が高い
* **豊富なDD種類**: BDD、ZDD、QDD、UnreducedBDD/ZDD、PiDD、SeqBDD
* **スレッドセーフ**: マルチスレッド環境での使用が可能
* **派生クラス**: GBase（グラフ列挙）、BDDCT（コスト制約付き列挙）

基本的な使い方
--------------

.. code-block:: cpp

   #include <sbdd2/sbdd2.hpp>
   using namespace sbdd2;

   int main() {
       // マネージャーの作成
       DDManager mgr;

       // 変数の作成
       mgr.new_var();
       mgr.new_var();

       // BDDの作成と演算
       BDD x1 = mgr.var_bdd(1);
       BDD x2 = mgr.var_bdd(2);
       BDD f = x1 & x2;  // x1 AND x2

       // ZDDの作成と演算
       ZDD s1 = ZDD::single(mgr, 1);
       ZDD s2 = ZDD::single(mgr, 2);
       ZDD family = s1 + s2;  // {{1}, {2}}

       return 0;
   }

インデックス
------------

* :ref:`genindex`
* :ref:`search`
