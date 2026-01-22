ZDDクラス
=========

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
   ZDD s1 = ZDD::single(mgr, 1);  // {{1}}
   ZDD s2 = ZDD::single(mgr, 2);  // {{2}}

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

   ZDD a = ZDD::single(mgr, 1) + ZDD::single(mgr, 2);  // {{1}, {2}}
   ZDD b = ZDD::single(mgr, 3) + ZDD::single(mgr, 4);  // {{3}, {4}}

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

   ZDD s12 = ZDD::single(mgr, 1).product(ZDD::single(mgr, 2));  // {{1,2}}

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

   ZDD s12 = ZDD::single(mgr, 1).product(ZDD::single(mgr, 2));  // {{1,2}}

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

   ZDD a = ZDD::single(mgr, 1).product(ZDD::single(mgr, 2));  // {{1,2}}
   ZDD b = ZDD::single(mgr, 2).product(ZDD::single(mgr, 3));  // {{2,3}}

   // Meet: 要素ごとの積集合
   ZDD meet_result = zdd_meet(a, b);  // {{2}} ({1,2} ∩ {2,3} = {2})

厳密カウント（GMP）
~~~~~~~~~~~~~~~~~~~

大規模な集合族では、集合数がdouble精度（2^53まで）を超える場合があります。
GMPがインストールされている場合、``exact_count()`` で任意精度の厳密なカウントが可能です。

.. code-block:: cpp

   // 20変数のべき集合 = 2^20 = 1048576 個の集合
   ZDD ps = get_power_set(mgr, 20);

   // 通常のカウント（double）
   double approx = ps.card();  // 1048576.0

   // 厳密カウント（GMP使用、文字列で返す）
   #ifdef SBDD2_HAS_GMP
   std::string exact = ps.exact_count();  // "1048576"

   // より大規模な例（60変数）
   // 2^60 = 1152921504606846976 はdouble精度を超える
   ZDD large_ps = get_power_set(mgr, 60);
   std::string large_count = large_ps.exact_count();
   // "1152921504606846976"
   #endif

.. note::
   ``exact_count()`` は ``SBDD2_HAS_GMP`` が定義されている場合のみ使用可能です。
   GMPがインストールされていれば、CMakeが自動検出します。
