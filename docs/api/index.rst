API リファレンス
================

このセクションでは、SAPPOROBDD 2.0の全てのクラスと関数のAPIリファレンスを提供します。

.. toctree::
   :maxdepth: 2

   core
   bdd
   zdd
   zdd_helper
   mtbdd
   mvdd
   extended
   utility
   tdzdd
   exception

クラス一覧
----------

コアクラス
~~~~~~~~~~

→ :doc:`../guide/bdd_guide` | :doc:`../guide/zdd_guide`

* :cpp:class:`sbdd2::DDManager` - ノード管理、キャッシュ、GC
* :cpp:class:`sbdd2::DDNode` - 128ビットノード構造
* :cpp:class:`sbdd2::DDNodeRef` - ノードへの読み取り専用参照
* :cpp:class:`sbdd2::DDBase` - BDD/ZDDの共通基底クラス

決定図クラス
~~~~~~~~~~~~

→ :doc:`../guide/bdd_guide` | :doc:`../guide/zdd_guide` | :doc:`../guide/mvdd_guide`

* :cpp:class:`sbdd2::BDD` - 縮約BDD
* :cpp:class:`sbdd2::ZDD` - 縮約ZDD
* :cpp:class:`sbdd2::MTBDD` - Multi-Terminal BDD
* :cpp:class:`sbdd2::MTZDD` - Multi-Terminal ZDD
* :cpp:class:`sbdd2::MVBDD` - Multi-Valued BDD
* :cpp:class:`sbdd2::MVZDD` - Multi-Valued ZDD
* :cpp:class:`sbdd2::UnreducedBDD` - 非縮約BDD
* :cpp:class:`sbdd2::UnreducedZDD` - 非縮約ZDD
* :cpp:class:`sbdd2::QDD` - 準縮約DD

派生クラス
~~~~~~~~~~

* :cpp:class:`sbdd2::PiDD` - 順列DD
* :cpp:class:`sbdd2::SeqBDD` - シーケンスBDD

ユーティリティクラス
~~~~~~~~~~~~~~~~~~~~

→ :doc:`../guide/gbase_guide` | :doc:`../application/graph_problems`

* :cpp:class:`sbdd2::GBase` - グラフベース（パス/サイクル列挙）
* :cpp:class:`sbdd2::BDDCT` - コストテーブル

例外クラス
~~~~~~~~~~

* :cpp:class:`sbdd2::DDException` - DD操作の基本例外
* :cpp:class:`sbdd2::DDMemoryException` - メモリ割り当て失敗
* :cpp:class:`sbdd2::DDArgumentException` - 不正な引数
* :cpp:class:`sbdd2::DDIOException` - I/Oエラー
* :cpp:class:`sbdd2::DDIncompatibleException` - 非互換なDD型またはマネージャー

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

イテレータクラス
~~~~~~~~~~~~~~~~

* :cpp:class:`sbdd2::DictIterator` - 辞書順イテレータ
* :cpp:class:`sbdd2::WeightIterator` - 重み順イテレータ
* :cpp:class:`sbdd2::RandomIterator` - ランダム順イテレータ

I/O関連
~~~~~~~

→ :doc:`../guide/io_guide`

* ``DDFileFormat`` - ファイル形式列挙型
* ``ExportOptions`` - エクスポートオプション
* ``ImportOptions`` - インポートオプション
* ``SvgExportOptions`` - SVGエクスポートオプション

TdZdd互換インターフェース
~~~~~~~~~~~~~~~~~~~~~~~~~

→ :doc:`../guide/tdzdd_guide`

* ``DdSpec`` - Specベースクラス（状態あり）
* ``StatelessDdSpec`` - Specベースクラス（状態なし）
* ``HybridDdSpec`` - Specベースクラス（スカラー + 配列状態）
* ``build_zdd`` / ``build_bdd`` - シングルスレッドビルダー
* ``build_zdd_mp`` / ``build_bdd_mp`` - 並列ビルダー（OpenMP）
* ``build_mvzdd`` / ``build_mvbdd`` - MVDD用ビルダー（ARITY >= 3）
* ``zddUnion`` / ``zddIntersection`` - Spec演算子

VarArity Spec（実行時ARITY指定）
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

* ``VarArityHybridDdSpec`` - 実行時ARITYのHybridSpec
* ``VarArityDdSpec`` - 実行時ARITYのスカラー状態Spec
* ``VarArityStatelessDdSpec`` - 実行時ARITYの状態なしSpec
* ``build_mvzdd_va`` / ``build_mvbdd_va`` - VarArity用ビルダー
* ``build_mvzdd_va_mp`` / ``build_mvbdd_va_mp`` - VarArity用並列ビルダー
* ``zddUnionVA`` / ``zddIntersectionVA`` - VarArity用Spec演算子

厳密カウント（GMP / BigInt）
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

* ``BDD::exact_count()`` - 任意精度のBDD充足割当数
* ``ZDD::exact_count()`` - 任意精度のZDD集合数

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
