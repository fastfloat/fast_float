#include "fast_float/fast_float.h"
#include <iostream>
#include <string>
#include <system_error>

template <typename UC> bool test(std::string s, double expected) {
  std::basic_string<UC> input(s.begin(), s.end());
  double result;
  auto answer =
      fast_float::from_chars(input.data(), input.data() + input.size(), result);
  if (answer.ec != std::errc()) {
    std::cerr << "parsing of \"" << s << "\" should succeed\n";
    return false;
  }
  if (result != expected && !(std::isnan(result) && std::isnan(expected))) {
    std::cerr << "parsing of \"" << s << "\" succeeded, expected " << expected
              << " got " << result << "\n";
    return false;
  }
  return true;
}

int main() {
  if (!test<char>("4.2", 4.2)) {
    std::cout << "test failure for char" << std::endl;
    return EXIT_FAILURE;
  }

  if (!test<wchar_t>("4.2", 4.2)) {
    std::cout << "test failure for wchar_t" << std::endl;
    return EXIT_FAILURE;
  }

  if (!test<char16_t>("4.2", 4.2)) {
    std::cout << "test failure for char16_t" << std::endl;
    return EXIT_FAILURE;
  }

  if (!test<char32_t>("4.2", 4.2)) {
    std::cout << "test failure for char32_t" << std::endl;
    return EXIT_FAILURE;
  }

  std::cout << "all ok" << std::endl;
  return EXIT_SUCCESS;
}
