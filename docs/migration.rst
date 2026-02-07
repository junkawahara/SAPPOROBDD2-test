SAPPOROBDD++ から SAPPOROBDD 2.0 への移行ガイド
=================================================

.. contents:: 目次
   :depth: 3
   :local:

概要
----

このドキュメントは、SAPPOROBDD++（バージョン1、名前空間 ``sapporobdd``）から
SAPPOROBDD 2.0（バージョン2、名前空間 ``sbdd2``）への移行を支援するためのガイドです。

移行の理由
~~~~~~~~~~

SAPPOROBDD 2.0は、SAPPOROBDD++をベースに全面的に再設計されたライブラリです。
以下の理由から、新しいプロジェクトではSAPPOROBDD 2.0の使用を推奨します。

* **新アーキテクチャ**: 128ビットノード構造、内部ハッシュ法による高速なノード管理
* **C++11対応**: モダンなC++で記述され、移植性が大幅に向上
* **性能向上**: キャッシュ効率の改善、メモリ使用量の最適化
* **スレッドセーフ**: マルチスレッド環境での安全な操作が可能
* **インスタンスベース設計**: グローバル状態を排除し、複数のマネージャーを同時利用可能
* **新機能**: MTBDD/MTZDD、MVBDD/MVZDD、TdZdd統合、厳密カウントなど多数の新機能

バージョンの対応
~~~~~~~~~~~~~~~~

.. list-table::
   :header-rows: 1
   :widths: 30 35 35

   * - 項目
     - SAPPOROBDD++（旧）
     - SAPPOROBDD 2.0（新）
   * - バージョン
     - Version 1
     - Version 2
   * - 名前空間
     - ``sapporobdd``
     - ``sbdd2``
   * - 言語規格
     - C++03
     - C++11
   * - 設計方針
     - グローバル管理
     - インスタンスベース


ビルドシステムの変更
--------------------

ビルド方法
~~~~~~~~~~

SAPPOROBDD++ではMakefileを使用していましたが、SAPPOROBDD 2.0ではCMakeを使用します。

**旧（SAPPOROBDD++）:**

.. code-block:: bash

   cd src
   source INSTALL

**新（SAPPOROBDD 2.0）:**

.. code-block:: bash

   mkdir build && cd build
   cmake ..
   make -j4

インクルードパス
~~~~~~~~~~~~~~~~

**旧:**

.. code-block:: cpp

   #include "BDD.h"

**新:**

.. code-block:: cpp

   #include <sbdd2/sbdd2.hpp>

名前空間
~~~~~~~~

**旧:**

.. code-block:: cpp

   using namespace sapporobdd;

**新:**

.. code-block:: cpp

   using namespace sbdd2;

CMakeによるリンク
~~~~~~~~~~~~~~~~~

SAPPOROBDD 2.0をCMakeプロジェクトから利用する場合：

.. code-block:: cmake

   find_package(sbdd2 REQUIRED)
   target_link_libraries(your_target sbdd2)


初期化の変更
------------

SAPPOROBDD++ではグローバル関数で初期化を行っていましたが、
SAPPOROBDD 2.0ではDDManagerインスタンスを作成して使用します。

**旧（グローバル初期化）:**

.. code-block:: cpp

   #include "BDD.h"
   using namespace sapporobdd;

   int main() {
       BDD_Init(1024, 1024);  // グローバル初期化
       BDD_NewVar();           // グローバル変数作成
       BDD_NewVar();

       BDD x1 = BDDvar(1);
       BDD x2 = BDDvar(2);
       BDD f = x1 & x2;

       return 0;
   }

**新（インスタンスベース初期化）:**

.. code-block:: cpp

   #include <sbdd2/sbdd2.hpp>
   using namespace sbdd2;

   int main() {
       DDManager mgr;           // デフォルトサイズで作成
       // DDManager mgr(init_size);  // サイズ指定も可能
       mgr.new_var();            // インスタンスメソッドで変数作成
       mgr.new_var();

       BDD x1 = mgr.var_bdd(1);
       BDD x2 = mgr.var_bdd(2);
       BDD f = x1 & x2;

       return 0;
   }

主な変更点：

* ``BDD_Init()`` はDDManagerのコンストラクタに置き換え
* ``BDD_NewVar()`` は ``mgr.new_var()`` に置き換え
* ``BDDvar(v)`` は ``mgr.var_bdd(v)`` に置き換え
* 複数のDDManagerインスタンスを同時に使用可能（ただし異なるマネージャー間でのDD操作は不可）


BDD API 対応表
--------------

基本操作
~~~~~~~~

.. list-table::
   :header-rows: 1
   :widths: 40 40 20

   * - SAPPOROBDD++（旧）
     - SAPPOROBDD 2.0（新）
     - 備考
   * - ``BDD()``
     - ``BDD()``
     - 変更なし
   * - ``BDD(0)``
     - ``BDD::zero(mgr)``
     - 定数偽
   * - ``BDD(1)``
     - ``BDD::one(mgr)``
     - 定数真
   * - ``BDDvar(v)``
     - ``mgr.var_bdd(v)`` / ``BDD::var(mgr, v)``
     - 変数BDD

ノード操作
~~~~~~~~~~

.. list-table::
   :header-rows: 1
   :widths: 40 40 20

   * - SAPPOROBDD++（旧）
     - SAPPOROBDD 2.0（新）
     - 備考
   * - ``f.Top()``
     - ``f.top()``
     - 根の変数番号
   * - ``f.At0(v)``
     - ``f.at0(v)``
     - 0-枝の取得
   * - ``f.At1(v)``
     - ``f.at1(v)``
     - 1-枝の取得
   * - ``f.GetID()``
     - ``f.id()``
     - 内部ID
   * - ``f.Size()``
     - ``f.size()``
     - ノード数

ブール演算
~~~~~~~~~~

.. list-table::
   :header-rows: 1
   :widths: 40 40 20

   * - SAPPOROBDD++（旧）
     - SAPPOROBDD 2.0（新）
     - 備考
   * - ``~f``
     - ``~f``
     - 否定（変更なし）
   * - ``f & g``
     - ``f & g``
     - 論理積（変更なし）
   * - ``f | g``
     - ``f | g``
     - 論理和（変更なし）
   * - ``f ^ g``
     - ``f ^ g``
     - 排他的論理和（変更なし）

量化演算
~~~~~~~~

.. list-table::
   :header-rows: 1
   :widths: 40 40 20

   * - SAPPOROBDD++（旧）
     - SAPPOROBDD 2.0（新）
     - 備考
   * - ``f.Exist(cube)``
     - ``f.exist(v)`` / ``f.exist(vars)``
     - 存在量化
   * - ``f.Univ(cube)``
     - ``f.forall(v)`` / ``f.forall(vars)``
     - 全称量化
   * - ``f.Smooth(v)``
     - ``f.exist(v)``
     - 名前変更

その他の操作
~~~~~~~~~~~~

.. list-table::
   :header-rows: 1
   :widths: 40 40 20

   * - SAPPOROBDD++（旧）
     - SAPPOROBDD 2.0（新）
     - 備考
   * - ``f.Support()``
     - ``f.support()``
     - vectorを返す
   * - ``f.Cofact(cube)``
     - ``f.restrict(v, value)``
     - 変数ごとに適用
   * - ``f.Swap(v1, v2)``
     - （未実装）
     - 将来実装予定
   * - （なし）
     - ``f.ite(t, e)``
     - ITE演算（新規）
   * - （なし）
     - ``f.compose(v, g)``
     - 合成演算（新規）
   * - （なし）
     - ``f.restrict(v, value)``
     - 制限演算（新規）
   * - （なし）
     - ``f.count(max_var)`` / ``f.card()``
     - カウント（新規）
   * - （なし）
     - ``f.one_sat()``
     - 充足割当（新規）

マネージャー関数
~~~~~~~~~~~~~~~~

.. list-table::
   :header-rows: 1
   :widths: 40 40 20

   * - SAPPOROBDD++（旧）
     - SAPPOROBDD 2.0（新）
     - 備考
   * - ``BDD_Init()``
     - ``DDManager mgr;``
     - コンストラクタ
   * - ``BDD_NewVar()``
     - ``mgr.new_var()``
     -
   * - ``BDD_NewVarOfLev(lev)``
     - ``mgr.new_var_of_lev(lev)``
     -
   * - ``BDD_VarUsed()``
     - ``mgr.var_count()``
     -
   * - ``BDD_LevOfVar(v)``
     - ``mgr.lev_of_var(v)``
     -
   * - ``BDD_VarOfLev(lev)``
     - ``mgr.var_of_lev(lev)``
     -
   * - ``BDD_GC()``
     - ``mgr.gc()``
     -


ZDD API 対応表
--------------

終端定数
~~~~~~~~

.. list-table::
   :header-rows: 1
   :widths: 40 40 20

   * - SAPPOROBDD++（旧）
     - SAPPOROBDD 2.0（新）
     - 備考
   * - ``ZDD()`` / ``ZDD(0)``
     - ``ZDD::empty(mgr)``
     - 空集合族
   * - ``ZDD(1)``
     - ``ZDD::single(mgr)``
     - 単位集合族 {∅}
   * - （直接対応なし）
     - ``ZDD::singleton(mgr, v)``
     - 単元素集合族 {{v}}（新規）

集合族演算
~~~~~~~~~~

.. list-table::
   :header-rows: 1
   :widths: 40 40 20

   * - SAPPOROBDD++（旧）
     - SAPPOROBDD 2.0（新）
     - 備考
   * - ``f + g``
     - ``f + g``
     - 和（変更なし）
   * - ``f - g``
     - ``f - g``
     - 差（変更なし）
   * - ``f & g`` / ``f.Intersec(g)``
     - ``f & g``
     - 積（変更なし）
   * - ``f * g``
     - ``f * g`` / ``f.join(g)``
     - 直積
   * - ``f / g``
     - ``f / g``
     - 商（変更なし）
   * - ``f % g``
     - ``f % g``
     - 剰余（変更なし）

ノード操作
~~~~~~~~~~

.. list-table::
   :header-rows: 1
   :widths: 40 40 20

   * - SAPPOROBDD++（旧）
     - SAPPOROBDD 2.0（新）
     - 備考
   * - ``f.Top()``
     - ``f.top()``
     -
   * - ``f.OnSet(v)``
     - ``f.onset(v)``
     -
   * - ``f.OffSet(v)``
     - ``f.offset(v)``
     -
   * - ``f.OnSet0(v)``
     - ``f.onset0(v)``
     -
   * - ``f.Change(v)``
     - ``f.change(v)``
     -

情報取得
~~~~~~~~

.. list-table::
   :header-rows: 1
   :widths: 40 40 20

   * - SAPPOROBDD++（旧）
     - SAPPOROBDD 2.0（新）
     - 備考
   * - ``f.Card()``
     - ``f.card()``
     -
   * - ``f.Lit()``
     - ``f.lit()``
     -
   * - ``f.Len()``
     - ``f.len()``
     -
   * - ``f.Size()``
     - ``f.size()``
     -
   * - ``f.GetID()``
     - ``f.id()``
     -
   * - ``f.Support()``
     - ``f.support()``
     - vectorを返す

集合操作
~~~~~~~~

.. list-table::
   :header-rows: 1
   :widths: 40 40 20

   * - SAPPOROBDD++（旧）
     - SAPPOROBDD 2.0（新）
     - 備考
   * - ``f.Swap(v1, v2)``
     - ``f.swap(v1, v2)``
     -
   * - ``f.Restrict(g)``
     - ``f.restrict(g)``
     -
   * - ``f.Permit(g)``
     - ``f.permit(g)``
     -
   * - ``f.PermitSym(n)``
     - ``f.permit_sym(n)``
     -
   * - ``f.Always()``
     - ``f.always()``
     -
   * - ``f.SymChk(v1, v2)``
     - ``f.sym_chk(v1, v2)``
     -
   * - ``ZDD_Meet(f, g)``
     - ``f.meet(g)``
     -

SAPPOROBDD 2.0 の新規ZDD機能
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. list-table::
   :header-rows: 1
   :widths: 40 40

   * - 新API
     - 説明
   * - ``f.enumerate()``
     - 全集合の列挙
   * - ``f.one_set()``
     - 一つの集合を取得
   * - ``f.to_bdd()``
     - BDDへの変換
   * - ``f.exact_count()``
     - 厳密カウント（GMP使用）


I/O API 対応表
--------------

ファイル入出力
~~~~~~~~~~~~~~

.. list-table::
   :header-rows: 1
   :widths: 40 40 20

   * - SAPPOROBDD++ / SBDD_helper（旧）
     - SAPPOROBDD 2.0（新）
     - 備考
   * - ``BDD_Import(fp)``
     - ``import_bdd(mgr, stream)``
     -
   * - ``f.Export(fp)``
     - ``export_bdd(f, stream)``
     -
   * - ``exportBDDAsBinary(fp, bdd)``
     - ``export_bdd(bdd, stream, opts)``
     - SBDD_helperから統合
   * - ``importBDDAsBinary(fp)``
     - ``import_bdd(mgr, stream, opts)``
     - SBDD_helperから統合

ファイルフォーマット
~~~~~~~~~~~~~~~~~~~~

SAPPOROBDD 2.0では、複数のファイルフォーマットをサポートしています。
``DDFileFormat`` 列挙型で指定します。

.. list-table::
   :header-rows: 1
   :widths: 20 50

   * - フォーマット
     - 説明
   * - ``DDFileFormat::AUTO``
     - ファイル拡張子から自動判定
   * - ``DDFileFormat::BINARY``
     - バイナリ形式
   * - ``DDFileFormat::TEXT``
     - テキスト形式
   * - ``DDFileFormat::DOT``
     - Graphviz DOT形式
   * - ``DDFileFormat::KNUTH``
     - Knuth形式

また、SVGエクスポートやlib_bddフォーマットなど、新しい出力形式もサポートしています。


PiDD / SeqBDD API 対応表
-------------------------

PiDD（Permutation DD）およびSeqBDD（Sequence BDD）は、SAPPOROBDD 2.0でも
同じクラス名で提供されますが、名前空間が ``sbdd2`` に変更されています。

初期化の変更
~~~~~~~~~~~~

**旧（グローバル初期化）:**

.. code-block:: cpp

   PiDD_Init();
   PiDD_NewVar();

**新（インスタンスベース初期化）:**

.. code-block:: cpp

   DDManager mgr;
   PiDD::init(mgr);
   PiDD::new_var();

SeqBDDについても同様のパターンで移行します。
メソッド名はBDD/ZDDと同様にsnake_caseに統一されています。


例外処理の変更
--------------

SAPPOROBDD++ではBDDExceptionクラス階層を使用していましたが、
SAPPOROBDD 2.0ではDD全般に対応した新しい例外階層を使用します。

例外クラスの対応
~~~~~~~~~~~~~~~~

.. list-table::
   :header-rows: 1
   :widths: 40 40 20

   * - SAPPOROBDD++（旧）
     - SAPPOROBDD 2.0（新）
     - 備考
   * - ``BDDException``
     - ``DDException``
     - 基底クラス
   * - （メモリ関連エラー）
     - ``DDMemoryException``
     - 新規分離
   * - （引数関連エラー）
     - ``DDArgumentException``
     - 新規分離
   * - （I/O関連エラー）
     - ``DDIOException``
     - 新規分離
   * - （なし）
     - ``DDIncompatibleException``
     - 異なるマネージャー間の操作エラー（新規）

使用例
~~~~~~

.. code-block:: cpp

   try {
       DDManager mgr;
       // 操作
   } catch (const DDMemoryException& e) {
       std::cerr << "メモリ不足: " << e.what() << std::endl;
   } catch (const DDArgumentException& e) {
       std::cerr << "不正な引数: " << e.what() << std::endl;
   } catch (const DDIOException& e) {
       std::cerr << "I/Oエラー: " << e.what() << std::endl;
   } catch (const DDIncompatibleException& e) {
       std::cerr << "非互換エラー: " << e.what() << std::endl;
   } catch (const DDException& e) {
       std::cerr << "その他のエラー: " << e.what() << std::endl;
   }


SBDD_helper 関数の対応表
-------------------------

SBDD_helper（``SBDD_helper.h``）で提供されていた関数の多くは、
SAPPOROBDD 2.0ではZDDクラスのメソッドや ``zdd_helper.hpp`` に統合されています。

.. list-table::
   :header-rows: 1
   :widths: 40 40 20

   * - SBDD_helper（旧）
     - SAPPOROBDD 2.0（新）
     - 備考
   * - ``getPowerSet(n)``
     - ``zdd_helper`` の関数群
     - ヘルパー関数に統合
   * - ``printZBDDElements(f)``
     - ``f.enumerate()`` + 手動出力
     - enumerate()で列挙後出力
   * - ``exportBDDAsBinary(fp, bdd)``
     - ``export_bdd(bdd, stream)``
     - 統合
   * - ``importBDDAsBinary(fp)``
     - ``import_bdd(mgr, stream)``
     - 統合
   * - ``exportZBDDAsBinary(fp, zdd)``
     - ``export_bdd(zdd, stream)``
     - 統合
   * - ``importZBDDAsBinary(fp)``
     - ``import_bdd(mgr, stream)``
     - 統合
   * - ``getZBDDLit(f)``
     - ``f.lit()``
     - ZDDメソッドに統合
   * - ``getZBDDLen(f)``
     - ``f.len()``
     - ZDDメソッドに統合


SAPPOROBDD 2.0 で削除された機能
--------------------------------

以下のクラスおよび機能はSAPPOROBDD 2.0では提供されていません。
これらを使用しているコードは、代替手段への書き換えが必要です。

.. list-table::
   :header-rows: 1
   :widths: 25 50

   * - 削除された機能
     - 説明・代替手段
   * - ``BDDV`` （BDD Vector）
     - BDDのベクトル表現。 ``std::vector<BDD>`` で代替してください。
   * - ``ZDDV`` （ZDD Vector）
     - ZDDのベクトル表現。 ``std::vector<ZDD>`` で代替してください。
   * - ``BtoI`` / ``CtoI``
     - BDD/ZDDによる整数表現クラス。独自実装が必要です。
   * - ``SOP`` / ``SOPV``
     - 積和標準形クラス。BDDの組み合わせで代替してください。
   * - ``BDDDG``
     - BDD分解グラフ。将来のバージョンで再実装される可能性があります。


SAPPOROBDD 2.0 の新機能
------------------------

SAPPOROBDD 2.0では、多数の新機能が追加されています。

MTBDD / MTZDD（多終端DD）
~~~~~~~~~~~~~~~~~~~~~~~~~~

複数の終端値を持つ決定図です。
数値関数やコスト関数の表現に適しています。

MVBDD / MVZDD（多値DD）
~~~~~~~~~~~~~~~~~~~~~~~~

各変数が2値ではなく多値を取れる決定図です。
多値変数の制約充足問題などに利用できます。

.. code-block:: cpp

   // VarArity Specを使用した多値DD構築
   NQueensVarArity spec(8);
   MVZDD solutions = build_mvzdd_va(mgr, spec);

TdZdd 統合
~~~~~~~~~~~

TdZdd（トップダウンDD構築ライブラリ）のSpecインターフェースが統合されています。
既存のTdZdd用Specクラスをそのまま使用してBDD/ZDDを構築できます。

.. code-block:: cpp

   #include "sbdd2/tdzdd/DdSpec.hpp"
   #include "sbdd2/tdzdd/Sbdd2Builder.hpp"

   using namespace sbdd2;
   using namespace sbdd2::tdzdd;

   // TdZdd Specから直接ZDDを構築
   MySpec spec;
   ZDD zdd = build_zdd(mgr, spec);
   BDD bdd = build_bdd(mgr, spec);

   // DFS版（メモリ効率が良い）
   ZDD zdd_dfs = build_zdd_dfs(mgr, spec);

QDD（準縮約DD）
~~~~~~~~~~~~~~~~

準縮約（Quasi-reduced）決定図です。
全変数のノードが必ず出現する形式のDDを扱います。

UnreducedBDD / UnreducedZDD
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

非縮約の決定図です。トップダウン構築法で手動構築し、
最後に ``reduce()`` で縮約BDD/ZDDに変換できます。

.. code-block:: cpp

   UnreducedBDD node = UnreducedBDD::node(mgr, 1, zero, one);
   BDD result = node.reduce();

ZDD イテレータ
~~~~~~~~~~~~~~

ZDDに含まれる集合を効率的に走査するためのイテレータが提供されています。

* ``DictIterator``: 辞書順イテレータ
* ``WeightIterator``: 重み付きイテレータ
* ``RandomIterator``: ランダムイテレータ

ZDD インデックス
~~~~~~~~~~~~~~~~

ZDDに含まれる集合に対するランキング（集合からインデックスへ）と
アンランキング（インデックスから集合へ）の操作が可能です。

I/O の強化
~~~~~~~~~~

複数のファイルフォーマット（バイナリ、テキスト、DOT、Knuth形式）に対応し、
SVGエクスポートも可能です。

GMP 厳密カウント
~~~~~~~~~~~~~~~~

GMPライブラリがインストールされている場合、``exact_count()`` メソッドで
任意精度の厳密なカウントが可能です。double精度（2^53）を超える
大規模な集合族でも正確なカウントを取得できます。

.. code-block:: cpp

   // double精度カウント
   double approx = f.card();

   // GMP厳密カウント
   mpz_class exact = f.exact_count();


移行チェックリスト
------------------

以下の手順に従って、SAPPOROBDD++からSAPPOROBDD 2.0へ移行してください。

1. **ビルドシステムの変更**

   - Makefileを削除し、CMakeLists.txtを作成する
   - ``cmake ..`` と ``make`` でビルドできることを確認する

2. **インクルードの変更**

   - ``#include "BDD.h"`` を ``#include <sbdd2/sbdd2.hpp>`` に置き換える
   - SBDD_helperを使用している場合、それらのインクルードも削除する

3. **名前空間の変更**

   - ``using namespace sapporobdd;`` を ``using namespace sbdd2;`` に置き換える
   - または ``sapporobdd::`` プレフィックスを ``sbdd2::`` に変更する

4. **初期化コードの変更**

   - ``BDD_Init(init_size, max_size)`` を ``DDManager mgr(init_size);`` に置き換える
   - ``BDD_NewVar()`` を ``mgr.new_var()`` に置き換える
   - グローバル変数で管理していたBDD/ZDDを、DDManagerのスコープ内で管理するように修正する

5. **BDD APIの移行**

   - 定数BDDの生成を ``BDD(0)``/``BDD(1)`` から ``BDD::zero(mgr)``/``BDD::one(mgr)`` に変更する
   - ``BDDvar(v)`` を ``mgr.var_bdd(v)`` に変更する
   - PascalCaseのメソッド名をsnake_caseに変更する（例: ``f.Top()`` を ``f.top()`` に）
   - ``f.Exist(cube)`` を ``f.exist(v)`` に変更する（キューブではなく変数番号を指定）
   - ``f.Univ(cube)`` を ``f.forall(v)`` に変更する
   - ``f.Support()`` の戻り値がBDDからvectorに変更されていることに注意する

6. **ZDD APIの移行**

   - ``ZDD(0)`` を ``ZDD::empty(mgr)`` に変更する
   - ``ZDD(1)`` を ``ZDD::single(mgr)`` に変更する
   - PascalCaseのメソッド名をsnake_caseに変更する
   - ``f.OnSet(v)`` を ``f.onset(v)`` のように変更する

7. **I/O APIの移行**

   - SBDD_helperの入出力関数を ``export_bdd()``/``import_bdd()`` に置き換える
   - ファイルポインタ（``FILE*``）をストリーム（``std::iostream``）に変更する

8. **例外処理の変更**

   - ``BDDException`` を ``DDException`` に置き換える
   - 必要に応じてより具体的な例外クラス（``DDMemoryException`` 等）をキャッチする

9. **削除された機能の対処**

   - ``BDDV``/``ZDDV`` を ``std::vector<BDD>``/``std::vector<ZDD>`` に置き換える
   - ``BtoI``/``CtoI``、``SOP``/``SOPV``、``BDDDG`` を使用しているコードを書き換える

10. **テストの実施**

    - 既存のテストケースをSAPPOROBDD 2.0のAPIで書き換える
    - 移行前後で結果が一致することを確認する
    - 特にカウント値やBDD/ZDDのノード数を比較する

11. **新機能の活用（任意）**

    - ``f.ite(t, e)``、``f.compose(v, g)`` 等の新しいBDD演算を活用する
    - TdZdd統合を利用して、Specベースのトップダウン構築を導入する
    - GMP厳密カウントを利用して、大規模な問題で正確なカウントを取得する
    - ZDDイテレータを活用して効率的な列挙を行う

12. **最終確認**

    - 全てのコンパイルエラーが解消されていることを確認する
    - 全てのテストが通過することを確認する
    - パフォーマンスが旧バージョンと同等以上であることを確認する


完全な移行例
------------

以下に、簡単なプログラムの移行前後の全体像を示します。

移行前（SAPPOROBDD++）
~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   #include "BDD.h"
   #include "SBDD_helper.h"
   using namespace sapporobdd;

   int main() {
       BDD_Init(1024, 1024);
       BDD_NewVar();
       BDD_NewVar();
       BDD_NewVar();

       BDD x1 = BDDvar(1);
       BDD x2 = BDDvar(2);
       BDD x3 = BDDvar(3);

       // 多数決関数
       BDD f = (x1 & x2) | (x2 & x3) | (x1 & x3);

       // 存在量化
       BDD g = f.Exist(x1);

       // ノード数
       std::cout << "Size: " << f.Size() << std::endl;

       // サポート
       BDD sup = f.Support();

       // ファイル出力
       FILE* fp = fopen("output.bdd", "wb");
       exportBDDAsBinary(fp, f);
       fclose(fp);

       return 0;
   }

移行後（SAPPOROBDD 2.0）
~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   #include <sbdd2/sbdd2.hpp>
   #include <fstream>
   using namespace sbdd2;

   int main() {
       DDManager mgr;
       mgr.new_var();
       mgr.new_var();
       mgr.new_var();

       BDD x1 = mgr.var_bdd(1);
       BDD x2 = mgr.var_bdd(2);
       BDD x3 = mgr.var_bdd(3);

       // 多数決関数（演算子は変更なし）
       BDD f = (x1 & x2) | (x2 & x3) | (x1 & x3);

       // 存在量化（変数番号を直接指定）
       BDD g = f.exist(1);

       // ノード数
       std::cout << "Size: " << f.size() << std::endl;

       // サポート（vectorが返る）
       std::vector<bddvar> sup = f.support();

       // ファイル出力（ストリームを使用）
       std::ofstream ofs("output.bdd", std::ios::binary);
       export_bdd(f, ofs);

       return 0;
   }
