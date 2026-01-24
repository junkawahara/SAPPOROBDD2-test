Multi-Valued DD
===============

Multi-Valued BDD（MVBDD）とMulti-Valued ZDD（MVZDD）は、各変数が k 個の値 {0, 1, ..., k-1} を取れる決定図です。
内部的には通常の BDD/ZDD で One-Hot 風のエンコーディングを用いてエミュレートして実装されています。

エンコーディング方式
--------------------

MVDD の変数 x (k=4 の例) は、内部的に k-1=3 個の DD 変数 (x_1, x_2, x_3) で表現されます:

* x=1: x_1 の 1-arc を選択
* x=2: x_1 の 0-arc → x_2 の 1-arc を選択
* x=3: x_1 の 0-arc → x_2 の 0-arc → x_3 の 1-arc を選択
* x=0: 全ての内部変数の 0-arc を選択

MVZDD
-----

Multi-Valued ZDD。集合族を表現し、ZDD の縮約規則を使用します。

.. doxygenclass:: sbdd2::MVZDD
   :members:
   :undoc-members:

MVBDD
-----

Multi-Valued BDD。ブール関数を表現し、BDD の縮約規則を使用します。

.. doxygenclass:: sbdd2::MVBDD
   :members:
   :undoc-members:

MVDDBase
--------

MVBDD/MVZDD の共通基底クラス。

.. doxygenclass:: sbdd2::MVDDBase
   :members:
   :undoc-members:

MVDDVarTable
------------

MVDD 変数のマッピングを管理するテーブル。

.. doxygenclass:: sbdd2::MVDDVarTable
   :members:
   :undoc-members:

使用例
------

基本的な MVZDD の使用
~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   DDManager mgr;

   // k=4 の MVZDD を作成（各変数は 0, 1, 2, 3 の値を取れる）
   MVZDD f = MVZDD::empty(mgr, 4);

   // MVDD 変数を追加
   bddvar mv1 = f.new_var();  // 変数1
   bddvar mv2 = f.new_var();  // 変数2

   // 単一要素: 変数1が値2を取る
   MVZDD s1 = MVZDD::singleton(f, 1, 2);

   // 単一要素: 変数2が値1を取る
   MVZDD s2 = MVZDD::singleton(f, 2, 1);

   // 集合族演算
   MVZDD u = s1 + s2;  // 和集合
   MVZDD i = s1 * s2;  // 積集合（この場合は空）
   MVZDD d = u - s1;   // 差集合

   // カーディナリティ
   double count = u.card();  // 2.0

ITE による構築
~~~~~~~~~~~~~~

.. code-block:: cpp

   DDManager mgr;
   MVZDD f = MVZDD::empty(mgr, 4);
   f.new_var();

   MVZDD base = MVZDD::single(mgr, 4);
   base = MVZDD(base.manager(), f.var_table(), base.to_zdd());
   MVZDD empty_zdd = MVZDD(f.manager(), f.var_table(), ZDD::empty(mgr));

   // 変数1の各値に対応する子を指定
   // children[i] = 変数1が値iを取る場合の部分集合族
   std::vector<MVZDD> children = {empty_zdd, base, empty_zdd, base};
   // 値0 -> empty, 値1 -> base, 値2 -> empty, 値3 -> base

   MVZDD result = MVZDD::ite(f, 1, children);

   // 評価
   result.evaluate({0});  // false
   result.evaluate({1});  // true
   result.evaluate({2});  // false
   result.evaluate({3});  // true

評価と列挙
~~~~~~~~~~

.. code-block:: cpp

   DDManager mgr;
   MVZDD f = MVZDD::empty(mgr, 4);
   f.new_var();
   f.new_var();

   MVZDD s1 = MVZDD::singleton(f, 1, 1);
   MVZDD s2 = MVZDD::singleton(f, 1, 2);
   MVZDD u = s1 + s2;

   // 評価: 割り当てが集合族に含まれるか
   bool b1 = u.evaluate({1, 0});  // true
   bool b2 = u.evaluate({2, 0});  // true
   bool b3 = u.evaluate({0, 0});  // false

   // 全ての充足割り当てを列挙
   auto sats = u.all_sat();
   // sats = {{1, 0}, {2, 0}}

基本的な MVBDD の使用
~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   DDManager mgr;

   // k=4 の MVBDD を作成
   MVBDD f = MVBDD::one(mgr, 4);

   // MVDD 変数を追加
   f.new_var();
   f.new_var();

   // リテラル: 変数1が値1を取る
   MVBDD lit1 = MVBDD::single(f, 1, 1);

   // リテラル: 変数2が値2を取る
   MVBDD lit2 = MVBDD::single(f, 2, 2);

   // ブール演算
   MVBDD a = lit1 & lit2;   // AND
   MVBDD o = lit1 | lit2;   // OR
   MVBDD x = lit1 ^ lit2;   // XOR
   MVBDD n = ~lit1;         // NOT

   // 評価
   bool b = a.evaluate({1, 2});  // true (両方の条件を満たす)
   bool b2 = a.evaluate({1, 1}); // false

MVBDD の ITE 構築
~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   DDManager mgr;
   MVBDD f = MVBDD::zero(mgr, 4);
   f.new_var();

   MVBDD one = MVBDD(f.manager(), f.var_table(), BDD::one(mgr));
   MVBDD zero = MVBDD(f.manager(), f.var_table(), BDD::zero(mgr));

   // 変数1が値1または値3の場合に true
   std::vector<MVBDD> children = {zero, one, zero, one};
   MVBDD result = MVBDD::ite(f, 1, children);

   result.evaluate({0});  // false
   result.evaluate({1});  // true
   result.evaluate({2});  // false
   result.evaluate({3});  // true

内部 DD への変換
~~~~~~~~~~~~~~~~

.. code-block:: cpp

   DDManager mgr;
   MVZDD f = MVZDD::empty(mgr, 4);
   f.new_var();

   MVZDD s = MVZDD::singleton(f, 1, 2);

   // 内部 ZDD を取得
   ZDD z = s.to_zdd();

   // ZDD から MVZDD を構築
   MVZDD s2 = MVZDD::from_zdd(f, z);

   // MVBDD でも同様
   MVBDD b = MVBDD::zero(mgr, 4);
   b.new_var();
   MVBDD lit = MVBDD::single(b, 1, 1);

   // 内部 BDD を取得
   BDD bdd = lit.to_bdd();

   // BDD から MVBDD を構築
   MVBDD lit2 = MVBDD::from_bdd(b, bdd);

異なる k 値の MVDD
~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   DDManager mgr;

   // Binary (k=2): 通常の 2 値
   MVZDD f2 = MVZDD::empty(mgr, 2);
   f2.new_var();

   // Ternary (k=3): 3 値
   MVZDD f3 = MVZDD::empty(mgr, 3);
   f3.new_var();

   // 5-valued (k=5)
   MVZDD f5 = MVZDD::empty(mgr, 5);
   f5.new_var();

   // 内部変数数は k-1
   // f2: 1 内部変数
   // f3: 2 内部変数
   // f5: 4 内部変数

縮約規則
--------

**MVZDD（ZDD 縮約規則）**

* 内部 ZDD の縮約規則がそのまま適用される
* 1-arc がゼロ終端を指すノードは縮約される
* 変数の値が 0 の場合、対応する内部変数は ZDD から省略される

**MVBDD（BDD 縮約規則）**

* 内部 BDD の縮約規則がそのまま適用される
* 0-arc と 1-arc が同じ先を指す場合、ノードは縮約される
* 全ての値が同じ結果を持つ変数は省略される

制限事項
--------

* k の範囲: 2 ≤ k ≤ 100
* 異なる k 値を持つ MVDD 同士の演算はサポートされない
* 同じ DDManager で異なる k 値の MVDD を共存させる場合、変数空間は共有されるが、MVDD 変数のマッピングは別々に管理される

ノード数
--------

* ``size()``: 内部 DD のノード数
* ``mvzdd_node_count()`` / ``mvbdd_node_count()``: MVDD としての論理ノード数

.. code-block:: cpp

   DDManager mgr;
   MVZDD f = MVZDD::empty(mgr, 4);
   f.new_var();

   MVZDD s = MVZDD::singleton(f, 1, 2);

   // 内部 ZDD ノード数
   std::size_t internal_nodes = s.size();

   // MVZDD としてのノード数
   std::size_t mvzdd_nodes = s.mvzdd_node_count();
