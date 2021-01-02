#ifndef FASTFLOAT_PARSE_NUMBER_H
#define FASTFLOAT_PARSE_NUMBER_H
#include "ascii_number.h"
#include "decimal_to_binary.h"
#include "simple_decimal_conversion.h"

#include <cassert>
#include <cmath>
#include <cstring>
#include <limits>
#include <system_error>

namespace fast_float {


namespace {
/**
 * Special case +inf, -inf, nan, infinity, -infinity.
 * The case comparisons could be made much faster given that we know that the
 * strings a null-free and fixed.
 **/
template <typename T>
from_chars_result parse_infnan(const char *first, const char *last, T &value)  noexcept  {
  from_chars_result answer;
  answer.ec = std::errc(); // be optimistic
  if (last - first >= 3) {
    if (fastfloat_strncasecmp(first, "nan", 3)) {
      answer.ptr = first + 3;
      value = std::numeric_limits<T>::quiet_NaN();
      return answer;
    }
    if (fastfloat_strncasecmp(first, "inf", 3)) {
      if ((last - first >= 8) && fastfloat_strncasecmp(first, "infinity", 8)) {
        answer.ptr = first + 8;
      } else {
        answer.ptr = first + 3;
      }
      value = std::numeric_limits<T>::infinity();
      return answer;
    }
    if (last - first >= 4) {
      if (fastfloat_strncasecmp(first, "+nan", 4) || fastfloat_strncasecmp(first, "-nan", 4)) {
        answer.ptr = first + 4;
        value = std::numeric_limits<T>::quiet_NaN();
        if (first[0] == '-') {
          value = -value;
        }
        return answer;
      }

      if (fastfloat_strncasecmp(first, "+inf", 4) || fastfloat_strncasecmp(first, "-inf", 4)) {
        if ((last - first >= 8) && fastfloat_strncasecmp(first + 1, "infinity", 8)) {
          answer.ptr = first + 9;
        } else {
          answer.ptr = first + 4;
        }
        value = std::numeric_limits<T>::infinity();
        if (first[0] == '-') {
          value = -value;
        }
        return answer;
      }
    }
  }
  answer.ec = std::errc::invalid_argument;
  answer.ptr = first;
  return answer;
}
} // namespace


template<typename T>
from_chars_result from_chars(const char *first, const char *last,
                             T &value, chars_format fmt /*= chars_format::general*/)  noexcept  {
  static_assert (std::is_same<T, double>::value || std::is_same<T, float>::value, "only float and double are supported");


  from_chars_result answer;
  while ((first != last) && fast_float::is_space(uint8_t(*first))) {
    first++;
  }
  if (first == last) {
    answer.ec = std::errc::invalid_argument;
    answer.ptr = first;
    return answer;
  }
  parsed_number_string pns = parse_number_string(first, last, fmt);
  if (!pns.valid) {
    return parse_infnan(first, last, value);
  }
  answer.ec = std::errc(); // be optimistic
  answer.ptr = pns.lastmatch;
  adjusted_mantissa am;
  // Most times, we have pns.too_many_digits = false.
  if(pns.too_many_digits) {
    // credit: R. Oudompheng who first implemented this fast path.
    // It does the job of accelerating the slow path since most
    // long streams of digits are determined after 19 digits.
    // Note that mantissa+1 cannot overflow since mantissa < 10**19 and so
    // mantissa+1 <= 10**19 < 2**64.
    adjusted_mantissa am1 = compute_float<binary_format<T>>(pns.exponent, pns.mantissa);
    adjusted_mantissa am2 = compute_float<binary_format<T>>(pns.exponent, pns.mantissa+1);
    // They must both agree and be both a successful result.
    if(( am1 == am2 ) && (am1.power2 >= 0)) {
      am = am1;
    } else {
      // long way! (uncommon)
      decimal d = parse_decimal(first, last);
      am = compute_float<binary_format<T>>(d);
    }
  } else {
    // We are entering the common path where the number of digits is no more than 19.
    //
    // Next is Clinger's fast path.
    if (binary_format<T>::min_exponent_fast_path() <= pns.exponent && pns.exponent <= binary_format<T>::max_exponent_fast_path() && pns.mantissa <=binary_format<T>::max_mantissa_fast_path()) {
      value = T(pns.mantissa);
      if (pns.exponent < 0) { value = value / binary_format<T>::exact_power_of_ten(-pns.exponent); }
      else { value = value * binary_format<T>::exact_power_of_ten(pns.exponent); }
      if (pns.negative) { value = -value; }
      return answer;
    }
    // Then we have our main routine.
    am = compute_float<binary_format<T>>(pns.exponent, pns.mantissa);
    // If we called compute_float<binary_format<T>>(pns.exponent, pns.mantissa) and we have an invalid power (am.power2 < 0),
    // then we need to go the long way around again. This is very uncommon.
    if(am.power2 < 0) { // long way! (uncommon)
      decimal d = parse_decimal(first, last);
      am = compute_float<binary_format<T>>(d); 
    }
  }
  uint64_t word = am.mantissa;
  word |= uint64_t(am.power2) << binary_format<T>::mantissa_explicit_bits();
  word = pns.negative
  ? word | (uint64_t(1) << binary_format<T>::sign_index()) : word;
#if FASTFLOAT_IS_BIG_ENDIAN == 1
   if (std::is_same<T, float>::value) {
     ::memcpy(&value, (char *)&word + 4, sizeof(T)); // extract value at offset 4-7 if float on big-endian
   } else {
     ::memcpy(&value, &word, sizeof(T));
   }
#else
   // For little-endian systems:
   ::memcpy(&value, &word, sizeof(T));
#endif
  return answer;
}

} // namespace fast_float

#endif
