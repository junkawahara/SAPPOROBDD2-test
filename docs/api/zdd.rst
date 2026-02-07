ZDDクラス
=========

.. seealso::

   ZDDの詳細ガイドは :doc:`../guide/zdd_guide` を参照してください。
   グラフ問題への応用は :doc:`../application/graph_problems`、
   組合せ列挙パターンは :doc:`../application/combinatorial_enumeration` を参照してください。

ZDD
---

ゼロ抑制二分決定図（Zero-suppressed Binary Decision Diagram）クラス。
集合族を効率的に表現します。

.. doxygenclass:: sbdd2::ZDD
   :members:
   :undoc-members:

使用例
------

基本的な使い方
~~~~~~~~~~~~~~

.. code-block:: cpp

   DDManager mgr;
   for (int i = 0; i < 5; ++i) mgr.new_var();

   // 単一要素集合
   ZDD s1 = ZDD::singleton(mgr, 1);  // {{1}}
   ZDD s2 = ZDD::singleton(mgr, 2);  // {{2}}

   // 集合族の和
   ZDD family = s1 + s2;  // {{1}, {2}}

   // カウント
   double count = family.card();  // 2.0

onset/offset演算
~~~~~~~~~~~~~~~~

.. code-block:: cpp

   ZDD family = s1 + s2 + s1.product(s2);  // {{1}, {2}, {1,2}}

   // 要素1を含む集合（1を除去）
   ZDD with_1 = family.onset(1);  // {{}, {2}}

   // 要素1を含まない集合
   ZDD without_1 = family.offset(1);  // {{2}}

   // 要素1を含む集合（1を保持）
   ZDD with_1_keep = family.onset0(1);  // {{1}, {1,2}}

直積演算
~~~~~~~~

.. code-block:: cpp

   ZDD a = ZDD::singleton(mgr, 1) + ZDD::singleton(mgr, 2);  // {{1}, {2}}
   ZDD b = ZDD::singleton(mgr, 3) + ZDD::singleton(mgr, 4);  // {{3}, {4}}

   // 直積: 各集合の和を取る
   ZDD prod = a.product(b);  // {{1,3}, {1,4}, {2,3}, {2,4}}

列挙
~~~~

.. code-block:: cpp

   ZDD family = s1 + s2 + s1.product(s2);

   // 全集合を列挙
   auto sets = family.enumerate();
   for (const auto& s : sets) {
       std::cout << "{";
       for (auto elem : s) std::cout << elem << " ";
       std::cout << "}" << std::endl;
   }
   // 出力:
   // {1 }
   // {2 }
   // {1 2 }

   // 1つの集合を取得
   auto one = family.one_set();

変数操作
~~~~~~~~

.. code-block:: cpp

   ZDD s12 = ZDD::singleton(mgr, 1).product(ZDD::singleton(mgr, 2));  // {{1,2}}

   // 2つの変数を交換
   ZDD swapped = s12.swap(1, 2);  // {{1,2}} (この場合は同じ)

   // シフト演算子
   ZDD shifted = s1 << 2;  // {{3}} (変数番号を+2)
   ZDD unshifted = shifted >> 2;  // {{1}} (変数番号を-2)

制限・許可演算
~~~~~~~~~~~~~~

.. code-block:: cpp

   ZDD f = s1 + s2 + s1.product(s2);  // {{1}, {2}, {1,2}}
   ZDD g = s1;  // {{1}}

   // restrict: gで許可される変数のみを含む集合に制限
   ZDD restricted = f.restrict(g);

   // permit: gで指定される変数の部分集合のみを許可
   ZDD permitted = f.permit(g);

   // permit_sym: 最初のn個の変数で許可
   ZDD perm_sym = f.permit_sym(2);

統計情報
~~~~~~~~

.. code-block:: cpp

   ZDD family = s1 + s2 + s1.product(s2);  // {{1}, {2}, {1,2}}

   // リテラル総数（全集合の要素数の合計）
   uint64_t lits = family.lit();  // 4 (1+1+2)

   // 最大集合サイズ
   uint64_t max_len = family.len();  // 2

   // 必ず現れる要素
   ZDD always_elems = family.always();  // {{}} (共通要素なし)

対称性チェック
~~~~~~~~~~~~~~

.. code-block:: cpp

   ZDD u = s1 + s2;  // {{1}, {2}}

   // 対称性チェック: 1と2を入れ替えても同じか
   int is_sym = u.sym_chk(1, 2);  // 1 (対称)

   // 対称グループを取得
   ZDD sym_groups = u.sym_grp();

   // 変数vと対称な変数の集合
   ZDD sym_with_1 = u.sym_set(1);

インプリケーションチェック
~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   ZDD s12 = ZDD::singleton(mgr, 1).product(ZDD::singleton(mgr, 2));  // {{1,2}}

   // 1が含まれるなら2も含まれるか
   int implies = s12.imply_chk(1, 2);  // 1 (1 => 2)

   // 双方向インプリケーション
   int co_implies = s12.co_imply_chk(1, 2);  // 1

   // vがインプライする変数の集合
   ZDD implied_by_1 = s12.imply_set(1);

多項式演算
~~~~~~~~~~

.. code-block:: cpp

   ZDD poly = s1 + s2;  // {{1}, {2}}

   // 多項式かどうか（複数の集合を持つか）
   int is_polynomial = poly.is_poly();  // 1

   // 最小因子（共通要素）
   ZDD div = poly.divisor();

Meet演算
~~~~~~~~

.. code-block:: cpp

   ZDD a = ZDD::singleton(mgr, 1).product(ZDD::singleton(mgr, 2));  // {{1,2}}
   ZDD b = ZDD::singleton(mgr, 2).product(ZDD::singleton(mgr, 3));  // {{2,3}}

   // Meet: 要素ごとの積集合
   ZDD meet_result = zdd_meet(a, b);  // {{2}} ({1,2} ∩ {2,3} = {2})

厳密カウント（GMP / BigInt）
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

大規模な集合族では、集合数がdouble精度（2^53まで）を超える場合があります。
GMPまたはBigIntライブラリが利用可能な場合、``exact_count()`` で任意精度の厳密なカウントが可能です。

.. code-block:: cpp

   // 20変数のべき集合 = 2^20 = 1048576 個の集合
   ZDD ps = get_power_set(mgr, 20);

   // 通常のカウント（double）
   double approx = ps.card();  // 1048576.0

   // 厳密カウント（文字列で返す）
   #if defined(SBDD2_HAS_GMP) || defined(SBDD2_HAS_BIGINT)
   std::string exact = ps.exact_count();  // "1048576"

   // より大規模な例（60変数）
   // 2^60 = 1152921504606846976 はdouble精度を超える
   ZDD large_ps = get_power_set(mgr, 60);
   std::string large_count = large_ps.exact_count();
   // "1152921504606846976"
   #endif

.. note::
   ``exact_count()`` は ``SBDD2_HAS_GMP`` または ``SBDD2_HAS_BIGINT`` が定義されている場合に使用可能です。
   CMakeがGMPを自動検出し、見つからない場合はBigIntライブラリをフォールバックとして使用します。

ZDDイテレータ
-------------

ZDDに含まれる集合を様々な順序で列挙するためのイテレータクラスを提供します。

DictIterator（辞書順イテレータ）
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. doxygenclass:: sbdd2::DictIterator
   :members:
   :undoc-members:

ZDD内の集合を辞書順で列挙します。O(1)のメモリオーバーヘッドで効率的に列挙できます。

.. code-block:: cpp

   ZDD zdd = ...;

   // 辞書順で列挙
   for (auto it = zdd.dict_begin(); it != zdd.dict_end(); ++it) {
       std::set<bddvar> s = *it;
       for (auto elem : s) {
           std::cout << elem << " ";
       }
       std::cout << std::endl;
   }

   // 逆順で列挙
   for (auto it = zdd.dict_rbegin(); it != zdd.dict_rend(); ++it) {
       // ...
   }

WeightIterator（重み順イテレータ）
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. doxygenclass:: sbdd2::WeightIterator
   :members:
   :undoc-members:

ZDD内の集合を重み順（昇順または降順）で列挙します。
内部的にZDDのコピーを保持し、各イテレーションで現在の最小/最大集合を取り除きます。

.. code-block:: cpp

   ZDD zdd = ...;
   std::vector<int64_t> weights = {0, 10, 20, 30};  // weights[var] = 変数varの重み

   // 重み昇順で列挙
   for (auto it = zdd.weight_min_begin(weights); it != zdd.weight_min_end(); ++it) {
       std::set<bddvar> s = *it;
       // sは現在最も軽い集合
   }

   // 重み降順で列挙
   for (auto it = zdd.weight_max_begin(weights); it != zdd.weight_max_end(); ++it) {
       std::set<bddvar> s = *it;
       // sは現在最も重い集合
   }

RandomIterator（ランダム順イテレータ）
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. doxygenclass:: sbdd2::RandomIterator
   :members:
   :undoc-members:

ZDD内の集合をランダム順で列挙します（重複なし）。
内部的にZDDのコピーを保持し、各イテレーションでサンプルした集合を取り除きます。

.. code-block:: cpp

   ZDD zdd = ...;
   std::mt19937 rng(42);  // 乱数生成器

   // ランダム順で列挙（重複なし）
   for (auto it = zdd.random_begin(rng); it != zdd.random_end(); ++it) {
       std::set<bddvar> s = *it;
       // sはランダムに選ばれた集合
   }

ZDDインデックス
---------------

ZDDの効率的なアクセスのためのインデックスデータ構造です。

ZDDIndexData
~~~~~~~~~~~~

.. doxygenstruct:: sbdd2::ZDDIndexData
   :members:

各ノードから1終端までの経路数をdoubleで格納します。2^53までの精度で正確な値を保持できます。

**メンバ変数**:

* ``level_nodes`` - レベルごとのノードリスト
* ``node_to_idx`` - ノードからレベル内インデックスへのマッピング
* ``count_cache`` - ノードから経路数へのマッピング
* ``height`` - ZDDの高さ（ルートノードのレベル）

ZDDExactIndexData（厳密整数版）
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

各ノードから1終端までの経路数を厳密整数型（GMP の ``mpz_class`` または ``bigint::BigInt``）で格納します。

**メンバ変数**:

* ``level_nodes`` - レベルごとのノードリスト
* ``node_to_idx`` - ノードからレベル内インデックスへのマッピング
* ``count_cache`` - ノードから経路数へのマッピング（``exact_int_t`` 型）
* ``height`` - ZDDの高さ

.. note::
   ``ZDDExactIndexData`` は ``SBDD2_HAS_GMP`` または ``SBDD2_HAS_BIGINT`` が定義されている場合に使用可能です。
   どちらもインストールされていない環境ではコンパイルされません。

インデックスの使用
~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   DDManager mgr;
   for (int i = 1; i <= 10; ++i) mgr.new_var();

   ZDD zdd = get_power_set(mgr, 10);

   // インデックスを構築
   zdd.build_index();

   // インデックスを使った高速カウント
   double count = zdd.indexed_count();

   // 辞書順でi番目の集合を取得（O(n)）
   auto set5 = zdd.get_set(5);

   // 集合の辞書順での順位を取得
   std::vector<bddvar> query = {1, 3, 5};
   uint64_t order = zdd.order_of(query);

ハッシュ関数
~~~~~~~~~~~~

.. doxygenstruct:: sbdd2::ArcHash
   :members:

.. doxygenstruct:: sbdd2::ArcEqual
   :members:

これらの構造体はZDDインデックス内でArcをハッシュテーブルのキーとして使用するために定義されています。

イテレータの使用例
------------------

辞書順アクセス
~~~~~~~~~~~~~~

.. code-block:: cpp

   DDManager mgr;
   for (int i = 1; i <= 5; ++i) mgr.new_var();

   ZDD family = ZDD::singleton(mgr, 1) + ZDD::singleton(mgr, 2) +
                ZDD::singleton(mgr, 1).product(ZDD::singleton(mgr, 2));
   // family = {{1}, {2}, {1,2}}

   std::cout << "辞書順:" << std::endl;
   for (auto it = family.dict_begin(); it != family.dict_end(); ++it) {
       std::set<bddvar> s = *it;
       std::cout << "  {";
       for (auto e : s) std::cout << e << " ";
       std::cout << "}" << std::endl;
   }
   // 出力:
   //   {1 }
   //   {1 2 }
   //   {2 }

重み順アクセス
~~~~~~~~~~~~~~

.. code-block:: cpp

   DDManager mgr;
   for (int i = 1; i <= 5; ++i) mgr.new_var();

   ZDD family = get_power_set(mgr, 5);

   // 重みを設定
   std::vector<int64_t> weights = {0, 10, 20, 5, 15, 25};
   // weights[i] = 変数iの重み

   std::cout << "重み昇順で最初の5つ:" << std::endl;
   int count = 0;
   for (auto it = family.weight_min_begin(weights);
        it != family.weight_min_end() && count < 5;
        ++it, ++count) {
       std::set<bddvar> s = *it;
       int64_t w = 0;
       for (auto e : s) w += weights[e];
       std::cout << "  重み=" << w << ": {";
       for (auto e : s) std::cout << e << " ";
       std::cout << "}" << std::endl;
   }

ランダムサンプリング
~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   DDManager mgr;
   for (int i = 1; i <= 10; ++i) mgr.new_var();

   ZDD family = get_power_set(mgr, 10);

   std::mt19937 rng(42);

   // 10個のランダムな集合を取得（重複なし）
   std::cout << "ランダムに10個:" << std::endl;
   int count = 0;
   for (auto it = family.random_begin(rng);
        it != family.random_end() && count < 10;
        ++it, ++count) {
       std::set<bddvar> s = *it;
       std::cout << "  {";
       for (auto e : s) std::cout << e << " ";
       std::cout << "}" << std::endl;
   }

インデックスを使った操作
~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   DDManager mgr;
   for (int i = 1; i <= 20; ++i) mgr.new_var();

   ZDD family = get_power_set_with_card(mgr, 20, 5);  // 20C5 = 15504 集合

   // インデックスを構築（一度だけ）
   family.build_index();

   // 辞書順で1000番目の集合を取得（O(変数数)）
   auto set1000 = family.get_set(1000);

   // その集合の順位を確認
   uint64_t order = family.order_of(set1000);
   // order == 1000

   // 高速カウント
   double count = family.indexed_count();
   // count == 15504.0
