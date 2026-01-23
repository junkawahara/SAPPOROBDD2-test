高度なトピック
==============

内部アーキテクチャ
------------------

ノード構造
~~~~~~~~~~

SAPPOROBDD 2.0のノードは128ビット（16バイト）で構成されています。

.. code-block:: text

   Word 0 (64ビット):
   +-------------------+--------------------+
   |    0-arc (44)     | 1-arc下位 (20)     |
   +-------------------+--------------------+
     bits 0-43           bits 44-63

   Word 1 (64ビット):
   +-----------+---+--------+--------+-----+
   | 1-arc上位 | R | refcnt |  var   | rsv |
   +-----------+---+--------+--------+-----+
     bits 0-23   24  25-40    41-60   61-63

   R: 既約フラグ (1ビット)
   refcnt: 参照カウンタ (16ビット)
   var: 変数番号 (20ビット)
   rsv: 予約 (3ビット)

アーク構造
~~~~~~~~~~

各アークは44ビットで、以下の情報を保持します：

* **ビット0**: 否定フラグ
* **ビット1**: 定数フラグ（終端ノードかどうか）
* **ビット2-43**: ノードインデックス（42ビット、最大約4.4兆ノード）

内部ハッシュ法
~~~~~~~~~~~~~~

ノードの一意性を保証するために、内部ハッシュ法（二次探索）を使用しています。

.. code-block:: cpp

   // ハッシュ関数（FNV-1a風）
   size_t hash = 14695981039346656037ULL;
   hash ^= var;
   hash *= 1099511628211ULL;
   hash ^= arc0.data;
   hash *= 1099511628211ULL;
   hash ^= arc1.data;
   hash *= 1099511628211ULL;

   // 二次探索
   for (size_t i = 0; i < table_size; ++i) {
       size_t idx = (hash + i * i) % table_size;
       // ...
   }

メモリ管理
----------

参照カウント
~~~~~~~~~~~~

各ノードは16ビットの参照カウンタを持ちます。

* カウントが0になっても即座には解放されない（遅延回収）
* 最大値65535に達すると、以降は増減しない（永続ノード化）
* GC時に参照カウント0のノードが回収される

ガベージコレクション
~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   // 手動GC
   mgr.gc();

   // 自動GC（内部的に呼ばれる）
   mgr.gc_if_needed();

GCは以下のタイミングで実行されます：

1. ノードテーブルの充填率が閾値を超えた場合
2. メモリ不足が検出された場合
3. 明示的に ``gc()`` が呼ばれた場合

演算キャッシュ
~~~~~~~~~~~~~~

演算結果は自動的にキャッシュされます。

.. code-block:: cpp

   // キャッシュをクリア
   mgr.cache_clear();

   // 統計情報
   std::cout << "キャッシュサイズ: " << mgr.cache_size() << std::endl;

パフォーマンスチューニング
--------------------------

テーブルサイズの最適化
~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   // 大規模な問題用に大きなテーブルを確保
   DDManager mgr(1 << 24, 1 << 22);  // 16Mノード、4Mキャッシュ

変数順序の重要性
~~~~~~~~~~~~~~~~

BDD/ZDDのサイズは変数順序に大きく依存します。
適切な変数順序を選択することで、指数関数的にサイズを削減できることがあります。

.. code-block:: cpp

   // 良い例: 関連する変数を近くに配置
   // 悪い例: ランダムな順序

   // 現在のライブラリでは静的な変数順序を使用
   // 将来のバージョンで動的リオーダリングをサポート予定

キャッシュ効率
~~~~~~~~~~~~~~

* 演算キャッシュは固定サイズのハッシュテーブル
* 衝突時は古いエントリを上書き
* 大規模な演算では適切なキャッシュサイズが重要

スレッドセーフティ
------------------

SAPPOROBDD 2.0はスレッドセーフです。

* ノードテーブルアクセスは ``table_mutex_`` で保護
* キャッシュアクセスは ``cache_mutex_`` で保護
* 変数カウンタは ``std::atomic`` を使用

.. code-block:: cpp

   // マルチスレッドでの使用例
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

注意点：

* 異なるマネージャー間でのBDD/ZDD操作は未定義動作
* 同じマネージャーを複数スレッドで共有する場合は正しく動作

例外処理
--------

SAPPOROBDD 2.0は以下の例外を使用します：

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
   } catch (const DDException& e) {
       std::cerr << "その他のエラー: " << e.what() << std::endl;
   }

VarArity Spec による柔軟なDD構築
--------------------------------

通常のTdZdd互換Specでは、ARITY（各ノードの分岐数）をテンプレート引数として
コンパイル時に指定する必要があります。VarArity Specを使用すると、
ARITYを実行時に指定できます。

VarArity Specの利点
~~~~~~~~~~~~~~~~~~~

1. **実行時パラメータ化**: ユーザー入力やファイルからARITYを読み取れる
2. **コード再利用**: 異なるARITY値に対して同じコードを使用可能
3. **動的問題対応**: 問題サイズに応じてARITYを変更可能

使用例：多値変数の制約充足問題
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   #include "sbdd2/tdzdd/VarArityDdSpec.hpp"
   #include "sbdd2/tdzdd/Sbdd2Builder.hpp"

   using namespace sbdd2;
   using namespace sbdd2::tdzdd;

   // N-クイーン問題のVarArity版
   // 各行のクイーンの列位置を多値変数で表現
   class NQueensVarArity
       : public VarArityHybridDdSpec<NQueensVarArity, int, int> {
       int n_;  // ボードサイズ

   public:
       NQueensVarArity(int n) : n_(n) {
           setArity(n);           // ARITY = n（各行でn通りの列位置）
           setArraySize(n);       // 配置状態を保持
       }

       int getRoot(int& row, int* cols) {
           row = 0;  // 現在の行
           for (int i = 0; i < n_; ++i) cols[i] = -1;  // 未配置
           return n_;  // n行から開始
       }

       int getChild(int& row, int* cols, int level, int col) {
           // col番目の列にクイーンを配置
           int currentRow = n_ - level;

           // 衝突チェック
           for (int r = 0; r < currentRow; ++r) {
               if (cols[r] == col) return 0;  // 同一列
               if (std::abs(cols[r] - col) == currentRow - r) return 0;  // 対角線
           }

           cols[currentRow] = col;
           --level;

           return (level == 0) ? -1 : level;
       }
   };

   int main() {
       DDManager mgr;

       // 8-クイーン問題
       NQueensVarArity spec8(8);
       MVZDD solutions8 = build_mvzdd_va(mgr, spec8);
       std::cout << "8-Queens: " << solutions8.card() << " solutions\n";

       // 同じコードで10-クイーン問題
       NQueensVarArity spec10(10);
       MVZDD solutions10 = build_mvzdd_va(mgr, spec10);
       std::cout << "10-Queens: " << solutions10.card() << " solutions\n";

       return 0;
   }

ARITYの制約
~~~~~~~~~~~

* **範囲**: 1 ～ 100
* **MVZDD/MVBDDの制約**: k >= 2 が必要（ARITY=1は通常のZDD/BDDで対応）
* **エラー**: 範囲外のARITYは ``DDArgumentException`` をスロー

.. code-block:: cpp

   // エラーハンドリング例
   try {
       MyVarAritySpec spec(200);  // ARITY > 100
   } catch (const DDArgumentException& e) {
       std::cerr << "エラー: " << e.what() << std::endl;
       // "ARITY must be between 1 and 100"
   }

VarArity Spec演算
~~~~~~~~~~~~~~~~~

同じARITYを持つVarArity Spec同士は演算できます。

.. code-block:: cpp

   VarAritySpec1 spec1(5);  // ARITY = 5
   VarAritySpec2 spec2(5);  // ARITY = 5

   // 和集合
   auto unionSpec = zddUnionVA(spec1, spec2);
   MVZDD result = build_mvzdd_va(mgr, unionSpec);

   // 積集合
   auto intersectSpec = zddIntersectionVA(spec1, spec2);
   MVZDD result2 = build_mvzdd_va(mgr, intersectSpec);

異なるARITYのSpec同士の演算は ``DDArgumentException`` をスローします。

.. code-block:: cpp

   VarAritySpec1 spec3(3);  // ARITY = 3
   VarAritySpec2 spec4(4);  // ARITY = 4

   try {
       auto unionSpec = zddUnionVA(spec3, spec4);  // エラー
   } catch (const DDArgumentException& e) {
       // "VarArity spec operations require both specs to have the same ARITY"
   }
