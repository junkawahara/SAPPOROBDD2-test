変更履歴
========

Version 2.0.0 (2026-01-22 ~ 2026-01-26)
-----------------------------------------

SAPPOROBDD 2.0 の初回リリースです。
SAPPOROBDD++ を基盤として、新しいアーキテクチャで完全に再設計されました。

新アーキテクチャ
~~~~~~~~~~~~~~~~

* C++11ベースの新設計（C++14以降の機能は使用しない）
* 名前空間 ``sbdd2`` の導入
* ``DDManager`` ベースの設計（グローバル変数を廃止）
* 128ビットノード構造による効率的なメモリ利用
* 内部ハッシュ法（二次探索）による高速なノード管理
* 否定枝表現によるO(1) NOT演算

コアクラス
~~~~~~~~~~

* **DDManager**: ノード管理、キャッシュ、GC、変数管理を統合
* **DDNode**: 128ビットノード構造体
* **BDD**: 既約BDDクラス（演算子: ``&``, ``|``, ``^``, ``~``）
* **ZDD**: 既約ZDDクラス（演算子: ``+``, ``-``, ``&``, ``*``, ``/``, ``%``）
* **DDBase**: BDD/ZDDの共通基底クラス
* **DDNodeRef**: ノード参照管理
* **Arc**: 44ビットアーク構造

拡張DDクラス
~~~~~~~~~~~~

* **UnreducedBDD**: 非既約BDD（トップダウン構築用）
* **UnreducedZDD**: 非既約ZDD
* **QDD**: 準既約DD（Quasi-reduced Decision Diagram）

マルチターミナルDD
~~~~~~~~~~~~~~~~~~

* **MTBDD<T>**: マルチターミナルBDD（終端に任意型の値を持つ）
* **MTZDD<T>**: マルチターミナルZDD

多値DD
~~~~~~

* **MVBDD**: 多値BDD（各変数がk通りの値を取る）
* **MVZDD**: 多値ZDD

派生クラス
~~~~~~~~~~

* **PiDD**: 置換DD（Permutation Decision Diagram）
* **SeqBDD**: 系列BDD（Sequence BDD）
* **GBase**: グラフ基盤クラス（パス/閉路列挙、フロンティア法）
* **BDDCT**: コスト制約テーブル（コスト制約付き列挙）

TdZdd統合
~~~~~~~~~

TdZdd（岩下博明氏開発）と互換性のあるインターフェースを提供：

* **DdSpec階層**: ``DdSpec``, ``PodDdSpec``, ``HybridDdSpec``, ``StatelessDdSpec``
* **Sbdd2Builder**: BFS（幅優先）ベースのDD構築
* **DFSBuilder**: DFS（深さ優先）ベースのDD構築（メモリ効率重視）
* **MPBuilder**: OpenMPによる並列DD構築
* **DdEval**: DD上の評価フレームワーク
* **VarAritySpec**: 実行時ARITY指定対応のSpec（``VarArityHybridDdSpec``）
* BDD/ZDD/MVBDD/MVZDD の各種ビルダー関数

ZDD拡張
~~~~~~~

* **ZDD Index**: カウントキャッシュによる効率的なインデックス操作

  * 辞書順アクセス（Dict）: ``get_set()``, ``get_index()``
  * 重み最適化（Weight）: ``min_weight()``, ``max_weight()``
  * ランダムサンプリング（Random）: ``sample()``

* **ランキング/アンランキング**: 集合族の辞書順でのランク操作
* **ZDDヘルパー関数**: ``onset()``, ``offset()``, ``change()``,
  ``restrict()``, ``permit()``, ``product()``（→ ``join()``）

I/O
~~~

複数の入出力形式をサポート：

* **バイナリ形式**: 高速なシリアライズ/デシリアライズ
* **テキスト形式**: 人間が読めるノードテーブル
* **DOT形式**: Graphviz可視化
* **Knuth形式**: Knuthの ``bdd`` プログラム互換
* **Graphillion形式**: Graphillionライブラリ互換
* **lib_bdd形式**: Rust製BDDライブラリ互換
* **SVG形式**: Web表示用のベクタ画像

その他
~~~~~~

* **厳密カウント（GMP / BigInt）**: ``exact_count()`` メソッドによる任意精度カウント
  （double精度 :math:`2^{53}` を超える場合に使用。GMPが見つからない場合はBigIntをフォールバックとして使用）
* **スレッドセーフティ**: ノードテーブル、キャッシュへのアクセスをmutexで保護、
  変数カウンタは ``std::atomic`` を使用
* **例外階層**: ``DDException`` を基底とする構造化された例外
  （``DDMemoryException``, ``DDArgumentException``, ``DDIOException``,
  ``DDIncompatibleException``）
* **変数レベル管理**: 変数番号とレベルの分離、``lev_of_var()``,
  ``var_of_lev()``, ``new_var_of_lev()``
* **サンプルプログラム**: N-Queens、ハミルトン閉路、CNF SAT
  （bdd-benchmarkベース）
