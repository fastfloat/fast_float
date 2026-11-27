/*
 * Exercise chars_format::hex (hexadecimal floating-point numbers).
 *
 * We check hand-written cases, then compare against std::strtod/std::strtof
 * (which take the same form, with a "0x" prefix) on random inputs.
 */
#include <cerrno>
#include <cinttypes>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>
#include <random>
#include <string>
#include <vector>

#include "fast_float/fast_float.h"

namespace {

template <typename T> struct bits_of;

template <> struct bits_of<double> {
  using type = uint64_t;
};

template <> struct bits_of<float> {
  using type = uint32_t;
};

template <typename T> typename bits_of<T>::type to_bits(T x) {
  typename bits_of<T>::type b;
  std::memcpy(&b, &x, sizeof(x));
  return b;
}

template <typename T> T reference_parse(std::string const &s);

template <> double reference_parse<double>(std::string const &s) {
  return std::strtod(s.c_str(), nullptr);
}

template <> float reference_parse<float>(std::string const &s) {
  return std::strtof(s.c_str(), nullptr);
}

template <typename T> bool same(T a, T b) {
  if (std::isnan(a) && std::isnan(b)) {
    return std::signbit(a) == std::signbit(b);
  }
  return to_bits(a) == to_bits(b);
}

int failures = 0;

struct hex_case {
  char const *input;
  double expected;
  size_t consumed;
  std::errc ec;
  fast_float::chars_format fmt;
};

constexpr double inf = std::numeric_limits<double>::infinity();
constexpr fast_float::chars_format hex = fast_float::chars_format::hex;

void check_cases() {
  using fast_float::chars_format;
  std::vector<hex_case> const cases = {
      {"1", 1.0, 1, std::errc(), hex},
      {"1p0", 1.0, 3, std::errc(), hex},
      {"1P0", 1.0, 3, std::errc(), hex},
      {"a", 10.0, 1, std::errc(), hex},
      {"A.8", 10.5, 3, std::errc(), hex},
      {"-1.8p1", -3.0, 6, std::errc(), hex},
      {"1.921fb54442d18p+1", 3.141592653589793, 18, std::errc(), hex},
      {"1p-1", 0.5, 4, std::errc(), hex},
      {".8", 0.5, 2, std::errc(), hex},
      {"1.", 1.0, 2, std::errc(), hex},
      {"0", 0.0, 1, std::errc(), hex},
      {"-0", -0.0, 2, std::errc(), hex},
      {"0.000p-99999", 0.0, 12, std::errc(), hex},
      {"0p99999", 0.0, 7, std::errc(), hex},
      // The "0x" prefix is assumed, so it cannot appear: we parse the 0.
      {"0x1p0", 0.0, 1, std::errc(), hex},
      // An incomplete exponent is not part of the number.
      {"1p", 1.0, 1, std::errc(), hex},
      {"1p+", 1.0, 1, std::errc(), hex},
      {"1p-x", 1.0, 1, std::errc(), hex},
      {"1.8g", 1.5, 3, std::errc(), hex},
      // Limits of double.
      {"1.fffffffffffffp1023", 1.7976931348623157e308, 20, std::errc(), hex},
      {"1p-1022", 2.2250738585072014e-308, 7, std::errc(), hex},
      {"1p-1074", 4.9406564584124654e-324, 7, std::errc(), hex},
      {"0.0000000000001p-1022", 4.9406564584124654e-324, 21, std::errc(), hex},
      {"fffffffffffff8p-1078", 2.2250738585072014e-308, 20, std::errc(), hex},
      // Rounding: exactly halfway between max double and 2^1024 rounds to even
      // which is infinity.
      {"1.fffffffffffff8p1023", inf, 21, std::errc::result_out_of_range, hex},
      {"1.fffffffffffff7ffffffp1023", 1.7976931348623157e308, 27, std::errc(),
       hex},
      {"1p1024", inf, 6, std::errc::result_out_of_range, hex},
      {"-1p1024", -inf, 7, std::errc::result_out_of_range, hex},
      {"1p99999999999999999999", inf, 22, std::errc::result_out_of_range, hex},
      // Half of the smallest subnormal rounds to zero (ties to even), anything
      // above rounds to the smallest subnormal.
      {"1p-1075", 0.0, 7, std::errc::result_out_of_range, hex},
      {"-1p-1075", -0.0, 8, std::errc::result_out_of_range, hex},
      {"1.0000000000000000000001p-1075", 4.9406564584124654e-324, 30,
       std::errc(), hex},
      {"1.8p-1075", 4.9406564584124654e-324, 9, std::errc(), hex},
      {"1p-99999999999999999999", 0.0, 23, std::errc::result_out_of_range, hex},
      // Ties to even on normal numbers.
      {"1.00000000000008p0", 1.0, 18, std::errc(), hex},
      {"1.00000000000018p0", 1.0000000000000004, 18, std::errc(), hex},
      {"1.000000000000080000000000000000000001p0", 1.0000000000000002, 40,
       std::errc(), hex},
      // Many digits before the point.
      {"10000000000000000000000000000000", 2.1267647932558654e+37, 32,
       std::errc(), hex},
      {"00000000000000000000000000000001.8", 1.5, 34, std::errc(), hex},
      // Special values.
      {"inf", inf, 3, std::errc(), hex},
      {"-infinity", -inf, 9, std::errc(), hex},
      // Errors.
      {"", 0.0, 0, std::errc::invalid_argument, hex},
      {".", 0.0, 0, std::errc::invalid_argument, hex},
      {"-", 0.0, 0, std::errc::invalid_argument, hex},
      {"+1", 0.0, 0, std::errc::invalid_argument, hex},
      {"p1", 0.0, 0, std::errc::invalid_argument, hex},
      {"g", 0.0, 0, std::errc::invalid_argument, hex},
      {"inf", 0.0, 0, std::errc::invalid_argument,
       hex | chars_format::no_infnan},
      // Options.
      {"+1", 1.0, 2, std::errc(), hex | chars_format::allow_leading_plus},
      {"  1p4", 16.0, 5, std::errc(), hex | chars_format::skip_white_space},
      // scientific without fixed: the exponent is required.
      {"1p4", 16.0, 3, std::errc(), hex | chars_format::scientific},
      {"1", 0.0, 0, std::errc::invalid_argument,
       hex | chars_format::scientific},
      {"1p", 0.0, 0, std::errc::invalid_argument,
       hex | chars_format::scientific},
      // fixed without scientific: the exponent is not allowed.
      {"1p4", 1.0, 1, std::errc(), hex | chars_format::fixed},
      // both: the exponent is optional.
      {"1p4", 16.0, 3, std::errc(), hex | chars_format::general},
      {"1", 1.0, 1, std::errc(), hex | chars_format::general},
  };
  for (auto const &c : cases) {
    size_t const len = std::strlen(c.input);
    double result = 12345.0;
    auto answer = fast_float::from_chars(c.input, c.input + len, result, c.fmt);
    bool ok =
        (answer.ec == c.ec) && (size_t(answer.ptr - c.input) == c.consumed);
    if (ok && c.ec != std::errc::invalid_argument) {
      ok = same(result, c.expected);
    }
    if (ok && c.ec == std::errc::invalid_argument) {
      ok = (result == 12345.0); // left unmodified
    }
    if (!ok) {
      std::printf("FAIL: \"%s\" -> %a (consumed %zu, ec %d), expected %a "
                  "(consumed %zu, ec %d)\n",
                  c.input, result, size_t(answer.ptr - c.input), int(answer.ec),
                  c.expected, c.consumed, int(c.ec));
      failures++;
    }
  }
}

void check_float_cases() {
  struct float_case {
    char const *input;
    float expected;
    std::errc ec;
  };

  constexpr float finf = std::numeric_limits<float>::infinity();
  std::vector<float_case> const cases = {
      {"1.fffffep127", 3.40282347e+38f, std::errc()},
      {"1.ffffffp127", finf, std::errc::result_out_of_range},
      {"1p-149", 1.40129846e-45f, std::errc()},
      {"1p-150", 0.0f, std::errc::result_out_of_range},
      {"1.000001p-150", 1.40129846e-45f, std::errc()},
      {"1p-126", 1.17549435e-38f, std::errc()},
      {"0.fffffffp-126", 1.17549435e-38f, std::errc()},
      {"1.000001p0", 1.0f, std::errc()},
      {"1.000003p0", 1.00000024f, std::errc()},
  };
  for (auto const &c : cases) {
    size_t const len = std::strlen(c.input);
    float result;
    auto answer = fast_float::from_chars(c.input, c.input + len, result, hex);
    if (answer.ec != c.ec || answer.ptr != c.input + len ||
        !same(result, c.expected)) {
      std::printf("FAIL (float): \"%s\" -> %a, expected %a\n", c.input,
                  double(result), double(c.expected));
      failures++;
    }
  }
}

void check_wide() {
  std::u16string const s16 = u"-1.8p1";
  double result;
  auto answer =
      fast_float::from_chars(s16.data(), s16.data() + s16.size(), result, hex);
  if (answer.ec != std::errc() || answer.ptr != s16.data() + s16.size() ||
      result != -3.0) {
    std::printf("FAIL: char16_t\n");
    failures++;
  }
  std::u32string const s32 = U"ff.8p-1";
  auto answer32 =
      fast_float::from_chars(s32.data(), s32.data() + s32.size(), result, hex);
  if (answer32.ec != std::errc() || answer32.ptr != s32.data() + s32.size() ||
      result != 127.75) {
    std::printf("FAIL: char32_t\n");
    failures++;
  }
  // A code unit whose low byte is a hexadecimal digit is not a digit.
  std::u16string const bad = u"1İ";
  answer =
      fast_float::from_chars(bad.data(), bad.data() + bad.size(), result, hex);
  if (answer.ec != std::errc() || answer.ptr != bad.data() + 1 ||
      result != 1.0) {
    std::printf("FAIL: char16_t with high bits\n");
    failures++;
  }
  // A comma as the decimal point.
  std::string const comma = "1,8p1";
  fast_float::parse_options options{hex, ','};
  auto answer_comma = fast_float::from_chars_advanced(
      comma.data(), comma.data() + comma.size(), result, options);
  if (answer_comma.ec != std::errc() || result != 3.0) {
    std::printf("FAIL: comma\n");
    failures++;
  }
}

// Every finite double printed with %a parses back to itself.
template <typename T> void check_roundtrip(std::mt19937_64 &rng, size_t n) {
  using U = typename bits_of<T>::type;
  for (size_t i = 0; i < n; i++) {
    U const b = U(rng());
    T x;
    std::memcpy(&x, &b, sizeof(x));
    if (!std::isfinite(x)) {
      continue;
    }
    char buffer[64];
    std::snprintf(buffer, sizeof(buffer), "%a", double(x));
    // Drop the sign and the "0x".
    char const *start = buffer + (buffer[0] == '-' ? 3 : 2);
    std::string s = (buffer[0] == '-' ? "-" : "");
    s += start;
    T result;
    auto answer =
        fast_float::from_chars(s.data(), s.data() + s.size(), result, hex);
    if (answer.ec != std::errc() || answer.ptr != s.data() + s.size() ||
        !same(result, x)) {
      std::printf("FAIL roundtrip: %s\n", s.c_str());
      failures++;
      return;
    }
  }
}

std::string random_hex_string(std::mt19937_64 &rng) {
  static char const digits[] = "0123456789abcdefABCDEF";
  std::string s;
  if (rng() % 4 == 0) {
    s += '-';
  }
  size_t const ndigits = 1 + size_t(rng() % 40);
  size_t const point = size_t(rng() % (ndigits + 2)); // > ndigits: no point
  int const style = int(rng() % 4);
  for (size_t i = 0; i < ndigits; i++) {
    if (i == point) {
      s += '.';
    }
    char c;
    switch (style) {
    case 0: // uniform
      c = digits[rng() % 22];
      break;
    case 1: // near ties: one leading digit, then mostly zeros or 8s
      c = (i == 0) ? '1' : ((rng() % 8 == 0) ? digits[rng() % 16] : '0');
      break;
    case 2: // long runs of f
      c = (rng() % 8 == 0) ? digits[rng() % 16] : 'f';
      break;
    default: // leading zeros
      c = (i < ndigits / 2) ? '0' : digits[rng() % 16];
      break;
    }
    s += c;
  }
  if (point == ndigits) {
    s += '.';
  }
  if (rng() % 8 != 0) {
    s += (rng() % 2) ? 'p' : 'P';
    int const r = int(rng() % 3);
    if (r == 0) {
      s += '-';
    } else if (r == 1) {
      s += '+';
    }
    // Exponents that reach subnormals and overflow from any digit count.
    s += std::to_string(rng() % 1300);
  }
  return s;
}

template <typename T> void check_random(std::mt19937_64 &rng, size_t n) {
  for (size_t i = 0; i < n; i++) {
    std::string const s = random_hex_string(rng);
    std::string prefixed = s;
    size_t const sign = (s[0] == '-') ? 1 : 0;
    prefixed.insert(sign, "0x");
    T const expected = reference_parse<T>(prefixed);
    T result;
    auto answer =
        fast_float::from_chars(s.data(), s.data() + s.size(), result, hex);
    bool ok = answer.ptr == s.data() + s.size() && same(result, expected);
    if (ok) {
      bool const out_of_range =
          std::isinf(expected) ||
          (expected == 0 &&
           s.find_first_of("123456789abcdefABCDEF") < s.find_first_of("pP"));
      ok = (answer.ec ==
            (out_of_range ? std::errc::result_out_of_range : std::errc()));
    }
    if (!ok) {
      std::printf("FAIL random: %s -> %a (ec %d) expected %a\n", s.c_str(),
                  double(result), int(answer.ec), double(expected));
      failures++;
      if (failures > 20) {
        return;
      }
    }
  }
}

// Put every byte value at every position of a run of digits, so that the
// blocks of 8 digits see every kind of non-digit byte, and compare the value
// and the number of characters consumed with strtod.
void check_all_bytes() {
  std::string const base = "1234567890abcdefABCDEF0123456789";
  for (int b = 0; b < 256; b++) {
    for (size_t k = 0; k < 18; k++) {
      if (k == 0 && (b == '-' || b == '+')) {
        continue; // a sign, which strtod does not take after "0x"
      }
      for (size_t point : {size_t(0), size_t(3), size_t(100)}) {
        std::string s = base;
        if (point < s.size()) {
          s.insert(point, ".");
        }
        s[k] = char(b);
        double result = 0;
        auto answer =
            fast_float::from_chars(s.data(), s.data() + s.size(), result, hex);
        std::string prefixed = "0x" + s;
        char *end = nullptr;
        double const expected = std::strtod(prefixed.c_str(), &end);
        size_t const consumed =
            (end == prefixed.c_str()) ? 0 : size_t(end - prefixed.c_str()) - 2;
        // strtod reads "0x" as the number 0 when no digit follows.
        bool const strtod_parsed_zero = (consumed == size_t(-1));
        bool ok;
        if (strtod_parsed_zero || consumed == 0) {
          ok = (answer.ec == std::errc::invalid_argument);
        } else {
          // An exponent can follow the first digits ("1p345...").
          std::errc const ec = (std::isinf(expected) || expected == 0)
                                   ? std::errc::result_out_of_range
                                   : std::errc();
          ok = (answer.ec == ec) &&
               (size_t(answer.ptr - s.data()) == consumed) &&
               same(result, expected);
        }
        if (!ok) {
          std::printf("FAIL: byte %d at %zu (point %zu)\n", b, k, point);
          failures++;
          return;
        }
      }
    }
  }
}

// The exponent must not stop counting while a long significand still moves
// it: 4 * 67108864 = 2^28, so 0.(67108863 zeros)1 is 2^-268435456.
void check_long_exponent() {
  std::string const zeros(67108863, '0');
  struct long_case {
    std::string input;
    double expected;
    std::errc ec;
  };
  long_case const cases[] = {
      {"0." + zeros + "1p268435456", 1.0, std::errc()},
      {"0." + zeros + "1p268435457", 2.0, std::errc()},
      {"0." + zeros + "1p2684354560", std::numeric_limits<double>::infinity(),
       std::errc::result_out_of_range},
      {"1" + zeros + "0p-268435456", 1.0, std::errc()},
      {"1" + zeros + "0p-2684354560", 0.0, std::errc::result_out_of_range},
      {"1" + zeros + "0p-99999999999999999999999999", 0.0,
       std::errc::result_out_of_range},
      {"0." + zeros + "1p99999999999999999999999999",
       std::numeric_limits<double>::infinity(), std::errc::result_out_of_range},
  };
  for (auto const &c : cases) {
    double result = 12345.0;
    auto answer =
        fast_float::from_chars(c.input.data(), c.input.data() + c.input.size(),
                               result, fast_float::chars_format::hex);
    if (answer.ec != c.ec || answer.ptr != c.input.data() + c.input.size() ||
        !same(result, c.expected)) {
      std::printf("FAIL: long input ending in %s -> %a (ec %d), expected %a "
                  "(ec %d)\n",
                  c.input.c_str() + c.input.size() - 16, result,
                  int(answer.ec), c.expected, int(c.ec));
      failures++;
    }
  }
}

} // namespace

int main() {
  check_cases();
  check_long_exponent();
  check_float_cases();
  check_wide();
  check_all_bytes();
  std::mt19937_64 rng(1234);
  check_roundtrip<double>(rng, 1000000);
  check_roundtrip<float>(rng, 1000000);
  check_random<double>(rng, 1000000);
  check_random<float>(rng, 1000000);
  if (failures != 0) {
    std::printf("%d failures\n", failures);
    return EXIT_FAILURE;
  }
  std::printf("all hex float tests pass\n");
  return EXIT_SUCCESS;
}
