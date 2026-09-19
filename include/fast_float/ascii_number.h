#ifndef FASTFLOAT_ASCII_NUMBER_H
#define FASTFLOAT_ASCII_NUMBER_H

#include <cctype>
#include <cstdint>
#include <cstring>
#include <iterator>
#include <limits>
#include <type_traits>

#include "float_common.h"

#if FASTFLOAT_X86_SIMD
#include <immintrin.h>
#elif FASTFLOAT_ARM_NEON
#include <arm_neon.h>
#endif

namespace fast_float {

template <typename UC>
fastfloat_really_inline FASTFLOAT_CONSTEVAL bool has_simd_opt() noexcept {
#ifdef FASTFLOAT_USE_SIMD
  return std::is_same<UC, char16_t>::value;
#else
  return false;
#endif
}

// Next function can be micro-optimized, but compilers are entirely
// able to optimize it well.
template <typename UC>
fastfloat_really_inline FASTFLOAT_CONSTEXPR14 bool is_integer(UC c) noexcept {
  const auto d = c - UC('0');
  // UC may be signed.
  return d >= 0 && d <= 9;
}

#if FASTFLOAT_IS_BIG_ENDIAN

#if FASTFLOAT_HAS_BYTESWAP
namespace {
using std::byteswap;
}
#else

fastfloat_really_inline constexpr uint64_t byteswap(uint64_t val) noexcept {
  return (val & 0xFF00000000000000) >> 56 | (val & 0x00FF000000000000) >> 40 |
         (val & 0x0000FF0000000000) >> 24 | (val & 0x000000FF00000000) >> 8 |
         (val & 0x00000000FF000000) << 8 | (val & 0x0000000000FF0000) << 24 |
         (val & 0x000000000000FF00) << 40 | (val & 0x00000000000000FF) << 56;
}

fastfloat_really_inline constexpr uint32_t byteswap(uint32_t val) noexcept {
  return (val >> 24) | ((val >> 8) & 0x0000FF00u) | ((val << 8) & 0x00FF0000u) |
         (val << 24);
}

#endif

#endif

// Read UCs into an unsigned integer. Truncates UC if not char.
template <typename T, typename UC>
fastfloat_really_inline FASTFLOAT_CONSTEXPR20 T
read_chars_to_unsigned(UC const *chars) noexcept {
  if (is_constant_evaluated() || !std::is_same<UC, char>::value) {
    T val = 0;
    for (uint_fast8_t i = 0; i != sizeof(T); ++i) {
      val |= T(uint8_t(*chars)) << (i * 8);
      ++chars;
    }
    return val;
  }
  T val;
  std::memcpy(&val, chars, sizeof(T));
#if FASTFLOAT_IS_BIG_ENDIAN
  // Need to read as-if the number was in little-endian order.
  val = byteswap(val);
#endif
  return val;
}

#if FASTFLOAT_USE_SIMD

#if FASTFLOAT_X86_SIMD

fastfloat_really_inline uint64_t simd_read8(__m128i const data) {
  // _mm_packus_epi16 is SSE2, converts 8×u16 → 8×u8
  __m128i const packed = _mm_packus_epi16(data, data);

#if FASTFLOAT_64BIT
  return static_cast<uint64_t>(_mm_cvtsi128_si64(packed));
#elif FASTFLOAT_VISUAL_STUDIO
  // Visual Studio doesn't support _mm_cvtsi128_si64 on 32-bit targets, so we
  // use the union trick. Let's compiler do it works well, because it is a POD
  // type.
  return packed.m128i_u64[0];
#else
  uint64_t value;
  // Visual Studio + older versions of GCC don't support _mm_storeu_si64
  _mm_storel_epi64(reinterpret_cast<__m128i *>(&value), packed);
  return value;
#endif
}

fastfloat_really_inline uint64_t simd_read8(char16_t const *chars) {
  FASTFLOAT_SIMD_DISABLE_WARNINGS
  // unaligned SIMD instruction -> all fine.
  return simd_read8(_mm_loadu_si128(reinterpret_cast<__m128i const *>(chars)));
  FASTFLOAT_SIMD_RESTORE_WARNINGS
}

#elif FASTFLOAT_ARM_NEON

fastfloat_really_inline uint64_t simd_read8(uint16x8_t const &data) {
  uint8x8_t utf8_packed = vmovn_u16(data);
  return vget_lane_u64(vreinterpret_u64_u8(utf8_packed), 0);
}

fastfloat_really_inline uint64_t simd_read8(char16_t const *chars) {
  FASTFLOAT_SIMD_DISABLE_WARNINGS
  return simd_read8(vld1q_u16(reinterpret_cast<uint16_t const *>(chars)));
  FASTFLOAT_SIMD_RESTORE_WARNINGS
}

#endif // FASTFLOAT_X86_SIMD

#endif

// MSVC SFINAE is broken pre-VS2017
#if defined(_MSC_VER) && _MSC_VER <= 1900
template <typename UC>
#else
template <typename UC, FASTFLOAT_ENABLE_IF(!has_simd_opt<UC>()) = 0>
#endif
// dummy for compile
uint64_t simd_read8(UC const *) {
  return 0;
}

// credit  @aqrit
fastfloat_really_inline FASTFLOAT_CONSTEXPR14 uint32_t
parse_8_digits(uint64_t val) noexcept {
  uint64_t const mask = 0x000000FF000000FF;
  uint64_t const mul1 = 0x000F424000000064; // 100 + (1000000ULL << 32)
  uint64_t const mul2 = 0x0000271000000001; // 1 + (10000ULL << 32)
  val -= 0x3030303030303030;
  val = (val * 10) + (val >> 8); // val = (val * 2561) >> 8;
  val = (((val & mask) * mul1) + (((val >> 16) & mask) * mul2)) >> 32;
  return static_cast<uint32_t>(val);
}

// Call this if chars are definitely 8 digits.
template <typename UC>
fastfloat_really_inline FASTFLOAT_CONSTEXPR20 uint32_t
parse_8_digits(UC const *chars) noexcept {
  if (is_constant_evaluated() || !has_simd_opt<UC>()) {
    return parse_8_digits(
        read_chars_to_unsigned<uint64_t>(chars)); // truncation okay
  }
  return parse_8_digits(simd_read8(chars));
}

// credit @aqrit
fastfloat_really_inline constexpr bool
is_made_of_8_digits(uint64_t val) noexcept {
  return !((((val + 0x4646464646464646) | (val - 0x3030303030303030)) &
            0x8080808080808080));
}

fastfloat_really_inline constexpr bool
is_made_of_4_digits(uint32_t val) noexcept {
  return !((((val + 0x46464646) | (val - 0x30303030)) & 0x80808080));
}

fastfloat_really_inline FASTFLOAT_CONSTEXPR14 uint32_t
parse_4_digits(uint32_t val) noexcept {
  val -= 0x30303030;
  val = (val * 10) + (val >> 8);
  return (((val & 0x00FF00FF) * 0x00640001) >> 16) & 0xFFFF;
}

#if FASTFLOAT_USE_SIMD

#if FASTFLOAT_X86_SIMD

#if FASTFLOAT_X86_SIMD >= 31
// credit @hedgehoginthecpp
fastfloat_really_inline __m128i parse_4x4_digits(__m128i data) noexcept {
  // 1. convert from ASCII '0' .. '9' to numbers 0 .. 9
  const __m128i ascii0 = _mm_set1_epi8('0');
  const __m128i t0 = _mm_subs_epu8(data, ascii0);

  // 2. convert to 2-digit numbers
  const __m128i mul_1_10 =
      _mm_setr_epi8(10, 1, 10, 1, 10, 1, 10, 1, 10, 1, 10, 1, 10, 1, 10, 1);
  const __m128i t1 = _mm_maddubs_epi16(t0, mul_1_10);

  // 3. convert to 4-digit numbers
  const __m128i mul_1_100 = _mm_setr_epi16(100, 1, 100, 1, 100, 1, 100, 1);
  return _mm_madd_epi16(t1, mul_1_100);
}
#endif

// credit @hedgehoginthecpp
fastfloat_really_inline uint64_t
convert_4x4_to_16_digits(__m128i data) noexcept {
  // 4. convert to 16-digit number
  // v[0] * 10^12 + v[1] * 10^8 + v[2] * 10^4 + v[3]
  const uint64_t lo = static_cast<uint32_t>(_mm_cvtsi128_si32(data));
  const uint64_t hi =
      static_cast<uint32_t>(_mm_cvtsi128_si32(_mm_srli_si128(data, 4)));
  const uint64_t a = lo * 10000ULL + hi;
  const uint64_t lo2 =
      static_cast<uint32_t>(_mm_cvtsi128_si32(_mm_srli_si128(data, 8)));
  const uint64_t hi2 =
      static_cast<uint32_t>(_mm_cvtsi128_si32(_mm_srli_si128(data, 12)));
  const uint64_t b = lo2 * 10000ULL + hi2;
  return a * 100000000ULL + b;
}

#if FASTFLOAT_X86_SIMD >= 42
// credit @hedgehoginthecpp
fastfloat_really_inline bool parse_if_16_digits(char const *chars,
                                                uint64_t &value) noexcept {
  FASTFLOAT_SIMD_DISABLE_WARNINGS
  const __m128i data =
      _mm_loadu_si128(reinterpret_cast<__m128i const *>(chars));
  FASTFLOAT_SIMD_RESTORE_WARNINGS
  /*
   * PCMPxSTRI range comparison.
   *
   * First operand:
   *
   *   ['0','9']
   *
   * Second operand:
   *
   *   16 input bytes
   *
   * Negative polarity asks for the first byte which is
   * NOT inside the digit range.
   */
  const __m128i ranges =
      _mm_setr_epi8('0', '9', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

  const auto numbers =
      _mm_cmpestri(ranges, 2, data, 16,
                   _SIDD_UBYTE_OPS | _SIDD_CMP_RANGES |
                       _SIDD_NEGATIVE_POLARITY | _SIDD_LEAST_SIGNIFICANT);

  if (numbers != 16)
    return false;

  value = value * 10000000000000000ULL +
          convert_4x4_to_16_digits(parse_4x4_digits(data));

  return true;
}
#endif

#if FASTFLOAT_X86_SIMD >= 31
// credit @hedgehoginthecpp
fastfloat_really_inline uint64_t parse_16_digits(char const *p) noexcept {
  const __m128i data = _mm_loadu_si128(reinterpret_cast<const __m128i *>(p));
  return convert_4x4_to_16_digits(parse_4x4_digits(data));
}
#endif

// credit @hedgehoginthecpp
fastfloat_really_inline FASTFLOAT_CONSTEXPR20 void
parse_digits_until_19(char const *&p, char const *pend, am_mant_t &mantissa) {
#if FASTFLOAT_X86_SIMD >= 31
  if (!is_constant_evaluated()) {
    // If mantissa < 10^2, a 16-digit block is guaranteed < 10^18 - 1.
    while (std::distance(p, pend) >= 16 && mantissa < 100) {
      auto const value = parse_16_digits(p);
      mantissa = mantissa * 10000000000000000ULL + value;
      p += 16;
    }
  }
#endif
  // If mantissa < 10^10, a 8-digit block is guaranteed < 10^18 - 1.
  while (std::distance(p, pend) >= 8 && mantissa < 10000000000ULL) {
    auto const value = parse_8_digits(p);
    mantissa = mantissa * 100000000ULL + value;
    p += 8;
  }
  // If mantissa < 10^12, a 4-digit block is guaranteed < 10^18 - 1.
  if (std::distance(p, pend) >= 4 && mantissa < 1000000000000ULL) {
    auto const value = read_chars_to_unsigned<uint32_t>(p);
    mantissa = mantissa * 10000 + parse_4_digits(value);
    p += 4;
  }
  // If mantissa < 10^19, we should parse digits one by one.
  while (p != pend && mantissa < minimal_nineteen_digit_integer) {
    mantissa = mantissa * 10 + static_cast<uint8_t>(*p - '0');
    ++p;
  }
  // While mantissa >= 10^19, we should stop parsing digits.
}

template <typename UC, FASTFLOAT_ENABLE_IF(!std::is_same<UC, char>::value) = 0>
fastfloat_really_inline FASTFLOAT_CONSTEXPR20 void
parse_digits_until_19(UC const *&p, UC const *pend,
                      am_mant_t &mantissa) noexcept {
  do {
    mantissa = mantissa * 10 + static_cast<uint8_t>(*p - UC('0'));
  } while (++p != pend && mantissa < minimal_nineteen_digit_integer);
}
#else
template <typename UC>
fastfloat_really_inline FASTFLOAT_CONSTEXPR20 void
parse_digits_until_19(UC const *&p, UC const *pend,
                      am_mant_t &mantissa) noexcept {
  do {
    mantissa = mantissa * 10 + static_cast<uint8_t>(*p - UC('0'));
  } while (++p != pend && mantissa < minimal_nineteen_digit_integer);
}
#endif

// Call this if chars might not be 8 digits.
// Using this style (instead of is_made_of_8_digits() then
// parse_8_digits()) ensures we don't load SIMD registers twice.
fastfloat_really_inline FASTFLOAT_CONSTEXPR20 bool
simd_parse_if_8_digits(char16_t const *chars, uint64_t &i) noexcept {
  if (is_constant_evaluated()) {
    return false;
  }
#if FASTFLOAT_X86_SIMD
  FASTFLOAT_SIMD_DISABLE_WARNINGS
  // Load 8 UTF-16 characters (16 bytes)
  // unaligned SIMD instruction -> all fine.
  __m128i const data =
      _mm_loadu_si128(reinterpret_cast<__m128i const *>(chars));
  FASTFLOAT_SIMD_RESTORE_WARNINGS

  // Branchless "are all digits?" trick from Lemire:
  // (x - '0') <= 9  <=> (x + 32720) <= 32729
  // encoded as signed comparison: (x + 32720) > -32759 ? not digit : digit
  // http://0x80.pl/articles/simd-parsing-int-sequences.html
  __m128i const t0 = _mm_add_epi16(data, _mm_set1_epi16(32720));
  __m128i const mask = _mm_cmpgt_epi16(t0, _mm_set1_epi16(-32759));

  // If mask == 0 → all digits valid.
  if (_mm_movemask_epi8(mask) == 0) {
    i = i * 100000000 + parse_8_digits(simd_read8(data));
    return true;
  }
#elif FASTFLOAT_ARM_NEON
  FASTFLOAT_SIMD_DISABLE_WARNINGS
  uint16x8_t const data = vld1q_u16(reinterpret_cast<uint16_t const *>(chars));
  FASTFLOAT_SIMD_RESTORE_WARNINGS

  // (x - '0') <= 9
  // http://0x80.pl/articles/simd-parsing-int-sequences.html
  uint16x8_t const t0 = vsubq_u16(data, vmovq_n_u16('0'));
  uint16x8_t const mask = vcltq_u16(t0, vmovq_n_u16('9' - '0' + 1));

  if (vminvq_u16(mask) == 0xFFFF) {
    i = i * 100000000 + parse_8_digits(simd_read8(data));
    return true;
  }
#else
  (void)chars;
  (void)i;
#endif
  return false;
}
#else
template <typename UC>
fastfloat_really_inline FASTFLOAT_CONSTEXPR20 void
parse_digits_until_19(UC const *&p, UC const *pend,
                      am_mant_t &mantissa) noexcept {
  do {
    mantissa = mantissa * 10 + static_cast<uint8_t>(*p - UC('0'));
  } while (++p != pend && mantissa < minimal_nineteen_digit_integer);
}
#endif

// MSVC SFINAE is broken pre-VS2017
#if defined(_MSC_VER) && _MSC_VER <= 1900
template <typename UC>
#else
template <typename UC, FASTFLOAT_ENABLE_IF(!has_simd_opt<UC>()) = 0>
#endif
// dummy for compiler
constexpr bool simd_parse_if_8_digits(UC const *, uint64_t &) {
  return false;
}

template <typename UC, FASTFLOAT_ENABLE_IF(!std::is_same<UC, char>::value) = 0>
fastfloat_really_inline FASTFLOAT_CONSTEXPR20 void
loop_parse_if_digits(UC const *&p, UC const *const pend, uint64_t &i) noexcept {
  if (!is_constant_evaluated()) {
    if FASTFLOAT_CONSTEXPR17 (has_simd_opt<UC>()) {
      while (std::distance(p, pend) >= 8 &&
             simd_parse_if_8_digits(p, i)) { // may overflow, that's ok
        p += 8;
      }
    }
  }
  // Finalizer
  while (p != pend && is_integer(*p)) {
    i = i * 10 + static_cast<uint8_t>(*p - UC('0')); // may overflow, that's ok
    ++p;
  }
}

fastfloat_really_inline FASTFLOAT_CONSTEXPR20 void
loop_parse_if_digits(char const *&p, char const *const pend,
                     uint64_t &i) noexcept {
#if FASTFLOAT_USE_SIMD && FASTFLOAT_X86_SIMD >= 42
  if (!is_constant_evaluated()) {
    // SSE4.2 handles 16 bytes at once.
    while (std::distance(p, pend) >= 16)
      if (parse_if_16_digits(p, i)) {
        p += 16;
      } else {
        break;
      }
  }
#endif
  // Optimizes better than parse_if_eight_digits_unrolled() for char.
  while (std::distance(p, pend) >= 8 /*sizeof(uint64_t)*/) {
    auto const val = read_chars_to_unsigned<uint64_t>(p);
    if (is_made_of_8_digits(val)) {
      i = i * 100000000 + parse_8_digits(val); // may overflow, that's ok
      p += sizeof(uint64_t);
    } else {
      break;
    }
  }
  // Consume a remaining 4-7 digit run in a single SWAR step instead of
  // byte-by-byte (reuses the existing 4-digit helpers). The parsed result is
  // identical either way. Historically gated to clang because gcc regressed on
  // short remainders, but that verdict predates the span-elision restructure;
  // with the leaner hot path the 4-digit step now wins on gcc as well.
  if (std::distance(p, pend) >= 4 /*sizeof(uint32_t)*/) {
    auto const val = read_chars_to_unsigned<uint32_t>(p);
    if (is_made_of_4_digits(val)) {
      i = i * 10000 + parse_4_digits(val); // may overflow, that's ok
      p += sizeof(uint32_t);
    }
  }
  // Finalizer
  while (p != pend && is_integer(*p)) {
    i = i * 10 + static_cast<uint8_t>(*p - '0'); // may overflow, that's ok
    ++p;
  }
}

enum class parse_error : uint_fast8_t {
  no_error,
  // A sign must be followed by an integer or dot.
  missing_integer_or_dot_after_sign,
  // The mantissa must have at least one digit.
  no_digits_in_mantissa,
  // Scientific notation requires an exponential part.
  missing_exponential_part,
#ifndef FASTFLOAT_ONLY_POSITIVE_C_NUMBER_WO_INF_NAN
  // [JavaScript-only, sloppy mode] The integer part is a legacy octal literal
  // (a leading zero followed by octal digits only), which is not a decimal
  // number. Parse it in base 8 instead.
  legacy_octal_integer_part,
  // [JSON/JavaScript-only] The integer part must not have leading zeros.
  leading_zeros_in_integer_part,
  // [JSON-only] The minus sign must be followed by an integer.
  missing_integer_after_sign,
  // [JSON-only] The integer part must have at least one digit.
  no_digits_in_integer_part,
  // [JSON-only] If there is a decimal point, there must be digits in the
  // fractional part.
  no_digits_in_fractional_part,
#endif
};

template <typename UC> struct parsed_number_string_t {
  FASTFLOAT_NO_UNIQUE_ADDRESS am_mant_t mantissa;
  FASTFLOAT_NO_UNIQUE_ADDRESS UC const *lastmatch;
  FASTFLOAT_NO_UNIQUE_ADDRESS am_pow_t exponent;

  FASTFLOAT_NO_UNIQUE_ADDRESS parse_error error;
#ifndef FASTFLOAT_ONLY_POSITIVE_C_NUMBER_WO_INF_NAN
  FASTFLOAT_NO_UNIQUE_ADDRESS bool negative;
#endif
  FASTFLOAT_NO_UNIQUE_ADDRESS bool invalid;
  FASTFLOAT_NO_UNIQUE_ADDRESS bool too_many_digits;

  // contains the range of the significant digits, fully optional, only matters
  // if it's length > 0
  FASTFLOAT_NO_UNIQUE_ADDRESS span<UC const> integer;
  FASTFLOAT_NO_UNIQUE_ADDRESS span<UC const> fraction;
};

using byte_span = span<char const>;
using parsed_number_string = parsed_number_string_t<char>;

// Helper for error creating
template <typename UC>
fastfloat_really_inline FASTFLOAT_CONSTEXPR20 parsed_number_string_t<UC> &
report_parse_error(parsed_number_string_t<UC> &answer, UC const *p,
                   parse_error error) noexcept {
  answer.invalid = true;
  answer.lastmatch = p;
  answer.error = error;
  return answer;
}

// Assuming that you use no more than 19 digits, this will
// parse an ASCII string.
//
// store_spans is a *runtime* flag (not a template parameter, deliberately: a
// template would create a second instantiation of this whole function and the
// extra icache pressure wipes out the gain). When false, the integer/fraction
// spans (read only by the rare digit_comp slow path) are not materialized,
// which keeps the fat parsed_number_string_t off the hot path. The caller
// re-parses with store_spans=true if the slow path is actually reached.
template <bool json_fmt, typename UC>
fastfloat_really_inline FASTFLOAT_CONSTEXPR20 parsed_number_string_t<UC>
parse_number_string(UC const *p, UC const *pend,
                    parse_options_t<UC> const options,
                    bool store_spans = true) noexcept {
  parsed_number_string_t<UC> answer{};
  FASTFLOAT_ASSUME(p < pend); // so dereference without checks
#ifndef FASTFLOAT_ONLY_POSITIVE_C_NUMBER_WO_INF_NAN
  // C++17 20.19.3.(7.1) explicitly forbids '+' sign here
  answer.negative = *p == UC('-');
  if (answer.negative ||
      (!json_fmt &&
       chars_format_t(options.format & chars_format::allow_leading_plus) &&
       *p == UC('+'))) {
    ++p;

    if fastfloat_unlikely (p == pend) {
      return report_parse_error<UC>(
          answer, p, parse_error::missing_integer_or_dot_after_sign);
    }
    if (json_fmt) {
      if fastfloat_unlikely (!is_integer(*p)) {
        // A sign must be followed by an integer
        return report_parse_error<UC>(answer, p,
                                      parse_error::missing_integer_after_sign);
      }
    } else if fastfloat_unlikely (!is_integer(*p) &&
                                  *p != options.decimal_point) {
      // A sign must be followed by an integer or the dot
      return report_parse_error<UC>(
          answer, p, parse_error::missing_integer_or_dot_after_sign);
    }
  }
#endif
  auto const *const start_digits = p;

  // HedgehogInTheCPP: compiler generate much better code in any mode without
  // manual unroling because it's less branching and also it's allow more
  // inlining which is better. A a multiplication by 10 is cheaper than an
  // arbitrary integer multiplication. might overflow, handled later
#if !defined(FASTFLOAT_ISNOT_CHECKED_BOUNDS) &&                                \
    !defined(FASTFLOAT_ONLY_POSITIVE_C_NUMBER_WO_INF_NAN)
  while (p != pend && is_integer(*p)) {
    answer.mantissa = static_cast<fast_float::am_mant_t>(
        answer.mantissa * 10 + static_cast<uint8_t>(*p - UC('0')));
    ++p;
  }
#else
  // External parser already check that this is num and it's exist
  do {
    answer.mantissa = static_cast<fast_float::am_mant_t>(
        answer.mantissa * 10 + static_cast<uint8_t>(*p - UC('0')));
    ++p;
  } while (p != pend && is_integer(*p));
#endif

  UC const *const end_of_integer_part = p;
  auto digit_count = static_cast<am_digits>(end_of_integer_part - start_digits);
  if fastfloat_unlikely (store_spans) {
    answer.integer = span<UC const>(start_digits, digit_count);
  }
#ifndef FASTFLOAT_ONLY_POSITIVE_C_NUMBER_WO_INF_NAN
  if fastfloat_unlikely (json_fmt && digit_count == 0) {
    // At least 1 digit in integer part
    return report_parse_error<UC>(answer, p,
                                  parse_error::no_digits_in_integer_part);
  }
  if (json_fmt || chars_format_t(options.format & detail::javascript_fmt)) {
    // ECMAScript DecimalIntegerLiteral is "0" or a non-zero digit followed by
    // digits: no leading zeros. Unlike JSON, the integer part may be empty
    // (".5"); the no_digits_in_mantissa check below still rejects ".".
    if fastfloat_unlikely (start_digits[0] == UC('0') && digit_count > 1) {
      if fastfloat_unlikely (json_fmt ||
                             !chars_format_t(options.format &
                                             detail::javascript_sloppy_fmt)) {
        return report_parse_error<UC>(
            answer, start_digits, parse_error::leading_zeros_in_integer_part);
      }
      if (!json_fmt) {
        // Sloppy mode (Annex B): a NonOctalDecimalIntegerLiteral has a leading
        // zero and at least one digit that is 8 or 9 ("08.5" is 8.5). With
        // octal digits only, it is a LegacyOctalIntegerLiteral ("0775"), which
        // is not a decimal number: report it so the caller can parse it in
        // base 8.
        bool has_non_octal_digit = false;
        for (UC const *q = start_digits; q != end_of_integer_part; ++q) {
          has_non_octal_digit |= (*q >= UC('8'));
        }
        if fastfloat_unlikely (!has_non_octal_digit) {
          return report_parse_error<UC>(answer, start_digits,
                                        parse_error::legacy_octal_integer_part);
        }
      }
    }
  }
#endif

  // We can now parse the fraction part of the mantissa.
  bool const has_decimal_point = p != pend && *p == options.decimal_point;
  if (has_decimal_point) {
    ++p;
    auto const *const before = p;
    // can occur at most twice without overflowing, but let it occur more, since
    // for integers with many digits, digit parsing is the primary bottleneck.
    loop_parse_if_digits(p, pend, answer.mantissa);

    answer.exponent = static_cast<am_pow_t>(before - p);
#ifndef FASTFLOAT_ONLY_POSITIVE_C_NUMBER_WO_INF_NAN
    if fastfloat_unlikely (json_fmt && answer.exponent == 0) {
      // At least 1 digit in fractional part
      return report_parse_error<UC>(answer, p,
                                    parse_error::no_digits_in_fractional_part);
    }
#endif
    if fastfloat_unlikely (store_spans) {
      answer.fraction =
          span<UC const>(before, static_cast<am_digits>(p - before));
    }
    digit_count -= static_cast<am_digits>(answer.exponent);
  }

  if fastfloat_unlikely (digit_count == 0) {
    // We must have encountered at least one integer!
    return report_parse_error<UC>(answer, p,
                                  parse_error::no_digits_in_mantissa);
  }
  // We have now parsed the integer and the fraction part of the mantissa.

  // Now we can parse the explicit exponential part.
  am_pow_t exp_number = 0; // explicit exponential part
  if (p != pend &&
      ((chars_format_t(options.format & chars_format::scientific) &&
        (UC('e') == *p || UC('E') == *p))
#ifndef FASTFLOAT_ONLY_POSITIVE_C_NUMBER_WO_INF_NAN
       || (chars_format_t(options.format & detail::fortran_fmt) &&
           (UC('+') == *p || UC('-') == *p || UC('d') == *p || UC('D') == *p))
#endif
           )) {
    auto const *location_of_e = p;
#ifndef FASTFLOAT_ONLY_POSITIVE_C_NUMBER_WO_INF_NAN
    if (UC('e') == *p || UC('E') == *p || UC('d') == *p || UC('D') == *p) {
      ++p;
    }
#else
    ++p;
#endif
    bool neg_exp = false;
    if (p != pend) {
      if (UC('-') == *p) {
        neg_exp = true;
        ++p;
      } else if (UC('+') == *p) {
        // '+' on exponent is allowed by C++17 20.19.3.(7.1)
        ++p;
      }
    }
    // We have now parsed the sign of the exponent.
    if fastfloat_unlikely (p == pend || !is_integer(*p)) {
      if fastfloat_unlikely (!chars_format_t(options.format &
                                             chars_format::fixed)) {
        // The exponential part is invalid for scientific notation, so it must
        // be a trailing token for fixed notation. However, fixed notation is
        // disabled, so report a scientific notation error.
        return report_parse_error<UC>(answer, p,
                                      parse_error::missing_exponential_part);
      }
      // Otherwise, we will be ignoring the 'e'.
      p = location_of_e;
    } else {
      // Now let's parse the explicit exponent.
      while (p != pend && is_integer(*p)) {
        if fastfloat_likely (exp_number < am_bias_limit) {
          // check for exponent overflow if we have big explicit exponent.
          auto const digit = static_cast<uint8_t>(*p - UC('0'));
          exp_number = 10 * exp_number + digit;
        }
        ++p;
      }
      if (neg_exp) {
        exp_number = -exp_number;
      }
      answer.exponent += exp_number;
    }
  } else if fastfloat_unlikely (chars_format_t(options.format &
                                               chars_format::scientific) &&
                                !chars_format_t(options.format &
                                                chars_format::fixed)) {
    // If it scientific and not fixed, we have to bail out.
    return report_parse_error<UC>(answer, p,
                                  parse_error::missing_exponential_part);
  }

  // We sucessfully parsed all parts of the number, let's save progress.
  answer.lastmatch = p;

  // Now we can check for errors.

  // If we frequently had to deal with long strings of digits,
  // we could extend our code by using a 128-bit integer instead
  // of a 64-bit integer. However, this is uncommon.
  //
  // We can deal with up to 19 digits.
  if fastfloat_unlikely (digit_count > 19) {
    // It is possible that the integer had an overflow.
    // We have to handle the case where we have 0.0000somenumber.
    // We need to be mindful of the case where we only have zeroes...
    // E.g., 0.000000000...000.
    auto const *start = start_digits;
    do {
      if (*start == UC('0')) {
        --digit_count;
      } else if (*start != options.decimal_point) {
        break;
      }
    } while (++start != pend);

    // We have to check if number has more than 19 significant digits.
    if fastfloat_unlikely (digit_count > 19) {
      answer.too_many_digits = true;
      // The truncation recompute below reads the integer/fraction spans. When
      // store_spans is false we didn't materialize them, so just flag
      // too_many_digits; the caller re-parses with store_spans=true to obtain
      // the corrected mantissa/exponent before taking the slow path.
      if fastfloat_unlikely (store_spans) {
        // Let us start again, this time, avoiding overflows.
        // We don't need to call if is_integer, since we use the
        // pre-tokenized spans from above.
        answer.mantissa = 0;
        p = answer.integer.ptr;
        parse_digits_until_19(p, p + answer.integer.len(), answer.mantissa);
        if (answer.mantissa >= minimal_nineteen_digit_integer) {
          // We have a big integers, so skip the fraction part completely.
          answer.exponent = am_pow_t(end_of_integer_part - p) + exp_number;
        } else if (answer.fraction.len()) {
          // We have a value with a significant fractional component.
          p = answer.fraction.ptr;
          parse_digits_until_19(p, p + answer.fraction.len(), answer.mantissa);
          answer.exponent = am_pow_t(answer.fraction.ptr - p) + exp_number;
        }
        // We have now corrected both exponent and mantissa, to a truncated
        // value
      }
    }
  }

  return answer;
}

template <typename T, typename UC>
fastfloat_really_inline FASTFLOAT_CONSTEXPR20 from_chars_result_t<UC>
parse_int_string(UC const *p, UC const *pend, T &value,
                 parse_options_t<UC> const options) noexcept {
  FASTFLOAT_ASSUME(p < pend); // so dereference without checks
  from_chars_result_t<UC> answer;

#ifndef FASTFLOAT_ONLY_POSITIVE_C_NUMBER_WO_INF_NAN
  auto const *const first = p;
  // Read sign
  auto const negative = (*p == UC('-'));
#ifdef FASTFLOAT_VISUAL_STUDIO
#pragma warning(push)
#pragma warning(disable : 4127)
#endif
  if (!std::is_signed<T>::value && negative) {
#ifdef FASTFLOAT_VISUAL_STUDIO
#pragma warning(pop)
#endif
    answer.ec = std::errc::invalid_argument;
    answer.ptr = first;
    return answer;
  }
  if (negative ||
      (chars_format_t(options.format & chars_format::allow_leading_plus) &&
       (*p == UC('+')))) {
    ++p;
  }
#endif

  auto const *const start_num = p;

  // Skip leading zeros
#if !defined(FASTFLOAT_ISNOT_CHECKED_BOUNDS) &&                                \
    !defined(FASTFLOAT_ONLY_POSITIVE_C_NUMBER_WO_INF_NAN)
  while (p != pend && *p == UC('0')) {
    ++p;
  }
#else
  // External parser already check that this is num and it's exist
  do {
    if (*p == UC('0')) {
      ++p;
    } else {
      break;
    }
  } while (p != pend);
#endif
  auto const has_leading_zeros = p > start_num;

  auto const *const start_digits = p;

  if (options.base == 10) {
    auto const len = static_cast<am_digits>(pend - p);
#ifndef FASTFLOAT_ISNOT_CHECKED_BOUNDS
    // External parser already check that this is num and it's exist
    if (len == 0) {
#endif
      if (has_leading_zeros) {
        value = 0;
        answer.ec = std::errc();
        answer.ptr = p;
        return answer;
      }
#ifndef FASTFLOAT_ISNOT_CHECKED_BOUNDS
      answer.ec = std::errc::invalid_argument;
      answer.ptr = first;
      return answer;
    }
#endif

    if FASTFLOAT_CONSTEXPR17 (std::is_same<T, std::uint8_t>::value &&
                              sizeof(UC) == 1) {
      uint32_t digits;

      if (len >= sizeof(uint32_t)) {
        digits = read_chars_to_unsigned<uint32_t>(p);
      } else {
        uint32_t const b0 = static_cast<uint8_t>(p[0]);
        uint32_t const b1 = (len > 1) ? static_cast<uint8_t>(p[1]) : 0x00u;
        uint32_t const b2 = (len > 2) ? static_cast<uint8_t>(p[2]) : 0x00u;
        uint32_t const b3 = 0x00u;
        digits = b0 | (b1 << 8) | (b2 << 16) | (b3 << 24);
      }

      uint32_t const magic =
          ((digits + 0x46464646u) | (digits - 0x30303030u)) & 0x80808080u;
      auto const tz = countr_zero_32(magic); // 7, 15, 23, 31, or 32
      auto nd = static_cast<am_digits>(tz >> 3);
      nd = nd < len ? nd : len;
#ifndef FASTFLOAT_ISNOT_CHECKED_BOUNDS
      // External parser already check that this is num and it's exist
      if (nd == 0) {
#endif
        if (has_leading_zeros) {
          value = 0;
          answer.ec = std::errc();
          answer.ptr = p;
          return answer;
        }
#ifndef FASTFLOAT_ISNOT_CHECKED_BOUNDS
        answer.ec = std::errc::invalid_argument;
        answer.ptr = first;
        return answer;
      }
#endif
      if (nd > 3) {
        const UC *q = p + nd;
        am_digits rem = len - nd;
        while (rem) {
          if (*q < UC('0') || *q > UC('9')) {
            break;
          }
          ++q;
          --rem;
        }
        answer.ec = std::errc::result_out_of_range;
        answer.ptr = q;
        return answer;
      }

      digits ^= 0x30303030u;
      digits <<= ((4 - nd) * 8);

      uint32_t const check = ((digits >> 24) & 0xff) |
                             ((digits >> 8) & 0xff00) |
                             ((digits << 8) & 0xff0000);
      if (check > 0x00020505) {
        answer.ec = std::errc::result_out_of_range;
        answer.ptr = p + nd;
        return answer;
      }
      value = static_cast<uint8_t>((0x640a01 * digits) >> 24);
      answer.ec = std::errc();
      answer.ptr = p + nd;
      return answer;
    }

    if FASTFLOAT_CONSTEXPR17 (std::is_same<T, std::uint16_t>::value &&
                              sizeof(UC) == 1) {
      if (len >= sizeof(uint32_t)) {
        auto const digits = read_chars_to_unsigned<uint32_t>(p);
        if (is_made_of_4_digits(digits)) {
          auto v = parse_4_digits(digits);
          if (len >= 5 && is_integer(p[4])) {
            v = v * 10 + static_cast<uint8_t>(p[4] - '0');
            if (len >= 6 && is_integer(p[5])) {
              const UC *q = p + 5;
              while (q != pend && is_integer(*q)) {
                ++q;
              }
              answer.ec = std::errc::result_out_of_range;
              answer.ptr = q;
              return answer;
            }
            if (v > std::numeric_limits<uint16_t>::max()) {
              answer.ec = std::errc::result_out_of_range;
              answer.ptr = p + 5;
              return answer;
            }
            value = static_cast<uint16_t>(v);
            answer.ec = std::errc();
            answer.ptr = p + 5;
            return answer;
          }
          // 4 digits
          value = static_cast<uint16_t>(v);
          answer.ec = std::errc();
          answer.ptr = p + 4;
          return answer;
        }
      }
    }
  }

  // Parse digits
  am_mant_t i = 0;
  if (options.base == 10) {
    loop_parse_if_digits(p, pend, i); // use SIMD if possible
  } else
    while (p != pend) {
      auto const digit = ch_to_digit(*p);
      if (digit >= options.base) {
        break;
      }
      i = am_mant_t(options.base) * i +
          digit; // might overflow, check this later
      ++p;
    }

  auto const digit_count = static_cast<am_digits>(p - start_digits);

#ifndef FASTFLOAT_ISNOT_CHECKED_BOUNDS
  // parser already check that this is num and it's exist
  if (digit_count == 0) {
#endif
    if (has_leading_zeros) {
      value = 0;
      answer.ec = std::errc();
      answer.ptr = p;
      return answer;
    }
#ifndef FASTFLOAT_ISNOT_CHECKED_BOUNDS
    answer.ec = std::errc::invalid_argument;
    answer.ptr = first;

    return answer;
  }
#endif

  answer.ptr = p;

  // check u64 overflow
  auto const max_digits = max_digits_u64(options.base);
  if (digit_count > max_digits) {
    answer.ec = std::errc::result_out_of_range;
    return answer;
  }
  // this check can be eliminated for all other types, but they will all require
  // a max_digits(base) equivalent
  if (digit_count == max_digits) {
    // At the max_digits boundary the accumulator `i` may have wrapped around
    // 2^64. A plain `i < min_safe_u64(base)` test is not sufficient: for any
    // base whose max_digits-length range exceeds 2^64 (base 10 reaches
    // ~5.4 * 2^64 at 20 digits) the value can wrap a whole multiple of 2^64 and
    // land back above min_safe, slipping through. Decide exactly in O(1) using
    // the leading digit, following the approach used in simdjson:
    //   ms   == min_safe_u64(base) == base^(max_digits-1), the smallest
    //           max_digits-length value.
    //   dmax == the largest leading digit whose number can still fit in u64.
    // The leading-digit band [d*ms, (d+1)*ms) has width ms < 2^64, so within
    // the single band where d == dmax the value straddles 2^64 at most once,
    // and a single threshold separates wrapped from non-wrapped values. A
    // leading digit above dmax always overflows; below dmax always fits.
    uint64_t const ms = min_safe_u64(options.base);
    uint64_t const dmax = std::numeric_limits<uint64_t>::max() / ms;
    uint64_t const lead = ch_to_digit(*start_digits);
    if (lead > dmax || (lead == dmax && i < dmax * ms)) {
      answer.ec = std::errc::result_out_of_range;
      return answer;
    }
  }

  // check other types overflow
  if (!std::is_same<T, am_mant_t>::value) {
    if (i > am_mant_t(std::numeric_limits<T>::max())
#ifndef FASTFLOAT_ONLY_POSITIVE_C_NUMBER_WO_INF_NAN
                + uint8_t(negative)
#endif
    ) {
      answer.ec = std::errc::result_out_of_range;
      return answer;
    }
  }

#ifdef FASTFLOAT_ONLY_POSITIVE_C_NUMBER_WO_INF_NAN
  value = T(i);
#else
  if (negative) {
#ifdef FASTFLOAT_VISUAL_STUDIO
#pragma warning(push)
#pragma warning(disable : 4146)
#pragma warning(disable : 4804)
#endif
    // this weird workaround is required because:
    // - converting unsigned to signed when its value is greater than signed max
    // is UB pre-C++23.
    // - reinterpret_casting (~i + 1) would work, but it is not constexpr
    // this is always optimized into a neg instruction (note: T is an integer
    // type)
    value = T(-std::numeric_limits<T>::max() -
              T(i - am_mant_t(std::numeric_limits<T>::max())));
#ifdef FASTFLOAT_VISUAL_STUDIO
#pragma warning(pop)
#endif
  } else {
    value = T(i);
  }
#endif

  answer.ec = std::errc();
  return answer;
}

} // namespace fast_float

#endif
