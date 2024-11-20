#include "fast_float/fast_float.h"
#include <iostream>
#include <string>
#include <system_error>

bool tester(std::string s, double expected,
            fast_float::chars_format fmt = fast_float::chars_format::general) {
  std::wstring input(s.begin(), s.end());
  double result;
  auto answer = fast_float::from_chars(
      input.data(), input.data() + input.size(), result, fmt);
  if (answer.ec != std::errc()) {
    std::cerr << "parsing of \"" << s << "\" should succeed\n";
    return false;
  }
  if (result != expected && !(std::isnan(result) && std::isnan(expected))) {
    std::cerr << "parsing of \"" << s << "\" succeeded, expected " << expected
              << " got " << result << "\n";
    return false;
  }
  input[0] += 256;
  answer = fast_float::from_chars(input.data(), input.data() + input.size(),
                                  result, fmt);
  if (answer.ec == std::errc()) {
    std::cerr << "parsing of altered \"" << s << "\" should fail\n";
    return false;
  }
  return true;
}

bool test_minus() { return tester("-42", -42); }

bool test_plus() {
  return tester("+42", 42,
                fast_float::chars_format::general |
                    fast_float::chars_format::allow_leading_plus);
}

bool test_space() {
  return tester(" 42", 42,
                fast_float::chars_format::general |
                    fast_float::chars_format::skip_white_space);
}

bool test_nan() {
  return tester("nan", std::numeric_limits<double>::quiet_NaN());
}

int main() {
  if (test_minus() && test_plus() && test_space() && test_nan()) {
    std::cout << "all ok" << std::endl;
    return EXIT_SUCCESS;
  }

  std::cout << "test failure" << std::endl;
  return EXIT_FAILURE;
}
