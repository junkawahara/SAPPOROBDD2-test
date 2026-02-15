サンプルプログラム
==================

このセクションでは、SAPPOROBDD 2.0を使用した実践的なサンプルプログラムを
詳しく解説します。各サンプルは ``examples/`` ディレクトリに収録されており、
問題の定義からBDD/ZDDによるモデリング、コードのウォークスルー、
実行結果までをステップバイステップで説明します。

サンプルプログラムは `bdd-benchmark <https://github.com/ssoelvsten/bdd-benchmark>`_
（MIT License、Steffan Soelvsten氏）をベースにしています。

前提知識
--------

各サンプルを理解するために、以下のドキュメントを事前にお読みください：

* :doc:`../quickstart` -- 基本的な使い方
* :doc:`../tutorial` -- BDD/ZDDの基本操作
* :doc:`../guide/index` -- 各機能の詳細ガイド

.. toctree::
   :maxdepth: 2
   :caption: サンプル一覧

   nqueens
   hamiltonian
   cnf
   tictactoe
   gameoflife
   apply
   relprod

ビルド方法
----------

サンプルプログラムは CMake でビルドできます。

.. code-block:: bash

   cd build
   cmake ..
   make examples

個別にビルドする場合：

.. code-block:: bash

   make nqueens
   make hamiltonian
   make cnf
   make tictactoe
   make gameoflife
   make apply
   make relprod
