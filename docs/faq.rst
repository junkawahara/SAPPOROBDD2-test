FAQ / トラブルシューティング
============================

よくある質問とトラブルシューティングをまとめています。

.. contents:: 目次
   :local:
   :depth: 2

メモリ関連
----------

Q: DDManagerのサイズはどう設定する?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

``DDManager`` のコンストラクタに初期サイズを指定できます。

.. code-block:: cpp

   // デフォルト（大半の用途で十分）
   DDManager mgr;

   // 大規模問題用に明示的に指定
   DDManager mgr(1 << 24, 1 << 22);  // 16Mノード、4Mキャッシュ

デフォルトのサイズは大半の用途で十分です。
メモリ不足が発生した場合や、大規模な問題を扱う場合にのみ調整してください。
テーブルは内部的に自動拡張されますが、初期サイズを大きく設定しておくことで
再ハッシュのオーバーヘッドを避けることができます。

Q: GCのタイミングは?
~~~~~~~~~~~~~~~~~~~~~

ガベージコレクション（GC）は、ノードテーブルの充填率が閾値を超えた場合に
自動的に実行されます。手動で実行することも可能です。

.. code-block:: cpp

   // 手動GC
   mgr.gc();

通常は自動GCに任せて問題ありません。ただし、大量の一時BDDを作成した後に
明示的にGCを呼ぶことで、メモリ使用量を削減できることがあります。

Q: 参照カウンタ飽和とは?
~~~~~~~~~~~~~~~~~~~~~~~~~

各ノードは16ビットの参照カウンタを持ちます（最大値 65535）。
参照カウンタが最大値に達すると **飽和** 状態となり、以降は増減しなくなります。
飽和したノードはGCで回収されることがなく、永続ノードとして扱われます。

通常の使用では参照カウンタが飽和することはまれですが、
同一のBDDノードを非常に多くの変数から参照している場合に発生する可能性があります。
飽和自体はエラーではなく、メモリ効率がわずかに低下するだけです。

変数順序関連
------------

Q: 変数順序の選び方は?
~~~~~~~~~~~~~~~~~~~~~~~

変数順序はBDD/ZDDのサイズに指数関数的な影響を与えるため、
適切な順序の選択は非常に重要です。

一般的なガイドライン：

* **関連する変数を近くに配置する**: 同じ制約に関わる変数は近いレベルに配置
* **問題構造を反映する**: 例えば回路検証では入力から出力への順序が有効
* **対称性を利用する**: 対称な変数は連続したレベルに配置

具体的な例：

* **グリッド問題**: 行優先または列優先の順序
* **回路問題**: トポロジカル順序
* **グラフ問題**: BFS/DFS順序

.. code-block:: cpp

   DDManager mgr;

   // デフォルトでは作成順に変数番号 = レベル
   mgr.new_var();  // v1: レベル 1
   mgr.new_var();  // v2: レベル 2
   mgr.new_var();  // v3: レベル 3

Q: new_var_of_lev()はいつ使う?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

``new_var_of_lev()`` は、特定のレベルに新しい変数を挿入する場合に使用します。
既存の変数のレベルは自動的にシフトされます。

.. code-block:: cpp

   DDManager mgr;
   mgr.new_var();          // v1: レベル 1
   mgr.new_var();          // v2: レベル 2
   mgr.new_var_of_lev(2);  // v3: レベル 2（v2はレベル3にシフト）

主な用途：

* 問題の構造上、特定の位置に変数を追加したい場合
* TdZdd Specとの連携で変数順序を制御する場合
* 変数追加後に既存のBDD/ZDDの変数順序を維持したい場合

パフォーマンス関連
------------------

Q: キャッシュサイズの設定は?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

演算キャッシュのサイズは ``DDManager`` が自動的に管理します。
通常はユーザーが明示的に設定する必要はありません。

キャッシュは固定サイズのハッシュテーブルとして実装されており、
衝突時は古いエントリを上書きします。
大規模な問題でキャッシュヒット率が低い場合は、
コンストラクタで大きなキャッシュサイズを指定できます。

.. code-block:: cpp

   // 大きなキャッシュを指定
   DDManager mgr(1 << 20, 1 << 24);  // 1Mノード、16Mキャッシュ

Q: マルチスレッドで使える?
~~~~~~~~~~~~~~~~~~~~~~~~~~~

SAPPOROBDD 2.0はスレッドセーフです。ノードテーブルとキャッシュへのアクセスは
mutexで保護されています。

.. code-block:: cpp

   DDManager mgr;

   std::thread t1([&mgr]() {
       BDD x1 = mgr.var_bdd(1);
       BDD x2 = mgr.var_bdd(2);
       BDD f = x1 & x2;
   });

   std::thread t2([&mgr]() {
       BDD x3 = mgr.var_bdd(3);
       BDD x4 = mgr.var_bdd(4);
       BDD g = x3 | x4;
   });

   t1.join();
   t2.join();

ただし、以下の点に注意してください：

* 1つの ``DDManager`` に対して複数スレッドがアクセスする場合、
  mutex によるロック競合がボトルネックとなる可能性があります
* 異なる ``DDManager`` のBDD/ZDD同士の演算は **未定義動作** です
* OpenMPによる並列ビルダー（``build_zdd_mp()``）も利用可能です

BDD/ZDD選択
------------

Q: BDDとZDDの使い分けは?
~~~~~~~~~~~~~~~~~~~~~~~~~

**BDD** と **ZDD** はそれぞれ異なる縮約規則を持ち、表現に適した対象が異なります。

.. list-table::
   :header-rows: 1
   :widths: 20 40 40

   * - 特性
     - BDD
     - ZDD
   * - 縮約規則
     - 0枝と1枝が同一なら除去
     - 1枝が0終端なら除去
   * - 適した対象
     - ブール関数
     - 集合族（特に疎な集合族）
   * - 典型的な応用
     - 回路検証、モデル検査
     - 組合せ列挙、グラフ問題
   * - 演算子
     - ``&`` (AND), ``|`` (OR), ``^`` (XOR), ``~`` (NOT)
     - ``+`` (Union), ``-`` (Diff), ``&`` (Intersect), ``*`` (Join)

経験則として：

* ブール関数の表現や操作 → **BDD**
* 組合せの列挙やカウント → **ZDD**
* 集合族で「含まない」要素が多い（疎）→ **ZDD** が有利

Q: MTBDD/MVBDDの違いは?
~~~~~~~~~~~~~~~~~~~~~~~~

**MTBDD（Multi-Terminal BDD）** と **MVBDD（Multi-Valued BDD）** は
異なる拡張です。

* **MTBDD<T>**: 終端ノードに任意の型 T の値を持つBDD。
  ブール関数ではなく、多値関数 :math:`f: \{0,1\}^n \to T` を表現します。

  .. code-block:: cpp

     MTBDD<int> f = ...;  // 終端が整数値

* **MVBDD**: 各変数が2値ではなく多値（k値）を取るBDD。
  :math:`f: \{0,1,...,k-1\}^n \to \{0,1\}` を表現します。

  .. code-block:: cpp

     MVBDD f = ...;  // 各変数がk通りの値を取る

同様に **MTZDD<T>** と **MVZDD** も存在します。

TdZdd関連
----------

Q: Specのデバッグ方法は?
~~~~~~~~~~~~~~~~~~~~~~~~~

TdZdd互換のSpecクラスをデバッグする際のコツ：

1. **小さな入力から始める**: まず2-3変数の問題で正しさを確認

   .. code-block:: cpp

      MySpec spec(3);  // 小さい問題でテスト
      ZDD result = build_zdd(mgr, spec);
      auto sets = result.enumerate();
      // 手動で正解と比較

2. **終端の検証**: ``getChild()`` が返す終端値を確認

   * ``0``: 0終端（この枝は無効）
   * ``-1``: 1終端（この枝は有効な解）
   * 正の値: 次のレベル

3. **状態の可視化**: ``getChild()`` 内でデバッグ出力を追加

   .. code-block:: cpp

      int getChild(int& state, int level, int value) {
          std::cerr << "level=" << level << " value=" << value
                    << " state=" << state << std::endl;
          // ...
      }

4. **既知の結果と比較**: BFS版とDFS版の両方で構築し、結果を比較

   .. code-block:: cpp

      ZDD bfs_result = build_zdd(mgr, spec);
      ZDD dfs_result = build_zdd_dfs(mgr, spec);
      assert(bfs_result == dfs_result);

Q: BFS/DFSの使い分けは?
~~~~~~~~~~~~~~~~~~~~~~~~

``build_zdd()`` （BFS版）と ``build_zdd_dfs()`` （DFS版）の使い分け：

.. list-table::
   :header-rows: 1
   :widths: 25 35 35

   * - 基準
     - BFS（build_zdd）
     - DFS（build_zdd_dfs）
   * - メモリ使用量
     - 全レベルの状態を保持
     - 再帰スタック+キャッシュのみ
   * - 適したケース
     - 状態数が均一
     - 枝刈りが効く問題
   * - メモリ制約
     - メモリに余裕がある場合
     - メモリが厳しい場合
   * - 並列化
     - OpenMP対応（build_zdd_mp）
     - 非対応

一般に、メモリ使用量が問題になる場合はDFS版を試してください。

互換性
------

Q: SAPPOROBDD++からの移行は?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

SAPPOROBDD++（旧バージョン）からSAPPOROBDD 2.0への移行については、
:doc:`migration` を参照してください。主な変更点：

* グローバル変数 → ``DDManager`` ベースの設計
* ``bddp`` 型 → ``BDD`` / ``ZDD`` クラス
* 名前空間 ``sbdd2`` の導入
* C++11以降の機能の活用

Q: Graphillionとの連携は?
~~~~~~~~~~~~~~~~~~~~~~~~~~

SAPPOROBDD 2.0は、Graphillion形式のI/O機能を提供しています。
Graphillionで構築したZDDをファイル経由でSAPPOROBDD 2.0に読み込んだり、
その逆も可能です。

.. code-block:: cpp

   // Graphillion形式での入出力
   zdd.export_graphillion("output.txt");
   ZDD imported = ZDD::import_graphillion(mgr, "input.txt");

Q: 他ライブラリとの連携は?
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

**lib_bdd形式** （Rust製BDDライブラリとの互換形式）によるインポート/エクスポートを
サポートしています。

.. code-block:: cpp

   // lib_bdd形式での入出力
   bdd.export_lib_bdd("output.lbdd");
   BDD imported = BDD::import_lib_bdd(mgr, "input.lbdd");

その他の対応形式：

* **バイナリ形式**: 高速なシリアライズ/デシリアライズ
* **テキスト形式**: 人間が読めるノードテーブル
* **DOT形式**: Graphviz可視化用
* **Knuth形式**: Knuthの ``bdd`` プログラム互換
* **SVG形式**: Web表示用のSVG画像

詳しくは :doc:`guide/io_guide` を参照してください。

トラブルシューティング
----------------------

DDIncompatibleException
~~~~~~~~~~~~~~~~~~~~~~~

**症状**: 異なるDDManager間でBDD/ZDD演算を行うと例外が発生する。

.. code-block:: text

   DDIncompatibleException: BDD/ZDD from different managers used in same operation

**原因**: 2つのBDD（またはZDD）が異なる ``DDManager`` インスタンスに属している。

**対処法**: 同じ演算に使用する全てのBDD/ZDDが同一の ``DDManager`` で
作成されていることを確認してください。

.. code-block:: cpp

   DDManager mgr1, mgr2;
   BDD a = mgr1.var_bdd(1);
   BDD b = mgr2.var_bdd(1);

   // エラー: 異なるマネージャー
   // BDD c = a & b;

   // 正しい: 同じマネージャー
   BDD b2 = mgr1.var_bdd(2);
   BDD c = a & b2;

BDDサイズ爆発
~~~~~~~~~~~~~

**症状**: BDD構築中にメモリが不足する、または演算に非常に時間がかかる。

**原因**: 変数順序が問題に適していない、または中間BDDが巨大になっている。

**対処法**:

1. **変数順序を変更する**: 関連する変数を近いレベルに配置

   .. code-block:: cpp

      // 例: グリッド問題で行と列をインターリーブ
      // 悪い: 全行の後に全列
      // 良い: 同じ位置の行変数と列変数を近くに

2. **中間演算を工夫する**: バランス木結合、存在量化による変数除去

   .. code-block:: cpp

      // 悪い: 線形結合
      BDD result = c1;
      for (auto& ci : clauses) result = result & ci;

      // 良い: バランス木結合
      // cnf.cpp の cnf_to_bdd_balanced() を参照

3. **ZDDの利用を検討する**: 疎な集合族の場合はZDDの方が効率的

4. **GBaseやTdZddの活用**: グラフ問題ではフロンティア法が有効

Sphinx ビルドエラー
~~~~~~~~~~~~~~~~~~~

**症状**: ドキュメントのビルドでエラーが発生する。

**対処法**:

1. Breathe の設定を確認

   .. code-block:: bash

      # Doxyfile が正しいパスを指定しているか確認
      cd docs
      doxygen Doxyfile

2. Sphinx のビルドコマンドを確認

   .. code-block:: bash

      # ロケール設定が必要
      LC_ALL=C python3 -m sphinx -b html . _build/html

3. 必要なパッケージがインストールされているか確認

   .. code-block:: bash

      pip install sphinx breathe sphinx_rtd_theme

.. seealso::

   * :doc:`introduction` -- ライブラリの概要
   * :doc:`quickstart` -- 基本的な使い方
   * :doc:`advanced` -- 高度なトピック
   * :doc:`api/index` -- APIリファレンス
