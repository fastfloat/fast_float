#include "fast_float/fast_float.h"
#include <fuzzer/FuzzedDataProvider.h>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <string>
#include <system_error>
#include <type_traits>
#include <vector>

#if defined(__has_include)
#if __has_include(<charconv>) && __cplusplus >= 201703L
#include <charconv>
#define FASTFLOAT_FUZZ_CHARCONV 1
#endif
#endif

static void fail(char const *check) {
  fprintf(stderr, "fast_float fuzz failure: %s\n", check);
  abort();
}

fast_float::chars_format arbitrary_format(FuzzedDataProvider &fdp) {
  using fast_float::chars_format;
  chars_format format;
  switch (fdp.ConsumeIntegralInRange<int>(0, 7)) {
  case 0:
    format = chars_format::scientific;
    break;
  case 1:
    format = chars_format::fixed;
    break;
  case 2:
    format = chars_format::json;
    break;
  case 3:
    format = chars_format::json_or_infnan;
    break;
  case 4:
    format = chars_format::fortran;
    break;
  case 5:
    format = chars_format::javascript;
    break;
  case 6:
    format = chars_format::javascript_sloppy;
    break;
  default:
    format = chars_format::general;
    break;
  }
  if (fdp.ConsumeBool()) {
    format = format | chars_format::allow_leading_plus;
  }
  if (fdp.ConsumeBool()) {
    format = format | chars_format::skip_white_space;
  }
  return format;
}

template <typename T> uint64_t bits_of(T value) {
  uint64_t bits = 0;
  memcpy(&bits, &value, sizeof(T));
  return bits;
}

// The same ASCII text must parse identically for every character type: same
// error code, same number of consumed code units, bit-identical value.
template <typename UC>
void check_widened(std::string const &input,
                   fast_float::from_chars_result_t<char> expected,
                   double expected_value, fast_float::chars_format format) {
  std::vector<UC> wide;
  wide.reserve(input.size());
  for (char c : input) {
    wide.push_back(UC(static_cast<unsigned char>(c)));
  }
  double value = 0.0;
  auto result = fast_float::from_chars(wide.data(), wide.data() + wide.size(),
                                       value, format);
  if (result.ec != expected.ec ||
      (result.ptr - wide.data()) != (expected.ptr - input.data())) {
    fail("wide result disagrees with char result");
  }
  if (expected.ec == std::errc() && bits_of(value) != bits_of(expected_value)) {
    fail("wide value disagrees with char value");
  }
}

// Every number format accepted by the library is pure ASCII, so a successful
// parse must never consume a code unit >= 0x80. A violation means some helper
// inspected only the low byte of a wide code unit (the bug behind #391).
// The generated sequences keep an ASCII low byte but take hostile high bits:
// top-bit-set units (near the minimum of a signed wchar_t, the shape behind
// #422), sign-extended negative units (the shape behind #395), and non-ASCII
// code points whose low byte is an ASCII digit (the shape behind #391).
template <typename UC>
void check_arbitrary_units(FuzzedDataProvider &fdp,
                           fast_float::chars_format format) {
  using unsigned_uc = typename std::make_unsigned<UC>::type;
  size_t count = fdp.ConsumeIntegralInRange<size_t>(0, 64);
  std::vector<UC> units;
  units.reserve(count);
  for (size_t i = 0; i < count; ++i) {
    unsigned_uc unit = fdp.ConsumeIntegral<uint8_t>();
    switch (fdp.ConsumeIntegralInRange<int>(0, 4)) {
    case 0: // top bit set: near the minimum value when UC is signed
      unit = unsigned_uc(unit | (unsigned_uc(1) << (sizeof(UC) * 8 - 1)));
      break;
    case 1: // all high bits set: a small negative value when UC is signed
      unit = unsigned_uc(unit | ~unsigned_uc(0xFF));
      break;
    case 2: // arbitrary high byte: a non-ASCII code point, ASCII low byte
      unit = unsigned_uc(
          unit | unsigned_uc(unsigned_uc(fdp.ConsumeIntegral<uint8_t>()) << 8));
      break;
    default: // plain ASCII unit
      break;
    }
    units.push_back(UC(unit));
  }
  if (units.empty()) {
    return;
  }
  UC const *begin = units.data();
  UC const *end = begin + units.size();

  double as_double = 0.0;
  auto result = fast_float::from_chars(begin, end, as_double, format);
  if (result.ec == std::errc()) {
    for (UC const *p = begin; p != result.ptr; ++p) {
      if (static_cast<unsigned_uc>(*p) >= 0x80) {
        fail("float parse consumed a non-ASCII code unit");
      }
    }
  }
  uint16_t as_uint16 = 0;
  auto int_result = fast_float::from_chars(begin, end, as_uint16);
  if (int_result.ec == std::errc()) {
    for (UC const *p = begin; p != int_result.ptr; ++p) {
      if (static_cast<unsigned_uc>(*p) >= 0x80) {
        fail("integer parse consumed a non-ASCII code unit");
      }
    }
  }
}

// fast_float's integer parsing is specified to match std::from_chars.
template <typename T> void check_int(std::string const &input, int base) {
  T value{};
  auto result = fast_float::from_chars(
      input.data(), input.data() + input.size(), value, base);
#ifdef FASTFLOAT_FUZZ_CHARCONV
  T expected{};
  auto std_result = std::from_chars(input.data(), input.data() + input.size(),
                                    expected, base);
  if (result.ec != std_result.ec || result.ptr != std_result.ptr) {
    fail("integer result disagrees with std::from_chars");
  }
  if (result.ec == std::errc() && value != expected) {
    fail("integer value disagrees with std::from_chars");
  }
#else
  static_cast<void>(result);
#endif
}

template <typename T>
void check_int_all_bases(std::string const &input, int base) {
  check_int<T>(input, 10);
  check_int<T>(input, base);
}

extern "C" int LLVMFuzzerTestOneInput(uint8_t const *data, size_t size) {
  FuzzedDataProvider fdp(data, size);
  fast_float::chars_format format = arbitrary_format(fdp);
  int base = fdp.ConsumeIntegralInRange<int>(2, 36);

  std::string input_d = fdp.ConsumeRandomLengthString(128);
  double result_d = 0.0;
  auto answer_d = fast_float::from_chars(
      input_d.data(), input_d.data() + input_d.size(), result_d, format);
  float result_f = 0.0;
  fast_float::from_chars(input_d.data(), input_d.data() + input_d.size(),
                         result_f, format);
  check_widened<wchar_t>(input_d, answer_d, result_d, format);
  check_widened<char16_t>(input_d, answer_d, result_d, format);
  check_widened<char32_t>(input_d, answer_d, result_d, format);

  std::string input_i = fdp.ConsumeRandomLengthString(128);
  check_int_all_bases<uint8_t>(input_i, base);
  check_int_all_bases<int8_t>(input_i, base);
  check_int_all_bases<uint16_t>(input_i, base);
  check_int_all_bases<int16_t>(input_i, base);
  check_int_all_bases<uint32_t>(input_i, base);
  check_int_all_bases<int32_t>(input_i, base);
  check_int_all_bases<uint64_t>(input_i, base);
  check_int_all_bases<int64_t>(input_i, base);

  check_arbitrary_units<wchar_t>(fdp, format);
  check_arbitrary_units<char16_t>(fdp, format);
  check_arbitrary_units<char32_t>(fdp, format);
  return 0;
}
