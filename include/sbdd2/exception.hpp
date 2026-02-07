/**
 * @file exception.hpp
 * @brief DD操作に関する例外クラス階層の定義
 * @copyright MIT License
 *
 * SAPPOROBDD 2.0 で使用される例外クラスを定義します。
 * 標準ライブラリの例外クラスを継承し、DD操作に特化した例外階層を提供します。
 */

// SAPPOROBDD 2.0 - Exception classes
// MIT License

#ifndef SBDD2_EXCEPTION_HPP
#define SBDD2_EXCEPTION_HPP

#include <stdexcept>
#include <new>
#include <ios>
#include <string>

namespace sbdd2 {

/**
 * @brief DD操作の基底例外クラス
 *
 * すべてのDD固有例外の基底クラスです。
 * std::runtime_error を継承しています。
 *
 * @see DDMemoryException
 * @see DDArgumentException
 * @see DDIOException
 * @see DDIncompatibleException
 */
class DDException : public std::runtime_error {
public:
    /**
     * @brief std::string でメッセージを指定するコンストラクタ
     * @param msg エラーメッセージ
     */
    explicit DDException(const std::string& msg)
        : std::runtime_error(msg) {}

    /**
     * @brief C文字列でメッセージを指定するコンストラクタ
     * @param msg エラーメッセージ
     */
    explicit DDException(const char* msg)
        : std::runtime_error(msg) {}
};

/**
 * @brief メモリ割り当て失敗時の例外クラス
 *
 * DDノードの割り当てやテーブル拡張時にメモリ不足が発生した場合にスローされます。
 * std::bad_alloc を継承しています。
 *
 * @see DDException
 */
class DDMemoryException : public std::bad_alloc {
private:
    std::string msg_;  ///< エラーメッセージ

public:
    /**
     * @brief std::string でメッセージを指定するコンストラクタ
     * @param msg エラーメッセージ
     */
    explicit DDMemoryException(const std::string& msg)
        : msg_(msg) {}

    /**
     * @brief C文字列でメッセージを指定するコンストラクタ
     * @param msg エラーメッセージ
     */
    explicit DDMemoryException(const char* msg)
        : msg_(msg) {}

    /**
     * @brief エラーメッセージを取得する
     * @return エラーメッセージのC文字列
     */
    const char* what() const noexcept override {
        return msg_.c_str();
    }
};

/**
 * @brief 不正な引数に対する例外クラス
 *
 * DD操作に不正な引数が渡された場合にスローされます。
 * 例: 無効な変数番号、範囲外のインデックスなど。
 * std::invalid_argument を継承しています。
 *
 * @see DDException
 */
class DDArgumentException : public std::invalid_argument {
public:
    /**
     * @brief std::string でメッセージを指定するコンストラクタ
     * @param msg エラーメッセージ
     */
    explicit DDArgumentException(const std::string& msg)
        : std::invalid_argument(msg) {}

    /**
     * @brief C文字列でメッセージを指定するコンストラクタ
     * @param msg エラーメッセージ
     */
    explicit DDArgumentException(const char* msg)
        : std::invalid_argument(msg) {}
};

/**
 * @brief I/Oエラーに対する例外クラス
 *
 * DDのファイル読み書き時にエラーが発生した場合にスローされます。
 * std::ios_base::failure を継承しています。
 *
 * @see DDException
 */
class DDIOException : public std::ios_base::failure {
public:
    /**
     * @brief std::string でメッセージを指定するコンストラクタ
     * @param msg エラーメッセージ
     */
    explicit DDIOException(const std::string& msg)
        : std::ios_base::failure(msg) {}

    /**
     * @brief C文字列でメッセージを指定するコンストラクタ
     * @param msg エラーメッセージ
     */
    explicit DDIOException(const char* msg)
        : std::ios_base::failure(msg) {}
};

/**
 * @brief 非互換なDD型またはマネージャに対する例外クラス
 *
 * 異なるマネージャに属するDDを組み合わせた操作や、
 * 型が互換しないDD間の操作を行った場合にスローされます。
 * DDException を継承しています。
 *
 * @see DDException
 */
class DDIncompatibleException : public DDException {
public:
    /**
     * @brief std::string でメッセージを指定するコンストラクタ
     * @param msg エラーメッセージ
     */
    explicit DDIncompatibleException(const std::string& msg)
        : DDException(msg) {}

    /**
     * @brief C文字列でメッセージを指定するコンストラクタ
     * @param msg エラーメッセージ
     */
    explicit DDIncompatibleException(const char* msg)
        : DDException(msg) {}
};

} // namespace sbdd2

#endif // SBDD2_EXCEPTION_HPP
