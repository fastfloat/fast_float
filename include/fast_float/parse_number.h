#ifndef FASTFLOAT_PARSE_NUMBER_H
#define FASTFLOAT_PARSE_NUMBER_H

#include "ascii_number.h"
#include "decimal_to_binary.h"
#include "digit_comparison.h"

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
template <typename T>
from_chars_result parse_infnan(const char *first, const char *last, T &value)  noexcept  {
  from_chars_result answer;
  answer.ptr = first;
  answer.ec = std::errc(); // be optimistic
  bool minusSign = false;
  if (*first == '-') { // assume first < last, so dereference without checks; C++17 20.19.3.(7.1) explicitly forbids '+' here
      minusSign = true;
      ++first;
  }
  if (last - first >= 3) {
    if (fastfloat_strncasecmp(first, "nan", 3)) {
      answer.ptr = (first += 3);
      value = minusSign ? -std::numeric_limits<T>::quiet_NaN() : std::numeric_limits<T>::quiet_NaN();
      // Check for possible nan(n-char-seq-opt), C++17 20.19.3.7, C11 7.20.1.3.3. At least MSVC produces nan(ind) and nan(snan).
      if(first != last && *first == '(') {
        for(const char* ptr = first + 1; ptr != last; ++ptr) {
          if (*ptr == ')') {
            answer.ptr = ptr + 1; // valid nan(n-char-seq-opt)
            break;
          }
          else if(!(('a' <= *ptr && *ptr <= 'z') || ('A' <= *ptr && *ptr <= 'Z') || ('0' <= *ptr && *ptr <= '9') || *ptr == '_'))
            break; // forbidden char, not nan(n-char-seq-opt)
        }
      }
      return answer;
    }
    if (fastfloat_strncasecmp(first, "inf", 3)) {
      if ((last - first >= 8) && fastfloat_strncasecmp(first + 3, "inity", 5)) {
        answer.ptr = first + 8;
      } else {
        answer.ptr = first + 3;
      }
      value = minusSign ? -std::numeric_limits<T>::infinity() : std::numeric_limits<T>::infinity();
      return answer;
    }
  }
  answer.ec = std::errc::invalid_argument;
  return answer;
}

fastfloat_really_inline bool rounds_to_nearest() noexcept {
  // This function is meant to be equivalent to :
  // prior: #include <cfenv>
  //  return fegetround() == FE_TONEAREST;
  // However, it is expected to be much faster than the fegetround()
  // function call.
  //
  // volatile prevents the compiler from computing the function at compile-time
  static volatile float fmin = std::numeric_limits<float>::min();
  //
  // Explanation:
  // Only when fegetround() == FE_TONEAREST do we have that
  // fmin + 1.0f == 1.0f - fmin.
  //
  // FE_UPWARD:
  //  fmin + 1.0f = 0x1.00001 (1.00001)
  //  1.0f - fmin = 0x1 (1)
  //
  // FE_DOWNWARD or  FE_TOWARDZERO:
  //  fmin + 1.0f = 0x1 (1)
  //  1.0f - fmin = 0x0.999999 (0.999999)
  //
  //  fmin + 1.0f = 0x1 (1)
  //  1.0f - fmin = 0x0.999999 (0.999999)
  //
  // FE_TONEAREST:
  //  fmin + 1.0f = 0x1 (1)
  //  1.0f - fmin = 0x1 (1)
  //
  return (fmin + 1.0f == 1.0f - fmin);
}

} // namespace detail

template<typename T>
from_chars_result from_chars(const char *first, const char *last,
                             T &value, chars_format fmt /*= chars_format::general*/)  noexcept  {
  return from_chars_advanced(first, last, value, parse_options{fmt});
}

template<typename T>
from_chars_result from_chars_advanced(const char *first, const char *last,
                                      T &value, parse_options options)  noexcept  {

  static_assert (std::is_same<T, double>::value || std::is_same<T, float>::value, "only float and double are supported");


  from_chars_result answer;
  if (first == last) {
    answer.ec = std::errc::invalid_argument;
    answer.ptr = first;
    return answer;
  }
  parsed_number_string pns = parse_number_string(first, last, options);
  if (!pns.valid) {
    return detail::parse_infnan(first, last, value);
  }
  answer.ec = std::errc(); // be optimistic
  answer.ptr = pns.lastmatch;
  // Unfortunately, the conventional Clinger's fast path is only possible
  // when the system rounds to the nearest float.
  if(detail::rounds_to_nearest()) {
    // We have that fegetround() == FE_TONEAREST.
    // Next is Clinger's fast path.
    if (binary_format<T>::min_exponent_fast_path() <= pns.exponent && pns.exponent <= binary_format<T>::max_exponent_fast_path() && pns.mantissa <=binary_format<T>::max_mantissa_fast_path() && !pns.too_many_digits) {
      value = T(pns.mantissa);
      if (pns.exponent < 0) { value = value / binary_format<T>::exact_power_of_ten(-pns.exponent); }
      else { value = value * binary_format<T>::exact_power_of_ten(pns.exponent); }
      if (pns.negative) { value = -value; }
      return answer;
    }
  } else {
    // We do not have that fegetround() == FE_TONEAREST.
    // Next is a modified Clinger's fast path, inspired by Jakub Jelínek's proposal
    if (pns.exponent >= 0 && pns.exponent <= binary_format<T>::max_exponent_fast_path() && pns.mantissa <=binary_format<T>::max_mantissa_fast_path(pns.exponent) && !pns.too_many_digits) {
#if (defined(_WIN32) && defined(__clang__))
      // 32-bit ClangCL maps 0 to -0.0 when fegetround() == FE_DOWNWARD
      value = pns.mantissa ? T(pns.mantissa) : 0.0;
#else
      value = T(pns.mantissa);
#endif
      value = value * binary_format<T>::exact_power_of_ten(pns.exponent);
      if (pns.negative) { value = -value; }
      return answer;
    }
  }
  adjusted_mantissa am = compute_float<binary_format<T>>(pns.exponent, pns.mantissa);
  if(pns.too_many_digits && am.power2 >= 0) {
    if(am != compute_float<binary_format<T>>(pns.exponent, pns.mantissa + 1)) {
      am = compute_error<binary_format<T>>(pns.exponent, pns.mantissa);
    }
  }
  // If we called compute_float<binary_format<T>>(pns.exponent, pns.mantissa) and we have an invalid power (am.power2 < 0),
  // then we need to go the long way around again. This is very uncommon.
  if(am.power2 < 0) { am = digit_comp<T>(pns, am); }
  to_float(pns.negative, am, value);
  return answer;
}

} // namespace fast_float

#endif
