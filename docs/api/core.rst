コアクラス
==========

DDManager
---------

.. doxygenclass:: sbdd2::DDManager
   :members:
   :undoc-members:

Arc構造体
---------

.. doxygenstruct:: sbdd2::Arc
   :members:
   :undoc-members:

DDNode
------

.. doxygenclass:: sbdd2::DDNode
   :members:
   :undoc-members:

DDBase
------

.. doxygenclass:: sbdd2::DDBase
   :members:
   :undoc-members:

DDNodeRef
---------

.. doxygenclass:: sbdd2::DDNodeRef
   :members:
   :undoc-members:

キャッシュ関連
--------------

CacheOp
~~~~~~~

.. doxygenenum:: sbdd2::CacheOp

CacheEntry
~~~~~~~~~~

.. doxygenstruct:: sbdd2::CacheEntry
   :members:
   :undoc-members:

変数レベル管理
--------------

変数番号（VarID）とレベルのマッピングを管理できます。

.. code-block:: cpp

   DDManager mgr;

   // 通常の変数作成（VarID = Level）
   mgr.new_var();  // VarID=1, Level=1
   mgr.new_var();  // VarID=2, Level=2
   mgr.new_var();  // VarID=3, Level=3

   // 変数番号からレベルを取得
   bddvar lev = mgr.lev_of_var(2);  // 2

   // レベルから変数番号を取得
   bddvar var = mgr.var_of_lev(2);  // 2

   // 指定レベルに新しい変数を挿入
   bddvar new_var = mgr.new_var_of_lev(2);  // VarID=4, Level=2
   // 既存の変数のレベルがシフト:
   // VarID=1: Level=1 (変わらず)
   // VarID=2: Level=3 (元Level=2が3にシフト)
   // VarID=3: Level=4 (元Level=3が4にシフト)
   // VarID=4: Level=2 (新しく挿入)

   // 最上位レベルを取得
   bddvar top = mgr.top_lev();  // 4

   // 変数の相対位置を比較
   bool v1_above = mgr.var_is_above(1, 2);  // true (Level 1 < Level 3)
   bool v1_below = mgr.var_is_below(1, 2);  // false

   // より上位の変数を取得
   bddvar higher = mgr.var_of_min_lev(1, 2);  // 1 (Level 1 < Level 3)

定数
----

.. doxygendefine:: DEFAULT_NODE_TABLE_SIZE
.. doxygendefine:: DEFAULT_CACHE_SIZE
