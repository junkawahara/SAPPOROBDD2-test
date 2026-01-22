BDDクラス
=========

BDD
---

縮約された二分決定図（Reduced Binary Decision Diagram）クラス。

.. doxygenclass:: sbdd2::BDD
   :members:
   :undoc-members:

使用例
------

基本的な使い方
~~~~~~~~~~~~~~

.. code-block:: cpp

   DDManager mgr;
   mgr.new_var();
   mgr.new_var();

   // 変数BDDを取得
   BDD x1 = mgr.var_bdd(1);
   BDD x2 = mgr.var_bdd(2);

   // ブール演算
   BDD f_and = x1 & x2;   // AND
   BDD f_or = x1 | x2;    // OR
   BDD f_xor = x1 ^ x2;   // XOR
   BDD f_not = ~x1;       // NOT

量化演算
~~~~~~~~

.. code-block:: cpp

   BDD f = x1 & x2;

   // 存在量化: ∃x1. (x1 ∧ x2) = x2
   BDD ex = f.exist(1);

   // 全称量化: ∀x1. (x1 ∧ x2) = 0
   BDD fa = f.forall(1);

ITE演算
~~~~~~~

.. code-block:: cpp

   // if-then-else: x1 ? x2 : ~x2
   BDD ite_result = x1.ite(x2, ~x2);

   // 等価: (x1 ∧ x2) ∨ (¬x1 ∧ ¬x2)

カウントと列挙
~~~~~~~~~~~~~~

.. code-block:: cpp

   BDD f = x1 | x2;

   // 充足割当数
   double count = f.count(2);  // 3.0 (01, 10, 11)

   // 充足割当を1つ取得
   std::vector<int> sat = f.one_sat();

厳密カウント（GMP）
~~~~~~~~~~~~~~~~~~~

大きな変数数の場合、double精度（2^53まで）を超える場合があります。
GMPがインストールされている場合、``exact_count()`` で任意精度の厳密なカウントが可能です。

.. code-block:: cpp

   // 通常のカウント（double、2^53を超えると精度が失われる）
   double approx = f.count(var_count);

   // 厳密カウント（GMP使用、文字列で返す）
   #ifdef SBDD2_HAS_GMP
   std::string exact = f.exact_count();
   // 例: "1152921504606846976" (2^60)
   #endif

.. note::
   ``exact_count()`` は ``SBDD2_HAS_GMP`` が定義されている場合のみ使用可能です。
   GMPがインストールされていれば、CMakeが自動検出します。
