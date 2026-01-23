Multi-Terminal DD
=================

Multi-Terminal BDD（MTBDD）とMulti-Terminal ZDD（MTZDD）は、終端が0/1だけでなく任意の型Tの値を持てる決定図です。
代数的決定図（ADD: Algebraic Decision Diagram）としても知られています。

MTBDD
-----

Multi-Terminal BDD。BDDの縮約規則（0枝と1枝が同じなら子を返す）を使用します。

.. doxygenclass:: sbdd2::MTBDD
   :members:
   :undoc-members:

MTZDD
-----

Multi-Terminal ZDD。ZDDの縮約規則（1枝がゼロ終端なら0枝を返す）を使用します。

.. doxygenclass:: sbdd2::MTZDD
   :members:
   :undoc-members:

MTBDDTerminalTable
------------------

終端値を管理するテーブル。型ごとにDDManager内で管理されます。

.. doxygenclass:: sbdd2::MTBDDTerminalTable
   :members:
   :undoc-members:

MTDDBase
--------

MTBDD/MTZDDの共通基底クラス。

.. doxygenclass:: sbdd2::MTDDBase
   :members:
   :undoc-members:

ADDエイリアス
-------------

``ADD<T>`` は ``MTBDD<T>`` のエイリアスです。

.. code-block:: cpp

   template<typename T>
   using ADD = MTBDD<T>;

使用例
------

定数MTBDDの作成
~~~~~~~~~~~~~~~

.. code-block:: cpp

   DDManager mgr;
   mgr.new_var();

   // 定数MTBDD（終端値42）
   MTBDD<int> c42 = MTBDD<int>::constant(mgr, 42);

   // 浮動小数点型も可能
   MTBDD<double> cpi = MTBDD<double>::constant(mgr, 3.14159);

変数で分岐するMTBDD
~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   DDManager mgr;
   mgr.new_var();

   // 終端値を作成
   MTBDD<int> hi = MTBDD<int>::constant(mgr, 100);
   MTBDD<int> lo = MTBDD<int>::constant(mgr, 0);

   // 変数1で分岐: x1=1なら100、x1=0なら0
   MTBDD<int> x1 = MTBDD<int>::ite(mgr, 1, hi, lo);

演算
~~~~

.. code-block:: cpp

   DDManager mgr;
   mgr.new_var();
   mgr.new_var();

   MTBDD<int> hi = MTBDD<int>::constant(mgr, 10);
   MTBDD<int> lo = MTBDD<int>::constant(mgr, 5);
   MTBDD<int> x1 = MTBDD<int>::ite(mgr, 1, hi, lo);

   MTBDD<int> c3 = MTBDD<int>::constant(mgr, 3);

   // 加算: x1=1なら10+3=13、x1=0なら5+3=8
   MTBDD<int> sum = x1 + c3;

   // 減算、乗算
   MTBDD<int> diff = x1 - c3;
   MTBDD<int> prod = x1 * c3;

   // 最小値、最大値
   MTBDD<int> min_val = MTBDD<int>::min(x1, c3);
   MTBDD<int> max_val = MTBDD<int>::max(x1, c3);

BDDからの変換
~~~~~~~~~~~~~

.. code-block:: cpp

   DDManager mgr;
   mgr.new_var();
   mgr.new_var();

   BDD x1 = mgr.var_bdd(1);
   BDD x2 = mgr.var_bdd(2);
   BDD f = x1 & x2;  // x1 AND x2

   // BDDをMTBDDに変換（0→0.0、1→1.0）
   MTBDD<double> m = MTBDD<double>::from_bdd(f, 0.0, 1.0);

   // カスタム値での変換
   MTBDD<double> m2 = MTBDD<double>::from_bdd(f, -100.0, 100.0);

評価
~~~~

.. code-block:: cpp

   DDManager mgr;
   mgr.new_var();
   mgr.new_var();

   MTBDD<int> hi = MTBDD<int>::constant(mgr, 10);
   MTBDD<int> lo = MTBDD<int>::constant(mgr, 5);
   MTBDD<int> x1 = MTBDD<int>::ite(mgr, 1, hi, lo);

   // 変数割り当て（インデックス0は未使用）
   std::vector<bool> assignment = {false, true};  // x1=1
   int result = x1.evaluate(assignment);  // 10

MTZDD（ZDD縮約規則）
~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   DDManager mgr;
   mgr.new_var();

   MTZDD<int> zero = MTZDD<int>::constant(mgr, 0);  // ゼロ終端
   MTZDD<int> hi = MTZDD<int>::constant(mgr, 42);

   // ZDD縮約規則: 1枝がゼロ終端なら0枝を返す
   // この場合、highがゼロではないのでノードが作成される
   MTZDD<int> x1 = MTZDD<int>::ite(mgr, 1, hi, zero);

   // 1枝がゼロ終端の場合
   MTZDD<int> nonzero = MTZDD<int>::constant(mgr, 100);
   MTZDD<int> reduced = MTZDD<int>::ite(mgr, 1, zero, nonzero);
   // -> nonzeroが返される（ノードは作成されない）

ZDDからの変換
~~~~~~~~~~~~~

.. code-block:: cpp

   DDManager mgr;
   mgr.new_var();
   mgr.new_var();

   ZDD s1 = ZDD::single(mgr, 1);
   ZDD s2 = ZDD::single(mgr, 2);
   ZDD f = s1 + s2;  // {{1}, {2}}

   // ZDDをMTZDDに変換
   MTZDD<int> m = MTZDD<int>::from_zdd(f, 0, 1);

カスタム演算（apply）
~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   DDManager mgr;
   mgr.new_var();

   MTBDD<int> a = MTBDD<int>::constant(mgr, 10);
   MTBDD<int> b = MTBDD<int>::constant(mgr, 3);

   // カスタム演算（ラムダ式）
   MTBDD<int> result = a.apply(b, [](int x, int y) {
       return x * x + y * y;  // 10^2 + 3^2 = 109
   });

縮約規則の違い
--------------

MTBDDとMTZDDの主な違いは縮約規則です。

**MTBDD（BDD縮約規則）**

* 0枝と1枝が同じアークを指す場合、子ノードをそのまま返す
* 例: ``ite(v, c, c)`` は ``c`` を返す

**MTZDD（ZDD縮約規則）**

1. 1枝がゼロ終端（T{}と等しい値）を指す場合、0枝を返す
2. 0枝と1枝が同じアークを指す場合、子ノードを返す

* 例: ``ite(v, zero, x)`` は ``x`` を返す

型の要件
--------

テンプレート型 ``T`` は以下をサポートする必要があります:

* ``operator==`` - 等価性判定（終端テーブルでの重複検出）
* ``operator+``, ``operator-``, ``operator*`` - 四則演算（演算で使用する場合）
* ``operator<`` - 比較（min/maxで使用する場合）
* ``std::hash<T>`` - ハッシュ（終端テーブルで使用）

標準の数値型（``int``, ``double``, ``long`` など）はすべてサポートされています。
