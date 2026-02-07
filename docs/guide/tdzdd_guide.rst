.. _tdzdd_guide:

TdZdd Specガイド
================

この章では、SAPPOROBDD 2.0に統合されたTdZdd互換のSpecインターフェースを解説します。
Specインターフェースを使うことで、ZDD/BDDをトップダウンで構築する
カスタムアルゴリズムを効率的に実装できます。

.. contents:: 目次
   :local:
   :depth: 2

概要
----

TdZdd（Top-Down ZDD Construction）は、岩下洋哉氏が開発した
ZDDのトップダウン構築フレームワークです。
SAPPOROBDD 2.0には、このTdZddのSpecインターフェースが統合されており、
``sbdd2::tdzdd`` 名前空間で使用できます。

Specインターフェースの基本的な考え方:

1. **Spec** がZDD/BDDの構造を記述する（ノードの遷移規則を定義）
2. **Builder** がSpecからZDD/BDDを構築する
3. 構築時にノードの共有（状態のマージ）が自動的に行われる

.. code-block:: cpp

   #include <sbdd2/sbdd2.hpp>
   using namespace sbdd2;
   using namespace sbdd2::tdzdd;

Specの基本
----------

全てのSpecは、DDの各ノードでの遷移規則を定義します。
遷移規則は「現在の状態とレベルから、子の状態とレベルを計算する」関数です。

戻り値の規約
~~~~~~~~~~~~

``getRoot()`` と ``getChild()`` の戻り値は以下の意味を持ちます:

.. list-table:: 戻り値の意味
   :header-rows: 1
   :widths: 20 80

   * - 戻り値
     - 意味
   * - ``0``
     - 0終端（ZDD: 空集合、BDD: 偽）
   * - ``-1``
     - 1終端（ZDD: 基底集合、BDD: 真）
   * - 正の整数
     - そのレベルの非終端ノード

レベルは **高いほどルートに近い** という規約です（SAPPOROBDD規約と同じ）。
``getChild()`` が返すレベルは、現在のレベルより小さくなければなりません。

DdSpec
------

``DdSpec<S, T, AR>`` は、スカラー型の状態を持つSpecの基底クラスです。
最も一般的に使用されます。

.. list-table:: DdSpec テンプレートパラメータ
   :header-rows: 1
   :widths: 15 85

   * - パラメータ
     - 説明
   * - ``S``
     - 派生クラス自身の型（CRTP）
   * - ``T``
     - 状態の型
   * - ``AR``
     - アリティ（デフォルト: 2）

必ず実装する関数:

- ``int getRoot(T& state)`` - ルートノードの状態を設定し、レベルを返す
- ``int getChild(T& state, int level, int value)`` - 子ノードの状態を計算し、レベルを返す

.. code-block:: cpp

   // 例: n要素から最大k個を選ぶ組合せを列挙するSpec
   class CombinationSpec : public DdSpec<CombinationSpec, int, 2> {
       int n_;  // 要素数
       int k_;  // 最大選択数

   public:
       CombinationSpec(int n, int k) : n_(n), k_(k) {}

       int getRoot(int& state) {
           state = 0;  // 選択数 = 0
           return n_;   // ルートレベル = n
       }

       int getChild(int& state, int level, int value) {
           if (value == 1) {
               state++;  // 1枝: 要素を選択
           }
           if (state > k_) {
               return 0;  // k個を超えたら0終端
           }
           if (level == 1) {
               return -1;  // 最後のレベルに到達したら1終端
           }
           return level - 1;  // 次のレベルへ
       }
   };

   // 使い方
   DDManager mgr;
   CombinationSpec spec(5, 3);
   ZDD result = build_zdd(mgr, spec);
   std::cout << "5C0 + 5C1 + 5C2 + 5C3 = " << result.card() << std::endl;

状態の等価性
~~~~~~~~~~~~

``DdSpec`` はデフォルトで ``operator==`` を使って状態の等価性を判定し、
等価な状態を持つノードを共有します。
カスタムのハッシュや等価判定が必要な場合は、以下の関数をオーバーライドできます:

- ``std::size_t hashCode(T const& state) const`` - ハッシュ値の計算
- ``bool equalTo(T const& s1, T const& s2) const`` - 等価判定

.. code-block:: cpp

   class MySpec : public DdSpec<MySpec, MyState, 2> {
   public:
       // カスタムハッシュ
       std::size_t hashCode(MyState const& s) const {
           return std::hash<int>()(s.value1) ^ (std::hash<int>()(s.value2) << 16);
       }

       // カスタム等価判定
       bool equalTo(MyState const& s1, MyState const& s2) const {
           return s1.value1 == s2.value1 && s1.value2 == s2.value2;
       }

       // getRoot, getChild は省略
   };

StatelessDdSpec
---------------

``StatelessDdSpec<S, AR>`` は、状態を持たないSpecの基底クラスです。
決定的な構造（状態の共有が不要）の場合に使用します。

必ず実装する関数:

- ``int getRoot()`` - ルートレベルを返す
- ``int getChild(int level, int value)`` - 子のレベルを返す

.. code-block:: cpp

   // 例: べき集合（全ての部分集合）を生成するSpec
   class PowerSetSpec : public StatelessDdSpec<PowerSetSpec, 2> {
       int n_;

   public:
       explicit PowerSetSpec(int n) : n_(n) {}

       int getRoot() {
           return n_;
       }

       int getChild(int level, int value) {
           (void)value;  // 0でも1でも次のレベルへ
           if (level == 1) {
               return -1;  // 全てのレベルを処理したら1終端
           }
           return level - 1;
       }
   };

   // 使い方
   DDManager mgr;
   PowerSetSpec spec(4);
   ZDD result = build_zdd(mgr, spec);
   std::cout << "べき集合の要素数: " << result.card() << std::endl;  // 16

PodArrayDdSpec
--------------

``PodArrayDdSpec<S, T, AR>`` は、POD型の配列を状態として持つSpecの基底クラスです。
フロンティア法のように、配列サイズが問題サイズに依存する場合に使用します。

必ず実装する関数:

- ``int getRoot(T* state)`` - ルートノードの配列状態を設定し、レベルを返す
- ``int getChild(T* state, int level, int value)`` - 子ノードの配列状態を計算し、レベルを返す

コンストラクタで ``setArraySize(n)`` を呼んで配列サイズを設定する必要があります。

.. code-block:: cpp

   // 例: 独立集合（隣接する要素を同時に選ばない）を列挙するSpec
   class IndependentSetSpec : public PodArrayDdSpec<IndependentSetSpec, int, 2> {
       int n_;

   public:
       explicit IndependentSetSpec(int n) : n_(n) {
           setArraySize(1);  // 配列サイズ = 1（直前の選択状態のみ）
       }

       int getRoot(int* state) {
           state[0] = 0;  // 直前の要素は未選択
           return n_;
       }

       int getChild(int* state, int level, int value) {
           if (value == 1 && state[0] == 1) {
               return 0;  // 隣接する要素を両方選択 -> 0終端
           }
           state[0] = value;
           if (level == 1) {
               return -1;
           }
           return level - 1;
       }
   };

HybridDdSpec
------------

``HybridDdSpec<S, TS, TA, AR>`` は、スカラー状態とPOD配列状態の
両方を持つSpecの基底クラスです。

.. list-table:: HybridDdSpec テンプレートパラメータ
   :header-rows: 1
   :widths: 15 85

   * - パラメータ
     - 説明
   * - ``S``
     - 派生クラス自身の型（CRTP）
   * - ``TS``
     - スカラー状態の型
   * - ``TA``
     - 配列状態の要素型
   * - ``AR``
     - アリティ（デフォルト: 2）

必ず実装する関数:

- ``int getRoot(TS& scalar_state, TA* array_state)``
- ``int getChild(TS& scalar_state, TA* array_state, int level, int value)``

コンストラクタで ``setArraySize(n)`` を呼んで配列サイズを設定する必要があります。

.. code-block:: cpp

   // 例: 制約付き選択（スカラー状態: 選択数、配列状態: フロンティア）
   class ConstrainedSpec : public HybridDdSpec<ConstrainedSpec, int, int, 2> {
       int n_;
       int max_select_;

   public:
       ConstrainedSpec(int n, int max_select, int frontier_size)
           : n_(n), max_select_(max_select) {
           setArraySize(frontier_size);
       }

       int getRoot(int& count, int* frontier) {
           count = 0;
           for (int i = 0; i < getArraySize(); ++i) {
               frontier[i] = 0;
           }
           return n_;
       }

       int getChild(int& count, int* frontier, int level, int value) {
           if (value == 1) {
               count++;
               if (count > max_select_) return 0;
           }
           // frontier の更新（問題に応じた処理）
           if (level == 1) return -1;
           return level - 1;
       }
   };

ビルダー
--------

BFS（幅優先）ビルダー
~~~~~~~~~~~~~~~~~~~~~

``build_zdd()`` と ``build_bdd()`` は、幅優先探索でSpecからDD構築を行います。
これが標準のビルダーです。

.. code-block:: cpp

   DDManager mgr;

   // ZDDを構築
   CombinationSpec spec(10, 3);
   ZDD zdd = build_zdd(mgr, spec);

   // BDDを構築
   PowerSetSpec bdd_spec(5);
   BDD bdd = build_bdd(mgr, bdd_spec);

未縮約DDを構築するバージョンもあります:

.. code-block:: cpp

   // 未縮約ZDDを構築
   UnreducedZDD unreduced = build_unreduced_zdd(mgr, spec);

   // 手動で縮約
   ZDD reduced = unreduced.reduce();

レベルオフセット
~~~~~~~~~~~~~~~~

``offset`` パラメータを使うと、Specのレベルをずらして構築できます。
Specのレベル L は SAPPOROBDD2 のレベル (L + offset) にマッピングされます。

.. code-block:: cpp

   DDManager mgr;

   // レベル5〜10に変数を配置（offset = 5）
   CombinationSpec spec(5, 2);
   ZDD zdd = build_zdd(mgr, spec, 5);

DFSビルダー
-----------

``build_zdd_dfs()`` と ``build_bdd_dfs()`` は、
深さ優先探索でSpecからDD構築を行います。
BFSビルダーよりもメモリ効率が良い場合があります。

.. code-block:: cpp

   DDManager mgr;

   CombinationSpec spec(10, 3);

   // DFSでZDDを構築
   ZDD zdd_dfs = build_zdd_dfs(mgr, spec);

   // DFSでBDDを構築
   PowerSetSpec bdd_spec(5);
   BDD bdd_dfs = build_bdd_dfs(mgr, bdd_spec);

DFSビルダーは内部でメモ化キャッシュ（``DFSCache``）を使用し、
同じ状態の重複計算を避けます。

.. list-table:: BFSビルダーとDFSビルダーの比較
   :header-rows: 1
   :widths: 20 40 40

   * - 特性
     - BFS（build_zdd）
     - DFS（build_zdd_dfs）
   * - 探索方法
     - 幅優先（レベルごとに処理）
     - 深さ優先（再帰的に処理）
   * - メモリ使用量
     - 最大幅に比例
     - スタック深さに比例
   * - 並列化
     - OpenMP対応版あり
     - 非対応
   * - 適した問題
     - 幅が狭い問題
     - 幅が広いが深さが浅い問題

並列ビルダー
~~~~~~~~~~~~

OpenMPを使った並列構築も利用できます:

.. code-block:: cpp

   DDManager mgr;
   CombinationSpec spec(20, 5);

   // 並列BFS構築（OpenMP）
   ZDD zdd_mp = build_zdd_mp(mgr, spec);
   BDD bdd_mp = build_bdd_mp(mgr, spec);

Spec演算
--------

複数のSpecを演算で組み合わせることができます。

ZDD演算
~~~~~~~

.. code-block:: cpp

   CombinationSpec spec1(5, 2);  // 5要素から2個以下を選択
   CombinationSpec spec2(5, 3);  // 5要素から3個以下を選択

   // 和集合（Union）
   auto union_spec = zddUnion(spec1, spec2);
   ZDD union_zdd = build_zdd(mgr, union_spec);

   // 積集合（Intersection）
   auto intersect_spec = zddIntersection(spec1, spec2);
   ZDD intersect_zdd = build_zdd(mgr, intersect_spec);

BDD演算
~~~~~~~

.. code-block:: cpp

   // BDD AND
   auto and_spec = bddAnd(spec1, spec2);
   BDD and_bdd = build_bdd(mgr, and_spec);

   // BDD OR
   auto or_spec = bddOr(spec1, spec2);
   BDD or_bdd = build_bdd(mgr, or_spec);

Lookahead最適化
~~~~~~~~~~~~~~~

``zddLookahead()`` と ``bddLookahead()`` は、
Specの出力を先読みして冗長ノードを削減する最適化を行います。

ZDD Lookahead は、1枝が全て0終端に向かうノードを事前に除去します。
BDD Lookahead は、0枝と1枝が同じ先に向かうノードを事前に除去します。

.. code-block:: cpp

   CombinationSpec spec(10, 3);

   // ZDD Lookahead最適化を適用
   auto opt_spec = zddLookahead(spec);
   ZDD zdd = build_zdd(mgr, opt_spec);

   // BDD Lookahead最適化を適用
   auto bdd_opt_spec = bddLookahead(spec);
   BDD bdd = build_bdd(mgr, bdd_opt_spec);

Unreduction
~~~~~~~~~~~

``bddUnreduction()`` と ``zddUnreduction()`` は、
Specから非縮約（QDD: Quasi-reduced DD）を生成するラッパーです。
スキップされたノードを補完します。

.. code-block:: cpp

   CombinationSpec spec(5, 2);

   // BDD非縮約化
   auto unreduced_spec = bddUnreduction(spec, 5);
   BDD qdd = build_bdd(mgr, unreduced_spec);

   // ZDD非縮約化
   auto zdd_unreduced_spec = zddUnreduction(spec, 5);
   ZDD qzdd = build_zdd(mgr, zdd_unreduced_spec);

VarArity Spec
-------------

通常のSpecはテンプレートパラメータ ``AR`` でアリティ（子の数）を
コンパイル時に固定しますが、VarArity Specではランタイムにアリティを設定できます。
多値変数（MVBDD/MVZDD）の構築に使用します。

VarArityDdSpec
~~~~~~~~~~~~~~

``VarArityDdSpec<S, T>`` はスカラー状態を持つVarArity Specです。

.. code-block:: cpp

   // 例: ランタイムにアリティを決定するSpec
   class MyVarAritySpec : public VarArityDdSpec<MyVarAritySpec, int> {
       int n_;

   public:
       MyVarAritySpec(int n, int arity) : n_(n) {
           setArity(arity);  // コンストラクタでアリティを設定
       }

       int getRoot(int& state) {
           state = 0;
           return n_;
       }

       int getChild(int& state, int level, int value) {
           state += value;
           if (level == 1) return -1;
           return level - 1;
       }
   };

VarArityHybridDdSpec
~~~~~~~~~~~~~~~~~~~~

``VarArityHybridDdSpec<S, TS, TA>`` はスカラー状態とPOD配列状態を
両方持つVarArity Specです。

.. code-block:: cpp

   class MyVarArityHybridSpec
       : public VarArityHybridDdSpec<MyVarArityHybridSpec, int, int> {
       int n_;

   public:
       MyVarArityHybridSpec(int n, int arity, int array_size) : n_(n) {
           setArity(arity);
           setArraySize(array_size);
       }

       int getRoot(int& scalar, int* array) {
           scalar = 0;
           for (int i = 0; i < getArraySize(); ++i) {
               array[i] = 0;
           }
           return n_;
       }

       int getChild(int& scalar, int* array, int level, int value) {
           scalar += value;
           array[value % getArraySize()]++;
           if (level == 1) return -1;
           return level - 1;
       }
   };

VarArity Specのビルド
~~~~~~~~~~~~~~~~~~~~~

VarArity Specは ``build_mvzdd_va()`` や ``build_mvbdd_va()`` でビルドします。
並列版として ``build_mvzdd_va_mp()`` と ``build_mvbdd_va_mp()`` もあります。

.. code-block:: cpp

   DDManager mgr;
   MyVarAritySpec spec(5, 3);  // 5レベル、アリティ3

   // MVZDDとしてビルド
   MVZDD mvzdd = build_mvzdd_va(mgr, spec);

   // MVBDDとしてビルド
   MVBDD mvbdd = build_mvbdd_va(mgr, spec);

VarArity Spec演算
~~~~~~~~~~~~~~~~~

VarArity Specの和集合・積集合演算も用意されています:

.. code-block:: cpp

   MyVarAritySpec spec1(5, 3);
   MyVarAritySpec spec2(5, 3);

   // 両方のSpecは同じアリティでなければならない
   auto union_va = zddUnionVA(spec1, spec2);
   MVZDD union_result = build_mvzdd_va(mgr, union_va);

   auto intersect_va = zddIntersectionVA(spec1, spec2);
   MVZDD intersect_result = build_mvzdd_va(mgr, intersect_va);

DdEval
------

``DdEval<E, T, R, ARITY>`` は、DD（ZDD/BDD）に対してボトムアップ評価を行う
インターフェースです。

.. list-table:: DdEval テンプレートパラメータ
   :header-rows: 1
   :widths: 15 85

   * - パラメータ
     - 説明
   * - ``E``
     - 派生クラス自身の型（CRTP）
   * - ``T``
     - 各ノードの作業領域の型
   * - ``R``
     - 最終的な戻り値の型（デフォルト: T）
   * - ``ARITY``
     - アリティ（デフォルト: 2）

必ず実装する関数:

- ``void evalTerminal(T& v, int id)`` - 終端ノードの値を設定（id: 0=偽終端, 1=真終端）
- ``void evalNode(T& v, int level, DdValues<T, ARITY> const& values)`` - 非終端ノードの値を計算

.. code-block:: cpp

   // 例: ZDDの要素数（カーディナリティ）を計算する評価器
   class MyCardinalityEval : public DdEval<MyCardinalityEval, double, double, 2> {
   public:
       void evalTerminal(double& v, int id) {
           v = (id == 1) ? 1.0 : 0.0;
       }

       void evalNode(double& v, int level, DdValues<double, 2> const& values) {
           (void)level;
           v = values.get(0) + values.get(1);  // 0枝の値 + 1枝の値
       }
   };

   // 使い方
   DDManager mgr;
   mgr.new_var();
   mgr.new_var();
   ZDD z = ZDD::singleton(mgr, 1) + ZDD::singleton(mgr, 2);

   MyCardinalityEval eval;
   double count = zdd_evaluate(z, eval);
   std::cout << "要素数: " << count << std::endl;  // 2

DdValues
~~~~~~~~

``DdValues<T, ARITY>`` は子ノードの値とレベルをまとめたコンテナです。

.. code-block:: cpp

   void evalNode(double& v, int level, DdValues<double, 2> const& values) {
       double low_value = values.get(0);   // 0枝の子の値
       double high_value = values.get(1);  // 1枝の子の値

       int low_level = values.getLevel(0);   // 0枝の子のレベル
       int high_level = values.getLevel(1);  // 1枝の子のレベル

       v = low_value + high_value;
   }

組み込み評価器
~~~~~~~~~~~~~~

SAPPOROBDD 2.0には、よく使用される評価器が組み込まれています:

.. code-block:: cpp

   // CardinalityEval: ZDDの要素数を計算
   CardinalityEval card_eval;
   double count = zdd_evaluate(z, card_eval);

Specの直接評価
~~~~~~~~~~~~~~

``evaluate_spec()`` は、構築されたDD無しにSpecを直接評価します。
DDを構築するメモリが不要な場合に使用できます。

.. code-block:: cpp

   CombinationSpec spec(10, 3);
   CardinalityEval eval;

   // Specを直接評価（DD構築不要）
   double count = evaluate_spec(spec, eval);
   std::cout << "10C0 + 10C1 + 10C2 + 10C3 = " << count << std::endl;

zdd_subset
----------

``zdd_subset()`` は、既存のZDDに対してSpecをフィルタとして適用し、
Specの条件を満たす集合だけを抽出する操作です。

.. code-block:: cpp

   DDManager mgr;
   for (int i = 0; i < 5; ++i) mgr.new_var();

   // べき集合を作成
   PowerSetSpec ps_spec(5);
   ZDD power_set = build_zdd(mgr, ps_spec);

   // フィルタ: 最大2個までの選択に制限
   CombinationSpec filter_spec(5, 2);
   ZDD filtered = zdd_subset(mgr, power_set, filter_spec);

   std::cout << "フィルタ前: " << power_set.card() << std::endl;   // 32
   std::cout << "フィルタ後: " << filtered.card() << std::endl;     // 16 (5C0+5C1+5C2)

フロンティア法Spec
------------------

GBaseクラスが内部で使用しているフロンティア法は、
Specインターフェースとして直接実装することも可能です。
ここでは、簡略化したs-tパス列挙のSpecの設計方針を示します。

状態の設計
~~~~~~~~~~

フロンティア法の状態は、フロンティア上の各頂点の接続情報（mate配列）です:

.. code-block:: cpp

   // フロンティア法Specの概念的な構造
   class SimpleSimpathSpec
       : public PodArrayDdSpec<SimpleSimpathSpec, int, 2> {
       // 状態: mate配列
       // mate[v] = v と接続されている相手の頂点
       // mate[v] = v なら未接続
       // mate[v] = 0 ならフロンティア外

       int n_edges_;
       // ... グラフ情報

   public:
       SimpleSimpathSpec(int n_vertices, int n_edges, ...)
           : n_edges_(n_edges) {
           setArraySize(n_vertices + 1);  // mate配列のサイズ
       }

       int getRoot(int* mate) {
           // 初期化: 全頂点が自分自身と接続
           for (int i = 0; i <= n_vertices_; ++i) {
               mate[i] = i;
           }
           return n_edges_;  // ルートレベル = 辺数
       }

       int getChild(int* mate, int level, int value) {
           // 辺を使う(value=1)/使わない(value=0)の処理
           // mate配列を更新し、制約違反をチェック
           // ...
           return level - 1;  // 次の辺へ
       }
   };

独自のフロンティア法Specを実装する際は、
:doc:`gbase_guide` のGBaseクラスのソースコードも参考にしてください。

デバッグのヒント
----------------

Specの開発時に役立つデバッグ技法を紹介します。

状態の表示
~~~~~~~~~~

``printState()`` をオーバーライドすると、
ビルド中に各ノードの状態を確認できます。

.. code-block:: cpp

   class MySpec : public DdSpec<MySpec, int, 2> {
   public:
       // 状態のデバッグ出力
       void printState(std::ostream& os, int const& state) const {
           os << "count=" << state;
       }

       // getRoot, getChild は省略
   };

小さな入力でのテスト
~~~~~~~~~~~~~~~~~~~~~

まず小さな入力サイズでSpecをテストし、
列挙結果を手動で確認することを推奨します。

.. code-block:: cpp

   // 小さなサイズでテスト
   CombinationSpec spec(3, 2);
   ZDD zdd = build_zdd(mgr, spec);

   // 全要素を列挙して確認
   auto sets = zdd.enumerate();
   for (const auto& s : sets) {
       std::cout << "{";
       for (size_t i = 0; i < s.size(); ++i) {
           if (i > 0) std::cout << ", ";
           std::cout << s[i];
       }
       std::cout << "}" << std::endl;
   }

BFS vs DFS の比較
~~~~~~~~~~~~~~~~~~

同じSpecに対してBFSビルダーとDFSビルダーの結果を比較すると、
Specの正しさを確認できます。

.. code-block:: cpp

   CombinationSpec spec1(10, 3);
   CombinationSpec spec2(10, 3);  // 同じSpec

   ZDD zdd_bfs = build_zdd(mgr, spec1);
   ZDD zdd_dfs = build_zdd_dfs(mgr, spec2);

   assert(zdd_bfs == zdd_dfs);  // 結果は同じはず

.. seealso::

   - :doc:`bdd_guide` - BDDの基本操作
   - :doc:`zdd_guide` - ZDDの基本操作
   - :doc:`gbase_guide` - フロンティア法を使ったグラフ列挙
   - :doc:`mvdd_guide` - VarArity Specと多値DDの連携
