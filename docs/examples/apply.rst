Apply（BDD演算）
================

このサンプルでは、lib_bdd形式のファイルから複数のBDDを読み込み、
逐次的にANDまたはOR演算を適用するベンチマークを行います。
ソースコード: ``examples/apply.cpp``

問題の定義
----------

複数のBDDファイルを入力として受け取り、すべてのBDDに対して二項演算を
逐次的に適用します。各ステップで中間結果のノード数と計算時間を記録し、
BDD演算のパフォーマンスを測定します。

アルゴリズム
------------

1. **BDD読み込み**: lib_bdd形式のバイナリファイルから ``import_bdd_as_libbdd()`` で読み込み
2. **逐次演算**: 最初のBDDから開始し、残りのBDDに対して順にAND（またはOR）を適用
3. **結果計数**: 最終BDDの充足割当数を計算

使用API
-------

* ``DDManager``, ``BDD``
* ``import_bdd_as_libbdd()`` -- lib_bdd形式からのインポート
* ``BDD::size()`` -- ノード数
* ``BDD::count(n)`` -- 充足割当数
* 演算子: ``&`` (AND), ``|`` (OR)

実行方法
--------

.. code-block:: bash

   ./apply -f file1.bdd -f file2.bdd -f file3.bdd      # AND演算
   ./apply -f file1.bdd -f file2.bdd -f file3.bdd -o    # OR演算
