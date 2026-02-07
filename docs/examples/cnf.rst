CNF SAT問題
===========

このサンプルでは、BDDを用いてCNF（連言標準形）のブール充足可能性問題（SAT）を解きます。
ソースコード: ``examples/cnf.cpp``

問題の定義
----------

ブール充足可能性問題（SAT）は、与えられたブール論理式を真にする変数割当が
存在するかどうかを判定する問題です。CNF（Conjunctive Normal Form、連言標準形）は
論理式の標準的な表現形式で、以下のように構成されます：

* **リテラル**: 変数またはその否定（例: :math:`x_1`, :math:`\neg x_2`）
* **節（clause）**: リテラルの論理和（例: :math:`x_1 \vee \neg x_2 \vee x_3`）
* **CNF式**: 節の論理積（例: :math:`(x_1 \vee x_2) \wedge (\neg x_1 \vee x_3)`）

入力はDIMACS CNF形式で与えます。

DIMACS CNF形式
~~~~~~~~~~~~~~

.. code-block:: text

   c これはコメント行
   p cnf 3 4
   1 2 0
   -1 3 0
   2 -3 0
   1 -2 3 0

* ``c`` で始まる行はコメント
* ``p cnf <変数数> <節数>`` は問題行
* 各節はスペース区切りのリテラルで、``0`` で終端
* 正のリテラル ``n`` は変数 n が真、負のリテラル ``-n`` は変数 n が偽を表す

BDDによるモデリング
-------------------

CNF式をBDDに変換するアプローチは以下の通りです：

1. 各節を個別のBDDに変換する（リテラルの論理和）
2. 全ての節BDDの論理積を取る

.. math::

   \text{clause}_i &= \bigvee_{l \in C_i} \text{literal}(l)

   \text{CNF} &= \bigwedge_{i=1}^{m} \text{clause}_i

ここで、リテラル :math:`l` が正なら対応する変数のBDD、
負なら対応する変数のBDDの否定となります。

コードのウォークスルー
----------------------

DIMACSパーサ
~~~~~~~~~~~~

``parse_dimacs()`` 関数は、DIMACS CNF形式のファイルを読み込み、
``CNF`` 構造体に格納します。

.. code-block:: cpp

   struct Clause {
       std::vector<int> literals;  // 正 = 変数、負 = 否定変数
   };

   struct CNF {
       int num_vars;
       int num_clauses;
       std::vector<Clause> clauses;
   };

   CNF parse_dimacs(const std::string& filename) {
       CNF cnf;
       std::ifstream file(filename);
       std::string line;
       bool header_found = false;

       while (std::getline(file, line)) {
           if (line.empty()) continue;
           if (line[0] == 'c') continue;       // コメント

           if (line[0] == 'p') {
               // 問題行: p cnf <vars> <clauses>
               std::istringstream iss(line);
               std::string p, cnf_str;
               iss >> p >> cnf_str >> cnf.num_vars >> cnf.num_clauses;
               header_found = true;
               continue;
           }

           // 節の読み込み
           Clause clause;
           std::istringstream iss(line);
           int lit;
           while (iss >> lit) {
               if (lit == 0) break;  // 節の終端
               clause.literals.push_back(lit);
           }
           if (!clause.literals.empty()) {
               cnf.clauses.push_back(clause);
           }
       }
       return cnf;
   }

clause_to_bdd: 節のBDD変換
~~~~~~~~~~~~~~~~~~~~~~~~~~~

``clause_to_bdd()`` は、1つの節（リテラルの論理和）をBDDに変換します。

.. code-block:: cpp

   BDD clause_to_bdd(DDManager& mgr, const Clause& clause) {
       BDD result = mgr.bdd_zero();

       for (int lit : clause.literals) {
           int var = std::abs(lit);
           BDD var_bdd = mgr.var_bdd(var);

           if (lit > 0) {
               result = result | var_bdd;       // 正リテラル
           } else {
               result = result | (~var_bdd);    // 負リテラル
           }
       }
       return result;
   }

各リテラルに対応するBDDを作成し、OR演算で結合します。
否定リテラルには ``~`` 演算子を使用します（否定枝表現によりO(1)で実行）。

cnf_to_bdd_balanced: バランス木結合
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

``cnf_to_bdd_balanced()`` は、全節のBDDをバランス二分木の構造で
AND結合します。

.. code-block:: cpp

   BDD cnf_to_bdd_balanced(DDManager& mgr, const CNF& cnf) {
       // 全節をBDDに変換
       std::vector<BDD> clause_bdds;
       for (const Clause& clause : cnf.clauses) {
           clause_bdds.push_back(clause_to_bdd(mgr, clause));
       }

       // バランス木結合
       while (clause_bdds.size() > 1) {
           std::vector<BDD> next_level;

           for (size_t i = 0; i + 1 < clause_bdds.size(); i += 2) {
               next_level.push_back(clause_bdds[i] & clause_bdds[i + 1]);
           }

           // 奇数個の場合、最後の要素をそのまま次のレベルへ
           if (clause_bdds.size() % 2 == 1) {
               next_level.push_back(clause_bdds.back());
           }

           clause_bdds = std::move(next_level);
       }

       return clause_bdds[0];
   }

**バランス木結合が有効な理由：**

単純な左から右への結合（線形結合）では、BDDサイズが中間段階で極端に
大きくなることがあります。バランス木結合では、各段階で結合するBDDのサイズが
比較的均等になるため、中間BDDサイズの爆発を抑えることができます。

.. code-block:: text

   線形結合:   ((((c1 & c2) & c3) & c4) & c5)
   バランス木: ((c1 & c2) & (c3 & c4)) & c5

結果の確認と解の抽出
~~~~~~~~~~~~~~~~~~~~

BDD構築後、充足可能性の判定と解の抽出を行います。

.. code-block:: cpp

   // 充足可能性の判定
   bool is_sat = !result.is_zero();

   // 充足割当数のカウント（オプション）
   if (do_satcount && is_sat) {
       double count_d = result.count(cnf.num_vars);
       uint64_t count = static_cast<uint64_t>(count_d);
   }

   // 充足割当の1つを取得
   if (is_sat) {
       std::vector<int> solution = result.one_sat();
       for (int i = 1; i <= cnf.num_vars; i++) {
           if (solution[i] == 1) {
               std::cout << i << " ";       // 変数 i は真
           } else if (solution[i] == 0) {
               std::cout << "-" << i << " "; // 変数 i は偽
           }
           // solution[i] == -1 はドントケア
       }
   }

* ``is_zero()``: BDDが定数0（充足不能）かどうかを判定
* ``count(n)``: n 変数での充足割当数を返す
* ``one_sat()``: 充足割当を1つ返す（ドントケアを含む場合あり）

実行例と結果
------------

.. code-block:: bash

   # 充足可能性の判定
   ./cnf -f input.cnf

   # 充足割当数もカウント
   ./cnf -f input.cnf -c

以下の小さなCNFファイルを例とします：

.. code-block:: text
   :caption: example.cnf

   c 3変数、4節のCNF
   p cnf 3 4
   1 2 0
   -1 3 0
   2 -3 0
   1 -2 3 0

.. code-block:: text
   :caption: 実行結果

   ========================================
   CNF SAT Solver using BDD
   ========================================

   Parsing: example.cnf
     Variables: 3
     Clauses: 4

   Creating 3 variables...
   Building BDD...
     Tree level done, 2 BDDs remaining, nodes: ...
     Tree level done, 1 BDDs remaining, nodes: ...
     Build time: 0 ms
     BDD nodes: ...

   ========================================
   Result: SATISFIABLE

   Counting satisfying assignments...
     Satisfying assignments: 3

   One satisfying assignment:
     1 2 3
   ========================================

この例では、3変数4節のCNF式が充足可能で、3つの充足割当が存在します。

改善の可能性
------------

節の順序最適化
~~~~~~~~~~~~~~

節のAND結合の順序は、最終的なBDDサイズに影響しません（結果は同一）が、
中間BDDのサイズには大きく影響します。
ヒューリスティックとして、以下の戦略が有効です：

* 短い節（リテラル数が少ない節）を先に結合する
* 共通変数を持つ節を近くに配置する
* クラスタリングに基づく結合順序

動的変数順序
~~~~~~~~~~~~

変数順序はBDDサイズに指数関数的な影響を与えるため、
動的変数リオーダリングを適用することで大幅な改善が期待できます。
現在のSAPPOROBDD 2.0では静的な変数順序を使用していますが、
将来のバージョンで動的リオーダリングがサポートされる予定です。

BDD以外のアプローチとの比較
~~~~~~~~~~~~~~~~~~~~~~~~~~~

BDDベースのSATソルバーは、全ての充足割当を表現できるという利点がありますが、
変数数が多い場合はBDDサイズが爆発する可能性があります。

* **DPLL/CDCLベースSATソルバー**: 充足可能性の判定のみ（カウント不可）だが高速
* **BDDベースソルバー**: カウント、列挙が可能だがメモリ消費が大きい
* **知識コンパイル**: d-DNNFなどの中間的なアプローチ

厳密カウント
~~~~~~~~~~~~

充足割当数が :math:`2^{53}` を超える場合は、``exact_count()`` を使用してください。
GMPまたはBigIntライブラリが利用可能な場合に使用できます。

.. code-block:: cpp

   std::string exact = result.exact_count(cnf.num_vars);

.. seealso::

   * :doc:`../tutorial` -- BDD基本操作のチュートリアル
   * :doc:`../guide/index` -- BDD操作の詳細ガイド
   * :doc:`../application/index` -- SAT/制約充足の応用ガイド
