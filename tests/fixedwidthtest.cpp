#include <cstdlib>
#include <iostream>
#include <vector>
#include <cstring>
#include "fast_float/fast_float.h"
#include <cstdint>

#if __cplusplus >= 202300L
#include <stdfloat>
#endif

int main() {
  // Write some testcases for the parsing of floating point numbers in the
  // float32_t type. We use the from_chars function defined in this library.
#if __STDCPP_FLOAT32_T__
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
#else
  std::cout << "No std::float32_t type available." << std::endl;
#endif

#if __STDCPP_FLOAT64_T__
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
#else
  std::cout << "No std::float64_t type available." << std::endl;
#endif
  std::cout << "All tests passed successfully." << std::endl;
  return EXIT_SUCCESS;

  return 0;
}