SAT・制約充足問題
=================

このガイドでは、SAPPOROBDD 2.0を用いてブール充足可能性問題（SAT）や
制約充足問題（CSP）を解く方法を解説します。BDDはブール関数を
コンパクトに表現するデータ構造であり、制約の論理積や量化演算を
効率的に実行できるため、SAT/CSPの解法に適しています。

BDDによるブール充足問題
-----------------------

ブール充足可能性問題（SAT）は、与えられたブール論理式を真にする
変数割り当てが存在するかを判定する問題です。BDDを用いると、
論理式を表現するBDDが定数0でないことを確認するだけで
充足可能性を判定できます。

BDDによるSAT解法の利点は以下の通りです。

* **解の列挙**: BDDは全ての充足割り当てを暗黙的に保持しているため、
  解の数え上げや列挙が容易
* **制約の段階的追加**: 新しい制約をBDDの論理積で追加可能
* **量化によるプロジェクション**: 一部の変数を消去して
  残りの変数に関する制約を得ることが可能

一方、BDDのサイズが変数順序に敏感であることや、
最悪ケースで指数的なサイズになりうることには注意が必要です。

CNFからBDDへの変換パターン
---------------------------

SAT問題は通常、CNF（連言標準形）で与えられます。
CNFは節（clause）の論理積であり、各節はリテラルの論理和です。

基本パターン
~~~~~~~~~~~~

各節をBDDに変換し、それらの論理積を取ります。

.. code-block:: cpp

   #include <sbdd2/sbdd2.hpp>
   using namespace sbdd2;

   int main() {
       DDManager mgr;

       // 変数の作成
       int num_vars = 5;
       for (int i = 0; i < num_vars; ++i) mgr.new_var();

       // CNF: (x1 ∨ ¬x2 ∨ x3) ∧ (¬x1 ∨ x4) ∧ (x2 ∨ ¬x3 ∨ x5)
       // 節をリテラルのリストで表現（正: 正リテラル、負: 負リテラル）
       std::vector<std::vector<int>> cnf = {
           {1, -2, 3},
           {-1, 4},
           {2, -3, 5}
       };

       // 各節をBDDに変換
       std::vector<BDD> clause_bdds;
       for (const auto& clause : cnf) {
           BDD clause_bdd = BDD::zero(mgr);
           for (int lit : clause) {
               if (lit > 0) {
                   clause_bdd |= mgr.var_bdd(lit);
               } else {
                   clause_bdd |= ~mgr.var_bdd(-lit);
               }
           }
           clause_bdds.push_back(clause_bdd);
       }

       // 全ての節の論理積を取る
       BDD result = BDD::one(mgr);
       for (const auto& clause_bdd : clause_bdds) {
           result &= clause_bdd;
       }

       // 充足可能性の判定
       if (result != BDD::zero(mgr)) {
           std::cout << "充足可能" << std::endl;
           std::cout << "充足割当数: " << result.count(num_vars) << std::endl;
       } else {
           std::cout << "充足不能" << std::endl;
       }

       return 0;
   }

節の追加順序の工夫
~~~~~~~~~~~~~~~~~~

BDDのサイズを抑えるためには、節を追加する順序が重要です。
一般的な戦略として以下があります。

* **短い節を先に追加**: 少ないリテラルの節はBDDが小さいため、
  初期の段階でBDDサイズを抑制できる
* **関連する変数を含む節をまとめて追加**: 共通の変数を持つ節を
  近い順序で追加すると、中間BDDのサイズが小さくなる傾向がある

.. code-block:: cpp

   // 節をサイズ順にソート（短い節を先に処理）
   std::sort(cnf.begin(), cnf.end(),
       [](const std::vector<int>& a, const std::vector<int>& b) {
           return a.size() < b.size();
       });

制約充足問題のモデリング（N-Queens等）
---------------------------------------

制約充足問題（CSP）をBDDでモデリングする場合、
各制約をBDDとして表現し、それらの論理積を取ります。
ここではN-Queens問題を例に説明します。

N-Queens問題
~~~~~~~~~~~~

N-Queens問題は、N x Nのチェス盤にN個のクイーンを
互いに攻撃し合わないように配置する問題です。

制約は以下の通りです。

1. **各行にちょうど1つのクイーン**: 行制約
2. **各列にちょうど1つのクイーン**: 列制約
3. **各対角線に高々1つのクイーン**: 対角線制約

.. code-block:: cpp

   #include <sbdd2/sbdd2.hpp>
   using namespace sbdd2;

   int main() {
       int n = 8;  // 8-Queens問題
       DDManager mgr;

       // 変数 x_{i,j}: 行i、列jにクイーンを配置するか
       // 変数番号: (i-1)*n + j  (1 <= i,j <= n)
       for (int i = 0; i < n * n; ++i) mgr.new_var();

       // 変数BDDを取得するヘルパー
       auto var = [&](int row, int col) -> BDD {
           return mgr.var_bdd((row - 1) * n + col);
       };

       BDD result = BDD::one(mgr);

       // 行制約: 各行にちょうど1つ
       for (int i = 1; i <= n; ++i) {
           // 行iの少なくとも1つが真
           BDD at_least_one = BDD::zero(mgr);
           for (int j = 1; j <= n; ++j) {
               at_least_one |= var(i, j);
           }
           result &= at_least_one;

           // 行iの高々1つが真（排他制約）
           for (int j1 = 1; j1 <= n; ++j1) {
               for (int j2 = j1 + 1; j2 <= n; ++j2) {
                   result &= ~(var(i, j1) & var(i, j2));
               }
           }
       }

       // 列制約: 各列に高々1つ
       for (int j = 1; j <= n; ++j) {
           for (int i1 = 1; i1 <= n; ++i1) {
               for (int i2 = i1 + 1; i2 <= n; ++i2) {
                   result &= ~(var(i1, j) & var(i2, j));
               }
           }
       }

       // 対角線制約
       for (int i1 = 1; i1 <= n; ++i1) {
           for (int j1 = 1; j1 <= n; ++j1) {
               for (int i2 = i1 + 1; i2 <= n; ++i2) {
                   int j2 = j1 + (i2 - i1);  // 右下方向
                   if (j2 >= 1 && j2 <= n) {
                       result &= ~(var(i1, j1) & var(i2, j2));
                   }
                   j2 = j1 - (i2 - i1);  // 左下方向
                   if (j2 >= 1 && j2 <= n) {
                       result &= ~(var(i1, j1) & var(i2, j2));
                   }
               }
           }
       }

       std::cout << n << "-Queens 解の数: "
                 << result.count(n * n) << std::endl;

       return 0;
   }

.. note::

   N-Queens問題をBDDで解く場合、変数数は :math:`N^2` となり、
   Nが大きくなるとBDDサイズが急激に増大します。大きなNに対しては、
   :doc:`../advanced` で紹介したVarArity Specを用いた
   多値変数での表現がより効率的です。

量化によるプロジェクション
---------------------------

BDDの量化演算を用いると、一部の変数を消去して
残りの変数に関する制約を得ることができます。
これは、SAT/CSP問題における変数の射影に対応します。

存在量化（Existential Quantification）
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

存在量化 :math:`\exists x_i. f` は、「変数 :math:`x_i` に
適切な値を割り当てることで :math:`f` を真にできる」ような
残りの変数の割り当てを表します。

.. code-block:: cpp

   DDManager mgr;
   for (int i = 0; i < 4; ++i) mgr.new_var();

   BDD x1 = mgr.var_bdd(1);
   BDD x2 = mgr.var_bdd(2);
   BDD x3 = mgr.var_bdd(3);
   BDD x4 = mgr.var_bdd(4);

   // 制約: (x1 ∧ x2) ∨ (x3 ∧ x4)
   BDD f = (x1 & x2) | (x3 & x4);

   // x1, x2を存在量化で消去
   // → x3, x4に関する制約のみ残る
   BDD projected = f.exist(1).exist(2);

   std::cout << "射影後の充足割当数: "
             << projected.count(4) << std::endl;

全称量化（Universal Quantification）
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

全称量化 :math:`\forall x_i. f` は、「変数 :math:`x_i` の
全ての値に対して :math:`f` が真」となるような
残りの変数の割り当てを表します。

.. code-block:: cpp

   // 全称量化: x1の全ての値でfが真となる割り当て
   BDD universal = f.forall(1);

量化の応用例
~~~~~~~~~~~~

量化演算は以下のような場面で活用できます。

* **変数プロジェクション**: 注目する変数だけを残して他を消去
* **パラメータ解析**: パラメータ変数を量化して、
  制約を満たすパラメータ範囲を求める
* **抽象化**: 詳細な変数を消去して概略的な特性を抽出

関連積演算（状態遷移の前方/後方像計算）
-----------------------------------------

関連積（Relational Product）は、BDDで表現された状態遷移関係と
状態集合を組み合わせて、前方像（Forward Image）や
後方像（Backward Image）を計算する手法です。
これは有限状態システムの到達可能性解析に広く使われます。

基本概念
~~~~~~~~

状態遷移システムにおいて、以下の要素をBDDで表現します。

* **状態集合** :math:`S(x)`: 現在の状態をBDD変数 :math:`x` で表現
* **遷移関係** :math:`T(x, x')`: 現状態 :math:`x` と次状態 :math:`x'` の関係をBDDで表現

前方像計算は、「現在の状態集合 :math:`S` から1ステップで到達可能な状態集合」を求めます。

.. code-block:: text

   前方像: Image(S, T) = ∃x. (S(x) ∧ T(x, x'))
   （結果はx'の変数で表現される）

概念的な実装
~~~~~~~~~~~~

.. code-block:: cpp

   DDManager mgr;

   // 現状態変数 x1, x2 と次状態変数 x1', x2'
   // 変数番号: x1=1, x1'=2, x2=3, x2'=4
   for (int i = 0; i < 4; ++i) mgr.new_var();

   BDD x1 = mgr.var_bdd(1);    // 現状態 x1
   BDD x1p = mgr.var_bdd(2);   // 次状態 x1'
   BDD x2 = mgr.var_bdd(3);    // 現状態 x2
   BDD x2p = mgr.var_bdd(4);   // 次状態 x2'

   // 遷移関係: x1' = x1 XOR x2, x2' = x1 AND x2
   BDD trans_x1 = ~(x1p ^ (x1 ^ x2));  // x1' ↔ (x1 XOR x2)
   BDD trans_x2 = ~(x2p ^ (x1 & x2));  // x2' ↔ (x1 AND x2)
   BDD trans = trans_x1 & trans_x2;

   // 初期状態集合: {(x1=0, x2=0)}
   BDD init = ~x1 & ~x2;

   // 前方像計算
   BDD conjoined = init & trans;         // S(x) ∧ T(x, x')
   BDD forward = conjoined.exist(1).exist(3);  // ∃x1, x2 で消去

   // forwardはx1', x2'の変数で次状態集合を表現

.. note::

   実際の到達可能性解析では、前方像計算を不動点に達するまで
   繰り返す必要があります。また、変数の対応関係（renaming）を
   適切に管理する必要があります。変数のインターリーブ
   （:math:`x_1, x_1', x_2, x_2', \ldots` の順序）が
   BDDサイズの効率化に重要です。

到達可能性解析の応用
~~~~~~~~~~~~~~~~~~~~

関連積演算を用いた到達可能性解析は、以下のような問題に応用できます。

* **モデル検査**: ハードウェア/ソフトウェアの性質検証
* **プランニング**: 初期状態から目標状態への到達可能性判定
* **ゲーム解析**: ゲームの勝利戦略の計算

これらの応用では、BDDの変数順序の最適化が
性能に大きく影響します。特に、現状態変数と次状態変数を
交互に配置（インターリーブ）することが一般的に推奨されます。
