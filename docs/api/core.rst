コアクラス
==========

.. seealso::

   DDManagerの使い方は :doc:`../guide/bdd_guide` および :doc:`../guide/zdd_guide` を参照してください。

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

**ビットレイアウト（44ビット）**:

* bit 0: 否定フラグ
* bit 1: 定数フラグ（終端ノードかどうか）
* bits 2-43: ノードインデックス（42ビット）

**主要メソッド**:

* ``terminal(bool value)`` - 終端ノードへのアークを作成
* ``node(bddindex index, bool negated = false)`` - 通常ノードへのアークを作成
* ``is_negated()`` - 否定フラグを取得
* ``is_constant()`` - 定数（終端）フラグを取得
* ``index()`` - ノードインデックスを取得
* ``terminal_value()`` - 終端値を取得
* ``negated()`` - 否定したアークを返す

**プレースホルダ機能（TdZdd移植用）**:

トップダウンDD構築時に、子ノードがまだ作成されていない場合に使用します。

* ``placeholder(int level, uint64_t col)`` - プレースホルダArcを作成
* ``is_placeholder()`` - プレースホルダかどうか判定
* ``placeholder_level()`` - プレースホルダのレベルを取得
* ``placeholder_col()`` - プレースホルダの列番号を取得

**定数**:

.. cpp:var:: const Arc sbdd2::ARC_TERMINAL_0

   終端0を指すアーク定数

.. cpp:var:: const Arc sbdd2::ARC_TERMINAL_1

   終端1を指すアーク定数

**使用例**:

.. code-block:: cpp

   // 終端ノード0へのアーク
   Arc zero = Arc::terminal(false);

   // 終端ノード1へのアーク
   Arc one = Arc::terminal(true);

   // ノードインデックス100への通常アーク
   Arc node_arc = Arc::node(100);

   // ノードインデックス100への否定アーク
   Arc neg_arc = Arc::node(100, true);

   // アークの否定
   Arc neg = node_arc.negated();  // 否定フラグを反転

   // 定数の使用
   if (arc == ARC_TERMINAL_0) {
       // 0終端
   }

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

変数番号（Variable Number）とレベル（Level）のマッピングを管理できます。

レベルの規約（SAPPOROBDDオリジナル準拠）
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

* **終端（Terminal）: レベル 0**
* **大きいレベル = 根に近い** （DDの上位）
* **小さいレベル = 終端に近い** （DDの下位）

この規約は、オリジナルのSAPPOROBDDライブラリおよびTdZddと同じです。

.. code-block:: text

   レベル N:    [根]     ← 最上位（レベル最大）
                 |
   レベル N-1: [中間]
                 |
   レベル 1:   [最下]   ← 最下位（終端の1つ上）
                 |
   レベル 0:    終端     ← Terminal

基本操作
~~~~~~~~

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

   // 最上位レベルを取得
   bddvar top = mgr.top_lev();  // 3

変数順序の変更
~~~~~~~~~~~~~~

.. code-block:: cpp

   // 指定レベルに新しい変数を挿入
   bddvar new_var = mgr.new_var_of_lev(2);  // VarID=4, Level=2
   // 既存の変数のレベルがシフト:
   // VarID=1: Level=1 (変わらず)
   // VarID=2: Level=3 (元Level=2が3にシフト)
   // VarID=3: Level=4 (元Level=3が4にシフト)
   // VarID=4: Level=2 (新しく挿入)

変数の比較
~~~~~~~~~~

.. code-block:: cpp

   // 変数の相対位置を比較
   // 大きいレベル = 根に近い
   bool v2_above = mgr.var_is_above_or_equal(2, 1);  // true (Level 3 > Level 1)
   bool v1_below = mgr.var_is_below(1, 2);  // true (Level 1 < Level 3)

   // より上位の変数を取得（レベルが大きい方）
   bddvar higher = mgr.var_of_top_lev(1, 2);  // 2 (Level 3 > Level 1)

主要メソッド一覧
~~~~~~~~~~~~~~~~

.. list-table::
   :widths: 40 60
   :header-rows: 1

   * - メソッド
     - 説明
   * - ``lev_of_var(v)``
     - 変数番号 v のレベルを返す
   * - ``var_of_lev(lev)``
     - レベル lev の変数番号を返す
   * - ``top_lev()``
     - 最上位（最大）レベルを返す
   * - ``new_var_of_lev(lev)``
     - 指定レベルに新変数を挿入
   * - ``var_is_above_or_equal(v1, v2)``
     - v1 が v2 以上のレベルか判定
   * - ``var_is_below(v1, v2)``
     - v1 が v2 より下のレベルか判定
   * - ``var_of_top_lev(v1, v2)``
     - より上位の変数を返す

定数
----

.. cpp:var:: constexpr std::size_t sbdd2::DEFAULT_NODE_TABLE_SIZE = 1 << 20

   デフォルトノードテーブルサイズ（1Mノード = 約100万ノード）

.. cpp:var:: constexpr std::size_t sbdd2::DEFAULT_CACHE_SIZE = 1 << 18

   デフォルトキャッシュサイズ（256Kエントリ）
