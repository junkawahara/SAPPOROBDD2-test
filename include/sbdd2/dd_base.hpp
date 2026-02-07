/**
 * @file dd_base.hpp
 * @brief BDD/ZDDの共通基底クラス DDBase の定義
 * @copyright MIT License
 *
 * すべてのDD型（BDD, ZDD, QDD等）の共通基底クラスを提供します。
 * 参照カウントの管理、終端判定、基本的なアクセサを実装しています。
 */

// SAPPOROBDD 2.0 - DD Base class
// MIT License

#ifndef SBDD2_DD_BASE_HPP
#define SBDD2_DD_BASE_HPP

#include "types.hpp"
#include "dd_manager.hpp"
#include "dd_node_ref.hpp"
#include <string>

namespace sbdd2 {

/**
 * @brief すべてのDD型の共通基底クラス
 *
 * BDD, ZDD, QDD などすべての決定図クラスの共通基底クラスです。
 * DDManager へのポインタと Arc（辺）を保持し、参照カウントを自動管理します。
 * コピー・ムーブ時に参照カウントを適切に増減します。
 *
 * @see BDD
 * @see ZDD
 * @see QDD
 * @see DDManager
 * @see DDNodeRef
 */
class DDBase {
protected:
    DDManager* manager_;  ///< このDDを管理するマネージャへのポインタ
    Arc arc_;             ///< このDDのルート辺

    /**
     * @brief デフォルトコンストラクタ（無効なDDを生成）
     *
     * manager_ を nullptr、arc_ をデフォルト値で初期化します。
     */
    DDBase() : manager_(nullptr), arc_() {}

    /**
     * @brief マネージャとArcを指定するコンストラクタ
     * @param mgr DDマネージャへのポインタ
     * @param a ルート辺
     *
     * 定数でない辺の場合、参照カウントをインクリメントします。
     */
    DDBase(DDManager* mgr, Arc a) : manager_(mgr), arc_(a) {
        if (manager_ && !arc_.is_constant()) {
            manager_->inc_ref(arc_);
        }
    }

    /**
     * @brief コピーコンストラクタ
     * @param other コピー元のDDBase
     *
     * 定数でない辺の場合、参照カウントをインクリメントします。
     */
    DDBase(const DDBase& other)
        : manager_(other.manager_), arc_(other.arc_)
    {
        if (manager_ && !arc_.is_constant()) {
            manager_->inc_ref(arc_);
        }
    }

    /**
     * @brief ムーブコンストラクタ
     * @param other ムーブ元のDDBase
     *
     * 所有権を移動し、元のオブジェクトを無効化します。
     * 参照カウントは変化しません。
     */
    DDBase(DDBase&& other) noexcept
        : manager_(other.manager_), arc_(other.arc_)
    {
        other.manager_ = nullptr;
        other.arc_ = Arc();
    }

    /**
     * @brief デストラクタ
     *
     * 定数でない辺の場合、参照カウントをデクリメントします。
     */
    ~DDBase() {
        if (manager_ && !arc_.is_constant()) {
            manager_->dec_ref(arc_);
        }
    }

    /**
     * @brief コピー代入演算子
     * @param other コピー元のDDBase
     * @return 自身への参照
     *
     * 古い辺の参照カウントをデクリメントし、新しい辺の参照カウントをインクリメントします。
     */
    DDBase& operator=(const DDBase& other) {
        if (this != &other) {
            if (manager_ && !arc_.is_constant()) {
                manager_->dec_ref(arc_);
            }
            manager_ = other.manager_;
            arc_ = other.arc_;
            if (manager_ && !arc_.is_constant()) {
                manager_->inc_ref(arc_);
            }
        }
        return *this;
    }

    /**
     * @brief ムーブ代入演算子
     * @param other ムーブ元のDDBase
     * @return 自身への参照
     *
     * 古い辺の参照カウントをデクリメントし、所有権を移動します。
     */
    DDBase& operator=(DDBase&& other) noexcept {
        if (this != &other) {
            if (manager_ && !arc_.is_constant()) {
                manager_->dec_ref(arc_);
            }
            manager_ = other.manager_;
            arc_ = other.arc_;
            other.manager_ = nullptr;
            other.arc_ = Arc();
        }
        return *this;
    }

public:
    /**
     * @brief DDが有効かどうかを判定する
     * @return マネージャが設定されていれば true
     */
    bool is_valid() const { return manager_ != nullptr; }

    /**
     * @brief bool への明示的変換演算子
     * @return is_valid() の結果
     * @see is_valid()
     */
    explicit operator bool() const { return is_valid(); }

    /**
     * @brief 終端ノードかどうかを判定する
     * @return 終端ノードであれば true
     */
    bool is_terminal() const { return arc_.is_constant(); }

    /**
     * @brief 0終端かどうかを判定する
     * @return 0終端であれば true
     * @see is_one()
     * @see is_terminal()
     */
    bool is_zero() const {
        if (!arc_.is_constant()) return false;
        bool val = arc_.terminal_value() != arc_.is_negated();
        return !val;
    }

    /**
     * @brief 1終端かどうかを判定する
     * @return 1終端であれば true
     * @see is_zero()
     * @see is_terminal()
     */
    bool is_one() const {
        if (!arc_.is_constant()) return false;
        bool val = arc_.terminal_value() != arc_.is_negated();
        return val;
    }

    /**
     * @brief 根の変数番号を取得する
     * @return 根の変数番号（終端の場合は0）
     */
    bddvar top() const;

    /**
     * @brief DD のID（マネージャ内で一意）を取得する
     * @return 辺のデータ値
     */
    bddindex id() const { return arc_.data; }

    /**
     * @brief 内部のArc（辺）を取得する
     * @return このDDのルート辺
     * @see Arc
     */
    Arc arc() const { return arc_; }

    /**
     * @brief DDマネージャへのポインタを取得する
     * @return DDManagerへのポインタ
     * @see DDManager
     */
    DDManager* manager() const { return manager_; }

    /**
     * @brief DDNodeRef（読み取り専用ノード参照）を取得する
     * @return このDDのルートノードを指すDDNodeRef
     *
     * DDの木構造を走査する際に使用します。
     * DDNodeRef は参照カウントを管理しないため、元の DD より長く生存させないでください。
     *
     * @see DDNodeRef
     */
    DDNodeRef ref() const {
        return DDNodeRef(manager_, arc_);
    }

    /**
     * @brief DD のノード数を取得する
     * @return DDに含まれるノードの数
     */
    std::size_t size() const;

    /**
     * @brief サポート（出現する変数の集合）を取得する
     * @return 変数番号のベクタ（昇順）
     */
    std::vector<bddvar> support() const;

    /**
     * @brief 等価比較演算子
     * @param other 比較対象のDDBase
     * @return 同一のマネージャかつ同一の辺であれば true
     */
    bool operator==(const DDBase& other) const {
        return manager_ == other.manager_ && arc_ == other.arc_;
    }

    /**
     * @brief 非等価比較演算子
     * @param other 比較対象のDDBase
     * @return 等価でなければ true
     */
    bool operator!=(const DDBase& other) const {
        return !(*this == other);
    }

    /**
     * @brief 順序比較演算子（コンテナ格納用）
     * @param other 比較対象のDDBase
     * @return this が other より小さければ true
     *
     * std::set や std::map などの順序付きコンテナで使用するための順序を定義します。
     */
    bool operator<(const DDBase& other) const {
        if (manager_ != other.manager_) {
            return manager_ < other.manager_;
        }
        return arc_.data < other.arc_.data;
    }
};

} // namespace sbdd2

#endif // SBDD2_DD_BASE_HPP
