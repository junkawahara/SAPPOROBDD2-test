N-Queens問題
============

このサンプルでは、BDDを用いてN-Queens問題（Nクイーン問題）を解きます。
ソースコード: ``examples/nqueens.cpp``

問題の定義
----------

N-Queens問題は、N x N のチェス盤上に N 個のクイーンを、
互いに攻撃し合わないように配置する方法の数を求める問題です。

クイーンは以下の方向に任意の距離だけ移動できます：

* 水平方向（同じ行）
* 垂直方向（同じ列）
* 対角方向（斜め）

したがって、有効な配置では以下の制約を満たす必要があります：

1. 各行にちょうど1つのクイーンが存在する
2. 各列にちょうど1つのクイーンが存在する
3. 同じ対角線上に2つ以上のクイーンが存在しない

BDDによるモデリング
-------------------

変数の定義
~~~~~~~~~~

N x N のチェス盤の各マスにブール変数 :math:`x_{i,j}` を割り当てます
（:math:`0 \leq i, j \leq N-1`）。

* :math:`x_{i,j} = 1`: マス (i, j) にクイーンが配置されている
* :math:`x_{i,j} = 0`: マス (i, j) にクイーンが配置されていない

合計 :math:`N^2` 個のブール変数を使用します。変数番号は行優先で
1 から :math:`N^2` まで割り当てます：

.. code-block:: text

   変数番号 = N * row + col + 1

例えば 8x8 盤の場合、変数1が (0,0)、変数2が (0,1)、...、変数64が (7,7) に対応します。

制約のBDD表現
~~~~~~~~~~~~~

制約は以下の3段階で構築します：

1. **セル制約** :math:`S(i,j)`: マス (i,j) にクイーンを置いた場合の制約。
   マス (i,j) の変数が真であり、かつ同行・同列・同対角上の全てのマスの変数が偽であるBDD。

2. **行制約** :math:`R(r)`: 行 r のいずれかのマスにクイーンが置かれている制約。
   行 r 内の全マスのセル制約の論理和（OR）。

3. **盤面制約** :math:`B`: 全行の制約を満たすBDD。
   全行の行制約の論理積（AND）。

.. math::

   S(i,j) &= x_{i,j} \wedge \bigwedge_{\substack{(r,c) \neq (i,j) \\ \text{conflict}(r,c,i,j)}} \neg x_{r,c}

   R(r) &= \bigvee_{c=0}^{N-1} S(r, c)

   B &= \bigwedge_{r=0}^{N-1} R(r)

コードのウォークスルー
----------------------

変数の作成
~~~~~~~~~~

メイン関数では、まず DDManager を作成し、:math:`N^2` 個の変数を登録します。

.. code-block:: cpp

   DDManager mgr;

   // N*N 個の変数を作成
   for (int i = 0; i < N * N; i++) {
       mgr.new_var();
   }

変数番号の変換ヘルパーは以下のように定義されています：

.. code-block:: cpp

   // (row, col) -> 変数番号（1-indexed）
   inline int label_of_position(int r, int c) {
       return (rows() * r) + c + 1;
   }

queens_S: セル制約の構築
~~~~~~~~~~~~~~~~~~~~~~~~

``queens_S(mgr, i, j)`` は、マス (i, j) にクイーンを配置したときの制約BDDを構築します。

.. code-block:: cpp

   BDD queens_S(DDManager& mgr, int i, int j) {
       BDD result = mgr.bdd_one();

       for (int row = MAX_ROW(); row >= 0; row--) {
           for (int col = MAX_COL(); col >= 0; col--) {
               int var = label_of_position(row, col);
               BDD x = mgr.var_bdd(var);

               if (row == i && col == j) {
                   // このマスにクイーンを配置
                   result = result & x;
                   continue;
               }

               int row_diff = std::abs(row - i);
               int col_diff = std::abs(col - j);

               bool same_row = (row == i);
               bool same_col = (col == j);
               bool same_diag = (row_diff == col_diff);

               if (same_row || same_col || same_diag) {
                   // 競合するマス: 変数を偽に制約
                   result = result & (~x);
               }
           }
       }
       return result;
   }

この関数の動作：

1. 全マスを走査する（変数番号の降順）
2. マス (i,j) 自体は変数を真に制約（ ``result & x``）
3. 同行・同列・同対角にあるマスは変数を偽に制約（ ``result & (~x)``）
4. 競合しないマスには制約を加えない

queens_R: 行制約の構築
~~~~~~~~~~~~~~~~~~~~~~

``queens_R(mgr, r)`` は、行 r の各列にクイーンを置くセル制約の論理和を構築します。

.. code-block:: cpp

   BDD queens_R(DDManager& mgr, int r) {
       BDD result = queens_S(mgr, r, 0);

       for (int c = 1; c < cols(); c++) {
           result = result | queens_S(mgr, r, c);
       }
       return result;
   }

これは「行 r のいずれかのマスにクイーンが配置されている」ことを表します。
OR演算により、各列の配置のうちいずれかが成立すれば真となります。

queens_B: 盤面全体の制約
~~~~~~~~~~~~~~~~~~~~~~~~

``queens_B(mgr)`` は、全行の行制約の論理積を構築します。

.. code-block:: cpp

   BDD queens_B(DDManager& mgr) {
       if (rows() == 1 && cols() == 1) {
           return queens_S(mgr, 0, 0);
       }

       BDD result = queens_R(mgr, 0);

       for (int r = 1; r < rows(); r++) {
           result = result & queens_R(mgr, r);

           std::cout << "  Row " << row_to_string(r) << " done, "
                     << "nodes: " << mgr.alive_count() << std::endl;
       }
       return result;
   }

各行を順番に AND で結合し、全ての行制約を同時に満たすBDDを構築します。
構築過程で生存ノード数を出力することで、BDDサイズの推移を確認できます。

解の数え上げ
~~~~~~~~~~~~

最終的なBDDが構築されたら、``count()`` で解の数を求めます。

.. code-block:: cpp

   BDD result = queens_B(mgr);

   // N*N 個の変数に対する充足割当数を数える
   double solutions_d = result.count(N * N);
   uint64_t solutions = static_cast<uint64_t>(solutions_d);

``count(N * N)`` は、:math:`N^2` 個の変数に対する充足割当の数を返します。
これがN-Queens問題の解の数となります。

実行例と結果
------------

.. code-block:: bash

   # 8x8 盤（デフォルト）
   ./nqueens

   # 任意のサイズを指定
   ./nqueens -n 10
   ./nqueens -n 12

代表的な結果を以下に示します：

.. list-table:: N-Queens問題の解の数
   :header-rows: 1
   :widths: 15 25

   * - N
     - 解の数
   * - 4
     - 2
   * - 5
     - 10
   * - 6
     - 4
   * - 7
     - 40
   * - 8
     - 92
   * - 9
     - 352
   * - 10
     - 724
   * - 11
     - 2,680
   * - 12
     - 14,200
   * - 13
     - 73,712
   * - 14
     - 365,596

.. note::

   N が大きくなるにつれてBDDのノード数が急激に増加します。
   N = 14 程度が現実的な限界となることが多いです。

改善の可能性
------------

変数順序の最適化
~~~~~~~~~~~~~~~~

現在の実装では行優先の自然な変数順序を使用していますが、
変数順序を工夫することでBDDサイズを大幅に削減できる可能性があります。

例えば、対角線を考慮した変数順序や、問題の対称性を利用した順序付けが考えられます。

インクリメンタル構築
~~~~~~~~~~~~~~~~~~~~

現在は全行の制約を一度に AND で結合していますが、
構築途中で中間結果を簡約化（例えば存在量化による変数の除去）することで、
中間BDDサイズを抑制できる可能性があります。

TdZdd互換Specの活用
~~~~~~~~~~~~~~~~~~~~

:doc:`../advanced` で解説している ``VarArityHybridDdSpec`` を使用すると、
各行のクイーン位置を多値変数で表現でき、より自然なモデリングが可能です。
変数数が :math:`N^2` 個から N 個に削減されるため、効率的な場合があります。

GMP厳密カウント
~~~~~~~~~~~~~~~

N が大きい場合（N >= 16 程度）、解の数が ``double`` の精度（:math:`2^{53}`）を
超えるため、``exact_count()`` を使用して厳密な値を求めることが推奨されます。

.. code-block:: cpp

   // GMP を使用した厳密カウント
   std::string exact = result.exact_count(N * N);
   std::cout << "厳密解: " << exact << std::endl;

.. seealso::

   * :doc:`../tutorial` -- BDD基本操作のチュートリアル
   * :doc:`../advanced` -- VarAritySpecによる多値変数の利用
   * :doc:`../guide/index` -- BDD操作の詳細ガイド
