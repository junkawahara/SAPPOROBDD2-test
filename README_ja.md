# SAPPOROBDD 2.0

[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

[English README](README.md)

SAPPOROBDD 2.0 は、C++11 で記述された高性能 BDD（二分決定図）/ ZDD（ゼロ抑制二分決定図）ライブラリです。[SAPPOROBDD++](https://github.com/junkawahara/SAPPOROBDD) をベースに完全に再設計されており、128ビットノード構造、インスタンスベース設計、スレッドセーフティなどのモダンなアーキテクチャを備えています。

## 主要機能

### コアDDクラス
- **BDD** / **ZDD** -- 演算子オーバーロードを備えた既約決定図
- **UnreducedBDD** / **UnreducedZDD** -- トップダウン構築用の非既約DD。`reduce()` で既約形式に変換可能
- **QDD** -- 準既約決定図（Quasi-reduced DD）

### 拡張DDクラス
- **MTBDD\<T\>** / **MTZDD\<T\>** -- 任意の終端値を持つマルチターミナルDD
- **MVBDD** / **MVZDD** -- 各変数がk個の値を取る多値DD
- **PiDD** -- 置換DD（Permutation DD）
- **SeqBDD** -- 系列BDD（Sequence BDD）

### グラフ・組合せ
- **GBase** -- フロンティア法によるグラフ上のパス・閉路列挙
- **BDDCT** -- コスト制約付き列挙

### TdZdd統合
- トップダウンDD構築のための互換Specインターフェース
- BFS、DFS、並列（OpenMP）ビルダー

### ZDD拡張機能
- イテレータ: 辞書順、重み順、ランダム順
- ランキング/アンランキング用インデックス構造
- GMP または BigInt フォールバックによる厳密カウント
- ヘルパー関数: べき集合、重みフィルタリング、ランダム生成

### I/Oフォーマット
- バイナリ、テキスト、DOT（Graphviz）、Knuth、Graphillion、lib\_bdd、SVG

### アーキテクチャ
- 128ビットノード構造（44ビットアーク、16ビット参照カウント、20ビット変数番号）
- 否定枝によるO(1) NOT演算
- 二次探索による内部ハッシュ法
- mutexによるスレッドセーフ操作

## ビルド要件

- CMake 3.1以上
- C++11対応コンパイラ（GCC 4.8+, Clang 3.4+, MSVC 2015+）
- （オプション）Google Test -- テスト実行用
- （オプション）GMP -- 任意精度の厳密カウント用

## ビルド・インストール

```bash
git clone <REPOSITORY_URL>
cd SAPPOROBDD2

mkdir build && cd build
cmake ..
make -j4

# テストを実行（オプション）
./tests/sbdd2_tests

# インストール（オプション）
sudo make install
```

### CMakeプロジェクトでの利用

```cmake
add_subdirectory(path/to/SAPPOROBDD2)
target_link_libraries(your_target sbdd2_static)
```

## クイックスタート

```cpp
#include <sbdd2/sbdd2.hpp>
using namespace sbdd2;

int main() {
    DDManager mgr;

    // 変数を作成
    for (int i = 0; i < 5; ++i) mgr.new_var();

    // BDD演算
    BDD x1 = mgr.var_bdd(1);
    BDD x2 = mgr.var_bdd(2);
    BDD f = (x1 & x2) | (~x1 & x2);  // x2
    std::cout << "充足割当数: " << f.count(5) << std::endl;

    // ZDD演算
    ZDD s1 = ZDD::singleton(mgr, 1);  // {{1}}
    ZDD s2 = ZDD::singleton(mgr, 2);  // {{2}}
    ZDD family = s1 + s2;             // {{1}, {2}}
    ZDD joined = s1 * s2;             // {{1, 2}}
    std::cout << "集合数: " << family.card() << std::endl;

    return 0;
}
```

## ドキュメント

**[オンラインドキュメント](https://junkawahara.github.io/SAPPOROBDD2-test/)**

ドキュメントは `docs/` ディレクトリからローカルでもビルドできます（Sphinx + Breathe）。

```bash
cd docs
doxygen Doxyfile
LC_ALL=C python3 -m sphinx -b html . _build/html
```

ドキュメントの内容:
- [クイックスタート](docs/quickstart.rst) -- 入門ガイド
- [チュートリアル](docs/tutorial.rst) -- 実践的な使用例
- [APIリファレンス](docs/api/index.rst) -- 完全なAPI仕様
- [移行ガイド](docs/migration.rst) -- SAPPOROBDD++からの移行
- [サンプルプログラム](docs/examples/index.rst) -- N-Queens、ハミルトン閉路、CNF SAT 等

## ライセンス

本プロジェクトは MIT License の下で公開されています。詳細は [LICENSE](LICENSE) を参照してください。

Copyright (c) <AUTHOR_NAME>
