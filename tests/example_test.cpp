
#include "fast_float/fast_float.h"
#include <iostream>
#include <string>
#include <system_error>


bool many() {
  const std::string input =   "234532.3426362,7869234.9823,324562.645";
  double result;
  auto answer = fast_float::from_chars(input.data(), input.data()+input.size(), result);
  if(answer.ec != std::errc()) { return false; }
  if(result != 234532.3426362) { return false; }
  if(answer.ptr[0] != ',') { return false; }
  answer = fast_float::from_chars(answer.ptr + 1, input.data()+input.size(), result);
  if(answer.ec != std::errc()) { return false; }
  if(result != 7869234.9823) { return false; }
  if(answer.ptr[0] != ',') { return false; }
  answer = fast_float::from_chars(answer.ptr + 1, input.data()+input.size(), result);
  if(answer.ec != std::errc()) { return false; }
  if(result != 324562.645) { return false; }
  return true;
}


void many_loop() {
  const std::string input =   "234532.3426362,7869234.9823,324562.645";
  double result;
  const char* pointer = input.data();
  const char* end_pointer = input.data() + input.size();

  while(pointer < end_pointer) {
    auto answer = fast_float::from_chars(pointer, end_pointer, result);
    if(answer.ec != std::errc()) {
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

int main() {
    const std::string input =  "3.1416 xyz ";
    double result;
    auto answer = fast_float::from_chars(input.data(), input.data()+input.size(), result);
    if((answer.ec != std::errc()) || ((result != 3.1416))) { std::cerr << "parsing failure\n"; return EXIT_FAILURE; }
    std::cout << "parsed the number " << result << std::endl;

    if(!many()) {
        printf("Bug\n");
        return EXIT_FAILURE;
    }
    many_loop();
    return EXIT_SUCCESS;
}
