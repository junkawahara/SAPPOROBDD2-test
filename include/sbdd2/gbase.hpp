/**
 * @file gbase.hpp
 * @brief GBase（グラフベース）クラスの定義。フロンティア法によるパス・サイクル列挙を行う。
 *
 * グラフ構造を保持し、simpath アルゴリズムを用いて
 * 指定頂点間の単純パスや単純サイクルを ZDD として列挙する。
 *
 * @see ZDD
 * @see DDManager
 */

// SAPPOROBDD 2.0 - GBase (Graph Base) class
// MIT License

#ifndef SBDD2_GBASE_HPP
#define SBDD2_GBASE_HPP

#include "zdd.hpp"
#include <vector>
#include <string>
#include <cstdio>

namespace sbdd2 {

/** @brief グラフの頂点番号型（最大65534頂点） */
using GBVertex = std::uint16_t;

/** @brief グラフの辺番号型（最大65534辺） */
using GBEdge = std::uint16_t;

/**
 * @brief グラフベースクラス。フロンティア法によるパス・サイクル列挙を行う。
 *
 * グラフの頂点と辺を管理し、simpath アルゴリズムを用いて
 * 単純パスや単純サイクルの集合を ZDD として構築する。
 * 辺の固定制約やハミルトン制約にも対応する。
 *
 * @see ZDD
 * @see DDManager
 * @see BDDCT
 */
class GBase {
public:
    /** @brief 辺の制約なし */
    static constexpr char FIX_NONE = 0;
    /** @brief 辺を使用しない制約 */
    static constexpr char FIX_0 = 1;  // Edge must NOT be used
    /** @brief 辺を必ず使用する制約 */
    static constexpr char FIX_1 = 2;  // Edge MUST be used

    /**
     * @brief グラフの頂点を表す構造体
     */
    struct Vertex {
        int tmp;         /**< @brief 作業用一時変数 */
        GBEdge degree;   /**< @brief 頂点の次数 */

        /** @brief デフォルトコンストラクタ。tmp=0, degree=0 で初期化する。 */
        Vertex() : tmp(0), degree(0) {}
    };

    /**
     * @brief グラフの辺を表す構造体
     */
    struct Edge {
        GBVertex endpoints[2];  /**< @brief 辺の両端点 */
        int tmp;                /**< @brief 作業用一時変数 */
        int cost;               /**< @brief 辺のコスト */
        char io[2];             /**< @brief 端点の入出力フラグ */
        char preset;            /**< @brief 辺の固定制約（FIX_NONE/FIX_0/FIX_1） */
        GBVertex mate_width;    /**< @brief mate 配列の幅 */

        /** @brief デフォルトコンストラクタ。コスト1、制約なしで初期化する。 */
        Edge() : tmp(0), cost(1), preset(FIX_NONE), mate_width(0) {
            endpoints[0] = endpoints[1] = 0;
            io[0] = io[1] = 0;
        }
    };

private:
    DDManager* manager_;
    GBVertex n_vertices_;
    GBEdge n_edges_;
    std::vector<Vertex> vertices_;
    std::vector<Edge> edges_;
    bool hamilton_mode_;
    ZDD constraint_;
    GBEdge last_in_;

public:
    /**
     * @brief デフォルトコンストラクタ。マネージャなしで初期化する。
     */
    GBase();

    /**
     * @brief DDManager を指定して構築するコンストラクタ
     * @param mgr 使用する DDManager への参照
     */
    explicit GBase(DDManager& mgr);

    /**
     * @brief デストラクタ
     */
    ~GBase() = default;

    /** @brief コピーコンストラクタ */
    GBase(const GBase&) = default;
    /** @brief ムーブコンストラクタ */
    GBase(GBase&&) noexcept = default;
    /** @brief コピー代入演算子 */
    GBase& operator=(const GBase&) = default;
    /** @brief ムーブ代入演算子 */
    GBase& operator=(GBase&&) noexcept = default;

    /**
     * @brief グラフの頂点数と辺数を指定して初期化する
     * @param n_vertices 頂点数
     * @param n_edges 辺数
     * @return 初期化に成功した場合 true
     */
    bool init(GBVertex n_vertices, GBEdge n_edges);

    /**
     * @brief 辺の両端点とコストを設定する
     * @param e 辺番号
     * @param v1 端点1
     * @param v2 端点2
     * @param cost 辺のコスト（デフォルト値: 1）
     * @return 設定に成功した場合 true
     */
    bool set_edge(GBEdge e, GBVertex v1, GBVertex v2, int cost = 1);

    /**
     * @brief ファイルポインタからグラフデータを読み込む
     * @param fp 入力ファイルポインタ
     * @return 読み込みに成功した場合 true
     */
    bool import(FILE* fp);

    /**
     * @brief ファイル名を指定してグラフデータを読み込む
     * @param filename 入力ファイル名
     * @return 読み込みに成功した場合 true
     */
    bool import(const std::string& filename);

    /**
     * @brief 格子グラフを生成する
     * @param rows 行数
     * @param cols 列数
     * @return 生成に成功した場合 true
     */
    bool set_grid(int rows, int cols);

    /**
     * @brief 未使用の頂点を除去してグラフを詰める
     * @return 処理に成功した場合 true
     */
    bool pack();

    /**
     * @brief 辺に固定制約を設定する
     * @param e 辺番号
     * @param fix_type 制約の種類（FIX_NONE, FIX_0, FIX_1）
     */
    void fix_edge(GBEdge e, char fix_type);

    /**
     * @brief ハミルトンパス・サイクルモードの有効/無効を設定する
     * @param enable true でハミルトンモードを有効にする
     */
    void set_hamilton(bool enable);

    /**
     * @brief 追加の ZDD 制約を設定する
     * @param constraint 制約を表す ZDD
     * @see ZDD
     */
    void set_constraint(const ZDD& constraint);

    /**
     * @brief 指定した2頂点間の全単純パスを ZDD として列挙する
     * @param start 始点の頂点番号
     * @param end 終点の頂点番号
     * @return 全単純パスの集合を表す ZDD
     * @see ZDD
     */
    ZDD sim_paths(GBVertex start, GBVertex end);

    /**
     * @brief グラフ上の全単純サイクルを ZDD として列挙する
     * @return 全単純サイクルの集合を表す ZDD
     * @see ZDD
     */
    ZDD sim_cycles();

    /**
     * @brief 辺番号に対応する BDD 変数番号を取得する
     * @param e 辺番号
     * @return 対応する BDD 変数番号
     */
    int bdd_var_of_edge(GBEdge e) const;

    /**
     * @brief BDD 変数番号に対応する辺番号を取得する
     * @param var BDD 変数番号
     * @return 対応する辺番号
     */
    GBEdge edge_of_bdd_var(int var) const;

    /**
     * @brief 頂点数を取得する
     * @return 頂点数
     */
    GBVertex vertex_count() const { return n_vertices_; }

    /**
     * @brief 辺数を取得する
     * @return 辺数
     */
    GBEdge edge_count() const { return n_edges_; }

    /**
     * @brief 使用中の DDManager へのポインタを取得する
     * @return DDManager へのポインタ
     * @see DDManager
     */
    DDManager* manager() const { return manager_; }

    /**
     * @brief グラフ情報を標準出力に表示する
     */
    void print() const;

    /**
     * @brief グラフ情報を文字列として取得する
     * @return グラフ情報の文字列表現
     */
    std::string to_string() const;

private:
    // Internal helpers for simpath algorithm
    ZDD simpath_rec(GBEdge e, GBVertex start, GBVertex end,
                    std::vector<GBVertex>& mate);
    void update_mate(std::vector<GBVertex>& mate, GBVertex v1, GBVertex v2);
};

} // namespace sbdd2

#endif // SBDD2_GBASE_HPP
