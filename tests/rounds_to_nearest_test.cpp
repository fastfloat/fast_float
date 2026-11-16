// Checks that defining FASTFLOAT_ROUNDS_TO_NEAREST does not change the results
// when the rounding mode is the default (round to nearest).
#define FASTFLOAT_ROUNDS_TO_NEAREST
#include "fast_float/fast_float.h"

#include <cfloat>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <random>

namespace {

bool check_double(char const *s) {
  double const expected = std::strtod(s, nullptr);
  double result = 0;
  auto const answer = fast_float::from_chars(s, s + std::strlen(s), result);
  if (answer.ec != std::errc() ||
      std::memcmp(&result, &expected, sizeof(double)) != 0) {
    std::cerr << "mismatch on " << s << "\n";
    return false;
  }
  return true;
}

bool check_float(char const *s) {
  float const expected = std::strtof(s, nullptr);
  float result = 0;
  auto const answer = fast_float::from_chars(s, s + std::strlen(s), result);
  if (answer.ec != std::errc() ||
      std::memcmp(&result, &expected, sizeof(float)) != 0) {
    std::cerr << "mismatch on " << s << " (float)\n";
    return false;
  }
  return true;
}

} // namespace

int main() {
#if (FLT_EVAL_METHOD == 0) || (FLT_EVAL_METHOD == 1)
  if (!fast_float::detail::rounds_to_nearest()) {
    std::cerr << "FASTFLOAT_ROUNDS_TO_NEAREST is ignored\n";
    return EXIT_FAILURE;
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
    if (!check_double(s) || !check_float(s)) {
      return EXIT_FAILURE;
    }
  }
  char const *double_cases[] = {"1e22", "-1e22", "1e-22",
                                "2.2250738585072014e-308",
                                "1.7976931348623157e308"};
  for (char const *s : double_cases) {
    if (!check_double(s)) {
      return EXIT_FAILURE;
    }
  }
  std::mt19937_64 gen(1234);
  std::uniform_real_distribution<double> prices(-1e6, 1e6);
  std::uniform_int_distribution<uint64_t> bits;
  char buffer[64];
  for (size_t i = 0; i < 100000; i++) {
    // short decimals: they take Clinger's fast path
    std::snprintf(buffer, sizeof(buffer), "%.2f", prices(gen));
    if (!check_double(buffer) || !check_float(buffer)) {
      return EXIT_FAILURE;
    }
    // arbitrary doubles: most take the other paths
    uint64_t const word = bits(gen);
    double d;
    std::memcpy(&d, &word, sizeof(d));
    if (d != d || d - d != 0) { // NaN or infinity
      continue;
    }
    std::snprintf(buffer, sizeof(buffer), "%.17g", d);
    if (!check_double(buffer)) {
      return EXIT_FAILURE;
    }
  }
  if (fast_float::integer_times_pow10(uint64_t(123456), -2) != 1234.56 ||
      fast_float::integer_times_pow10(int64_t(-123456), -2) != -1234.56) {
    std::cerr << "integer_times_pow10 mismatch\n";
    return EXIT_FAILURE;
  }
  std::cout << "ok\n";
  return EXIT_SUCCESS;
}
