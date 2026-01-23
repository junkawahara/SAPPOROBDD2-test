例外クラス
==========

SAPPOROBDD 2.0で使用される例外クラスを定義します。

.. note::

   これらの例外クラスを使用するには ``#include <sbdd2/exception.hpp>`` が必要です。

DDException
-----------

DD操作の基本例外クラスです。

.. doxygenclass:: sbdd2::DDException
   :members:
   :undoc-members:

**継承元**: ``std::runtime_error``

**使用例**:

.. code-block:: cpp

   try {
       // DD操作
   } catch (const DDException& e) {
       std::cerr << "DD error: " << e.what() << std::endl;
   }

DDMemoryException
-----------------

メモリ割り当て失敗を示す例外クラスです。

.. doxygenclass:: sbdd2::DDMemoryException
   :members:
   :undoc-members:

**継承元**: ``std::bad_alloc``

**発生条件**:

* ノードテーブルの拡張に失敗した場合
* キャッシュの割り当てに失敗した場合
* 大規模なDD構築でメモリが不足した場合

**使用例**:

.. code-block:: cpp

   try {
       ZDD large = build_large_zdd();
   } catch (const DDMemoryException& e) {
       std::cerr << "Out of memory: " << e.what() << std::endl;
       // メモリを解放してリトライするか、縮小版を構築
   }

DDArgumentException
-------------------

不正な引数を示す例外クラスです。

.. doxygenclass:: sbdd2::DDArgumentException
   :members:
   :undoc-members:

**継承元**: ``std::invalid_argument``

**発生条件**:

* 存在しない変数番号を指定した場合
* 範囲外のインデックスを指定した場合
* 不正なパラメータを渡した場合
* ARITY が範囲外（1〜100）の場合

**使用例**:

.. code-block:: cpp

   try {
       BDD x = mgr.var_bdd(1000);  // 変数1000が存在しない場合
   } catch (const DDArgumentException& e) {
       std::cerr << "Invalid argument: " << e.what() << std::endl;
   }

DDIOException
-------------

I/Oエラーを示す例外クラスです。

.. doxygenclass:: sbdd2::DDIOException
   :members:
   :undoc-members:

**継承元**: ``std::ios_base::failure``

**発生条件**:

* ファイルのオープンに失敗した場合
* ファイルの読み込みに失敗した場合
* ファイルの書き込みに失敗した場合
* 不正なファイル形式を読み込もうとした場合

**使用例**:

.. code-block:: cpp

   try {
       ZDD zdd = import_zdd(mgr, "nonexistent.zdd");
   } catch (const DDIOException& e) {
       std::cerr << "I/O error: " << e.what() << std::endl;
   }

DDIncompatibleException
-----------------------

非互換なDD型またはマネージャーを示す例外クラスです。

.. doxygenclass:: sbdd2::DDIncompatibleException
   :members:
   :undoc-members:

**継承元**: ``DDException``

**発生条件**:

* 異なる ``DDManager`` に属するDD同士で演算を行おうとした場合
* 異なるArity（k値）のMVDD同士で演算を行おうとした場合
* BDDとZDDを直接演算しようとした場合（明示的な変換が必要）

**使用例**:

.. code-block:: cpp

   DDManager mgr1, mgr2;
   mgr1.new_var();
   mgr2.new_var();

   BDD x1 = mgr1.var_bdd(1);
   BDD x2 = mgr2.var_bdd(1);  // 異なるマネージャー

   try {
       BDD result = x1 & x2;  // 異なるマネージャー間の演算
   } catch (const DDIncompatibleException& e) {
       std::cerr << "Incompatible: " << e.what() << std::endl;
   }

例外階層
--------

.. code-block:: text

   std::exception
   ├── std::runtime_error
   │   └── DDException
   │       └── DDIncompatibleException
   ├── std::bad_alloc
   │   └── DDMemoryException
   ├── std::invalid_argument
   │   └── DDArgumentException
   └── std::ios_base::failure
       └── DDIOException

例外処理のベストプラクティス
----------------------------

.. code-block:: cpp

   #include <sbdd2/sbdd2.hpp>
   #include <sbdd2/exception.hpp>

   void process_dd() {
       try {
           DDManager mgr;
           // DD操作...
       }
       catch (const DDMemoryException& e) {
           // メモリ不足: リソースを解放してリトライ
           std::cerr << "Memory exhausted: " << e.what() << std::endl;
       }
       catch (const DDArgumentException& e) {
           // プログラムのバグ: ログして終了
           std::cerr << "Bug detected: " << e.what() << std::endl;
           throw;
       }
       catch (const DDIOException& e) {
           // I/Oエラー: ユーザーに通知
           std::cerr << "File error: " << e.what() << std::endl;
       }
       catch (const DDIncompatibleException& e) {
           // 非互換エラー: プログラムのバグ
           std::cerr << "Incompatible operation: " << e.what() << std::endl;
           throw;
       }
       catch (const DDException& e) {
           // その他のDDエラー
           std::cerr << "DD error: " << e.what() << std::endl;
       }
   }
