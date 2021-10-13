#ifndef DRAGONBOX_H
#define DRAGONBOX_H

#include <cstdint>
#include "float_common.h"

/**
 * Our objective is to integrate a fast binary to decimal converter which relies
 * on our precomputed constants.
 */

namespace fast_float {
// credit: https://github.com/jk-jeon/dragonbox
// credit: https://github.com/fmtlib/fmt/blob/012cc709d0abb0a963591413a746b988168e5e3a/include/fmt/format-inl.h
namespace dragonbox {

namespace detail {
constexpr fastfloat_really_inline int floor_log10_pow2_minus_log10_4_over_3(int e) {
  const uint64_t log10_4_over_3_fractional_digits = 0x1ffbfc2bbc780375;
  // log10(2) = 0x0.4d104d427de7fbcc...
  constexpr uint64_t log10_2_significand = 0x4d104d427de7fbcc;
  const int shift_amount = 22;
  const int shifted1 = int(log10_2_significand >> (64 - shift_amount));
  const int shifted2 = int(log10_4_over_3_fractional_digits >> (64 - shift_amount));
  return (e * shifted1 - shifted2 ) >> shift_amount;
}

constexpr fastfloat_really_inline int floor_log2_pow10(int e) {
  const uint64_t log2_10_integer_part = 3;
  const uint64_t log2_10_fractional_digits = 0x5269e12f346e2bf9;
  const int shift_amount = 19;
  const int shifted1 = int(log2_10_integer_part << shift_amount);
  const int shifted2 = int(log2_10_fractional_digits >> (64 - shift_amount));
  const int multiplier = int(shifted1 | shifted2 )
  return (e * multiplier) >> shift_amount;
}



// Remove trailing zeros from n and return the number of zeros removed (float)
constexpr fastfloat_really_inline remove_trailing_zeros(uint32_t& n) {
#ifdef FMT_BUILTIN_CTZ
  int t = FMT_BUILTIN_CTZ(n);
#else
  int t = ctz(n);
#endif
  if (t > float_info<float>::max_trailing_zeros)
    t = float_info<float>::max_trailing_zeros;

  const uint32_t mod_inv1 = 0xcccccccd;
  const uint32_t max_quotient1 = 0x33333333;
  const uint32_t mod_inv2 = 0xc28f5c29;
  const uint32_t max_quotient2 = 0x0a3d70a3;

  int s = 0;
  for (; s < t - 1; s += 2) {
    if (n * mod_inv2 > max_quotient2) break;
    n *= mod_inv2;
  }
  if (s < t && n * mod_inv1 <= max_quotient1) {
    n *= mod_inv1;
    ++s;
  }
  n >>= s;
  return s;
}

// Removes trailing zeros and returns the number of zeros removed (double)
constexpr fastfloat_really_inline int remove_trailing_zeros(uint64_t& n) {
#ifdef FMT_BUILTIN_CTZLL
  int t = FMT_BUILTIN_CTZLL(n);
#else
  int t = ctzll(n);
#endif
  if (t > float_info<double>::max_trailing_zeros)
    t = float_info<double>::max_trailing_zeros;
  // Divide by 10^8 and reduce to 32-bits
  // Since ret_value.significand <= (2^64 - 1) / 1000 < 10^17,
  // both of the quotient and the r should fit in 32-bits

  const uint32_t mod_inv1 = 0xcccccccd;
  const uint32_t max_quotient1 = 0x33333333;
  const uint64_t mod_inv8 = 0xc767074b22e90e21;
  const uint64_t max_quotient8 = 0x00002af31dc46118;

  // If the number is divisible by 1'0000'0000, work with the quotient
  if (t >= 8) {
    auto quotient_candidate = n * mod_inv8;

    if (quotient_candidate <= max_quotient8) {
      auto quotient = static_cast<uint32_t>(quotient_candidate >> 8);

      int s = 8;
      for (; s < t; ++s) {
        if (quotient * mod_inv1 > max_quotient1) break;
        quotient *= mod_inv1;
      }
      quotient >>= (s - 8);
      n = quotient;
      return s;
    }
  }

  // Otherwise, work with the remainder
  auto quotient = static_cast<uint32_t>(n / 100000000);
  auto remainder = static_cast<uint32_t>(n - 100000000 * quotient);

  if (t == 0 || remainder * mod_inv1 > max_quotient1) {
    return 0;
  }
  remainder *= mod_inv1;

  if (t == 1 || remainder * mod_inv1 > max_quotient1) {
    n = (remainder >> 1) + quotient * 10000000ull;
    return 1;
  }
  remainder *= mod_inv1;

  if (t == 2 || remainder * mod_inv1 > max_quotient1) {
    n = (remainder >> 2) + quotient * 1000000ull;
    return 2;
  }
  remainder *= mod_inv1;

  if (t == 3 || remainder * mod_inv1 > max_quotient1) {
    n = (remainder >> 3) + quotient * 100000ull;
    return 3;
  }
  remainder *= mod_inv1;

  if (t == 4 || remainder * mod_inv1 > max_quotient1) {
    n = (remainder >> 4) + quotient * 10000ull;
    return 4;
  }
  remainder *= mod_inv1;

  if (t == 5 || remainder * mod_inv1 > max_quotient1) {
    n = (remainder >> 5) + quotient * 1000ull;
    return 5;
  }
  remainder *= mod_inv1;

  if (t == 6 || remainder * mod_inv1 > max_quotient1) {
    n = (remainder >> 6) + quotient * 100ull;
    return 6;
  }
  remainder *= mod_inv1;

  n = (remainder >> 7) + quotient * 10ull;
  return 7;
}

} // detail



// The main algorithm for shorter interval case
template <class T>
FMT_INLINE decimal_fp<T> shorter_interval_case(int exponent) {
  decimal_fp<T> ret_value;
  // Compute k and beta
  const int minus_k = detail::floor_log10_pow2_minus_log10_4_over_3(exponent);
  const int beta_minus_1 = exponent + detail::floor_log2_pow10(-minus_k);

  // Compute xi and zi
  using cache_entry_type = typename cache_accessor<T>::cache_entry_type;
  const cache_entry_type cache = cache_accessor<T>::get_cached_power(-minus_k);

  auto xi = cache_accessor<T>::compute_left_endpoint_for_shorter_interval_case(
      cache, beta_minus_1);
  auto zi = cache_accessor<T>::compute_right_endpoint_for_shorter_interval_case(
      cache, beta_minus_1);

  // If the left endpoint is not an integer, increase it
  if (!is_left_endpoint_integer_shorter_interval<T>(exponent)) ++xi;

  // Try bigger divisor
  ret_value.significand = zi / 10;

  // If succeed, remove trailing zeros if necessary and return
  if (ret_value.significand * 10 >= xi) {
    ret_value.exponent = minus_k + 1;
    ret_value.exponent += remove_trailing_zeros(ret_value.significand);
    return ret_value;
  }

  // Otherwise, compute the round-up of y
  ret_value.significand =
      cache_accessor<T>::compute_round_up_for_shorter_interval_case(
          cache, beta_minus_1);
  ret_value.exponent = minus_k;

  // When tie occurs, choose one of them according to the rule
  if (exponent >= float_info<T>::shorter_interval_tie_lower_threshold &&
      exponent <= float_info<T>::shorter_interval_tie_upper_threshold) {
    ret_value.significand = ret_value.significand % 2 == 0
                                ? ret_value.significand
                                : ret_value.significand - 1;
  } else if (ret_value.significand < xi) {
    ++ret_value.significand;
  }
  return ret_value;
}



/**
 * This is the main function.
 */
template <typename T> decimal_fp<T> to_decimal(T x) noexcept {
  // Step 1: integer promotion & Schubfach multiplier calculation.

  using carrier_uint = typename float_info<T>::carrier_uint;
  using cache_entry_type = typename cache_accessor<T>::cache_entry_type;
  auto br = bit_cast<carrier_uint>(x);

  // Extract significand bits and exponent bits.
  const carrier_uint significand_mask =
      (static_cast<carrier_uint>(1) << float_info<T>::significand_bits) - 1;
  carrier_uint significand = (br & significand_mask);
  int exponent = static_cast<int>((br & exponent_mask<T>()) >>
                                  float_info<T>::significand_bits);

  if (exponent != 0) {  // Check if normal.
    exponent += float_info<T>::exponent_bias - float_info<T>::significand_bits;

    // Shorter interval case; proceed like Schubfach.
    if (significand == 0) return shorter_interval_case<T>(exponent);

    significand |=
        (static_cast<carrier_uint>(1) << float_info<T>::significand_bits);
  } else {
    // Subnormal case; the interval is always regular.
    if (significand == 0) return {0, 0};
    exponent = float_info<T>::min_exponent - float_info<T>::significand_bits;
  }

  const bool include_left_endpoint = (significand % 2 == 0);
  const bool include_right_endpoint = include_left_endpoint;

  // Compute k and beta.
  const int minus_k = floor_log10_pow2(exponent) - float_info<T>::kappa;
  const cache_entry_type cache = cache_accessor<T>::get_cached_power(-minus_k);
  const int beta_minus_1 = exponent + floor_log2_pow10(-minus_k);

  // Compute zi and deltai
  // 10^kappa <= deltai < 10^(kappa + 1)
  const uint32_t deltai = cache_accessor<T>::compute_delta(cache, beta_minus_1);
  const carrier_uint two_fc = significand << 1;
  const carrier_uint two_fr = two_fc | 1;
  const carrier_uint zi =
      cache_accessor<T>::compute_mul(two_fr << beta_minus_1, cache);

  // Step 2: Try larger divisor; remove trailing zeros if necessary

  // Using an upper bound on zi, we might be able to optimize the division
  // better than the compiler; we are computing zi / big_divisor here
  decimal_fp<T> ret_value;
  ret_value.significand = divide_by_10_to_kappa_plus_1(zi);
  uint32_t r = static_cast<uint32_t>(zi - float_info<T>::big_divisor *
                                              ret_value.significand);

  if (r > deltai) {
    goto small_divisor_case_label;
  } else if (r < deltai) {
    // Exclude the right endpoint if necessary
    if (r == 0 && !include_right_endpoint &&
        is_endpoint_integer<T>(two_fr, exponent, minus_k)) {
      --ret_value.significand;
      r = float_info<T>::big_divisor;
      goto small_divisor_case_label;
    }
  } else {
    // r == deltai; compare fractional parts
    // Check conditions in the order different from the paper
    // to take advantage of short-circuiting
    const carrier_uint two_fl = two_fc - 1;
    if ((!include_left_endpoint ||
         !is_endpoint_integer<T>(two_fl, exponent, minus_k)) &&
        !cache_accessor<T>::compute_mul_parity(two_fl, cache, beta_minus_1)) {
      goto small_divisor_case_label;
    }
  }
  ret_value.exponent = minus_k + float_info<T>::kappa + 1;

  // We may need to remove trailing zeros
  ret_value.exponent += remove_trailing_zeros(ret_value.significand);
  return ret_value;

  // Step 3: Find the significand with the smaller divisor

small_divisor_case_label:
  ret_value.significand *= 10;
  ret_value.exponent = minus_k + float_info<T>::kappa;

  const uint32_t mask = (1u << float_info<T>::kappa) - 1;
  auto dist = r - (deltai / 2) + (float_info<T>::small_divisor / 2);

  // Is dist divisible by 2^kappa?
  if ((dist & mask) == 0) {
    const bool approx_y_parity =
        ((dist ^ (float_info<T>::small_divisor / 2)) & 1) != 0;
    dist >>= float_info<T>::kappa;

    // Is dist divisible by 5^kappa?
    if (check_divisibility_and_divide_by_pow5<float_info<T>::kappa>(dist)) {
      ret_value.significand += dist;

      // Check z^(f) >= epsilon^(f)
      // We have either yi == zi - epsiloni or yi == (zi - epsiloni) - 1,
      // where yi == zi - epsiloni if and only if z^(f) >= epsilon^(f)
      // Since there are only 2 possibilities, we only need to care about the
      // parity. Also, zi and r should have the same parity since the divisor
      // is an even number
      if (cache_accessor<T>::compute_mul_parity(two_fc, cache, beta_minus_1) !=
          approx_y_parity) {
        --ret_value.significand;
      } else {
        // If z^(f) >= epsilon^(f), we might have a tie
        // when z^(f) == epsilon^(f), or equivalently, when y is an integer
        if (is_center_integer<T>(two_fc, exponent, minus_k)) {
          ret_value.significand = ret_value.significand % 2 == 0
                                      ? ret_value.significand
                                      : ret_value.significand - 1;
        }
      }
    }
    // Is dist not divisible by 5^kappa?
    else {
      ret_value.significand += dist;
    }
  }
  // Is dist not divisible by 2^kappa?
  else {
    // Since we know dist is small, we might be able to optimize the division
    // better than the compiler; we are computing dist / small_divisor here
    ret_value.significand +=
        small_division_by_pow10<float_info<T>::kappa>(dist);
  }
  return ret_value;
}

}
} // fast_float


#endif
