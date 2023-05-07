#ifndef FASTFLOAT_ASCII_NUMBER_H
#define FASTFLOAT_ASCII_NUMBER_H

#include <cctype>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <type_traits>

#include "float_common.h"

#ifdef FASTFLOAT_SSE2
#include <emmintrin.h>
#endif


namespace fast_float {

template <typename UC>
fastfloat_really_inline constexpr bool has_simd_opts() {
#ifdef FASTFLOAT_HAS_SIMD
  return std::is_same<UC, char16_t>::value;
#else
  return false;
#endif
}

// Next function can be micro-optimized, but compilers are entirely
// able to optimize it well.
template <typename UC>
fastfloat_really_inline constexpr bool is_integer(UC c) noexcept {
  return !(c > UC('9') || c < UC('0'));
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

// Read 8 UC into a u64. Truncates UC if not char.
template <typename UC>
fastfloat_really_inline FASTFLOAT_CONSTEXPR20
uint64_t read8_to_u64(const UC *chars) {
  if (cpp20_and_in_constexpr() || !std::is_same<UC, char>::value) {
    uint64_t val = 0;
    for(int i = 0; i < 8; ++i) {
      val |= uint64_t(uint8_t(*chars)) << (i*8);
      ++chars;
    }
    return val;
  }
  uint64_t val;
  ::memcpy(&val, chars, sizeof(uint64_t));
#if FASTFLOAT_IS_BIG_ENDIAN == 1
  // Need to read as-if the number was in little-endian order.
  val = byteswap(val);
#endif
  return val;
}

#ifdef FASTFLOAT_SSE2

fastfloat_really_inline
uint64_t simd_read8_to_u64(const __m128i data) {
FASTFLOAT_SIMD_DISABLE_WARNINGS
  static const char16_t kmasks[] = { 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff };
  const __m128i masks = _mm_loadu_si128(reinterpret_cast<const __m128i*>(kmasks));

  // todo: with AVX512BW + AVX512VL, can use cvtepi16_epi8 instead of masking + pack
  __m128i masked = _mm_and_si128(data, masks);
  __m128i packed = _mm_packus_epi16(masked, masked);

  uint64_t val;
  _mm_storeu_si64(&val, packed);
  return val;
FASTFLOAT_SIMD_RESTORE_WARNINGS
}

fastfloat_really_inline
uint64_t simd_read8_to_u64(const char16_t* chars) {
FASTFLOAT_SIMD_DISABLE_WARNINGS
  return simd_read8_to_u64(_mm_loadu_si128(reinterpret_cast<const __m128i*>(chars)));
FASTFLOAT_SIMD_RESTORE_WARNINGS
}

#endif

// dummy for compile
template <typename UC, FASTFLOAT_ENABLE_IF(!has_simd_opts<UC>())>
uint64_t simd_read8_to_u64(UC const*) {
  return 0;
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


// Call this if chars are definitely 8 digits.
template <typename UC>
fastfloat_really_inline FASTFLOAT_CONSTEXPR20
uint32_t parse_eight_digits_unrolled(UC const * chars)  noexcept {
  if (cpp20_and_in_constexpr() || !has_simd_opts<UC>()) {
    return parse_eight_digits_unrolled(read8_to_u64(chars)); // truncation okay
  }
  return parse_eight_digits_unrolled(simd_read8_to_u64(chars));
}


// credit @aqrit
fastfloat_really_inline constexpr bool is_made_of_eight_digits_fast(uint64_t val)  noexcept {
  return !((((val + 0x4646464646464646) | (val - 0x3030303030303030)) &
     0x8080808080808080));
}

fastfloat_really_inline FASTFLOAT_CONSTEXPR20
bool parse_if_eight_digits_unrolled(const char* chars, uint64_t& i) noexcept {
  if (is_made_of_eight_digits_fast(read8_to_u64(chars))) {
    i = i * 100000000 + parse_eight_digits_unrolled(read8_to_u64(chars));
    return true;
  }
  return false;
}

// Call this if chars might not be 8 digits.
// Using this style (instead of is_made_of_eight_digits_fast() then parse_eight_digits_unrolled())
// ensures we don't load SIMD registers twice if we don't have to.
//
// Benchmark:
// https://quick-bench.com/q/Bbn0B4WmZsdgS3qDZWpggAY-jgs
//
fastfloat_really_inline FASTFLOAT_CONSTEXPR20
bool parse_if_eight_digits_unrolled(const char16_t* chars, uint64_t& i) noexcept {
#ifdef FASTFLOAT_SSE2
  if (cpp20_and_in_constexpr()) {
    return false;
  }    
FASTFLOAT_SIMD_DISABLE_WARNINGS
  const __m128i data = _mm_loadu_si128(reinterpret_cast<const __m128i*>(chars));

  // (x - '0') <= 9
  // http://0x80.pl/articles/simd-parsing-int-sequences.html
  const __m128i t0 = _mm_sub_epi16(data, _mm_set1_epi16(80));
  const __m128i t1 = _mm_cmpgt_epi16(t0, _mm_set1_epi16(-119));

  if (_mm_movemask_epi8(t1) == 0) {
    i = i * 100000000 + parse_eight_digits_unrolled(simd_read8_to_u64(data));
    return true;
  }
  else return false;
FASTFLOAT_SIMD_RESTORE_WARNINGS

#else // No SIMD available

  (void)chars; (void)i; // unused
  return false;
#endif
}

// todo, no simd optimization yet
fastfloat_really_inline FASTFLOAT_CONSTEXPR20
bool parse_if_eight_digits_unrolled(const char32_t*, uint64_t&) noexcept {
  return false;
}

template <typename UC>
struct parsed_number_string_t {
  int64_t exponent{0};
  uint64_t mantissa{0};
  int64_t exp_number{0};
  UC const * lastmatch{nullptr};
  bool negative{false};
  bool valid{false};
  bool too_many_digits{false};
  // contains the range of the significant digits
  span<const UC> integer{};  // non-nullable
  span<const UC> fraction{}; // nullable
};

using byte_span = span<const char>;
using parsed_number_string = parsed_number_string_t<char>;

// Assuming that you use no more than 19 digits, this will
// parse an ASCII string.
template <typename UC>
fastfloat_really_inline FASTFLOAT_CONSTEXPR20
parsed_number_string_t<UC> parse_number_string(UC const *p, UC const * pend, parse_options_t<UC> options) noexcept {
  chars_format const fmt = options.format;
  UC const decimal_point = options.decimal_point;

  parsed_number_string_t<UC> answer;
  answer.valid = false;
  answer.too_many_digits = false;
  answer.negative = (*p == UC('-'));
#ifdef FASTFLOAT_ALLOWS_LEADING_PLUS // disabled by default
  if ((*p == UC('-')) || (*p == UC('+'))) {
#else
  if (*p == UC('-')) { // C++17 20.19.3.(7.1) explicitly forbids '+' sign here
#endif
    ++p;
    if (p == pend) {
      return answer;
    }
    if (!is_integer(*p) && (*p != decimal_point)) { // a sign must be followed by an integer or the dot
      return answer;
    }
  }
  UC const * const start_digits = p;

  uint64_t i = 0; // an unsigned int avoids signed overflows (which are bad)

  while ((p != pend) && is_integer(*p)) {
    // a multiplication by 10 is cheaper than an arbitrary integer
    // multiplication
    i = 10 * i +
        uint64_t(*p - UC('0')); // might overflow, we will handle the overflow later
    ++p;
  }
  UC const * const end_of_integer_part = p;
  int64_t digit_count = int64_t(end_of_integer_part - start_digits);
  answer.integer = span<const UC>(start_digits, size_t(digit_count));
  int64_t exponent = 0;
  if ((p != pend) && (*p == decimal_point)) {
    ++p;
    UC const * before = p;
    // can occur at most twice without overflowing, but let it occur more, since
    // for integers with many digits, digit parsing is the primary bottleneck.
    while ((std::distance(p, pend) >= 8) && parse_if_eight_digits_unrolled(p, i)) {  // in rare cases, this will overflow, but that's ok
      p += 8;
    }
    while ((p != pend) && is_integer(*p)) {
      uint8_t digit = uint8_t(*p - UC('0'));
      ++p;
      i = i * 10 + digit; // in rare cases, this will overflow, but that's ok
    }
    exponent = before - p;
    answer.fraction = span<const UC>(before, size_t(p - before));
    digit_count -= exponent;
  }
  // we must have encountered at least one integer!
  if (digit_count == 0) {
    return answer;
  }
  int64_t exp_number = 0;            // explicit exponential part
  if ((fmt & chars_format::scientific) && (p != pend) && ((UC('e') == *p) || (UC('E') == *p))) {
    UC const * location_of_e = p;
    ++p;
    bool neg_exp = false;
    if ((p != pend) && (UC('-') == *p)) {
      neg_exp = true;
      ++p;
    } else if ((p != pend) && (UC('+') == *p)) { // '+' on exponent is allowed by C++17 20.19.3.(7.1)
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
        uint8_t digit = uint8_t(*p - UC('0'));
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
    UC const * start = start_digits;
    while ((start != pend) && (*start == UC('0') || *start == decimal_point)) {
      if(*start == UC('0')) { digit_count --; }
      start++;
    }

    // exponent/mantissa must be truncated later!
    // this is unlikely, so don't inline truncation code with the rest of parse_number_string()
    answer.too_many_digits = digit_count > 19;
  }
  answer.exponent = exponent;
  answer.mantissa = i;
  return answer;
}

template <typename UC>
fastfloat_really_inline FASTFLOAT_CONSTEXPR20
void parse_truncated_number_string(parsed_number_string_t<UC>& ps)
{
  // Let us start again, this time, avoiding overflows.
  // We don't need to check if is_integer, since we use the
  // pre-tokenized spans.
  uint64_t i = 0;
  int64_t exponent = 0;
  const UC* p = ps.integer.ptr;
  const UC* const int_end = p + ps.integer.len();
  const uint64_t minimal_nineteen_digit_integer{1000000000000000000};
  while ((i < minimal_nineteen_digit_integer) && (p != int_end)) {
    i = i * 10 + uint64_t(*p - UC('0'));
    ++p;
  }
  if (i >= minimal_nineteen_digit_integer) { // We have a big integers
    exponent = int_end - p + ps.exp_number;
  }
  else { // We have a value with a fractional component.
    p = ps.fraction.ptr;
    const UC* const frac_end = p + ps.fraction.len();
    while ((i < minimal_nineteen_digit_integer) && (p != frac_end)) {
      i = i * 10 + uint64_t(*p - UC('0'));
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
