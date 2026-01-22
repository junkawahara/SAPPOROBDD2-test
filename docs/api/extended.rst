拡張DDクラス
============

UnreducedBDD
------------

非縮約BDD。トップダウン構築法などで使用します。

.. doxygenclass:: sbdd2::UnreducedBDD
   :members:
   :undoc-members:

UnreducedZDD
------------

非縮約ZDD。トップダウン構築法などで使用します。

.. doxygenclass:: sbdd2::UnreducedZDD
   :members:
   :undoc-members:

QDD
---

準縮約DD（Quasi-reduced Decision Diagram）。
ノード共有はあるが、飛び越し規則は適用しません。

.. doxygenclass:: sbdd2::QDD
   :members:
   :undoc-members:

PiDD
----

順列DD（Permutation DD）。順列の集合を表現します。

.. doxygenclass:: sbdd2::PiDD
   :members:
   :undoc-members:

SeqBDD
------

シーケンスBDD。系列（シーケンス）の集合を表現します。

.. doxygenclass:: sbdd2::SeqBDD
   :members:
   :undoc-members:

使用例
------

UnreducedBDDからBDDへの変換
~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   DDManager mgr;
   mgr.new_var();

   // 非縮約BDDでノードを構築
   UnreducedBDD zero = UnreducedBDD::zero(mgr);
   UnreducedBDD one = UnreducedBDD::one(mgr);
   UnreducedBDD node = UnreducedBDD::node(mgr, 1, zero, one);

   // 縮約してBDDに変換
   BDD bdd = node.reduce();

   // または変換コンストラクタを使用
   BDD bdd2(node);

QDDの使用
~~~~~~~~~

.. code-block:: cpp

   DDManager mgr;
   mgr.new_var();
   mgr.new_var();

   QDD zero = QDD::zero(mgr);
   QDD one = QDD::one(mgr);

   // QDDノードを作成
   QDD node = QDD::node(mgr, 1, zero, one);

   // BDD/ZDDへの変換
   BDD bdd = node.to_bdd();
   ZDD zdd = node.to_zdd();

SeqBDDの使用
~~~~~~~~~~~~

.. code-block:: cpp

   DDManager mgr;
   for (int i = 0; i < 5; ++i) mgr.new_var();

   // シーケンス {1} を作成
   SeqBDD s1 = SeqBDD::single(mgr, 1);

   // シーケンス {2} を作成
   SeqBDD s2 = SeqBDD::single(mgr, 2);

   // 和（両方のシーケンスを含む集合）
   SeqBDD both = s1 + s2;

   // push操作（先頭に要素を追加）
   SeqBDD pushed = s1.push(3);  // {3, 1}

   // カウント
   double count = both.card();
