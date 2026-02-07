.. _zdd_guide:

ZDDガイド
=========

この章では、SAPPOROBDD 2.0のZDD（ゼロ抑制二分決定図）クラスの使い方を詳しく解説します。
ZDDは組合せ集合族の表現に特化したデータ構造であり、SAPPOROBDD 2.0の最も重要なクラスの一つです。

.. contents:: 目次
   :local:
   :depth: 2

ZDDクラスの概要
---------------

ZDDは **集合族（集合の集合）** :math:`\mathcal{F} \subseteq 2^V` を効率的に表現します。
ここで :math:`V = \{1, 2, \ldots, n\}` は要素の集合です。

BDDとは異なる縮約規則を使用します。

1. **ノード共有**: 等価なサブグラフは同一ノードとして共有される
2. **ゼロ抑制**: 1枝が終端0を指すノードは除去される（0枝側に直接接続）

ゼロ抑制規則により、ZDDは **疎な集合族** を特にコンパクトに表現できます。

.. code-block:: text

   例: F = {{1, 2}, {2, 3}, {1, 3}}

   この集合族をZDDで表現すると、各パス（根から1終端まで）が
   1つの集合に対応します。1枝をたどった変数が集合に含まれます。

.. code-block:: cpp

   #include <sbdd2/sbdd2.hpp>
   using namespace sbdd2;

ZDDの作成
---------

終端定数
~~~~~~~~

.. code-block:: cpp

   DDManager mgr;

   // 変数を作成
   for (int i = 0; i < 5; ++i) mgr.new_var();

   // 空集合族（0終端）: 集合を1つも含まない
   ZDD empty = ZDD::empty(mgr);    // ∅

   // 単一集合族（1終端）: 空集合のみを含む
   ZDD single = ZDD::single(mgr);  // {∅}

.. important::

   ``ZDD::empty(mgr)`` と ``ZDD::single(mgr)`` を混同しないでください。

   - ``empty`` は集合が1つも含まれない空の集合族 :math:`\emptyset`
   - ``single`` は空集合だけを含む集合族 :math:`\{\emptyset\}`

単一要素集合
~~~~~~~~~~~~

.. code-block:: cpp

   // {{v}} を作成: 要素vだけを含む集合だけを含む集合族
   ZDD s1 = ZDD::singleton(mgr, 1);   // {{1}}
   ZDD s2 = ZDD::singleton(mgr, 2);   // {{2}}
   ZDD s3 = ZDD::singleton(mgr, 3);   // {{3}}

集合族演算
----------

ZDDクラスは集合族に対する豊富な演算を提供します。

基本演算
~~~~~~~~

.. list-table:: ZDDの集合族演算子
   :header-rows: 1
   :widths: 20 15 25 40

   * - 演算
     - 演算子
     - 記法
     - 説明
   * - 和集合
     - ``+``
     - ``F + G``
     - :math:`F \cup G`
   * - 差集合
     - ``-``
     - ``F - G``
     - :math:`F \setminus G`
   * - 積集合
     - ``&``
     - ``F & G``
     - :math:`F \cap G`
   * - 直積
     - ``*`` / ``join()``
     - ``F * G``
     - :math:`\{S \cup T \mid S \in F, T \in G\}`
   * - 商
     - ``/``
     - ``F / G``
     - :math:`\{S \mid S \cup T \in F, \forall T \in G\}`
   * - 剰余
     - ``%``
     - ``F % G``
     - :math:`F - (F / G) \cdot G`

.. code-block:: cpp

   ZDD s1 = ZDD::singleton(mgr, 1);  // {{1}}
   ZDD s2 = ZDD::singleton(mgr, 2);  // {{2}}
   ZDD s3 = ZDD::singleton(mgr, 3);  // {{3}}

   // 和集合（Union）
   ZDD family = s1 + s2 + s3;         // {{1}, {2}, {3}}
   std::cout << "集合数: " << family.card() << std::endl;  // 3

   // 積集合（Intersection）
   ZDD inter = (s1 + s2) & (s2 + s3); // {{2}}

   // 差集合（Difference）
   ZDD diff = family - s1;             // {{2}, {3}}

直積（Join / Cross Product）
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

直積は2つの集合族の要素を組み合わせます。

.. code-block:: cpp

   ZDD s1 = ZDD::singleton(mgr, 1);  // {{1}}
   ZDD s2 = ZDD::singleton(mgr, 2);  // {{2}}

   // 直積: {{1}} * {{2}} = {{1, 2}}
   ZDD prod = s1 * s2;               // {{1, 2}}
   // または
   ZDD prod2 = s1.join(s2);          // 同じ結果

   // より複雑な例
   ZDD ab = s1 + s2;                 // {{1}, {2}}
   ZDD cd = ZDD::singleton(mgr, 3) + ZDD::singleton(mgr, 4);  // {{3}, {4}}
   ZDD cross = ab * cd;
   // {{1, 3}, {1, 4}, {2, 3}, {2, 4}}

商と剰余
~~~~~~~~~

.. code-block:: cpp

   // F = {{1, 2}, {1, 3}, {2, 3}}
   ZDD f = (s1 * s2) + (s1 * s3) + (s2 * s3);

   // G = {{1}}
   ZDD g = s1;

   // F / G = {S | S ∪ T ∈ F for all T ∈ G}
   //       = {S | {S ∪ {1}} ⊆ F}
   //       = {{2}, {3}}（{1,2} と {1,3} から {1} を除いたもの）
   ZDD quotient = f / g;

   // F % G = F - (F / G) * G
   ZDD remainder = f % g;

onset/offset/change
--------------------

onset
~~~~~

``onset(v)`` は要素 :math:`v` を含む集合から :math:`v` を除去した集合族を返します。

.. code-block:: cpp

   // F = {{1, 2}, {2, 3}, {1}}
   ZDD f = (s1 * s2) + (s2 * s3) + s1;

   // onset(1): 要素1を含む集合から1を除去
   ZDD f_onset1 = f.onset(1);
   // = {{2}, {∅}} （{1,2}→{2}, {1}→{∅}）

offset
~~~~~~

``offset(v)`` は要素 :math:`v` を含まない集合のみを残します。

.. code-block:: cpp

   // offset(1): 要素1を含まない集合だけ
   ZDD f_offset1 = f.offset(1);
   // = {{2, 3}}

onset0
~~~~~~

``onset0(v)`` は要素 :math:`v` を含む集合をそのまま（ :math:`v` を保持して）返します。

.. code-block:: cpp

   // onset0(1): 要素1を含む集合をそのまま
   ZDD f_onset0_1 = f.onset0(1);
   // = {{1, 2}, {1}}

change
~~~~~~

``change(v)`` は全集合で要素 :math:`v` をトグル（含む/含まないを反転）します。

.. code-block:: cpp

   ZDD s1 = ZDD::singleton(mgr, 1);  // {{1}}

   // change(2): 全集合で要素2をトグル
   ZDD toggled = s1.change(2);
   // = {{1, 2}} （{1} に 2 を追加）

   // change はべき集合の構築に便利
   ZDD base = ZDD::single(mgr);       // {∅}
   ZDD with_1 = base.change(1);       // {{1}}
   std::cout << (base + with_1).card() << std::endl;  // 2: {∅, {1}}

カウントと列挙
--------------

カウント
~~~~~~~~

.. code-block:: cpp

   ZDD f = s1 + s2 + s3 + (s1 * s2);
   // F = {{1}, {2}, {3}, {1, 2}}

   // card() / count(): 集合の数
   double n = f.card();          // 4.0
   double n2 = f.count();        // 4.0（card の別名）

   // 厳密カウント（GMP / BigInt）
   #if defined(SBDD2_HAS_GMP) || defined(SBDD2_HAS_BIGINT)
   std::string exact = f.exact_count();  // "4"
   #endif

列挙
~~~~

.. code-block:: cpp

   // enumerate(): 全集合を列挙
   auto sets = f.enumerate();
   for (const auto& s : sets) {
       std::cout << "{";
       for (size_t i = 0; i < s.size(); ++i) {
           if (i > 0) std::cout << ", ";
           std::cout << s[i];
       }
       std::cout << "}" << std::endl;
   }
   // 出力:
   // {1}
   // {2}
   // {3}
   // {1, 2}

.. warning::

   ``enumerate()`` は全集合をメモリに展開するため、集合数が大きい場合は
   大量のメモリを消費します。大規模な集合族にはイテレータの使用を推奨します。

1つの集合を取得
~~~~~~~~~~~~~~~~

.. code-block:: cpp

   // one_set(): 集合族から1つの集合を取得
   std::vector<bddvar> one = f.one_set();
   // 空集合族の場合は空のベクタを返す

統計情報
--------

.. code-block:: cpp

   ZDD f = s1 + s2 + (s1 * s2 * s3);
   // F = {{1}, {2}, {1, 2, 3}}

   // lit(): 全集合の要素数の合計
   std::uint64_t total_lit = f.lit();
   // = 1 + 1 + 3 = 5

   // len(): 最大集合サイズ（最大濃度）
   std::uint64_t max_len = f.len();
   // = 3

   // always(): 全ての集合に共通して含まれる要素
   ZDD common = f.always();
   // 共通要素がない場合は empty を返す

   // ノード情報
   std::cout << "ノード数: " << f.size() << std::endl;
   std::cout << "トップ変数: " << f.top() << std::endl;
   std::cout << "サポート: ";
   for (bddvar v : f.support()) {
       std::cout << v << " ";
   }
   std::cout << std::endl;

対称性・インプリケーション分析
------------------------------

対称性チェック
~~~~~~~~~~~~~~

2つの変数が対称（交換しても集合族が変わらない）かどうかを判定します。

.. code-block:: cpp

   ZDD f = s1 + s2;  // {{1}, {2}} — 変数1と2は対称

   // sym_chk(v1, v2): 対称性チェック
   int result = f.sym_chk(1, 2);
   // 1: 対称, 0: 非対称, -1: エラー
   std::cout << "対称: " << result << std::endl;  // 1

対称グループの取得
~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   // sym_grp(): 対称グループを取得
   ZDD groups = f.sym_grp();

   // sym_set(v): 変数vと対称な変数集合
   ZDD sym_with_1 = f.sym_set(1);

インプリケーション
~~~~~~~~~~~~~~~~~~

変数 :math:`v_1` が含まれるなら常に :math:`v_2` も含まれる、という関係を検出します。

.. code-block:: cpp

   // F = {{1, 2}, {1, 2, 3}}（1を含む集合は必ず2も含む）
   ZDD f = (s1 * s2) + (s1 * s2 * s3);

   // imply_chk(v1, v2): v1 → v2 のインプリケーションチェック
   int impl = f.imply_chk(1, 2);
   std::cout << "1 → 2: " << impl << std::endl;  // 1

   // co_imply_chk(v1, v2): v1 ↔ v2 の相互インプリケーション
   int co_impl = f.co_imply_chk(1, 2);

   // imply_set(v): vがインプライする全変数
   ZDD implied_by_1 = f.imply_set(1);

   // co_imply_set(v): vと相互インプリケーション関係にある全変数
   ZDD co_implied_1 = f.co_imply_set(1);

制限・許可演算
--------------

restrict
~~~~~~~~

``restrict(g)`` は、 :math:`g` に含まれる要素を少なくとも1つ含む集合のみを残します。

.. code-block:: cpp

   // F = {{1}, {2}, {1, 3}}
   ZDD f = s1 + s2 + (s1 * s3);

   // G = {{1}} — 要素1を含む集合だけを残す条件
   ZDD g = s1;
   ZDD restricted = f.restrict(g);
   // {{1}, {1, 3}}

permit
~~~~~~

``permit(g)`` は、 :math:`g` に含まれる集合の部分集合のみを残します。

.. code-block:: cpp

   // G = {{1, 2}} — {1, 2} の部分集合だけを許可
   ZDD g2 = s1 * s2;
   ZDD permitted = f.permit(g2);

permit_sym（濃度制限）
~~~~~~~~~~~~~~~~~~~~~~

``permit_sym(n)`` は濃度（集合のサイズ）が :math:`n` 以下の集合のみを残します。

.. code-block:: cpp

   // F = {{1}, {2}, {1, 2, 3}}
   ZDD f = s1 + s2 + (s1 * s2 * s3);

   // 濃度2以下の集合だけを残す
   ZDD small = f.permit_sym(2);
   // = {{1}, {2}}

べき集合の構築パターン
-----------------------

ZDDでべき集合（全ての部分集合の集合）を構築するパターンは非常に重要です。

ステップバイステップ
~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   DDManager mgr;
   for (int i = 0; i < 4; ++i) mgr.new_var();

   // 方法1: change を使ったインクリメンタル構築
   ZDD all = ZDD::single(mgr);  // {∅}
   for (int i = 1; i <= 4; ++i) {
       // 各要素について「含む/含まない」を選択
       all = all + all.change(i);
   }
   std::cout << "べき集合の大きさ: " << all.card() << std::endl;  // 16 = 2^4

ヘルパー関数の利用
~~~~~~~~~~~~~~~~~~

``zdd_helper.hpp`` にはべき集合生成のヘルパー関数があります。

.. code-block:: cpp

   #include <sbdd2/zdd_helper.hpp>

   // get_power_set: n変数のべき集合
   ZDD power = get_power_set(mgr, 4);
   std::cout << "2^4 = " << power.card() << std::endl;  // 16

   // get_power_set_with_card: 指定濃度のべき集合
   ZDD comb3 = get_power_set_with_card(mgr, 4, 2);
   std::cout << "C(4,2) = " << comb3.card() << std::endl;  // 6

   // get_single_set: 指定要素を含む単一集合
   std::vector<bddvar> elems = {1, 3, 4};
   ZDD single_set = get_single_set(mgr, elems);
   // = {{1, 3, 4}}

   // is_member: メンバーシップ判定
   bool found = is_member(power, elems);
   std::cout << "{1,3,4} ∈ べき集合: " << found << std::endl;  // true

   // make_dont_care: 指定変数をドントケアにする
   ZDD dc = make_dont_care(ZDD::single(mgr), {1, 2});
   // = {∅, {1}, {2}, {1,2}}

ZDDイテレータ
--------------

大量の集合を効率的に処理するために、3種類のイテレータを提供します。

DictIterator（辞書順イテレータ）
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   ZDD f = s1 + s2 + (s1 * s2) + (s2 * s3);
   // F = {{1}, {2}, {1, 2}, {2, 3}}

   // 辞書順で前方イテレーション
   for (auto it = f.dict_begin(); it != f.dict_end(); ++it) {
       const std::set<bddvar>& s = *it;
       std::cout << "{";
       for (bddvar v : s) std::cout << v << " ";
       std::cout << "}" << std::endl;
   }

   // 逆辞書順でイテレーション
   for (auto it = f.dict_rbegin(); it != f.dict_rend(); ++it) {
       // 辞書順で最後の集合から最初の集合へ
   }

WeightIterator（重みイテレータ）
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   // 重みの定義
   std::vector<int64_t> weights = {0, 10, 20, 5};  // weights[v] = 変数vの重み

   // 重み昇順でイテレーション
   for (auto it = f.weight_min_begin(weights);
        it != f.weight_min_end(); ++it) {
       const std::set<bddvar>& s = *it;
       // 重みの小さい集合から順に処理
   }

   // 重み降順でイテレーション
   for (auto it = f.weight_max_begin(weights);
        it != f.weight_max_end(); ++it) {
       // 重みの大きい集合から順に処理
   }

RandomIterator（ランダムイテレータ）
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   std::mt19937 rng(42);  // シード42

   // ランダムサンプリング（1つ）
   std::set<bddvar> sample = f.sample_randomly(rng);

   // ランダムイテレータ
   for (auto it = f.random_begin(rng); it != f.random_end(); ++it) {
       const std::set<bddvar>& s = *it;
       // ランダムな順序で処理
       break;  // 必要な数だけ取得して break
   }

ZDDインデックス
----------------

ZDDインデックスは、集合族内の集合に辞書順の番号を付け、
ランキング（集合から番号への変換）とアンランキング（番号から集合への変換）を
効率的に行う仕組みです。

インデックスの構築
~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   ZDD f = get_power_set(mgr, 4);  // 2^4 = 16 集合

   // インデックスを構築（自動的に行われるが、明示的にも可能）
   f.build_index();

   // インデックス情報
   std::cout << "高さ: " << f.index_height() << std::endl;
   std::cout << "ノード数: " << f.index_size() << std::endl;
   std::cout << "集合数: " << f.indexed_count() << std::endl;  // 16

ランキング（集合 -> 番号）
~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   // order_of(): 集合の辞書順番号を取得
   std::set<bddvar> target = {1, 3};
   int64_t order = f.order_of(target);
   std::cout << "{1, 3} の番号: " << order << std::endl;

   // 存在しない集合の場合は -1
   std::set<bddvar> absent = {5};
   int64_t not_found = f.order_of(absent);
   std::cout << "存在しない集合: " << not_found << std::endl;  // -1

アンランキング（番号 -> 集合）
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   // get_set(): 辞書順番号から集合を取得
   std::set<bddvar> result = f.get_set(0);  // 最初の集合
   std::cout << "番号0の集合: {";
   for (bddvar v : result) std::cout << v << " ";
   std::cout << "}" << std::endl;

厳密整数版インデックス（GMP / BigInt）
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   #if defined(SBDD2_HAS_GMP) || defined(SBDD2_HAS_BIGINT)
   // 厳密カウント版のインデックス
   f.build_exact_index();
   std::string exact_count = f.indexed_exact_count();

   // 厳密ランキング/アンランキング
   std::string exact_order = f.exact_order_of(target);
   std::set<bddvar> exact_set = f.exact_get_set("5");
   #endif

インデックスの管理
~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   // インデックスが構築済みか確認
   bool has = f.has_index();

   // インデックスキャッシュをクリア（メモリ節約）
   f.clear_index();

重みフィルタリング
------------------

``zdd_helper.hpp`` に含まれる重みフィルタリング関数を使って、
集合の総重みに基づいてフィルタリングできます。

.. code-block:: cpp

   #include <sbdd2/zdd_helper.hpp>

   // 重みの定義（インデックス0は未使用、weights[1]が変数1の重み）
   std::vector<long long> weights = {0, 10, 20, 15, 25, 30};

   ZDD all = get_power_set(mgr, 5);  // 32集合

   // 重み <= 30 の集合だけを残す
   ZDD light = weight_le(all, 30, weights);

   // 重み >= 40 の集合だけを残す
   ZDD heavy = weight_ge(all, 40, weights);

   // 重み < 25 の集合
   ZDD lt25 = weight_lt(all, 25, weights);

   // 重み > 50 の集合
   ZDD gt50 = weight_gt(all, 50, weights);

   // 重み == 30 の集合
   ZDD exact30 = weight_eq(all, 30, weights);

   // 重み != 30 の集合
   ZDD not30 = weight_ne(all, 30, weights);

   // 重み 20 以上 40 以下の集合
   ZDD range = weight_range(all, 20, 40, weights);

重み付き最適化
~~~~~~~~~~~~~~

ZDDクラスには、重み付き最適化のメソッドもあります。

.. code-block:: cpp

   std::vector<int64_t> w = {0, 10, 20, 15, 25, 30};

   // 最大重みを持つ集合を取得
   std::set<bddvar> max_set;
   int64_t max_w = all.max_weight(w, max_set);

   // 最小重みを持つ集合を取得
   std::set<bddvar> min_set;
   int64_t min_w = all.min_weight(w, min_set);

   // 全集合の重みの総和
   int64_t total = all.sum_weight(w);

よくある間違い
--------------

emptyとsingleの混同
~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   // 間違い: empty は「集合が0個」、single は「空集合が1個」
   ZDD f = ZDD::empty(mgr);   // ∅（集合族は空）
   ZDD g = ZDD::single(mgr);  // {∅}（空集合を1つ含む）

   std::cout << f.card() << std::endl;  // 0
   std::cout << g.card() << std::endl;  // 1

   // べき集合の構築で single から始める
   ZDD power = ZDD::single(mgr);  // 正しい: {∅} から始める
   // ZDD power = ZDD::empty(mgr);  // 間違い: ∅ に何を追加しても ∅

変数作成順序の間違い
~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   DDManager mgr;

   // 変数は使用前に作成する必要がある
   // mgr.new_var() を呼ぶ前に singleton を作ると例外
   // ZDD s = ZDD::singleton(mgr, 1);  // エラー！

   mgr.new_var();                      // 変数1を作成
   ZDD s = ZDD::singleton(mgr, 1);     // OK

   // 変数は必要な数だけ事前に作成する
   for (int i = 0; i < 10; ++i) mgr.new_var();

productとjoinの混同
~~~~~~~~~~~~~~~~~~~

``*`` 演算子（``join()``）は集合の **和集合** の直積であり、
算術的な乗算ではありません。

.. code-block:: cpp

   ZDD a = s1 + s2;          // {{1}, {2}}
   ZDD b = s3;               // {{3}}

   // join は各集合の和集合を作る
   ZDD j = a * b;            // {{1, 3}, {2, 3}}

   // & は集合族の積集合（共通部分）
   ZDD i = a & b;            // ∅（共通する集合がない）

実践例: グラフの独立集合列挙
----------------------------

.. code-block:: cpp

   #include <sbdd2/sbdd2.hpp>
   #include <sbdd2/zdd_helper.hpp>
   using namespace sbdd2;

   int main() {
       DDManager mgr;

       // 4頂点のパスグラフ: 1-2-3-4
       for (int i = 0; i < 4; ++i) mgr.new_var();

       // 全ての部分集合
       ZDD all = get_power_set(mgr, 4);  // 2^4 = 16集合

       // 隣接する頂点を同時に選ばない制約
       ZDD bad12 = ZDD::singleton(mgr, 1) * ZDD::singleton(mgr, 2);  // {{1,2}}
       ZDD bad23 = ZDD::singleton(mgr, 2) * ZDD::singleton(mgr, 3);  // {{2,3}}
       ZDD bad34 = ZDD::singleton(mgr, 3) * ZDD::singleton(mgr, 4);  // {{3,4}}

       // 禁止パターンを含む集合を除外
       ZDD bad = all * bad12 + all * bad23 + all * bad34;
       ZDD independent = all - bad;

       std::cout << "独立集合の数: " << independent.card() << std::endl;

       // 最大独立集合のサイズ
       std::cout << "最大サイズ: " << independent.len() << std::endl;

       // 列挙
       for (const auto& s : independent.enumerate()) {
           std::cout << "{";
           for (size_t i = 0; i < s.size(); ++i) {
               if (i > 0) std::cout << ", ";
               std::cout << s[i];
           }
           std::cout << "}" << std::endl;
       }

       return 0;
   }
