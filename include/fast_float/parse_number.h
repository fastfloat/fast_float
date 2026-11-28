#ifndef FASTFLOAT_PARSE_NUMBER_H
#define FASTFLOAT_PARSE_NUMBER_H

#include "ascii_number.h"
#include "decimal_to_binary.h"
#include "digit_comparison.h"
#include "float_common.h"

#include <cmath>
#include <cstring>
#include <limits>
#include <system_error>

namespace fast_float {

namespace detail {
/**
 * Special case +inf, -inf, nan, infinity, -infinity.
 * The case comparisons could be made much faster given that we know that the
 * strings a null-free and fixed.
 **/
template <typename T, typename UC>
from_chars_result_t<UC>
    FASTFLOAT_CONSTEXPR14 parse_infnan(UC const *first, UC const *last,
                                       T &value, chars_format fmt) noexcept {
  from_chars_result_t<UC> answer{};
  answer.ptr = first;
  answer.ec = std::errc(); // be optimistic
  // assume first < last, so dereference without checks;
  bool const minusSign = (*first == UC('-'));
  // C++17 20.19.3.(7.1) explicitly forbids '+' sign here
  if ((*first == UC('-')) ||
      (uint64_t(fmt & chars_format::allow_leading_plus) &&
       (*first == UC('+')))) {
    ++first;
  }
  if (last - first >= 3) {
    if (fastfloat_strncasecmp(first, str_const_nan<UC>(), 3)) {
      answer.ptr = (first += 3);
      value = minusSign ? -std::numeric_limits<T>::quiet_NaN()
                        : std::numeric_limits<T>::quiet_NaN();
      // Check for possible nan(n-char-seq-opt), C++17 20.19.3.7,
      // C11 7.20.1.3.3. At least MSVC produces nan(ind) and nan(snan).
      if (first != last && *first == UC('(')) {
        for (UC const *ptr = first + 1; ptr != last; ++ptr) {
          if (*ptr == UC(')')) {
            answer.ptr = ptr + 1; // valid nan(n-char-seq-opt)
            break;
          } else if (!((UC('a') <= *ptr && *ptr <= UC('z')) ||
                       (UC('A') <= *ptr && *ptr <= UC('Z')) ||
                       (UC('0') <= *ptr && *ptr <= UC('9')) || *ptr == UC('_')))
            break; // forbidden char, not nan(n-char-seq-opt)
        }
      }
      return answer;
    }
    if (fastfloat_strncasecmp(first, str_const_inf<UC>(), 3)) {
      if ((last - first >= 8) &&
          fastfloat_strncasecmp(first + 3, str_const_inf<UC>() + 3, 5)) {
        answer.ptr = first + 8;
      } else {
        answer.ptr = first + 3;
      }
      value = minusSign ? -std::numeric_limits<T>::infinity()
                        : std::numeric_limits<T>::infinity();
      return answer;
    }
  }
  answer.ec = std::errc::invalid_argument;
  return answer;
}

/**
 * Returns true if the floating-pointing rounding mode is to 'nearest'.
 * It is the default on most system. This function is meant to be inexpensive.
 * Credit : @mwalcott3
 *
 * If FASTFLOAT_ROUNDS_TO_NEAREST is defined, we trust the user that the
 * rounding mode is always 'nearest' and we skip the check (see README).
 */
fastfloat_really_inline bool rounds_to_nearest() noexcept {
  // https://lemire.me/blog/2020/06/26/gcc-not-nearest/
#if (FLT_EVAL_METHOD != 1) && (FLT_EVAL_METHOD != 0)
  // The arithmetic may be carried out with excess precision (e.g., x87):
  // Clinger's fast path could then round twice. FASTFLOAT_ROUNDS_TO_NEAREST
  // does not change this.
  return false;
#elif defined(FASTFLOAT_ROUNDS_TO_NEAREST)
  return true;
#else
  // See
  // A fast function to check your floating-point rounding mode
  // https://lemire.me/blog/2022/11/16/a-fast-function-to-check-your-floating-point-rounding-mode/
  //
  // This function is meant to be equivalent to :
  // prior: #include <cfenv>
  //  return fegetround() == FE_TONEAREST;
  // However, it is expected to be much faster than the fegetround()
  // function call.
  //
  // The volatile keyword prevents the compiler from computing the function
  // at compile-time.
  // There might be other ways to prevent compile-time optimizations (e.g.,
  // asm). The value does not need to be std::numeric_limits<float>::min(), any
  // small value so that 1 + x should round to 1 would do (after accounting for
  // excess precision, as in 387 instructions).
  static float volatile fmin = (std::numeric_limits<float>::min)();
  float fmini = fmin; // we copy it so that it gets loaded at most once.
//
// Explanation:
// Only when fegetround() == FE_TONEAREST do we have that
// fmin + 1.0f == 1.0f - fmin.
//
// FE_UPWARD:
//  fmin + 1.0f > 1
//  1.0f - fmin == 1
//
// FE_DOWNWARD or  FE_TOWARDZERO:
//  fmin + 1.0f == 1
//  1.0f - fmin < 1
//
// Note: This may fail to be accurate if fast-math has been
// enabled, as rounding conventions may not apply.
#ifdef FASTFLOAT_VISUAL_STUDIO
#pragma warning(push)
//  todo: is there a VS warning?
//  see
//  https://stackoverflow.com/questions/46079446/is-there-a-warning-for-floating-point-equality-checking-in-visual-studio-2013
#elif defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wfloat-equal"
#elif defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wfloat-equal"
#endif
  return (fmini + 1.0f == 1.0f - fmini);
#ifdef FASTFLOAT_VISUAL_STUDIO
#pragma warning(pop)
#elif defined(__clang__)
#pragma clang diagnostic pop
#elif defined(__GNUC__)
#pragma GCC diagnostic pop
#endif
#endif // FASTFLOAT_ROUNDS_TO_NEAREST
}

} // namespace detail

template <typename T> struct from_chars_caller {
  template <typename UC>
  fastfloat_clang_really_inline
      FASTFLOAT_CONSTEXPR20 static from_chars_result_t<UC>
      call(UC const *first, UC const *last, T &value,
           parse_options_t<UC> options) noexcept {
    return from_chars_advanced(first, last, value, options);
  }
};

#ifdef __STDCPP_FLOAT32_T__
template <> struct from_chars_caller<std::float32_t> {
  template <typename UC>
  FASTFLOAT_CONSTEXPR20 static from_chars_result_t<UC>
  call(UC const *first, UC const *last, std::float32_t &value,
       parse_options_t<UC> options) noexcept {
    // if std::float32_t is defined, and we are in C++23 mode; macro set for
    // float32; set value to float due to equivalence between float and
    // float32_t
    float val = 0.0f;
    auto ret = from_chars_advanced(first, last, val, options);
    // If no characters match the pattern, value must be left unmodified.
    if (ret.ec != std::errc::invalid_argument) {
      value = val;
    }
    return ret;
  }
};
#endif

#ifdef __STDCPP_FLOAT64_T__
template <> struct from_chars_caller<std::float64_t> {
  template <typename UC>
  FASTFLOAT_CONSTEXPR20 static from_chars_result_t<UC>
  call(UC const *first, UC const *last, std::float64_t &value,
       parse_options_t<UC> options) noexcept {
    // if std::float64_t is defined, and we are in C++23 mode; macro set for
    // float64; set value as double due to equivalence between double and
    // float64_t
    double val = 0.0;
    auto ret = from_chars_advanced(first, last, val, options);
    // If no characters match the pattern, value must be left unmodified.
    if (ret.ec != std::errc::invalid_argument) {
      value = val;
    }
    return ret;
  }
};
#endif

#ifdef __clang__
// Parser instantiated for a format fixed at compile time, so that every test
// on the format folds away inside it. GCC gets the same effect by inlining the
// whole parser into each caller, where the format is a constant; clang keeps
// it out of line and would otherwise re-test each format flag per conversion.
template <typename T, typename UC, chars_format Fmt>
FASTFLOAT_CONSTEXPR20 from_chars_result_t<UC>
from_chars_fixed_format(UC const *first, UC const *last, T &value) noexcept {
  return from_chars_caller<T>::call(first, last, value,
                                    parse_options_t<UC>(Fmt));
}
#endif

template <typename T, typename UC, typename>
FASTFLOAT_CONSTEXPR20 from_chars_result_t<UC>
from_chars(UC const *first, UC const *last, T &value,
           chars_format fmt /*= chars_format::general*/) noexcept {
#ifdef __clang__
  if (fmt == chars_format::general) {
    return from_chars_fixed_format<T, UC, chars_format::general>(first, last,
                                                                 value);
  }
#endif
  return from_chars_caller<T>::call(first, last, value,
                                    parse_options_t<UC>(fmt));
}

template <typename T>
fastfloat_really_inline FASTFLOAT_CONSTEXPR20 bool
clinger_fast_path_impl(uint64_t mantissa, int64_t exponent, bool is_negative,
                       T &value) noexcept {
  // The implementation of the Clinger's fast path is convoluted because
  // we want round-to-nearest in all cases, irrespective of the rounding mode
  // selected on the thread.
  // We proceed optimistically, assuming that detail::rounds_to_nearest()
  // returns true.
  if (binary_format<T>::min_exponent_fast_path() <= exponent &&
      exponent <= binary_format<T>::max_exponent_fast_path() &&
      mantissa <= binary_format<T>::max_mantissa_fast_path()) {
    // The mantissa bound above is a necessary condition for BOTH branches
    // below: the rounding-mode-dependent branch checks the tighter
    // max_mantissa_fast_path(exponent) <= max_mantissa_fast_path(). Testing
    // it before detail::rounds_to_nearest() spares long-mantissa inputs
    // (which can never take the fast path) the volatile-float probe.
    //
    // Unfortunately, the conventional Clinger's fast path is only possible
    // when the system rounds to the nearest float.
    //
    // We expect the next branch to almost always be selected.
    // We could check it first (before the previous branch), but
    // there might be performance advantages at having the check
    // be last.
    if (!cpp20_and_in_constexpr() && detail::rounds_to_nearest()) {
      // We have that fegetround() == FE_TONEAREST.
      // Next is Clinger's fast path.
      value = T(mantissa);
      if (exponent < 0) {
        value = value / binary_format<T>::exact_power_of_ten(-exponent);
      } else {
        value = value * binary_format<T>::exact_power_of_ten(exponent);
        // Only std::float16_t can overflow here (e.g., "656e2"); let the
        // slow path report result_out_of_range.
        if (binary_format<T>::fast_path_can_overflow() &&
            value > (std::numeric_limits<T>::max)()) {
          return false;
        }
      }
      if (is_negative) {
        value = -value;
      }
      return true;
    } else {
      // We do not have that fegetround() == FE_TONEAREST.
      // Next is a modified Clinger's fast path, inspired by Jakub Jelínek's
      // proposal
      if (exponent >= 0 &&
          mantissa <= binary_format<T>::max_mantissa_fast_path(exponent)) {
#if defined(__clang__) || defined(FASTFLOAT_32BIT)
        // Clang may map 0 to -0.0 when fegetround() == FE_DOWNWARD
        if (mantissa == 0) {
          value = is_negative ? T(-0.) : T(0.);
          return true;
        }
#endif
        value = T(mantissa) * binary_format<T>::exact_power_of_ten(exponent);
        if (is_negative) {
          value = -value;
        }
        return true;
      }
    }
  }
  return false;
}

/**
 * This function overload takes parsed_number_string_t structure that is created
 * and populated either by from_chars_advanced function taking chars range and
 * parsing options or other parsing custom function implemented by user.
 */
template <typename T, typename UC>
fastfloat_really_inline FASTFLOAT_CONSTEXPR20 from_chars_result_t<UC>
from_chars_advanced(parsed_number_string_t<UC> &pns, T &value) noexcept {
  static_assert(is_supported_float_type<T>::value,
                "only some floating-point types are supported");
  static_assert(is_supported_char_type<UC>::value,
                "only char, wchar_t, char16_t and char32_t are supported");

  from_chars_result_t<UC> answer;

  answer.ec = std::errc(); // be optimistic
  answer.ptr = pns.lastmatch;

  if (!pns.too_many_digits &&
      clinger_fast_path_impl(pns.mantissa, pns.exponent, pns.negative, value))
    return answer;

  adjusted_mantissa am =
      compute_float<binary_format<T>>(pns.exponent, pns.mantissa);
  if (pns.too_many_digits && am.power2 >= 0) {
    if (am != compute_float<binary_format<T>>(pns.exponent, pns.mantissa + 1)) {
      am = compute_error<binary_format<T>>(pns.exponent, pns.mantissa);
    }
  }
  // If we called compute_float<binary_format<T>>(pns.exponent, pns.mantissa)
  // and we have an invalid power (am.power2 < 0), then we need to go the long
  // way around again. This is very uncommon.
  if (am.power2 < 0) {
    am = digit_comp<T>(pns, am);
  }
  to_float(pns.negative, am, value);
  // Test for over/underflow.
  if ((pns.mantissa != 0 && am.mantissa == 0 && am.power2 == 0) ||
      am.power2 == binary_format<T>::infinite_power()) {
    answer.ec = std::errc::result_out_of_range;
  }
  return answer;
}

// Slow path: re-parse materializing the integer/fraction spans the hot no-span
// parse skipped, then run the full algorithm. The two callers reach it only
// through a fastfloat_unlikely branch, so the optimizer keeps this re-parse off
// the hot path on its own (no function-level noinline needed).
// from_chars_advanced already handles both the too_many_digits disambiguation
// and the am.power2<0 digit_comp recompute, so both slow branches collapse to
// one helper call.
template <typename T, typename UC>
FASTFLOAT_CONSTEXPR20 from_chars_result_t<UC>
parse_number_slow_path(UC const *first, UC const *last, T &value,
                       parse_options_t<UC> options, bool bjf) noexcept {
  parsed_number_string_t<UC> pns =
      bjf ? parse_number_string_impl<true, false, UC>(first, last, options,
                                                      true)
          : parse_number_string_impl<false, false, UC>(first, last, options,
                                                       true);
  return from_chars_advanced(pns, value);
}

// Cold: the whole chars_format::javascript conversion. Kept out of line and out
// of from_chars_float_advanced so the common frame holds neither the javascript
// parser body nor a call that would force the parsed number string onto the
// stack. Mirrors the main path, only with the javascript parser.
template <typename T, typename UC>
fastfloat_never_inline FASTFLOAT_CONSTEXPR20 from_chars_result_t<UC>
from_chars_float_javascript(UC const *first, UC const *last, T &value,
                            parse_options_t<UC> options) noexcept {
  chars_format const fmt = detail::adjust_for_feature_macros(options.format);
  if (uint64_t(fmt & chars_format::skip_white_space)) {
    while ((first != last) && fast_float::is_space(*first)) {
      first++;
    }
  }
  from_chars_result_t<UC> answer;
  answer.ec = std::errc::invalid_argument;
  answer.ptr = first;
  if (first == last) {
    return answer;
  }
  parsed_number_string_t<UC> pns =
      parse_number_string_javascript<UC>(first, last, options, true);
  if (!pns.valid) {
    if (uint64_t(fmt & chars_format::no_infnan)) {
      return answer;
    }
    return detail::parse_infnan(first, last, value, fmt);
  }
  return from_chars_advanced(pns, value);
}

namespace detail {
// Value of a hexadecimal digit, or a value of at least 16 if c is not one.
template <typename UC>
fastfloat_really_inline FASTFLOAT_CONSTEXPR14 uint32_t hex_digit(UC c) {
  if (cpp20_and_in_constexpr()) {
    // The lookup table cannot be read in a constant expression. Any code unit
    // (signed or not, of any width) maps to a distinct unsigned value, and
    // only the 22 hexadecimal digits map below 16.
    uint32_t const u = uint32_t(c);
    if (u - uint32_t('0') < 10) {
      return u - uint32_t('0');
    }
    if ((u | 0x20) - uint32_t('a') < 6) {
      return (u | 0x20) - uint32_t('a') + 10;
    }
    return 16;
  }
  // Branchless: the digits and the letters of a hexadecimal significand come
  // in no predictable order.
  return ch_to_digit(c);
}

// Parses 8 hexadecimal digits given as 8 bytes, the first digit in the least
// significant byte (see read8_to_u64). Returns false, leaving out untouched,
// if one of the bytes is not a hexadecimal digit.
fastfloat_really_inline FASTFLOAT_CONSTEXPR14 bool
parse_eight_hex_digits(uint64_t v, uint32_t &out) noexcept {
  // For a byte x < 0x80, (x + 0x80 - lo) has its high bit set if x >= lo and
  // (x + 0x7f - hi) has its high bit set if x > hi, and no carry leaves the
  // byte. A byte x >= 0x80 never passes either test (both sums have their
  // high bit set, or the first one wraps around below 0x80), even with a
  // carry from the byte below it, so it is rejected; the carry it may send to
  // the byte above does not matter.
  uint64_t const high = 0x8080808080808080;
  uint64_t const lower = v | 0x2020202020202020; // 'A'-'F' becomes 'a'-'f'
  uint64_t const is_digit =
      (v + 0x5050505050505050) & ~(v + 0x4646464646464646); // '0'-'9'
  uint64_t const is_letter =
      (lower + 0x1f1f1f1f1f1f1f1f) & ~(lower + 0x1919191919191919); // 'a'-'f'
  if (((is_digit | is_letter) & high) != high) {
    return false;
  }
  // '0'-'9' have their value in the low nibble, 'a'-'f' and 'A'-'F' have
  // their value minus 9.
  uint64_t const nibbles =
      (v & 0x0f0f0f0f0f0f0f0f) + ((is_letter & high) >> 7) * 9;
  // Put the first digit in the most significant byte, then pack the nibbles.
  uint64_t x = byteswap(nibbles);
  x = (x | (x >> 4)) & 0x00ff00ff00ff00ff;
  x = (x | (x >> 8)) & 0x0000ffff0000ffff;
  out = uint32_t(x | (x >> 16));
  return true;
}

// Adds one hexadecimal digit to the mantissa if it has room (at most 16
// significant digits). In the integer part, each digit dropped adds 4 to the
// exponent; in the fraction, each digit kept subtracts 4. A dropped nonzero
// digit sets sticky.
template <bool fraction>
fastfloat_really_inline FASTFLOAT_CONSTEXPR14 void
add_hex_digit(uint32_t digit, uint64_t &mantissa, int64_t &exponent,
              bool &sticky) noexcept {
  if ((mantissa >> 60) == 0) {
    mantissa = (mantissa << 4) | digit;
    if (fraction) {
      exponent -= 4;
    }
  } else {
    if (!fraction) {
      exponent += 4;
    }
    sticky |= (digit != 0);
  }
}

// Parses a run of hexadecimal digits (see add_hex_digit).
template <bool fraction, typename UC>
fastfloat_really_inline FASTFLOAT_CONSTEXPR20 void
parse_hex_digits(UC const *&p, UC const *last, uint64_t &mantissa,
                 int64_t &exponent, bool &sticky) noexcept {
  uint32_t digit;
  if ((p == last) || ((digit = hex_digit(*p)) >= 16)) {
    return;
  }
  add_hex_digit<fraction>(digit, mantissa, exponent, sticky);
  ++p;
  // Blocks of 8 digits, only for char (other code units would need to be
  // checked for high bits), while the mantissa has room for them. We start
  // after one digit, and only if a second one follows: printf("%a") and
  // std::to_chars write a single digit before the point, and failing a
  // block costs about as much as parsing one.
  if (!cpp20_and_in_constexpr() && std::is_same<UC, char>::value &&
      (p != last) && (hex_digit(*p) < 16)) {
    uint32_t eight;
    while ((last - p >= 8) && (mantissa >> 32) == 0 &&
           parse_eight_hex_digits(read8_to_u64(p), eight)) {
      mantissa = (mantissa << 32) | eight;
      if (fraction) {
        exponent -= 32;
      }
      p += 8;
    }
  }
  while ((p != last) && ((digit = hex_digit(*p)) < 16)) {
    add_hex_digit<fraction>(digit, mantissa, exponent, sticky);
    ++p;
  }
}

// Rounds m * 2^e2 + (a sticky fraction below m's last bit, if sticky is set)
// to the nearest value of type T, ties to even, and stores it with the sign.
// m must be nonzero. Returns false if the result overflows to infinity or
// underflows to zero. Only integer arithmetic is used, so the result does not
// depend on the rounding mode of the thread.
template <typename T>
FASTFLOAT_CONSTEXPR20 bool hex_to_float(bool negative, uint64_t m, int64_t e2,
                                        bool sticky, T &value) noexcept {
  constexpr int mbits = binary_format<T>::mantissa_explicit_bits();
  constexpr int bias = -binary_format<T>::minimum_exponent();
  constexpr int infinite_power = binary_format<T>::infinite_power();
  int const lz = leading_zeroes(m);
  m <<= lz;
  // Now the value is 1.xxx * 2^(e2 + 63 - lz). With a bias, this exponent
  // is the IEEE exponent field when the value is normal.
  int64_t biased = e2 + 63 - lz + bias;
  adjusted_mantissa am;
  if (biased >= infinite_power) {
    am.mantissa = 0;
    am.power2 = infinite_power;
    to_float(negative, am, value);
    return false;
  }
  // We keep mbits + 1 bits when normal, fewer when subnormal.
  int64_t shift = 63 - mbits;
  if (biased <= 0) {
    shift += 1 - biased;
    biased = 1; // undone below, if the result stays subnormal
  }
  // round_bit and rest are 0 or 1. Rounding is branchless: when the input
  // has more bits than T (e.g., a double parsed as a float), the round bit
  // is unpredictable.
  uint64_t kept;
  uint64_t round_bit;
  uint64_t rest;
  if (shift < 64) {
    kept = m >> shift;
    round_bit = (m >> (shift - 1)) & 1;
    rest = uint64_t(sticky) |
           uint64_t((m & ((uint64_t(1) << (shift - 1)) - 1)) != 0);
  } else if (shift == 64) {
    kept = 0;
    round_bit = 1; // m has its top bit set
    rest = uint64_t(sticky) | uint64_t((m << 1) != 0);
  } else {
    kept = 0;
    round_bit = 0;
    rest = 1;
  }
  kept += round_bit & (rest | (kept & 1));
  if (kept == (uint64_t(2) << mbits)) { // carried into the next binade
    kept >>= 1;
    biased++;
    if (biased >= infinite_power) {
      am.mantissa = 0;
      am.power2 = infinite_power;
      to_float(negative, am, value);
      return false;
    }
  }
  // A subnormal result has no implicit bit (the value can round up to the
  // smallest normal number, which then gets its implicit bit).
  if (kept < (uint64_t(1) << mbits)) {
    biased = 0;
  }
  am.mantissa = kept & ((uint64_t(1) << mbits) - 1);
  am.power2 = int32_t(biased);
  to_float(negative, am, value);
  return kept != 0;
}
} // namespace detail

// Cold: the whole chars_format::hex conversion. Follows C++17
// [charconv.from.chars]: the expected form is that of std::strtod for a
// hexadecimal floating-point number, without the "0x" or "0X" prefix (it is
// assumed). The binary exponent ('p' or 'P' followed by an optionally signed
// decimal integer) is optional, unless chars_format::scientific is set but not
// chars_format::fixed, in which case it is required. If chars_format::fixed is
// set but not chars_format::scientific, it is not allowed. The result is
// correctly rounded (nearest, ties to even) whatever the rounding mode.
template <typename T, typename UC>
fastfloat_never_inline FASTFLOAT_CONSTEXPR20 from_chars_result_t<UC>
from_chars_float_hex(UC const *first, UC const *last, T &value,
                     parse_options_t<UC> options) noexcept {
  chars_format const fmt = detail::adjust_for_feature_macros(options.format);
  if (uint64_t(fmt & chars_format::skip_white_space)) {
    while ((first != last) && fast_float::is_space(*first)) {
      first++;
    }
  }
  from_chars_result_t<UC> answer;
  answer.ec = std::errc::invalid_argument;
  answer.ptr = first;
  if (first == last) {
    return answer;
  }
  UC const *p = first;
  bool const negative = (*p == UC('-'));
  if (negative ||
      (uint64_t(fmt & chars_format::allow_leading_plus) && *p == UC('+'))) {
    ++p;
  }
  // The value is mantissa * 2^exponent, plus something less than one unit of
  // the mantissa when sticky is set. We keep up to 16 significant hexadecimal
  // digits (64 bits): this is more than enough for a correct rounding as long
  // as we remember whether any of the dropped digits was nonzero.
  uint64_t mantissa = 0;
  int64_t exponent = 0;
  bool sticky = false;
  UC const *const start_digits = p;
  detail::parse_hex_digits<false>(p, last, mantissa, exponent, sticky);
  bool has_digits = (p != start_digits);
  if ((p != last) && (*p == options.decimal_point)) {
    ++p;
    UC const *const start_fraction = p;
    detail::parse_hex_digits<true>(p, last, mantissa, exponent, sticky);
    has_digits |= (p != start_fraction);
  }
  if (!has_digits) {
    if (uint64_t(fmt & chars_format::no_infnan)) {
      return answer;
    }
    return detail::parse_infnan(first, last, value, fmt);
  }
  bool const scientific = uint64_t(fmt & chars_format::scientific) != 0;
  bool const fixed = uint64_t(fmt & chars_format::fixed) != 0;
  if ((scientific || !fixed) && (p != last) &&
      ((*p == UC('p')) || (*p == UC('P')))) {
    UC const *const location_of_p = p;
    ++p;
    bool neg_exp = false;
    if ((p != last) && (*p == UC('-'))) {
      neg_exp = true;
      ++p;
    } else if ((p != last) && (*p == UC('+'))) {
      ++p;
    }
    if ((p == last) || !is_integer(*p)) {
      if (scientific && !fixed) {
        return answer;
      }
      p = location_of_p; // not an exponent: we stop before the 'p'
    } else {
      // The digits shift the exponent by at most 4 per digit, so by less
      // than 2^59 for any string that fits in memory. Past 2^59, we saturate
      // at 2^62: no significand can bring that back into range, and adding
      // it to the exponent cannot overflow.
      int64_t exp_number = 0;
      while ((p != last) && is_integer(*p)) {
        if (exp_number < (int64_t(1) << 59)) {
          exp_number = 10 * exp_number + int64_t(*p - UC('0'));
        } else {
          exp_number = int64_t(1) << 62;
        }
        ++p;
      }
      exponent += neg_exp ? -exp_number : exp_number;
    }
  } else if (scientific && !fixed) {
    return answer; // the exponent is required
  }
  answer.ptr = p;
  if (mantissa == 0) {
    value = negative ? T(-0.) : T(0.);
    answer.ec = std::errc();
    return answer;
  }
  answer.ec = detail::hex_to_float(negative, mantissa, exponent, sticky, value)
                  ? std::errc()
                  : std::errc::result_out_of_range;
  return answer;
}

template <typename T, typename UC>
fastfloat_really_inline FASTFLOAT_CONSTEXPR20 from_chars_result_t<UC>
from_chars_float_advanced(UC const *first, UC const *last, T &value,
                          parse_options_t<UC> options) noexcept {

  static_assert(is_supported_float_type<T>::value,
                "only some floating-point types are supported");
  static_assert(is_supported_char_type<UC>::value,
                "only char, wchar_t, char16_t and char32_t are supported");

  chars_format const fmt = detail::adjust_for_feature_macros(options.format);

  // Leave for the javascript conversion before anything else is live, so this
  // compiles to a tail call and the common path keeps its registers. The cold
  // function repeats the leading-whitespace and empty-input handling below.
  // See the comment on -Wc++20-extensions further down in this function.
#ifdef __clang__
#pragma clang diagnostic push
#if (!defined(__APPLE_CC__) && __clang_major__ >= 10) || (__clang_major__ >= 13)
#pragma clang diagnostic ignored "-Wc++20-extensions"
#endif
#endif
  if fastfloat_unlikely (uint64_t(fmt & (detail::basic_javascript_fmt |
                                         chars_format::hex))) {
    if (uint64_t(fmt & chars_format::hex)) {
      return from_chars_float_hex<T, UC>(first, last, value, options);
    }
    return from_chars_float_javascript<T, UC>(first, last, value, options);
  }
#ifdef __clang__
#pragma clang diagnostic pop
#endif

  from_chars_result_t<UC> answer;
  if (uint64_t(fmt & chars_format::skip_white_space)) {
    while ((first != last) && fast_float::is_space(*first)) {
      first++;
    }
  }
  if (first == last) {
    answer.ec = std::errc::invalid_argument;
    answer.ptr = first;
    return answer;
  }
  bool const bjf = uint64_t(fmt & detail::basic_json_fmt) != 0;

  // Fast path: parse WITHOUT materializing the integer/fraction spans (read
  // only by the rare slow paths). Skipping their stores keeps the fat
  // parsed_number_string_t off the hot path. store_spans is a runtime argument,
  // so this reuses the single parse_number_string_impl instantiation.
  parsed_number_string_t<UC> pns =
      bjf ? parse_number_string_impl<true, false, UC>(first, last, options,
                                                      false)
          : parse_number_string_impl<false, false, UC>(first, last, options,
                                                       false);
  if (!pns.valid) {
    if (uint64_t(fmt & chars_format::no_infnan)) {
      answer.ec = std::errc::invalid_argument;
      answer.ptr = first;
      return answer;
    } else {
      return detail::parse_infnan(first, last, value, fmt);
    }
  }

  // Slow path A (rare): > 19 significant digits. The no-span parse left the
  // mantissa un-truncated and skipped the span-based recompute; the cold helper
  // re-parses with spans and runs the full algorithm.
  //
// We have to disable -Wc++20-extensions for the [[unlikely]] attribute
// See comment for @jwakely at
// https://github.com/fastfloat/fast_float/pull/387#discussion_r3366943539
// This is unfortunate.
#ifdef __clang__
#pragma clang diagnostic push
#if (!defined(__APPLE_CC__) && __clang_major__ >= 10) || (__clang_major__ >= 13)
#pragma clang diagnostic ignored "-Wc++20-extensions"
#endif
#endif
  if fastfloat_unlikely (pns.too_many_digits) {
    return parse_number_slow_path<T, UC>(first, last, value, options, bjf);
  }
  answer.ec = std::errc(); // be optimistic
  answer.ptr = pns.lastmatch;

  if (clinger_fast_path_impl(pns.mantissa, pns.exponent, pns.negative, value)) {
    return answer;
  }

  adjusted_mantissa am =
      compute_float<binary_format<T>>(pns.exponent, pns.mantissa);
  // Slow path B (rare): Eisel-Lemire could not resolve; digit_comp needs the
  // integer/fraction spans. Route to the cold helper (clinger there is a
  // dead-effect since it already failed here; the cold re-parse + digit_comp
  // via from_chars_advanced reproduces this branch).
  if fastfloat_unlikely (am.power2 < 0) {
    return parse_number_slow_path<T, UC>(first, last, value, options, bjf);
  }
  to_float(pns.negative, am, value);
  // Test for over/underflow. Marked unlikely so that clang keeps it as a
  // branch instead of folding it into a chain of conditional moves that
  // every conversion pays for.
  if fastfloat_clang_unlikely ((pns.mantissa != 0 && am.mantissa == 0 &&
                                am.power2 == 0) ||
                               am.power2 ==
                                   binary_format<T>::infinite_power()) {
    answer.ec = std::errc::result_out_of_range;
  }
#ifdef __clang__
#pragma clang diagnostic pop
#endif
  return answer;
}

template <typename T, typename UC, typename>
fastfloat_clang_really_inline FASTFLOAT_CONSTEXPR20 from_chars_result_t<UC>
from_chars(UC const *first, UC const *last, T &value, int base) noexcept {

  static_assert(is_supported_integer_type<T>::value,
                "only integer types are supported");
  static_assert(is_supported_char_type<UC>::value,
                "only char, wchar_t, char16_t and char32_t are supported");

  parse_options_t<UC> options;
  options.base = base;
  return from_chars_advanced(first, last, value, options);
}

template <typename T>
FASTFLOAT_CONSTEXPR20
    typename std::enable_if<is_supported_float_type<T>::value, T>::type
    integer_times_pow10(uint64_t mantissa, int decimal_exponent) noexcept {
  T value;
  if (clinger_fast_path_impl(mantissa, decimal_exponent, false, value))
    return value;

  adjusted_mantissa am =
      compute_float<binary_format<T>>(decimal_exponent, mantissa);
  to_float(false, am, value);
  return value;
}

template <typename T>
FASTFLOAT_CONSTEXPR20
    typename std::enable_if<is_supported_float_type<T>::value, T>::type
    integer_times_pow10(int64_t mantissa, int decimal_exponent) noexcept {
  const bool is_negative = mantissa < 0;
  // Negate as unsigned: -mantissa overflows for the most negative int64_t.
  const uint64_t m = is_negative ? uint64_t(0) - static_cast<uint64_t>(mantissa)
                                 : static_cast<uint64_t>(mantissa);

  T value;
  if (clinger_fast_path_impl(m, decimal_exponent, is_negative, value))
    return value;

  adjusted_mantissa am = compute_float<binary_format<T>>(decimal_exponent, m);
  to_float(is_negative, am, value);
  return value;
}

FASTFLOAT_CONSTEXPR20 inline double
integer_times_pow10(uint64_t mantissa, int decimal_exponent) noexcept {
  return integer_times_pow10<double>(mantissa, decimal_exponent);
}

FASTFLOAT_CONSTEXPR20 inline double
integer_times_pow10(int64_t mantissa, int decimal_exponent) noexcept {
  return integer_times_pow10<double>(mantissa, decimal_exponent);
}

// the following overloads are here to avoid surprising ambiguity for int,
// unsigned, etc.
template <typename T, typename Int>
FASTFLOAT_CONSTEXPR20 typename std::enable_if<
    is_supported_float_type<T>::value && std::is_integral<Int>::value &&
        sizeof(Int) <= sizeof(uint64_t) && !std::is_signed<Int>::value,
    T>::type
integer_times_pow10(Int mantissa, int decimal_exponent) noexcept {
  return integer_times_pow10<T>(static_cast<uint64_t>(mantissa),
                                decimal_exponent);
}

template <typename T, typename Int>
FASTFLOAT_CONSTEXPR20 typename std::enable_if<
    is_supported_float_type<T>::value && std::is_integral<Int>::value &&
        sizeof(Int) <= sizeof(int64_t) && std::is_signed<Int>::value,
    T>::type
integer_times_pow10(Int mantissa, int decimal_exponent) noexcept {
  return integer_times_pow10<T>(static_cast<int64_t>(mantissa),
                                decimal_exponent);
}

template <typename Int>
FASTFLOAT_CONSTEXPR20
    typename std::enable_if<std::is_integral<Int>::value &&
                                sizeof(Int) <= sizeof(uint64_t) &&
                                !std::is_signed<Int>::value,
                            double>::type
    integer_times_pow10(Int mantissa, int decimal_exponent) noexcept {
  return integer_times_pow10(static_cast<uint64_t>(mantissa), decimal_exponent);
}

template <typename Int>
FASTFLOAT_CONSTEXPR20
    typename std::enable_if<std::is_integral<Int>::value &&
                                sizeof(Int) <= sizeof(int64_t) &&
                                std::is_signed<Int>::value,
                            double>::type
    integer_times_pow10(Int mantissa, int decimal_exponent) noexcept {
  return integer_times_pow10(static_cast<int64_t>(mantissa), decimal_exponent);
}

// Clang declines to inline the integer parser once it carries the SWAR
// short-integer path, which leaves base and format as runtime values. This
// and from_chars(..., int base) are forced inline under clang only; see
// fastfloat_clang_really_inline.
template <typename T, typename UC>
fastfloat_clang_really_inline FASTFLOAT_CONSTEXPR20 from_chars_result_t<UC>
from_chars_int_advanced(UC const *first, UC const *last, T &value,
                        parse_options_t<UC> options) noexcept {

  static_assert(is_supported_integer_type<T>::value,
                "only integer types are supported");
  static_assert(is_supported_char_type<UC>::value,
                "only char, wchar_t, char16_t and char32_t are supported");

  chars_format const fmt = detail::adjust_for_feature_macros(options.format);
  int const base = options.base;

  from_chars_result_t<UC> answer;
  if (uint64_t(fmt & chars_format::skip_white_space)) {
    while ((first != last) && fast_float::is_space(*first)) {
      first++;
    }
  }
  if (first == last || base < 2 || base > 36) {
    answer.ec = std::errc::invalid_argument;
    answer.ptr = first;
    return answer;
  }

  return parse_int_string(first, last, value, options);
}

template <size_t TypeIx> struct from_chars_advanced_caller {
  static_assert(TypeIx > 0, "unsupported type");
};

template <> struct from_chars_advanced_caller<1> {
  template <typename T, typename UC>
  fastfloat_really_inline FASTFLOAT_CONSTEXPR20 static from_chars_result_t<UC>
  call(UC const *first, UC const *last, T &value,
       parse_options_t<UC> options) noexcept {
    return from_chars_float_advanced(first, last, value, options);
  }
};

template <> struct from_chars_advanced_caller<2> {
  template <typename T, typename UC>
  fastfloat_really_inline FASTFLOAT_CONSTEXPR20 static from_chars_result_t<UC>
  call(UC const *first, UC const *last, T &value,
       parse_options_t<UC> options) noexcept {
    return from_chars_int_advanced(first, last, value, options);
  }
};

template <typename T, typename UC>
fastfloat_really_inline FASTFLOAT_CONSTEXPR20 from_chars_result_t<UC>
from_chars_advanced(UC const *first, UC const *last, T &value,
                    parse_options_t<UC> options) noexcept {
  return from_chars_advanced_caller<
      size_t(is_supported_float_type<T>::value) +
      2 * size_t(is_supported_integer_type<T>::value)>::call(first, last, value,
                                                             options);
}

} // namespace fast_float

#endif
