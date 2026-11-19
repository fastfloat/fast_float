// Performance hints (fast_float::performance_hints) must not change the value,
// the error or the end pointer. We also check that the overloads taking hints
// do not disturb the other overloads.
#include "fast_float/fast_float.h"

#include <cfloat>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <limits>
#include <random>
#include <string>
#include <system_error>
#include <vector>

namespace {

using fast_float::chars_format;
using fast_float::performance_hints;

constexpr performance_hints sign = performance_hints::unpredictable_sign;
constexpr performance_hints nearest =
    performance_hints::assume_round_to_nearest;
constexpr performance_hints short_numbers = performance_hints::short_numbers;
constexpr performance_hints all = sign | nearest | short_numbers;

chars_format const formats[] = {
    chars_format::general,
    chars_format::general | chars_format::allow_leading_plus,
    chars_format::general | chars_format::skip_white_space,
    chars_format::scientific,
    chars_format::fixed,
    chars_format::json,
    chars_format::json_or_infnan,
    chars_format::fortran,
    chars_format::javascript,
    chars_format::javascript | chars_format::allow_leading_plus};

template <typename UC> void print(std::basic_string<UC> const &s) {
  for (UC c : s) {
    std::cerr << char(c);
  }
}

template <typename T, typename UC>
bool same(std::basic_string<UC> const &s,
          fast_float::from_chars_result_t<UC> r1, T expected,
          fast_float::from_chars_result_t<UC> r2, T actual, char const *what,
          chars_format fmt) {
  if (r1.ec != r2.ec || r1.ptr != r2.ptr ||
      std::memcmp(&expected, &actual, sizeof(T)) != 0) {
    std::cerr << what << ": mismatch on \"";
    print(s);
    std::cerr << "\" (format " << uint64_t(fmt) << ", size " << sizeof(T)
              << ", char size " << sizeof(UC) << ")\n";
    return false;
  }
  return true;
}

// from_chars<Hints> and from_chars_advanced<Hints> against the same calls
// without hints.
template <performance_hints Hints, typename T, typename UC>
bool same_result(std::basic_string<UC> const &s, chars_format fmt) {
  UC const *first = s.data();
  UC const *last = s.data() + s.size();
  T expected = T(-12345);
  T actual = T(-12345);
  auto r1 = fast_float::from_chars(first, last, expected, fmt);
  auto r2 = fast_float::from_chars<Hints>(first, last, actual, fmt);
  if (!same(s, r1, expected, r2, actual, "from_chars", fmt)) {
    return false;
  }
  fast_float::parse_options_t<UC> const options(fmt, UC(','));
  expected = T(-12345);
  actual = T(-12345);
  r1 = fast_float::from_chars_advanced(first, last, expected, options);
  r2 = fast_float::from_chars_advanced<Hints>(first, last, actual, options);
  return same(s, r1, expected, r2, actual, "from_chars_advanced", fmt);
}

// The parser's own result also tells which error it found, and where.
template <bool json, typename UC>
bool same_parse(std::basic_string<UC> const &s, chars_format fmt) {
  UC const *first = s.data();
  UC const *last = s.data() + s.size();
  fast_float::parse_options_t<UC> const options(fmt);
  auto const a = fast_float::parse_number_string_impl<json, false, UC, false>(
      first, last, options, true);
  auto const b = fast_float::parse_number_string_impl<json, false, UC, true>(
      first, last, options, true);
  bool const ok =
      a.valid == b.valid && a.error == b.error && a.lastmatch == b.lastmatch &&
      (!a.valid ||
       (a.negative == b.negative && a.mantissa == b.mantissa &&
        a.exponent == b.exponent && a.too_many_digits == b.too_many_digits));
  if (!ok) {
    std::cerr << "parse mismatch on \"";
    print(s);
    std::cerr << "\" (format " << uint64_t(fmt) << ", char size " << sizeof(UC)
              << ")\n";
  }
  return ok;
}

template <typename UC> bool check(std::string const &ascii) {
  std::basic_string<UC> const s(ascii.begin(), ascii.end());
  // The JavaScript formats have their own parser: no parse-level check.
  if (!same_parse<false>(s, chars_format::general) ||
      !same_parse<false>(s, chars_format::general |
                                chars_format::allow_leading_plus) ||
      !same_parse<true>(s, chars_format::json)) {
    return false;
  }
  for (chars_format fmt : formats) {
    if (!same_result<sign, double>(s, fmt) ||
        !same_result<sign, float>(s, fmt) ||
        !same_result<nearest, double>(s, fmt) ||
        !same_result<nearest, float>(s, fmt) ||
        !same_result<short_numbers, double>(s, fmt) ||
        !same_result<short_numbers, float>(s, fmt) ||
        !same_result<all, double>(s, fmt) || !same_result<all, float>(s, fmt)) {
      return false;
    }
  }
  return true;
}

bool check_all(std::string const &s) {
  return check<char>(s) && check<char16_t>(s) && check<char32_t>(s);
}

// With assume_round_to_nearest, the results must still be those of strtod in
// the default rounding mode.
bool check_nearest_double(char const *s) {
  double const expected = std::strtod(s, nullptr);
  double result = 0;
  auto const answer =
      fast_float::from_chars<nearest>(s, s + std::strlen(s), result);
  if (answer.ec != std::errc() ||
      std::memcmp(&result, &expected, sizeof(double)) != 0) {
    std::cerr << "nearest: mismatch on " << s << "\n";
    return false;
  }
  return true;
}

bool check_nearest_float(char const *s) {
  float const expected = std::strtof(s, nullptr);
  float result = 0;
  auto const answer =
      fast_float::from_chars<nearest>(s, s + std::strlen(s), result);
  if (answer.ec != std::errc() ||
      std::memcmp(&result, &expected, sizeof(float)) != 0) {
    std::cerr << "nearest: mismatch on " << s << " (float)\n";
    return false;
  }
  return true;
}

bool check_nearest() {
#if (FLT_EVAL_METHOD == 0) || (FLT_EVAL_METHOD == 1)
  if (!fast_float::detail::rounds_to_nearest<true>()) {
    std::cerr << "assume_round_to_nearest is ignored\n";
    return false;
  }
#endif
  char const *cases[] = {"0",
                         "-0",
                         "1",
                         "-1",
                         "0.1",
                         "-0.1",
                         "1e10",
                         "-1e10",
                         "1e-10",
                         "16777216",
                         "16777217",
                         "9007199254740992",
                         "9007199254740993",
                         "123456.789",
                         "-1234.56",
                         "1e23"};
  for (char const *s : cases) {
    if (!check_nearest_double(s) || !check_nearest_float(s)) {
      return false;
    }
  }
  std::mt19937_64 gen(1234);
  std::uniform_real_distribution<double> prices(-1e6, 1e6);
  std::uniform_int_distribution<uint64_t> bits;
  char buffer[64];
  for (size_t i = 0; i < 100000; i++) {
    // short decimals: they take Clinger's fast path
    std::snprintf(buffer, sizeof(buffer), "%.2f", prices(gen));
    if (!check_nearest_double(buffer) || !check_nearest_float(buffer)) {
      return false;
    }
    // arbitrary doubles: most take the other paths
    uint64_t const word = bits(gen);
    double d;
    std::memcpy(&d, &word, sizeof(d));
    if (d != d || d - d != 0) { // NaN or infinity
      continue;
    }
    std::snprintf(buffer, sizeof(buffer), "%.17g", d);
    if (!check_nearest_double(buffer)) {
      return false;
    }
  }
  return true;
}

template <typename T> bool same_bits(T a, T b) {
  return std::memcmp(&a, &b, sizeof(T)) == 0;
}

// integer_times_pow10<Hints> against integer_times_pow10.
template <performance_hints Hints> bool check_integer_times_pow10() {
  std::mt19937_64 gen(5678);
  for (size_t i = 0; i < 200000; i++) {
    uint64_t const word = gen();
    // mostly short mantissas, which take Clinger's fast path
    uint64_t const u = (i % 2 == 0) ? word % 100000000 : word;
    int64_t const s = (word & 1) ? -int64_t(u >> 1) : int64_t(u >> 1);
    int const e = int(gen() % 80) - 40;
    if (!same_bits(fast_float::integer_times_pow10<Hints>(u, e),
                   fast_float::integer_times_pow10(u, e)) ||
        !same_bits(fast_float::integer_times_pow10<Hints>(s, e),
                   fast_float::integer_times_pow10(s, e)) ||
        !same_bits(fast_float::integer_times_pow10<float, Hints>(u, e),
                   fast_float::integer_times_pow10<float>(u, e)) ||
        !same_bits(fast_float::integer_times_pow10<float, Hints>(s, e),
                   fast_float::integer_times_pow10<float>(s, e)) ||
        !same_bits(fast_float::integer_times_pow10<Hints>(int(s % 1000), e),
                   fast_float::integer_times_pow10(int(s % 1000), e)) ||
        !same_bits(fast_float::integer_times_pow10<Hints>(uint16_t(u), e),
                   fast_float::integer_times_pow10(uint16_t(u), e))) {
      std::cerr << "integer_times_pow10 mismatch on " << u << " or " << s
                << " times 10^" << e << " (hints " << uint64_t(Hints) << ")\n";
      return false;
    }
  }
  int64_t const most_negative = (std::numeric_limits<int64_t>::min)();
  if (fast_float::integer_times_pow10<Hints>(most_negative, 0) !=
      double(most_negative)) {
    std::cerr << "integer_times_pow10 mismatch on the most negative value\n";
    return false;
  }
  return true;
}

// The other overloads still resolve as before.
bool check_overloads() {
  std::string const input = "-12.5";
  char const *first = input.data();
  char const *last = input.data() + input.size();
  double d = 0;
  // explicit types
  fast_float::from_chars<double>(first, last, d);
  fast_float::from_chars<double, char>(first, last, d, chars_format::fixed);
  fast_float::from_chars_advanced<double>(first, last, d,
                                          fast_float::parse_options());
  // the address of an overload
  fast_float::from_chars_result_t<char> (*pointer)(char const *, char const *,
                                                   double &, chars_format) =
      &fast_float::from_chars<double, char>;
  if (pointer(first, last, d, chars_format::general).ec != std::errc() ||
      d != -12.5) {
    std::cerr << "function pointer to from_chars failed\n";
    return false;
  }
  // integers
  std::string const integer = "-125";
  int i = 0;
  fast_float::from_chars(integer.data(), integer.data() + integer.size(), i);
  long l = 0;
  fast_float::from_chars<long>(integer.data(), integer.data() + integer.size(),
                               l, 10);
  if (i != -125 || l != -125) {
    std::cerr << "integer from_chars failed\n";
    return false;
  }
  // integer_times_pow10
  if (fast_float::integer_times_pow10(-125, -1) != -12.5 ||
      fast_float::integer_times_pow10<double>(125u, -1) != 12.5 ||
      fast_float::integer_times_pow10<float>(int64_t(-125), -1) != -12.5f ||
      fast_float::integer_times_pow10(uint64_t(125), -1) != 12.5) {
    std::cerr << "integer_times_pow10 failed\n";
    return false;
  }
  return true;
}

} // namespace

int main() {
  if (!check_overloads() || !check_nearest() ||
      !check_integer_times_pow10<sign>() ||
      !check_integer_times_pow10<nearest>() ||
      !check_integer_times_pow10<short_numbers>() ||
      !check_integer_times_pow10<all>()) {
    return EXIT_FAILURE;
  }
  std::vector<std::string> const cases = {
      "-", "+", "-.", "+.", "-.5", "+.5", "-5", "+5", "--5", "-+5", "+-5",
      "-e5", "-5e", "-5e-3", "-0", "+0", "-0.0", "-1e400", "-1e-400", "-inf",
      "+inf", "-nan", "-infinity", "- 1", " -1", "  +1", "-,5", "-1,5", "-x",
      "-1.5x", "-123456789012345678901234567890", "-0x1p3", "-9007199254740993",
      "-2.2250738585072014e-308", "-1.7976931348623157e308", "-1d5", "-1+5",
      // around 8 characters
      "12345678", "123456789", "-1234567", "-12345678", "+1234567", "1234.567",
      "1234.5678", "-123.456", ".1234567", "1234567.", "0000000.", "-0000000",
      "99999999", "9.999999", "1.2.3", "12..3", "1,234", "12 34"};
  for (std::string const &s : cases) {
    if (!check_all(s)) {
      return EXIT_FAILURE;
    }
  }
  // Short random strings over an alphabet rich in signs, digits and dots.
  char const alphabet[] = "0123456789000.-+e ,xn";
  std::mt19937_64 gen(1234);
  for (size_t i = 0; i < 100000; i++) {
    size_t const length = 1 + size_t(gen() % 10);
    std::string s;
    for (size_t k = 0; k < length; k++) {
      s += alphabet[size_t(gen() % (sizeof(alphabet) - 1))];
    }
    if (!check_all(s)) {
      return EXIT_FAILURE;
    }
  }
  std::cout << "ok\n";
  return EXIT_SUCCESS;
}
