#ifndef FASTFLOAT_ASCII_NUMBER_H
#define FASTFLOAT_ASCII_NUMBER_H

#include <cctype>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <type_traits>

#include "float_common.h"

#if FASTFLOAT_SSE2
#include <emmintrin.h>
#endif


namespace fast_float {

// Next function can be micro-optimized, but compilers are entirely
// able to optimize it well.
template <typename CharT>
fastfloat_really_inline constexpr bool is_integer(CharT c) noexcept {
  return c >= static_cast<CharT>('0') && c <= static_cast<CharT>('9');
}

fastfloat_really_inline constexpr uint64_t byteswap(uint64_t val) {
  return (val & 0xFF00000000000000) >> 56
    | (val & 0x00FF000000000000) >> 40
    | (val & 0x0000FF0000000000) >> 24
    | (val & 0x000000FF00000000) >> 8
    | (val & 0x00000000FF000000) << 8
    | (val & 0x0000000000FF0000) << 24
    | (val & 0x000000000000FF00) << 40
    | (val & 0x00000000000000FF) << 56;
}

fastfloat_really_inline
uint64_t fast_read_u64(const char* chars) {
  uint64_t val;
  ::memcpy(&val, chars, sizeof(uint64_t));
  return val;
}

// https://quick-bench.com/q/fk6Y07KDGu8XZ9iUtQD8QJTc3Hg
fastfloat_really_inline
uint64_t fast_read_u64(const char16_t* chars) {
#if FASTFLOAT_SSE2
FASTFLOAT_SIMD_DISABLE_WARNINGS
  static const char16_t masks[] = {0xff, 0xff, 0xff, 0xff};
  const __m128i m_masks = _mm_loadu_si128(reinterpret_cast<const __m128i*>(masks));

  // mask hi bytes and pack
  const char* const p = reinterpret_cast<const char*>(chars);
  __m128i i1 = _mm_and_si128(_mm_loadu_si64(p), m_masks);
  __m128i i2 = _mm_and_si128(_mm_loadu_si64(p + 8), m_masks);
  __m128i packed = _mm_packus_epi16(i1, i2);

  // extract
  uint64_t val;
  _mm_storeu_si64(&val, _mm_shuffle_epi32(packed, 0x8));
  return val;
FASTFLOAT_SIMD_RESTORE_WARNINGS
#else
  unsigned char bytes[8];
  for (int i = 0; i < 8; ++i)
      bytes[i] = (unsigned char)chars[i];

  uint64_t val;
  ::memcpy(&val, bytes, sizeof(uint64_t));
  return val;
#endif
}

template <typename CharT>
fastfloat_really_inline FASTFLOAT_CONSTEXPR20
uint64_t read_u64(const CharT *chars) {
  if (cpp20_and_in_constexpr()) {
    uint64_t val = 0;
    for(int i = 0; i < 8; ++i) {
      val |= uint64_t(*chars) << (i*8);
      ++chars;
    }
    return val;
  }
  uint64_t val = fast_read_u64(chars);
#if FASTFLOAT_IS_BIG_ENDIAN == 1
  // Need to read as-if the number was in little-endian order.
  val = byteswap(val);
#endif
  return val;
}


fastfloat_really_inline FASTFLOAT_CONSTEXPR20
void write_u64(uint8_t *chars, uint64_t val) {
  if (cpp20_and_in_constexpr()) {
    for(int i = 0; i < 8; ++i) {
      *chars = uint8_t(val);
      val >>= 8;
      ++chars;
    }
    return;
  }
#if FASTFLOAT_IS_BIG_ENDIAN == 1
  // Need to read as-if the number was in little-endian order.
  val = byteswap(val);
#endif
  ::memcpy(chars, &val, sizeof(uint64_t));
}

// credit  @aqrit
fastfloat_really_inline FASTFLOAT_CONSTEXPR14
uint32_t parse_eight_digits_unrolled(uint64_t val) {
  const uint64_t mask = 0x000000FF000000FF;
  const uint64_t mul1 = 0x000F424000000064; // 100 + (1000000ULL << 32)
  const uint64_t mul2 = 0x0000271000000001; // 1 + (10000ULL << 32)
  val -= 0x3030303030303030;
  val = (val * 10) + (val >> 8); // val = (val * 2561) >> 8;
  val = (((val & mask) * mul1) + (((val >> 16) & mask) * mul2)) >> 32;
  return uint32_t(val);
}

// http://0x80.pl/articles/simd-parsing-int-sequences.html
#if FASTFLOAT_SSE2
fastfloat_really_inline
uint32_t parse_eight_digits_unrolled_c16(const __m128i val) {
  // x - '0'
  const __m128i s1digits16 = _mm_sub_epi16(val, _mm_set1_epi16('0'));
  // 10 * x(b) + x(b-1) -> 2 digit numbers
  const __m128i s2digits32 = _mm_madd_epi16(s1digits16, _mm_setr_epi16(10, 1, 10, 1, 10, 1, 10, 1));
  const __m128i s2digits16 = _mm_packus_epi16(s2digits32, s2digits32);
  // 100 * x(b) + x(b-1) -> 4 digit numbers
  const __m128i s4digits32 = _mm_madd_epi16(s2digits16, _mm_setr_epi16(100, 1, 100, 1, 100, 1, 100, 1));
  const __m128i s4digits16 = _mm_packus_epi16(s4digits32, s4digits32);
  // 10000 * x(b) + x(b-1) -> 8 digit number
  const __m128i s8digits32 = _mm_madd_epi16(s4digits16, _mm_setr_epi16(10000, 1, 10000, 1, 10000, 1, 10000, 1));

  uint32_t value;
  _mm_storeu_si32(&value, s8digits32);
  return value;
}
#endif

// credit @aqrit
fastfloat_really_inline constexpr bool is_made_of_eight_digits_fast(uint64_t val)  noexcept  {
  return !((((val + 0x4646464646464646) | (val - 0x3030303030303030)) &
     0x8080808080808080));
}

fastfloat_really_inline FASTFLOAT_CONSTEXPR20
uint32_t parse_eight_digits_unrolled(const char* chars)  noexcept {
    return parse_eight_digits_unrolled(read_u64(chars));
}

fastfloat_really_inline FASTFLOAT_CONSTEXPR20
uint32_t parse_eight_digits_unrolled(const char16_t* chars)  noexcept {
  if (cpp20_and_in_constexpr() || !has_simd()) {
    return parse_eight_digits_unrolled(read_u64(chars));
  }
#if !FASTFLOAT_HAS_SIMD
  return 0; // never reaches here, satisfy compiler
#else
FASTFLOAT_SIMD_DISABLE_WARNINGS
  return parse_eight_digits_unrolled_c16(_mm_loadu_si128(reinterpret_cast<const __m128i*>(chars)));
FASTFLOAT_SIMD_RESTORE_WARNINGS
#endif
}

fastfloat_really_inline FASTFLOAT_CONSTEXPR20
bool parse_if_eight_digits_unrolled(const char* chars, std::uint64_t& i) noexcept {
    const bool all = is_made_of_eight_digits_fast(read_u64(chars));
    if (all) i = i * 100000000 + parse_eight_digits_unrolled(read_u64(chars));
    return all;
}

// http://0x80.pl/articles/simd-parsing-int-sequences.html
fastfloat_really_inline FASTFLOAT_CONSTEXPR20
bool parse_if_eight_digits_unrolled(const char16_t* chars, std::uint64_t& i) noexcept {
  if (cpp20_and_in_constexpr() || !has_simd()) {
    for (int i = 0; i < 8; ++i) {
      if (chars[i] < u'0' || chars[i] > u'9')
        return false;
    }
    i = i * 100000000 + parse_eight_digits_unrolled(read_u64(chars));
    return true;
  }
#if !FASTFLOAT_HAS_SIMD
  return false; // never reaches here, satisfy compiler
#else
FASTFLOAT_SIMD_DISABLE_WARNINGS
  const __m128i data = _mm_loadu_si128(reinterpret_cast<const __m128i*>(chars));
  // (x - '0') <= 9
  const __m128i t0 = _mm_sub_epi16(data, _mm_set1_epi16(80));
  const __m128i t1 = _mm_cmpgt_epi16(t0, _mm_set1_epi16(-119));
  const bool is_digits = _mm_movemask_epi8(t1) == 0;

  if (is_digits) {
    i = i * 100000000 + parse_eight_digits_unrolled_c16(data);
    return true;
  }
  else return false;
FASTFLOAT_SIMD_RESTORE_WARNINGS
#endif
}


typedef span<const char> byte_span;

template <typename CharT>
struct parsed_number_string {
  int64_t exponent{0};
  uint64_t mantissa{0};
  int64_t exp_number{0};
  const CharT *lastmatch{nullptr};
  bool negative{false};
  bool valid{false};
  bool too_many_digits{false};
  // contains the range of the significant digits
  span<const CharT> integer{};  // non-nullable
  span<const CharT> fraction{}; // nullable
};

// Assuming that you use no more than 19 digits, this will
// parse an ASCII string.
template <typename CharT>
fastfloat_really_inline FASTFLOAT_CONSTEXPR20
parsed_number_string<CharT> parse_number_string(const CharT *p, const CharT *pend, parse_options options) noexcept {
  const chars_format fmt = options.format;
  const parse_rules rules = options.rules;
  const CharT decimal_point = static_cast<CharT>(options.decimal_point);

  parsed_number_string<CharT> answer;
  answer.valid = false;
  answer.too_many_digits = false;
  answer.negative = (*p == static_cast<CharT>('-'));
#if FASTFLOAT_ALLOWS_LEADING_PLUS // disabled by default
  if ((*p == static_cast<CharT>('-')) || (*p == static_cast<CharT>('+'))) {
#else
  if (*p == static_cast<CharT>('-')) { // C++17 20.19.3.(7.1) explicitly forbids '+' sign here
#endif
    ++p;
    if (p == pend) {
      return answer;
    }
    // a sign must be followed by an integer or the dot
    if (!is_integer(*p) && (rules == parse_rules::json_rules || *p != decimal_point))
        return answer;
  }
  const CharT *const start_digits = p;

  uint64_t i = 0; // an unsigned int avoids signed overflows (which are bad)

  while ((p != pend) && is_integer(*p)) {
    // a multiplication by 10 is cheaper than an arbitrary integer
    // multiplication
    i = 10 * i +
        uint64_t(*p - static_cast<CharT>('0')); // might overflow, we will handle the overflow later
    ++p;
  }
  const CharT *const end_of_integer_part = p;
  int64_t digit_count = int64_t(end_of_integer_part - start_digits);
  answer.integer = span<const CharT>(start_digits, size_t(digit_count));
  int64_t exponent = 0;
  const bool has_decimal_point = (p != pend) && (*p == decimal_point);
  if (has_decimal_point) {
    ++p;
    const CharT* before = p;
    // can occur at most twice without overflowing, but let it occur more, since
    // for integers with many digits, digit parsing is the primary bottleneck.
    while ((std::distance(p, pend) >= 8) && parse_if_eight_digits_unrolled(p, i)) {  // in rare cases, this will overflow, but that's ok
      p += 8;
    }
    while ((p != pend) && is_integer(*p)) {
      i = i * 10 + uint64_t(*p - static_cast<CharT>('0')); // in rare cases, this will overflow, but that's ok
      ++p;
    }
    exponent = before - p;
    answer.fraction = span<const CharT>(before, size_t(p - before));
    digit_count -= exponent;
  }
  // we must have encountered at least one integer (or two if a decimal point exists, with json rules).
  if (digit_count == 0 || (rules == parse_rules::json_rules && has_decimal_point && digit_count == 1)) {
    return answer;
  }
  int64_t exp_number = 0;            // explicit exponential part
  if ((fmt & chars_format::scientific) && (p != pend) && ((static_cast<CharT>('e') == *p) || (static_cast<CharT>('E') == *p))) {
    const CharT * location_of_e = p;
    ++p;
    bool neg_exp = false;
    if ((p != pend) && (static_cast<CharT>('-') == *p)) {
      neg_exp = true;
      ++p;
    } else if ((p != pend) && (static_cast<CharT>('+') == *p)) { // '+' on exponent is allowed by C++17 20.19.3.(7.1)
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
        uint8_t digit = uint8_t(*p - static_cast<CharT>('0'));
        if (exp_number < 0x10000000) {
          exp_number = 10 * exp_number + digit;
        }
        ++p;
      }
      if(neg_exp) { exp_number = - exp_number; }
      exponent += exp_number;
    }
  } else {
    // If it scientific and not fixed, we have to bail out.
    if((fmt & chars_format::scientific) && !(fmt & chars_format::fixed)) { return answer; }
  }
  
  // disallow leading zeros before the decimal point
  if (rules == parse_rules::json_rules && start_digits[0] == static_cast<CharT>('0') && digit_count >= 2 && is_integer(start_digits[1]))
      return answer;

  answer.lastmatch = p;
  answer.valid = true;
  answer.exp_number = exp_number;

  // If we frequently had to deal with long strings of digits,
  // we could extend our code by using a 128-bit integer instead
  // of a 64-bit integer. However, this is uncommon.
  //
  // We can deal with up to 19 digits.
  if (digit_count > 19) { // this is uncommon
    // It is possible that the integer had an overflow.
    // We have to handle the case where we have 0.0000somenumber.
    // We need to be mindful of the case where we only have zeroes...
    // E.g., 0.000000000...000.
    const CharT *start = start_digits;
    while ((start != pend) && (*start == static_cast<CharT>('0') || *start == decimal_point)) {
      if(*start == static_cast<CharT>('0')) { digit_count --; }
      start++;
    }

    // exponent/mantissa must be truncated later
    answer.too_many_digits = digit_count > 19;
  }
  answer.exponent = exponent;
  answer.mantissa = i;
  return answer;
}

template <typename CharT>
fastfloat_really_inline FASTFLOAT_CONSTEXPR20
void truncate_exponent_mantissa(parsed_number_string<CharT>& ps)
{
  // Let us start again, this time, avoiding overflows.
  // We don't need to check if is_integer, since we use the
  // pre-tokenized spans.
  uint64_t i = 0;
  int64_t exponent = 0;
  const CharT* p = ps.integer.ptr;
  const CharT* const int_end = p + ps.integer.len();
  const uint64_t minimal_nineteen_digit_integer{1000000000000000000};
  while ((i < minimal_nineteen_digit_integer) && (p != int_end)) {
    i = i * 10 + uint64_t(*p - static_cast<CharT>('0'));
    ++p;
  }
  if (i >= minimal_nineteen_digit_integer) { // We have a big integers
    exponent = int_end - p + ps.exp_number;
  }
  else { // We have a value with a fractional component.
    p = ps.fraction.ptr;
    const CharT* const frac_end = p + ps.fraction.len();
    while ((i < minimal_nineteen_digit_integer) && (p != frac_end)) {
      i = i * 10 + uint64_t(*p - static_cast<CharT>('0'));
      ++p;
    }
    exponent = ps.fraction.ptr - p + ps.exp_number;
  }
  // We have now corrected both exponent and i, to a truncated value

  ps.exponent = exponent;
  ps.mantissa = i;
}

} // namespace fast_float

#endif
