Relprod（関係積）
=================

このサンプルでは、BDDによる記号的モデル検査の基本操作である
関係積（Relational Product）を計算します。
ソースコード: ``examples/relprod.cpp``

問題の定義
----------

状態遷移系において、遷移関係BDDと状態集合BDDから後続状態集合（または先行状態集合）を
計算します。これは記号的モデル検査や到達可能性解析の基本演算です。

変数符号化の規約：

* 偶数変数 (2, 4, 6, ...): 現在状態ビット
* 奇数変数 (1, 3, 5, ...): 次状態ビット

アルゴリズム
------------

**後続状態（RelNext）**:

.. math::

   \text{Next}(\text{next}) = \exists \text{curr}.\ (\text{States}(\text{curr}) \land \text{Relation}(\text{curr}, \text{next}))

1. 状態BDDと遷移関係BDDをAND
2. 現在状態変数（偶数）を存在量化で除去

**先行状態（RelPrev）**:

.. math::

   \text{Prev}(\text{curr}) = \exists \text{next}.\ (\text{States}(\text{next}) \land \text{Relation}(\text{curr}, \text{next}))

1. 状態BDDと遷移関係BDDをAND
2. 次状態変数（奇数）を存在量化で除去

使用API
-------

* ``DDManager``, ``BDD``
* ``import_bdd_as_libbdd()`` -- lib_bdd形式からのインポート
* ``BDD::exist(v)`` -- 存在量化
* ``BDD::count(n)`` -- 充足割当数
* 演算子: ``&`` (AND)

実行方法
--------

.. code-block:: bash

   ./relprod -r relation.bdd -s states.bdd      # 後続状態
   ./relprod -r relation.bdd -s states.bdd -p    # 先行状態
