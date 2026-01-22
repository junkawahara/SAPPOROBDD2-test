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
