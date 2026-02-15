ZDDヘルパー関数
===============

ZDDの便利な操作関数を提供するヘルパーライブラリです。

.. note::

   これらの関数を使用するには ``#include <sbdd2/zdd_helper.hpp>`` が必要です。

集合生成関数
------------

get_power_set
~~~~~~~~~~~~~

変数集合のべき集合（すべての部分集合）を生成します。

.. doxygenfunction:: sbdd2::get_power_set(DDManager&, const Container&)

.. doxygenfunction:: sbdd2::get_power_set(DDManager&, int)

get_power_set_with_card
~~~~~~~~~~~~~~~~~~~~~~~

指定された濃度（要素数）の部分集合のみを生成します。

.. doxygenfunction:: sbdd2::get_power_set_with_card(DDManager&, const Container&, int)

.. doxygenfunction:: sbdd2::get_power_set_with_card(DDManager&, int, int)

get_single_set
~~~~~~~~~~~~~~

指定された変数を含む単一の集合を生成します。

.. doxygenfunction:: sbdd2::get_single_set

変換関数
--------

make_dont_care
~~~~~~~~~~~~~~

指定された変数をドントケア（あってもなくてもよい）にします。

.. doxygenfunction:: sbdd2::make_dont_care

is_member
~~~~~~~~~

指定された集合がZDDに含まれているかを判定します。

.. doxygenfunction:: sbdd2::is_member

重みフィルタ関数
----------------

各集合の重み（各要素の重みの合計）に基づいてフィルタリングします。
重みはlong long型（64ビット整数）です。

weight_range
~~~~~~~~~~~~

重みが指定範囲内の集合のみを抽出します。

.. doxygenfunction:: sbdd2::weight_range

weight_le / weight_lt
~~~~~~~~~~~~~~~~~~~~~

重みが指定値以下/未満の集合を抽出します。

.. doxygenfunction:: sbdd2::weight_le
.. doxygenfunction:: sbdd2::weight_lt

weight_ge / weight_gt
~~~~~~~~~~~~~~~~~~~~~

重みが指定値以上/より大きい集合を抽出します。

.. doxygenfunction:: sbdd2::weight_ge
.. doxygenfunction:: sbdd2::weight_gt

weight_eq / weight_ne
~~~~~~~~~~~~~~~~~~~~~

重みが指定値と等しい/等しくない集合を抽出します。

.. doxygenfunction:: sbdd2::weight_eq
.. doxygenfunction:: sbdd2::weight_ne

ランダム生成関数
----------------

get_uniformly_random_zdd
~~~~~~~~~~~~~~~~~~~~~~~~

ランダムなZDDを生成します。

.. doxygenfunction:: sbdd2::get_uniformly_random_zdd

get_random_zdd_with_card
~~~~~~~~~~~~~~~~~~~~~~~~

指定された濃度（集合の数）を持つランダムなZDDを生成します。

.. doxygenfunction:: sbdd2::get_random_zdd_with_card

使用例
------

べき集合の生成
~~~~~~~~~~~~~~

.. code-block:: cpp

   #include <sbdd2/zdd_helper.hpp>

   DDManager mgr;
   for (int i = 1; i <= 5; ++i) mgr.new_var();

   // 変数1,2,3のべき集合（2^3 = 8通り）
   std::vector<int> vars = {1, 2, 3};
   ZDD power = get_power_set(mgr, vars);
   std::cout << "集合数: " << power.card() << std::endl;  // 8

   // 変数1から5のべき集合
   ZDD power5 = get_power_set(mgr, 5);
   std::cout << "集合数: " << power5.card() << std::endl;  // 32

   // 濃度2の部分集合のみ（5C2 = 10通り）
   ZDD card2 = get_power_set_with_card(mgr, 5, 2);
   std::cout << "集合数: " << card2.card() << std::endl;  // 10

単一集合の生成
~~~~~~~~~~~~~~

.. code-block:: cpp

   // {1, 3, 5}という単一集合を生成
   std::vector<int> elems = {1, 3, 5};
   ZDD single = get_single_set(mgr, elems);
   std::cout << "集合数: " << single.card() << std::endl;  // 1

ドントケアの適用
~~~~~~~~~~~~~~~~

.. code-block:: cpp

   ZDD s1 = ZDD::singleton(mgr, 1);  // {{1}}

   // 変数2,3をドントケアにする
   // {{1}} -> {{1}, {1,2}, {1,3}, {1,2,3}}
   std::vector<int> dc_vars = {2, 3};
   ZDD expanded = make_dont_care(s1, dc_vars);
   std::cout << "集合数: " << expanded.card() << std::endl;  // 4

メンバーシップ判定
~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   ZDD family = ZDD::singleton(mgr, 1) + ZDD::singleton(mgr, 2) +
                ZDD::singleton(mgr, 1) * ZDD::singleton(mgr, 2);
   // {{1}, {2}, {1,2}}

   std::vector<int> test1 = {1, 2};
   std::vector<int> test2 = {1, 3};

   std::cout << is_member(family, test1) << std::endl;  // 1 (含まれる)
   std::cout << is_member(family, test2) << std::endl;  // 0 (含まれない)

重みフィルタリング
~~~~~~~~~~~~~~~~~~

.. code-block:: cpp

   // 全部分集合を作成
   ZDD all = get_power_set(mgr, 5);

   // 各変数の重み（変数iの重みはweights[i-1]）
   std::vector<long long> weights = {10, 20, 30, 40, 50};

   // 重みが50以下の集合
   ZDD light = weight_le(all, 50, weights);

   // 重みがちょうど60の集合
   ZDD exact60 = weight_eq(all, 60, weights);

   // 重みが30以上80以下の集合
   ZDD range = weight_range(all, 30, 80, weights);

ランダムZDD生成
~~~~~~~~~~~~~~~

.. code-block:: cpp

   #include <random>

   std::mt19937 rng(42);  // シード値42

   // ランダムなZDD（5変数）
   ZDD random_zdd = get_uniformly_random_zdd(mgr, 5, rng);

   // 約100個の集合を持つランダムZDD
   ZDD random_100 = get_random_zdd_with_card(mgr, 5, 100, rng);
