#include "fast_float/fast_float.h"
#include <iostream>
#include <string>
#include <system_error>
#include <type_traits>

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

// A wide code unit whose low byte is an ASCII space (0x20) but whose full value
// is not whitespace must not be skipped by skip_white_space. When wchar_t is
// signed such a unit can be negative, which used to slip past the range guard
// in is_space and get treated as a space.
bool test_non_space_with_space_low_byte() {
  if (!std::is_signed<wchar_t>::value) {
    return true; // only reproducible where wchar_t is signed
  }
  std::wstring input = L" 42";
  // 0x...FF20: low byte 0x20, high bits set, so the value is negative.
  input[0] = static_cast<wchar_t>(~static_cast<unsigned int>(0xFF) | 0x20u);
  double result;
  auto answer =
      fast_float::from_chars(input.data(), input.data() + input.size(), result,
                             fast_float::chars_format::general |
                                 fast_float::chars_format::skip_white_space);
  if (answer.ec == std::errc()) {
    std::cerr << "leading non-space code unit must not be skipped\n";
    return false;
  }
  return true;
}

// is_integer subtracts UC('0') from the code unit. When wchar_t is signed the
// subtraction is promoted to int and overflows for code units near the bottom
// of the range, so the sanitized build aborts on input the library is supposed
// to reject. Such a unit is not a digit, so both entry points must fail.
bool test_code_unit_below_zero_digit() {
  if (!std::is_signed<wchar_t>::value) {
    return true; // only reproducible where wchar_t is signed
  }
  wchar_t const input[] = {(std::numeric_limits<wchar_t>::min)(), L'1'};
  double result;
  auto answer = fast_float::from_chars(input, input + 2, result);
  if (answer.ec != std::errc::invalid_argument) {
    std::cerr << "a code unit below L'0' must not start a number\n";
    return false;
  }
  unsigned long long integer;
  auto int_answer = fast_float::from_chars(input, input + 2, integer, 10);
  if (int_answer.ec != std::errc::invalid_argument) {
    std::cerr << "a code unit below L'0' must not start an integer\n";
    return false;
  }
  return true;
}

int main() {
  if (test_minus() && test_plus() && test_space() && test_nan() &&
      test_non_space_with_space_low_byte() &&
      test_code_unit_below_zero_digit()) {
    std::cout << "all ok" << std::endl;
    return EXIT_SUCCESS;
  }

  std::cout << "test failure" << std::endl;
  return EXIT_FAILURE;
}
