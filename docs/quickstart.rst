クイックスタート
================

インストール
------------

ビルド要件
~~~~~~~~~~

* CMake 3.1以上
* C++11対応コンパイラ（GCC 4.8+, Clang 3.4+, MSVC 2015+）
* （オプション）Google Test（テスト実行用）

ビルド手順
~~~~~~~~~~

.. code-block:: bash

   # リポジトリをクローン
   git clone <SAPPOROBDD2のリポジトリURL>
   cd SAPPOROBDD2

   # ビルドディレクトリを作成
   mkdir build && cd build

   # CMakeを実行
   cmake ..

   # ビルド
   make -j4

   # テストを実行（オプション）
   ./tests/sbdd2_tests

インストール
~~~~~~~~~~~~

.. code-block:: bash

   sudo make install

CMakeでの使用
~~~~~~~~~~~~~

.. code-block:: cmake

   add_subdirectory(path/to/SAPPOROBDD2)
   target_link_libraries(your_target sbdd2_static)

基本的な使い方
--------------

ヘッダのインクルード
~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   #include <sbdd2/sbdd2.hpp>
   using namespace sbdd2;

DDManagerの作成
~~~~~~~~~~~~~~~

すべてのBDD/ZDD操作は ``DDManager`` を通じて行います。

.. code-block:: cpp

   // デフォルトサイズで作成
   DDManager mgr;

   // サイズを指定して作成
   DDManager mgr2(1 << 22, 1 << 20);  // 4Mノード、1Mキャッシュ

変数の作成
~~~~~~~~~~

.. code-block:: cpp

   // 変数を5つ作成
   for (int i = 0; i < 5; ++i) {
       mgr.new_var();
   }

   // 変数数を確認
   std::cout << "変数数: " << mgr.var_count() << std::endl;  // 5

BDDの操作
~~~~~~~~~

.. code-block:: cpp

   // 変数BDDを取得
   BDD x1 = mgr.var_bdd(1);
   BDD x2 = mgr.var_bdd(2);

   // ブール演算
   BDD f_and = x1 & x2;       // AND
   BDD f_or = x1 | x2;        // OR
   BDD f_xor = x1 ^ x2;       // XOR
   BDD f_not = ~x1;           // NOT
   BDD f_diff = x1 - x2;      // f AND NOT g

   // ITE演算
   BDD f_ite = x1.ite(x2, ~x2);  // x1 ? x2 : ~x2

   // 量化
   BDD f_exist = f_and.exist(1);   // ∃x1. (x1 ∧ x2)
   BDD f_forall = f_and.forall(1); // ∀x1. (x1 ∧ x2)

   // カウント
   double count = f_or.card();     // 充足割当数

ZDDの操作
~~~~~~~~~

.. code-block:: cpp

   // 単一要素集合を作成
   ZDD s1 = ZDD::singleton(mgr, 1);   // {{1}}
   ZDD s2 = ZDD::singleton(mgr, 2);   // {{2}}
   ZDD s3 = ZDD::singleton(mgr, 3);   // {{3}}

   // 集合族演算
   ZDD family = s1 + s2 + s3;      // 和: {{1}, {2}, {3}}
   ZDD inter = s1 & s2;            // 積集合: 空
   ZDD diff = family - s1;         // 差: {{2}, {3}}

   // 直積（join）
   ZDD prod = s1 * s2;             // {{1, 2}}

   // カウントと列挙
   double card = family.card();    // 3.0
   auto sets = family.enumerate(); // 全集合を列挙

   // onset/offset
   ZDD with_1 = family.onset(1);   // 要素1を含む集合
   ZDD without_1 = family.offset(1); // 要素1を含まない集合

終端定数
~~~~~~~~

.. code-block:: cpp

   // BDD終端
   BDD bdd_false = BDD::zero(mgr);
   BDD bdd_true = BDD::one(mgr);

   // ZDD終端
   ZDD empty_family = ZDD::empty(mgr);  // 空集合族 ∅
   ZDD single_family = ZDD::single(mgr);  // 単一集合族 {∅}

次のステップ
------------

基本的な使い方がわかったら、以下のガイドを参照してください：

* :doc:`tutorial` — 実践的なチュートリアル
* :doc:`guide/bdd_guide` — BDDの詳細ガイド
* :doc:`guide/zdd_guide` — ZDDの詳細ガイド
* :doc:`guide/gbase_guide` — GBaseによるグラフ列挙ガイド
* :doc:`guide/tdzdd_guide` — TdZdd Spec作成ガイド
* :doc:`application/index` — グラフ問題、SAT/制約充足などの応用ガイド
* :doc:`api/index` — APIリファレンス
* :doc:`migration` — SAPPOROBDD++からの移行ガイド
