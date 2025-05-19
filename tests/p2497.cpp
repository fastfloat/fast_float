#include "fast_float/fast_float.h"

#include <iostream>
#include <string>

int main() {
  std::string input = "3.1416 xyz ";
  double result;
  if(auto answer = fast_float::from_chars(input.data(), input.data() + input.size(), result)) {
    std::cout << "parsed the number " << result << std::endl;
    return EXIT_SUCCESS;
  }
  std::cerr << "failed to parse " << result << std::endl;
  return EXIT_FAILURE;
}