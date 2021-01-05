#ifndef FASTFLOAT_ASCII_NUMBER_H
#define FASTFLOAT_ASCII_NUMBER_H

#include <cstdio>
#include <cctype>
#include <cstdint>
#include <cstring>

#include "float_common.h"

namespace fast_float {

fastfloat_really_inline bool is_integer(char c)  noexcept  { return (c >= '0' && c <= '9'); }


// credit: https://johnnylee-sde.github.io/Fast-numeric-string-to-int/
fastfloat_really_inline uint32_t parse_eight_digits_unrolled(const char *chars)  noexcept  {
  uint64_t val;
  ::memcpy(&val, chars, sizeof(uint64_t));
  val = (val & 0x0F0F0F0F0F0F0F0F) * 2561 >> 8;
  val = (val & 0x00FF00FF00FF00FF) * 6553601 >> 16;
  return uint32_t((val & 0x0000FFFF0000FFFF) * 42949672960001 >> 32);
}

fastfloat_really_inline bool is_made_of_eight_digits_fast(uint64_t val)  noexcept  {
  return (((val & 0xF0F0F0F0F0F0F0F0) |
           (((val + 0x0606060606060606) & 0xF0F0F0F0F0F0F0F0) >> 4)) ==
          0x3333333333333333);
}


fastfloat_really_inline bool is_made_of_eight_digits_fast(const char *chars)  noexcept  {
  uint64_t val;
  ::memcpy(&val, chars, 8);
  return is_made_of_eight_digits_fast(val);
}


fastfloat_really_inline uint32_t parse_four_digits_unrolled(const char *chars)  noexcept  {
  uint32_t val;
  ::memcpy(&val, chars, sizeof(uint32_t));
  val = (val & 0x0F0F0F0F) * 2561 >> 8;
  return (val & 0x00FF00FF) * 6553601 >> 16;
}

fastfloat_really_inline bool is_made_of_four_digits_fast(const char *chars)  noexcept  {
  uint32_t val;
  ::memcpy(&val, chars, 4);
  return (((val & 0xF0F0F0F0) |
           (((val + 0x06060606) & 0xF0F0F0F0) >> 4)) ==
          0x33333333);
}

struct parsed_number_string {
  int64_t exponent;
  uint64_t mantissa;
  const char *lastmatch;
  bool negative;
  bool valid;
  bool too_many_digits;
};



// Assuming that you use no more than 19 digits, this will
// parse an ASCII string.
fastfloat_really_inline
parsed_number_string parse_number_string(const char *p, const char *pend, chars_format fmt) noexcept {
  parsed_number_string answer;
  answer.valid = false;
  answer.too_many_digits = false;
  answer.negative = (*p == '-');
  if ((*p == '-') || (*p == '+')) {
    ++p;
    if (p == pend) {
      return answer;
    }
    if (!is_integer(*p) && (*p != '.')) { // a  sign must be followed by an integer or the dot
      return answer;
    }
  }
  const char *const start_digits = p;
  // We can go forward up to 19 characters without overflow for sure, we might even go 20 characters
  // or more  if we have a decimal separator. We will adjust accordingly.
  const char *pend_overflow_free = p + 19 > pend ? pend : p + 19;
  uint64_t i = 0; // an unsigned int avoids signed overflows (which are bad)

  while ((p != pend_overflow_free) && is_integer(*p)) {
    // a multiplication by 10 is cheaper than an arbitrary integer
    // multiplication
    i = 10 * i +
        uint64_t(*p - '0');
    ++p;
  }
  int64_t exponent = 0;
  constexpr uint64_t minimal_nineteen_digit_integer{1000000000000000000};
  if (p == pend_overflow_free) { // uncommon path!
    // We enter this branch if we hit p == pend_overflow_free 
    // before the decimal separator so we have a big integer.
    // E.g., 23123123232132...3232321
    // This is very uncommon.
    while((i < minimal_nineteen_digit_integer) && (p != pend) && is_integer(*p)) {
      i = i * 10 + uint64_t(*p - '0');
      ++p;
    }
    const char *truncated_integer_part = p;
    while ((p != pend) && is_integer(*p)) { p++; }
    answer.too_many_digits = (p != truncated_integer_part);
    if (i > minimal_nineteen_digit_integer) {
      exponent = p - truncated_integer_part;
    }
    if((p != pend) && (*p == '.')) {
      p++; // skip the '.'
      const char *first_after_period = p;
      while((i < minimal_nineteen_digit_integer) && (p != pend) && is_integer(*p)) {
        i = i * 10 + uint64_t(*p - '0');
        ++p;
      }
      const char *truncated_point = p;
      if(p > first_after_period) {
        exponent = first_after_period - p;
      }
      // next we truncate:
      while ((p != pend) && is_integer(*p)) { p++; }
      answer.too_many_digits |= (p != truncated_point);
    }
  } else if (*p == '.') {
    pend_overflow_free++; // go one further thanks to '.'
    ++p;
    const char *first_after_period = p;
#if FASTFLOAT_IS_BIG_ENDIAN == 0
    // Fast approach only tested under little endian systems
    if ((p + 8 <= pend_overflow_free) && is_made_of_eight_digits_fast(p)) {
      i = i * 100000000 + parse_eight_digits_unrolled(p);
      p += 8;
      if ((p + 8 <= pend_overflow_free) && is_made_of_eight_digits_fast(p)) {
        i = i * 100000000 + parse_eight_digits_unrolled(p);
        p += 8;
      }
    }
#endif
    while ((p != pend_overflow_free) && is_integer(*p)) {
      uint8_t digit = uint8_t(*p - '0');
      ++p;
      i = i * 10 + digit;
    }
    if(p == pend_overflow_free) { // uncommon path!
      // We might have leading zeros as in 0.000001232132...
      // As long as i < minimal_nineteen_digit_integer then we know that we
      // did not hit 19 digits (omitting leading zeroes).
      while((i < minimal_nineteen_digit_integer) && (p != pend) && is_integer(*p)) {
        i = i * 10 + uint64_t(*p - '0');
        ++p;
      }
    }
    exponent = first_after_period - p;
    if((p != pend) && is_integer(*p)) { // uncommon path
      answer.too_many_digits = true;
      do { p++; } while ((p != pend) && is_integer(*p));
    }
  }
  // we must have encountered at least one integer!
  if ((start_digits == p) || ((start_digits == p - 1) && (*start_digits == '.') )) {
    return answer;
  }
  if ((fmt & chars_format::scientific) && (p != pend) && (('e' == *p) || ('E' == *p))) {
    const char * location_of_e = p;
    int64_t exp_number = 0;            // exponential part
    ++p;
    bool neg_exp = false;
    if ((p != pend) && ('-' == *p)) {
      neg_exp = true;
      ++p;
    } else if ((p != pend) && ('+' == *p)) {
      ++p;
    }
    if ((p == pend) || !is_integer(*p)) {
      if(!(fmt & chars_format::fixed)) {
        // We are in error.
        return answer;
      }
      // Otherwise, we will be ignoring the 'e'.
      p = location_of_e;
    } else {
      while ((p != pend) && is_integer(*p)) {
        uint8_t digit = uint8_t(*p - '0');
        if (exp_number < 0x10000) {
          exp_number = 10 * exp_number + digit;
        }
        ++p;
      }
      exponent += (neg_exp ? -exp_number : exp_number);
    }
  } else {
    // If it scientific and not fixed, we have to bail out.
    if((fmt & chars_format::scientific) && !(fmt & chars_format::fixed)) { return answer; }
  }
  answer.lastmatch = p;
  answer.valid = true;

  answer.exponent = exponent;
  answer.mantissa = i;
  return answer;
}

// This should always succeed since it follows a call to parse_number_string
// This function could be optimized. In particular, we could stop after 19 digits
// and try to bail out. Furthermore, we should be able to recover the computed
// exponent from the pass in parse_number_string.
decimal parse_decimal(const char *p, const char *pend) noexcept {
  decimal answer;
  answer.num_digits = 0;
  answer.decimal_point = 0;
  answer.negative = false;
  answer.truncated = false;
  // any whitespace has been skipped.
  answer.negative = (*p == '-');
  if ((*p == '-') || (*p == '+')) {
    ++p;
  }
  // skip leading zeroes
  while ((p != pend) && (*p == '0')) {
    ++p;
  }
  while ((p != pend) && is_integer(*p)) {
    if (answer.num_digits < max_digits) {
      answer.digits[answer.num_digits] = uint8_t(*p - '0');
    }
    answer.num_digits++;
    ++p;
  }
  const char *first_after_period{};
  if ((p != pend) && (*p == '.')) {
    ++p;
    first_after_period = p;
    // if we have not yet encountered a zero, we have to skip it as well
    if(answer.num_digits == 0) {
      // skip zeros
      while ((p != pend) && (*p == '0')) {
       ++p;
      }
    }
    // We expect that this loop will often take the bulk of the running time
    // because when a value has lots of digits, these digits often 
    while ((p + 8 <= pend) && (answer.num_digits + 8 < max_digits)) {
      uint64_t val;
      ::memcpy(&val, p, sizeof(uint64_t));
      if(! is_made_of_eight_digits_fast(val)) { break; }
      // We have eight digits, process them in one go!
      val -= 0x3030303030303030;
      ::memcpy(answer.digits + answer.num_digits, &val, sizeof(uint64_t));
      answer.num_digits += 8;
      p += 8;
    }
    while ((p != pend) && is_integer(*p)) {
      if (answer.num_digits < max_digits) {
        answer.digits[answer.num_digits] = uint8_t(*p - '0');
      }
      answer.num_digits++;
      ++p;
    }
    answer.decimal_point = int32_t(first_after_period - p);
  }
  if ((p != pend) && (('e' == *p) || ('E' == *p))) {
    ++p;
    bool neg_exp = false;
    if ((p != pend) && ('-' == *p)) {
      neg_exp = true;
      ++p;
    } else if ((p != pend) && ('+' == *p)) {
      ++p;
    }
    int32_t exp_number = 0; // exponential part
    while ((p != pend) && is_integer(*p)) {
      uint8_t digit = uint8_t(*p - '0');
      if (exp_number < 0x10000) {
        exp_number = 10 * exp_number + digit;
      }     
      ++p;
    }
    answer.decimal_point += (neg_exp ? -exp_number : exp_number);
  }
  answer.decimal_point += answer.num_digits;
  if(answer.num_digits > max_digits) {
    answer.truncated = true;
    answer.num_digits = max_digits;
  }
  return answer;
}
} // namespace fast_float

#endif
