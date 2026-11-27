#include <cstdlib>
#include <iostream>
#include <vector>
#include <cstring>
#include <limits>
#include <string_view>
#include "fast_float/fast_float.h"
#include <cstdint>
#include <cmath>
#include <cstdio>
#include <string>

#if __cplusplus >= 202300L
#include <stdfloat>
#endif

// If no characters match the pattern, value must be left unmodified
// (see https://github.com/fastfloat/fast_float/issues/424).
template <typename T> bool check_invalid_input_preserves_value() {
  std::vector<std::string_view> const invalid_inputs{"x",  "",   "-",  ".",
                                                     "e1", "-x", "na", "+"};
  for (auto const &f : invalid_inputs) {
    T result = T(42);
    auto answer = fast_float::from_chars(f.data(), f.data() + f.size(), result);
    if (answer.ec != std::errc::invalid_argument || answer.ptr != f.data()) {
      std::cerr << "Expected invalid_argument for input: \"" << f << "\""
                << std::endl;
      return false;
    }
    if (result != T(42)) {
      std::cerr << "Value modified on invalid input: \"" << f << "\" got "
                << result << std::endl;
      return false;
    }
  }
  // Out-of-range input must still store the documented value.
  std::string_view const too_large = "1e999";
  T result = T(42);
  auto answer = fast_float::from_chars(
      too_large.data(), too_large.data() + too_large.size(), result);
  if (answer.ec != std::errc::result_out_of_range ||
      result != std::numeric_limits<T>::infinity()) {
    std::cerr << "Expected infinity and result_out_of_range for \"1e999\""
              << std::endl;
    return false;
  }
  return true;
}

// Hexadecimal inputs (chars_format::hex) for a 16-bit type: every finite
// value, and every midpoint between consecutive positive values (ties to
// even), printed exactly with %a.
template <typename T> bool check_hex_16bit(uint16_t infinity_bits) {
  for (uint32_t b = 0; b < 65536; b++) {
    uint16_t const bits = uint16_t(b);
    T x;
    std::memcpy(&x, &bits, sizeof(x));
    double const d = double(x);
    if (!std::isfinite(d)) {
      continue;
    }
    for (int midpoint = 0; midpoint < 2; midpoint++) {
      double target = d;
      uint16_t expected = bits;
      if (midpoint) {
        uint16_t const next_bits = uint16_t(bits + 1);
        T next;
        std::memcpy(&next, &next_bits, sizeof(next));
        if (std::signbit(d) || !std::isfinite(double(next))) {
          continue; // negative values are symmetric
        }
        target = (d + double(next)) / 2; // exact in double
        expected = (bits & 1) ? next_bits : bits;
      }
      char buffer[64];
      std::snprintf(buffer, sizeof(buffer), "%a", target);
      std::string s = buffer;
      size_t const x_at = s.find('x');
      s = s.substr(0, x_at - 1) + s.substr(x_at + 1); // drop the "0x"
      T result;
      auto answer = fast_float::from_chars(
          s.data(), s.data() + s.size(), result, fast_float::chars_format::hex);
      uint16_t result_bits;
      std::memcpy(&result_bits, &result, sizeof(result));
      // Overflow to infinity, or a midpoint that rounds to zero, is out of
      // range.
      bool const out_of_range = ((expected & 0x7fff) == infinity_bits) ||
                                (target != 0 && (expected & 0x7fff) == 0);
      std::errc const ec =
          out_of_range ? std::errc::result_out_of_range : std::errc();
      if (result_bits != expected || answer.ptr != s.data() + s.size() ||
          answer.ec != ec) {
        std::cerr << "hex parsing failed for \"" << s << "\"" << std::endl;
        return false;
      }
    }
  }
  return true;
}

int main() {
  // Write some testcases for the parsing of floating point numbers in the
  // float32_t type. We use the from_chars function defined in this library.
#ifdef __STDCPP_FLOAT32_T__
  std::vector<std::float32_t> const float32_test_expected{
      123.456f, -78.9f, 0.0001f, 3.40282e+038f};
  std::vector<std::string_view> const float32_test{"123.456", "-78.9", "0.0001",
                                                   "3.40282e+038"};
  std::cout << "runing float32 test" << std::endl;
  for (std::size_t i = 0; i < float32_test.size(); ++i) {
    auto const &f = float32_test[i];
    std::float32_t result;
    auto answer = fast_float::from_chars(f.data(), f.data() + f.size(), result);

    if (answer.ec != std::errc()) {
      std::cerr << "Failed to parse: \"" << f << "\"" << std::endl;
      return EXIT_FAILURE;
    }
    if (result != float32_test_expected[i]) {
      std::cerr << "Test failed for input: \"" << f << "\" expected "
                << float32_test_expected[i] << " got " << result << std::endl;
      return EXIT_FAILURE;
    }
  }
  if (!check_invalid_input_preserves_value<std::float32_t>()) {
    return EXIT_FAILURE;
  }
#else
  std::cout << "No std::float32_t type available." << std::endl;
#endif

#ifdef __STDCPP_FLOAT64_T__
  // Test cases for std::float64_t
  std::vector<std::float64_t> const float64_test_expected{
      1.23e4, -5.67e-8, 1.7976931348623157e+308, -1.7976931348623157e+308};
  std::vector<std::string_view> const float64_test{"1.23e4", "-5.67e-8",
                                                   "1.7976931348623157e+308",
                                                   "-1.7976931348623157e+308"};
  std::cout << "runing float64 test" << std::endl;
  for (std::size_t i = 0; i < float64_test.size(); ++i) {
    auto const &f = float64_test[i];
    std::float64_t result;
    auto answer = fast_float::from_chars(f.data(), f.data() + f.size(), result);

    if (answer.ec != std::errc()) {
      std::cerr << "Failed to parse: \"" << f << "\"" << std::endl;
      return EXIT_FAILURE;
    }
    if (result != float64_test_expected[i]) {
      std::cerr << "Test failed for input: \"" << f << "\" expected "
                << float64_test_expected[i] << " got " << result << std::endl;
      return EXIT_FAILURE;
    }
  }
  if (!check_invalid_input_preserves_value<std::float64_t>()) {
    return EXIT_FAILURE;
  }
#else
  std::cout << "No std::float64_t type available." << std::endl;
#endif
#ifdef __STDCPP_FLOAT16_T__
  std::cout << "runing float16 hex test" << std::endl;
  if (!check_hex_16bit<std::float16_t>(0x7c00)) {
    return EXIT_FAILURE;
  }
#endif
#ifdef __STDCPP_BFLOAT16_T__
  std::cout << "runing bfloat16 hex test" << std::endl;
  if (!check_hex_16bit<std::bfloat16_t>(0x7f80)) {
    return EXIT_FAILURE;
  }
#endif
  std::cout << "All tests passed successfully." << std::endl;
  return EXIT_SUCCESS;
}
