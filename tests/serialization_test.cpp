#include "fast_float/binary_to_decimal.h"

#include <iostream>

int main() {
  std::cout << "serialization test" << std::endl;
  char buffer[20];
  for(size_t i = 0; i < 20; i++) {
      buffer[i] = '\0';
  }
  fast_float::serialize(12345,buffer);
  std::cout << buffer << std::endl;
  return EXIT_SUCCESS;
}
