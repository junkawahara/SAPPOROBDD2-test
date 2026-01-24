拡張DDクラス
============

UnreducedBDD
------------

非縮約BDD。トップダウン構築法などで使用します。

.. doxygenclass:: sbdd2::UnreducedBDD
   :members:
   :undoc-members:

UnreducedZDD
------------

非縮約ZDD。トップダウン構築法などで使用します。

.. doxygenclass:: sbdd2::UnreducedZDD
   :members:
   :undoc-members:

QDD
---

準縮約DD（Quasi-reduced Decision Diagram）。
ノード共有はあるが、飛び越し規則は適用しません。

.. doxygenclass:: sbdd2::QDD
   :members:
   :undoc-members:

PiDD
----

順列DD（Permutation DD）。順列の集合を表現します。

.. doxygenclass:: sbdd2::PiDD
   :members:
   :undoc-members:

**PiDD定数**:

.. cpp:var:: constexpr int PiDD::MAX_VAR = 254

   PiDDで扱える最大変数数

**主要メソッド**:

* ``init(mgr)`` - PiDDシステムを初期化
* ``new_var()`` - 新しい変数を追加
* ``var_used()`` - 使用中の変数数を取得
* ``empty()`` - 空集合を返す
* ``base()`` - 恒等順列のみを含む集合
* ``single(x, y)`` - 単一の転置(x,y)を含む集合
* ``swap(x, y)`` - 要素xとyを交換
* ``cofact(x, y)`` - 余因子
* ``odd()`` - 奇順列のみ抽出
* ``even()`` - 偶順列のみ抽出
* ``card()`` - 順列数をカウント
* ``enumerate()`` - 全順列を列挙

SeqBDD
------

シーケンスBDD。系列（シーケンス）の集合を表現します。

.. doxygenclass:: sbdd2::SeqBDD
   :members:
   :undoc-members:

**主要メソッド**:

* ``empty(mgr)`` - 空集合を返す
* ``base(mgr)`` - 空シーケンスのみを含む集合
* ``single(mgr, v)`` - 単一要素シーケンス {v}
* ``onset(v)`` - vで始まるシーケンス（vを除去）
* ``offset(v)`` - vで始まらないシーケンス
* ``onset0(v)`` - vで始まるシーケンス（vを保持）
* ``push(v)`` - 先頭にvを追加
* ``card()`` - シーケンス数をカウント
* ``lit()`` - 全シーケンスのリテラル総数
* ``len()`` - 最長シーケンスの長さ
* ``enumerate()`` - 全シーケンスを列挙

使用例
------

UnreducedBDDからBDDへの変換
~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   DDManager mgr;
   mgr.new_var();

   // 非縮約BDDでノードを構築
   UnreducedBDD zero = UnreducedBDD::zero(mgr);
   UnreducedBDD one = UnreducedBDD::one(mgr);
   UnreducedBDD node = UnreducedBDD::node(mgr, 1, zero, one);

   // 縮約してBDDに変換
   BDD bdd = node.reduce();

   // または変換コンストラクタを使用
   BDD bdd2(node);

QDDの使用
~~~~~~~~~

.. code-block:: cpp

   DDManager mgr;
   mgr.new_var();
   mgr.new_var();

   QDD zero = QDD::zero(mgr);
   QDD one = QDD::one(mgr);

   // QDDノードを作成
   QDD node = QDD::node(mgr, 1, zero, one);

   // BDD/ZDDへの変換
   BDD bdd = node.to_bdd();
   ZDD zdd = node.to_zdd();

SeqBDDの使用
~~~~~~~~~~~~

.. code-block:: cpp

   DDManager mgr;
   for (int i = 0; i < 5; ++i) mgr.new_var();

   // シーケンス {1} を作成
   SeqBDD s1 = SeqBDD::single(mgr, 1);

   // シーケンス {2} を作成
   SeqBDD s2 = SeqBDD::single(mgr, 2);

   // 和（両方のシーケンスを含む集合）
   SeqBDD both = s1 + s2;

   // push操作（先頭に要素を追加）
   SeqBDD pushed = s1.push(3);  // {3, 1}

   // カウント
   double count = both.card();

PiDDの使用
~~~~~~~~~~

.. code-block:: cpp

   DDManager mgr;

   // PiDDシステムを初期化
   PiDD::init(mgr);

   // 変数を作成（順列の要素数）
   PiDD::new_var();
   PiDD::new_var();
   PiDD::new_var();

   // 恒等順列
   PiDD id = PiDD::single();

   // 転置 (1,2) を含む順列
   PiDD trans12 = PiDD::single(1, 2);  // 1と2を交換

   // 全ての順列を構築
   PiDD all_perms = PiDD::single();
   for (int i = 1; i <= PiDD::var_used(); ++i) {
       for (int j = i + 1; j <= PiDD::var_used(); ++j) {
           all_perms = all_perms + all_perms * PiDD::single(i, j);
       }
   }

   // 偶順列のみを抽出
   PiDD even_perms = all_perms.even();

   // 奇順列のみを抽出
   PiDD odd_perms = all_perms.odd();

   // 順列数を数える
   std::cout << "全順列数: " << all_perms.card() << std::endl;
   std::cout << "偶順列数: " << even_perms.card() << std::endl;
   std::cout << "奇順列数: " << odd_perms.card() << std::endl;

   // 全順列を列挙
   auto perms = all_perms.enumerate();
   for (const auto& p : perms) {
       std::cout << "(";
       for (auto e : p) std::cout << e << " ";
       std::cout << ")" << std::endl;
   }

PiDDによる順列群の操作
~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   DDManager mgr;
   PiDD::init(mgr);

   for (int i = 0; i < 4; ++i) PiDD::new_var();

   // 巡回群 C4 を構築
   PiDD c4 = PiDD::single();  // 恒等置換
   PiDD cycle = PiDD::single(1, 2) * PiDD::single(2, 3) * PiDD::single(3, 4);
   c4 = c4 + cycle;
   c4 = c4 + cycle * cycle;
   c4 = c4 + cycle * cycle * cycle;

   std::cout << "C4の位数: " << c4.card() << std::endl;

   // 対称群 S4 の部分群を生成
   PiDD generators = PiDD::single(1, 2) + PiDD::single(1, 2) * PiDD::single(3, 4);
   PiDD subgroup = PiDD::single();
   PiDD prev;
   do {
       prev = subgroup;
       subgroup = subgroup + subgroup * generators;
   } while (subgroup != prev);

   std::cout << "生成された部分群の位数: " << subgroup.card() << std::endl;

SeqBDDによる文字列の集合操作
~~~~~~~~~~~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   DDManager mgr;
   for (int i = 1; i <= 26; ++i) mgr.new_var();  // a-zを表す

   // "abc" = {1, 2, 3} を作成
   SeqBDD abc = SeqBDD::single(mgr, 1).push(2).push(3);  // 逆順に追加
   // 実際は {3, 2, 1} になるので注意

   // 正しく "abc" を作成するには
   SeqBDD a = SeqBDD::single(mgr, 1);
   SeqBDD b = SeqBDD::single(mgr, 2);
   SeqBDD c = SeqBDD::single(mgr, 3);
   SeqBDD abc_correct = a * b * c;  // 連接

   // "ab" または "bc" を含む文字列の集合
   SeqBDD ab = a * b;
   SeqBDD bc = b * c;
   SeqBDD patterns = ab + bc;

   // 統計情報
   std::cout << "パターン数: " << patterns.card() << std::endl;
   std::cout << "リテラル総数: " << patterns.lit() << std::endl;
   std::cout << "最大長: " << patterns.len() << std::endl;

   // 全シーケンスを列挙
   auto seqs = patterns.enumerate();
   for (const auto& s : seqs) {
       std::cout << "  [";
       for (auto e : s) std::cout << e << " ";
       std::cout << "]" << std::endl;
   }
