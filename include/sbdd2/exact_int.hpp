/**
 * @file exact_int.hpp
 * @brief SAPPOROBDD 2.0 - 厳密整数型の抽象化
 * @author SAPPOROBDD Team
 * @copyright MIT License
 *
 * GMP (mpz_class) または BigInt ライブラリのいずれかを使用して
 * 厳密な整数演算を提供する。GMP が優先され、GMP がない場合は
 * BigInt をフォールバックとして使用する。
 */

#ifndef SBDD2_EXACT_INT_HPP
#define SBDD2_EXACT_INT_HPP

#if defined(SBDD2_HAS_GMP)

#include <gmpxx.h>
#include <string>
#include <random>

namespace sbdd2 {

/// @brief 厳密整数型（GMP版）
using exact_int_t = mpz_class;

/// @brief 厳密整数を文字列に変換
inline std::string exact_int_to_str(const exact_int_t& v) {
    return v.get_str();
}

/// @brief 2^n を計算
inline exact_int_t exact_int_pow2(unsigned int n) {
    exact_int_t result;
    mpz_ui_pow_ui(result.get_mpz_t(), 2, n);
    return result;
}

/// @brief [0, upper) の一様乱数を生成
template<typename RNG>
exact_int_t exact_int_random(const exact_int_t& upper, RNG& rng) {
    gmp_randclass gmp_rng(gmp_randinit_default);
    std::uniform_int_distribution<unsigned long> seed_dist;
    gmp_rng.seed(seed_dist(rng));
    return gmp_rng.get_z_range(upper);
}

} // namespace sbdd2

#elif defined(SBDD2_HAS_BIGINT)

#include <bigint/bigint.hpp>
#include <string>
#include <random>

namespace sbdd2 {

/// @brief 厳密整数型（BigInt版）
using exact_int_t = bigint::BigInt;

/// @brief 厳密整数を文字列に変換
inline std::string exact_int_to_str(const exact_int_t& v) {
    return v.to_string();
}

/// @brief 2^n を計算
inline exact_int_t exact_int_pow2(unsigned int n) {
    return exact_int_t(1) << static_cast<std::size_t>(n);
}

/// @brief [0, upper) の一様乱数を生成
template<typename RNG>
exact_int_t exact_int_random(const exact_int_t& upper, RNG& rng) {
    return bigint::uniform_random(upper, rng);
}

} // namespace sbdd2

#endif // SBDD2_HAS_GMP / SBDD2_HAS_BIGINT

#endif // SBDD2_EXACT_INT_HPP
