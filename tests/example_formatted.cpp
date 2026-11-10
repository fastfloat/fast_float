#include "fast_float/fast_float.h"

#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <string>
#include <system_error>

// Parses formatted numbers such as "$1,234.56", "-12.5%" or "1,000" in one
// pass: we skip ',', '$' and '%' while accumulating the digits, and
// fast_float::integer_times_pow10 does the conversion. The result is correctly
// rounded. Returns false if the input is not a number.
bool parse_formatted(std::string const &s, double &out) {
  uint64_t mantissa = 0;
  int significant_digits = 0; // leading zeros excluded
  int exponent = 0;           // minus the number of digits after the point
  bool negative = false, seen_dot = false, seen_digit = false, started = false;
  for (char c : s) {
    if (c >= '0' && c <= '9') {
      seen_digit = started = true;
      if (mantissa != 0 || c != '0') {
        significant_digits++;
      }
      // overflows beyond 19 significant digits, handled below
      mantissa = 10 * mantissa + uint64_t(c - '0');
      exponent -= seen_dot;
    } else if (c == ',' || c == '$' || c == '%') {
      continue;
    } else if (c == '.' && !seen_dot) {
      seen_dot = started = true;
    } else if (c == '-' && !started) {
      negative = started = true;
    } else {
      return false;
    }
  }
  if (!seen_digit) {
    return false;
  }
  if (significant_digits > 19) {
    // Rare: the mantissa does not fit in 64 bits, let from_chars do it.
    std::string clean;
    for (char c : s) {
      if ((c >= '0' && c <= '9') || c == '.' || c == '-') {
        clean += c;
      }
    }
    return fast_float::from_chars(clean.data(), clean.data() + clean.size(),
                                  out)
               .ec == std::errc();
  }
  out = fast_float::integer_times_pow10(mantissa, exponent);
  if (negative) {
    out = -out;
  }
  return true;
}

int main() {
  struct test_case {
    char const *input;
    bool valid;
    double expected;
  };

  test_case const cases[] = {
      {"$1,234.56", true, 1234.56},
      {"$-1,234.56", true, -1234.56},
      {"-12.5%", true, -12.5},
      {"1,000", true, 1000},
      {"0.1", true, 0.1},
      {"-0", true, -0.0},
      {"123,456,789,012,345,678,901.5", true, 123456789012345678901.5},
      {"0.00000000000000000000000000001234", true, 1.234e-29},
      {"", false, 0},
      {"-", false, 0},
      {"n/a", false, 0},
      {"1.2.3", false, 0},
      {"12abc", false, 0},
  };
  bool ok = true;
  for (test_case const &t : cases) {
    double result = 0;
    bool const valid = parse_formatted(t.input, result);
    bool const good =
        (valid == t.valid) &&
        (!valid || std::memcmp(&result, &t.expected, sizeof(double)) == 0);
    std::cout.precision(17);
    std::cout << (good ? "ok   " : "FAIL ") << '"' << t.input << "\" -> ";
    if (valid) {
      std::cout << result << "\n";
    } else {
      std::cout << "not a number\n";
    }
    ok = ok && good;
  }
  return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
