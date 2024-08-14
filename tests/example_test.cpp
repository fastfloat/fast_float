
#include "fast_float/fast_float.h"
#include <iostream>
#include <string>
#include <system_error>

bool many() {
  const std::string input = "234532.3426362,7869234.9823,324562.645";
  double result;
  auto answer =
      fast_float::from_chars(input.data(), input.data() + input.size(), result);
  if (answer.ec != std::errc()) {
    return false;
  }
  if (result != 234532.3426362) {
    return false;
  }
  if (answer.ptr[0] != ',') {
    return false;
  }
  answer = fast_float::from_chars(answer.ptr + 1, input.data() + input.size(),
                                  result);
  if (answer.ec != std::errc()) {
    return false;
  }
  if (result != 7869234.9823) {
    return false;
  }
  if (answer.ptr[0] != ',') {
    return false;
  }
  answer = fast_float::from_chars(answer.ptr + 1, input.data() + input.size(),
                                  result);
  if (answer.ec != std::errc()) {
    return false;
  }
  if (result != 324562.645) {
    return false;
  }
  return true;
}

void many_loop() {
  const std::string input = "234532.3426362,7869234.9823,324562.645";
  double result;
  const char *pointer = input.data();
  const char *end_pointer = input.data() + input.size();

  while (pointer < end_pointer) {
    auto answer = fast_float::from_chars(pointer, end_pointer, result);
    if (answer.ec != std::errc()) {
      std::cerr << "error while parsing" << std::endl;
      break;
    }
    std::cout << "parsed: " << result << std::endl;
    pointer = answer.ptr;
    if ((answer.ptr < end_pointer) && (*pointer == ',')) {
      pointer++;
    }
  }
}

#if FASTFLOAT_IS_CONSTEXPR
// consteval forces compile-time evaluation of the function in C++20.
consteval double parse(std::string_view input) {
  double result;
  auto answer =
      fast_float::from_chars(input.data(), input.data() + input.size(), result);
  if (answer.ec != std::errc()) {
    return -1.0;
  }
  return result;
}

// This function should compile to a function which
// merely returns 3.1415.
constexpr double constexptest() { return parse("3.1415 input"); }
#endif

int main() {
  const std::string input = "3.1416 xyz ";
  double result;
  auto answer =
      fast_float::from_chars(input.data(), input.data() + input.size(), result);
  if ((answer.ec != std::errc()) || ((result != 3.1416))) {
    std::cerr << "parsing failure\n";
    return EXIT_FAILURE;
  }
  std::cout << "parsed the number " << result << std::endl;

  if (!many()) {
    printf("Bug\n");
    return EXIT_FAILURE;
  }
  many_loop();
#if FASTFLOAT_IS_CONSTEXPR
  if constexpr (constexptest() != 3.1415) {
    return EXIT_FAILURE;
  }
#endif
  return EXIT_SUCCESS;
}
