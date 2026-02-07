.. _bdd_guide:

BDDガイド
=========

この章では、SAPPOROBDD 2.0のBDD（二分決定図）クラスの使い方を解説します。

.. contents:: 目次
   :local:
   :depth: 2

BDDクラスの概要
---------------

BDDはブール関数 :math:`f: \{0, 1\}^n \to \{0, 1\}` を効率的に表現するデータ構造です。
SAPPOROBDD 2.0の ``BDD`` クラスは **縮約BDD（ROBDD: Reduced Ordered BDD）** を実装しており、
以下の縮約規則が適用されています。

1. **ノード共有**: 等価なサブグラフは同一ノードとして共有される
2. **冗長ノードの除去**: 0枝と1枝が同じノードを指す場合、そのノードは除去される

これらの規則により、変数順序が固定された場合に表現が一意になります。

.. code-block:: cpp

   #include <sbdd2/sbdd2.hpp>
   using namespace sbdd2;

BDDの作成
---------

DDManagerと変数
~~~~~~~~~~~~~~~

BDD操作はすべて ``DDManager`` を通じて行います。
BDDを作成する前に、使用する変数をマネージャーに登録する必要があります。

.. code-block:: cpp

   // DDManagerを作成
   DDManager mgr;

   // サイズを指定して作成（大規模問題用）
   DDManager mgr2(1 << 22, 1 << 20);  // 4Mノード、1Mキャッシュ

   // 変数を3つ作成（変数番号 1, 2, 3）
   mgr.new_var();
   mgr.new_var();
   mgr.new_var();

   // 変数数を確認
   std::cout << "変数数: " << mgr.var_count() << std::endl;  // 3

変数BDDの取得
~~~~~~~~~~~~~

``mgr.var_bdd(v)`` または ``BDD::var(mgr, v)`` で変数 :math:`x_v` を表すBDDを取得します。

.. code-block:: cpp

   BDD x1 = mgr.var_bdd(1);       // x1
   BDD x2 = BDD::var(mgr, 2);     // x2（同等の方法）

定数の作成
~~~~~~~~~~

.. code-block:: cpp

   BDD bdd_false = BDD::zero(mgr);   // 定数0（偽）
   BDD bdd_true = BDD::one(mgr);     // 定数1（真）

ブール演算
----------

BDDクラスは、C++の演算子オーバーロードによりブール演算を直感的に記述できます。

.. list-table:: BDDのブール演算子
   :header-rows: 1
   :widths: 20 20 30 30

   * - 演算
     - 演算子
     - 記法
     - 説明
   * - AND
     - ``&``
     - ``f & g``
     - 論理積 :math:`f \wedge g`
   * - OR
     - ``|``
     - ``f | g``
     - 論理和 :math:`f \vee g`
   * - XOR
     - ``^``
     - ``f ^ g``
     - 排他的論理和 :math:`f \oplus g`
   * - NOT
     - ``~``
     - ``~f``
     - 否定 :math:`\neg f`
   * - DIFF
     - ``-``
     - ``f - g``
     - 差 :math:`f \wedge \neg g`

.. code-block:: cpp

   BDD x1 = mgr.var_bdd(1);
   BDD x2 = mgr.var_bdd(2);
   BDD x3 = mgr.var_bdd(3);

   // 基本演算
   BDD f_and = x1 & x2;         // x1 AND x2
   BDD f_or  = x1 | x2;         // x1 OR x2
   BDD f_xor = x1 ^ x2;         // x1 XOR x2
   BDD f_not = ~x1;              // NOT x1
   BDD f_diff = x1 - x2;        // x1 AND (NOT x2)

   // 複合代入
   BDD f = x1;
   f &= x2;                      // f = f AND x2
   f |= x3;                      // f = f OR x3

ITE演算
~~~~~~~

``f.ite(t, e)`` は ``f ? t : e`` を計算します。
すべてのブール演算はITEで表現可能です。

.. code-block:: cpp

   // f ? g : h  （fが真ならg、偽ならh）
   BDD result = x1.ite(x2, x3);  // x1 ? x2 : x3

   // AND は ITE で表現可能: f & g == f.ite(g, zero)
   BDD f_and = x1.ite(x2, BDD::zero(mgr));

   // OR は ITE で表現可能: f | g == f.ite(one, g)
   BDD f_or = x1.ite(BDD::one(mgr), x2);

余因子とShannon展開
--------------------

余因子はBDDの基本操作であり、変数に特定の値を代入した結果を表します。

.. code-block:: cpp

   BDD f = (x1 & x2) | x3;

   // 0-余因子: x1=0 を代入
   BDD f_x1_0 = f.at0(1);  // f|_{x1=0} = x3

   // 1-余因子: x1=1 を代入
   BDD f_x1_1 = f.at1(1);  // f|_{x1=1} = x2 | x3

Shannon展開の関係式:

.. math::

   f = (x_v \wedge f|_{x_v=1}) \vee (\neg x_v \wedge f|_{x_v=0})

子ノードアクセス
~~~~~~~~~~~~~~~~

``low()`` と ``high()`` はトップノードの0枝・1枝の子を直接取得します。

.. code-block:: cpp

   BDD f = x1 & x2;

   // トップ変数の0枝（low）と1枝（high）
   BDD f_low = f.low();    // トップ変数が0の場合の子
   BDD f_high = f.high();  // トップ変数が1の場合の子

.. note::

   ``at0(v)``/``at1(v)`` は任意の変数に対して使用できますが、
   ``low()``/``high()`` はトップ変数に対する操作です。

量化演算
--------

存在量化
~~~~~~~~

.. code-block:: cpp

   BDD f = x1 & x2;

   // 存在量化: ∃x1. f = f|_{x1=0} ∨ f|_{x1=1}
   BDD f_exist = f.exist(1);   // = x2

全称量化
~~~~~~~~

.. code-block:: cpp

   // 全称量化: ∀x1. f = f|_{x1=0} ∧ f|_{x1=1}
   BDD f_forall = f.forall(1);  // = zero (∀x1. x1 ∧ x2 = 0)

複数変数の量化
~~~~~~~~~~~~~~

.. code-block:: cpp

   BDD f = x1 & x2 & x3;

   // 複数変数をまとめて量化
   std::vector<bddvar> vars = {1, 2};
   BDD f_exist_multi = f.exist(vars);    // ∃x1 ∃x2. (x1 ∧ x2 ∧ x3) = x3
   BDD f_forall_multi = f.forall(vars);  // ∀x1 ∀x2. (x1 ∧ x2 ∧ x3) = 0

カウントとSAT
--------------

充足割当数
~~~~~~~~~~

.. code-block:: cpp

   BDD f = x1 | x2;

   // card(): 全変数に対する充足割当数
   double c = f.card();
   std::cout << "充足割当数: " << c << std::endl;

   // count(max_var): 指定変数数に対する充足割当数
   double c2 = f.count(3);  // 変数3つ中の充足割当数
   std::cout << "充足割当数 (3変数): " << c2 << std::endl;

充足割当の取得
~~~~~~~~~~~~~~

.. code-block:: cpp

   // one_sat(): 充足割当を1つ取得
   std::vector<int> sat = f.one_sat();
   // sat[v] = 0(false), 1(true), -1(don't care)
   for (size_t i = 1; i < sat.size(); ++i) {
       if (sat[i] >= 0) {
           std::cout << "x" << i << " = " << sat[i] << std::endl;
       }
   }

厳密カウント（GMP）
~~~~~~~~~~~~~~~~~~~~

2^53 を超える大きな値でも正確にカウントできます。

.. code-block:: cpp

   #ifdef SBDD2_HAS_GMP
   // exact_count(): GMP を使った厳密カウント
   std::string exact = f.exact_count();
   std::cout << "厳密カウント: " << exact << std::endl;
   #endif

ノード情報
----------

.. code-block:: cpp

   BDD f = (x1 & x2) | x3;

   // トップ変数番号
   bddvar v = f.top();
   std::cout << "トップ変数: " << v << std::endl;

   // ノード数
   std::size_t s = f.size();
   std::cout << "ノード数: " << s << std::endl;

   // サポート（BDD内に現れる変数の集合）
   std::vector<bddvar> sup = f.support();
   std::cout << "サポート: ";
   for (bddvar sv : sup) {
       std::cout << sv << " ";
   }
   std::cout << std::endl;

   // 終端判定
   std::cout << "終端か: " << f.is_terminal() << std::endl;
   std::cout << "0終端か: " << f.is_zero() << std::endl;
   std::cout << "1終端か: " << f.is_one() << std::endl;

変数操作
--------

合成（compose）
~~~~~~~~~~~~~~~

変数 :math:`x_v` を別のBDD :math:`g` で置き換えます。

.. code-block:: cpp

   BDD f = x1 & x2;

   // x1 を x2 | x3 で置き換え
   BDD g = x2 | x3;
   BDD f_composed = f.compose(1, g);
   // f[x1 := x2 | x3] = (x2 | x3) & x2 = x2

制限（restrict）
~~~~~~~~~~~~~~~~

変数に定数値を代入します。余因子と同等です。

.. code-block:: cpp

   BDD f = x1 & x2;

   // x1 = true に制限（1-余因子と同じ）
   BDD f_restricted = f.restrict(1, true);   // = x2

   // x1 = false に制限（0-余因子と同じ）
   BDD f_restricted2 = f.restrict(1, false);  // = 0

I/Oと可視化
------------

BDDの入出力と可視化については :doc:`io_guide` を参照してください。

.. code-block:: cpp

   // 文字列表現
   std::cout << f.to_string() << std::endl;

   // 標準出力に出力
   f.print();

   // ZDDへの変換
   ZDD z = f.to_zdd();

否定枝表現について
------------------

SAPPOROBDD 2.0のBDDは **否定枝表現（Complemented Edges）** を採用しています。
これにより、以下の特徴があります。

O(1)のNOT演算
~~~~~~~~~~~~~~

NOT演算はアーク上の否定ビットを反転するだけで完了するため、
新しいノードを一切作成せずに O(1) で実行できます。

.. code-block:: cpp

   BDD f = x1 & x2 & x3;
   BDD not_f = ~f;  // O(1) で完了

   // サイズは同じ（新しいノードを作成しない）
   assert(f.size() == not_f.size());

等価性判定
~~~~~~~~~~

2つのBDDが等価かどうかは、アーク（インデックス+否定ビット）の比較で判定されます。
ノードのトラバースは不要で、 O(1) です。

.. code-block:: cpp

   BDD f = x1 & x2;
   BDD g = x1 & x2;

   // O(1) で等価性を判定
   if (f == g) {
       std::cout << "等価です" << std::endl;
   }

.. note::

   否定枝表現のため、``f`` と ``~f`` は内部的に同一ノードを共有し、
   否定ビットだけが異なります。

よくある間違い
--------------

変数の作成忘れ
~~~~~~~~~~~~~~

変数を作成せずにBDDを操作しようとすると、エラーが発生します。

.. code-block:: cpp

   DDManager mgr;
   // mgr.new_var(); を忘れている！

   // エラー: 変数1はまだ存在しない
   // BDD x1 = mgr.var_bdd(1);  // 例外がスローされる

   // 正しい手順
   mgr.new_var();                // 変数1を作成
   BDD x1 = mgr.var_bdd(1);     // OK

異なるマネージャーのBDDを混在させる
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

異なる ``DDManager`` のBDD同士を演算することはできません。

.. code-block:: cpp

   DDManager mgr1;
   DDManager mgr2;
   mgr1.new_var();
   mgr2.new_var();

   BDD f = mgr1.var_bdd(1);
   BDD g = mgr2.var_bdd(1);

   // エラー: 異なるマネージャーのBDDを演算
   // BDD h = f & g;  // 未定義動作！

   // 正しくは同じマネージャーを使う
   BDD x1 = mgr1.var_bdd(1);
   mgr1.new_var();
   BDD x2 = mgr1.var_bdd(2);
   BDD h = x1 & x2;  // OK

変数番号とレベルの混同
~~~~~~~~~~~~~~~~~~~~~~

デフォルトでは変数番号とレベルは一致しますが、``new_var_of_lev()`` で
変数を挿入した場合は異なる場合があります。
APIに渡す値が変数番号かレベルかを常に意識してください。

.. code-block:: cpp

   DDManager mgr;
   mgr.new_var();  // v1: レベル1
   mgr.new_var();  // v2: レベル2

   // var_bdd() には変数番号を渡す
   BDD x1 = mgr.var_bdd(1);   // 変数番号1のBDD

   // at0(), at1(), exist(), forall() なども変数番号を使用
   BDD f = x1.at0(1);          // 変数番号1で余因子

実践例: 多数決関数
------------------

3変数の多数決関数（2つ以上が真なら真）をBDDで表現する例です。

.. code-block:: cpp

   #include <sbdd2/sbdd2.hpp>
   using namespace sbdd2;

   int main() {
       DDManager mgr;
       mgr.new_var();
       mgr.new_var();
       mgr.new_var();

       BDD x1 = mgr.var_bdd(1);
       BDD x2 = mgr.var_bdd(2);
       BDD x3 = mgr.var_bdd(3);

       // 多数決関数: (x1 & x2) | (x2 & x3) | (x1 & x3)
       BDD majority = (x1 & x2) | (x2 & x3) | (x1 & x3);

       std::cout << "ノード数: " << majority.size() << std::endl;
       std::cout << "充足割当数: " << majority.count(3) << std::endl;  // 4

       // 量化の例: ∃x1. majority
       BDD exist_x1 = majority.exist(1);
       std::cout << "∃x1 充足数: " << exist_x1.count(3) << std::endl;

       return 0;
   }
