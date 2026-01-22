API リファレンス
================

このセクションでは、SAPPOROBDD 2.0の全てのクラスと関数のAPIリファレンスを提供します。

.. toctree::
   :maxdepth: 2

   core
   bdd
   zdd
   zdd_helper
   extended
   utility

クラス一覧
----------

コアクラス
~~~~~~~~~~

* :cpp:class:`sbdd2::DDManager` - ノード管理、キャッシュ、GC
* :cpp:class:`sbdd2::DDNode` - 128ビットノード構造
* :cpp:class:`sbdd2::DDNodeRef` - ノードへの読み取り専用参照
* :cpp:class:`sbdd2::DDBase` - BDD/ZDDの共通基底クラス

決定図クラス
~~~~~~~~~~~~

* :cpp:class:`sbdd2::BDD` - 縮約BDD
* :cpp:class:`sbdd2::ZDD` - 縮約ZDD
* :cpp:class:`sbdd2::UnreducedBDD` - 非縮約BDD
* :cpp:class:`sbdd2::UnreducedZDD` - 非縮約ZDD
* :cpp:class:`sbdd2::QDD` - 準縮約DD

派生クラス
~~~~~~~~~~

* :cpp:class:`sbdd2::PiDD` - 順列DD
* :cpp:class:`sbdd2::SeqBDD` - シーケンスBDD

ユーティリティクラス
~~~~~~~~~~~~~~~~~~~~

* :cpp:class:`sbdd2::GBase` - グラフベース（パス/サイクル列挙）
* :cpp:class:`sbdd2::BDDCT` - コストテーブル

ヘルパー関数
~~~~~~~~~~~~

* ``get_power_set`` - べき集合生成
* ``get_power_set_with_card`` - 指定濃度のべき集合
* ``get_single_set`` - 単一集合生成
* ``make_dont_care`` - ドントケア変換
* ``is_member`` - メンバーシップ判定
* ``weight_*`` - 重みフィルタ関数群
* ``get_uniformly_random_zdd`` - ランダムZDD生成
* ``get_random_zdd_with_card`` - 指定濃度のランダムZDD

型定義
------

.. cpp:type:: sbdd2::bddvar

   変数番号型（uint32_t、1から開始）

.. cpp:type:: sbdd2::bddindex

   ノードインデックス型（uint64_t、最大2^42）

.. cpp:type:: sbdd2::bddrefcount

   参照カウント型（uint16_t、最大65535）

定数
----

.. cpp:var:: constexpr bddvar BDDVAR_MAX

   変数番号の最大値（2^20 - 1）

.. cpp:var:: constexpr bddindex BDDINDEX_MAX

   ノードインデックスの最大値（2^42 - 1）

.. cpp:var:: constexpr bddrefcount BDDREFCOUNT_MAX

   参照カウントの最大値（65535）
