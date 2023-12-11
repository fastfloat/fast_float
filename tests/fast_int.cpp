#include <cstdlib>
#include <iostream>
#include <vector>
#include "fast_float/fast_float.h"

int main()
{

  const std::vector<int> expected_int_basic_test{0, 10, -40, 1001};
  const std::vector<std::string> accept_int_basic_test {"0", "10", "-40", "1001 with text"};
  //const std::vector<int> expected_int_basic_test{ 0, 1, -50, 9, 1001, std::numeric_limits<int>::max(), std::numeric_limits<int>::min()};
  //const std::vector<std::string> accept_int_basic_test{ "0", "1", "-50", "9.999", "1001 with text", "2147483647", "-2147483648"};
  //const std::vector<std::string> reject_int_basic_test{ "2147483648", "-2147483649", "+50"};

  /*
  const std::vector<int> expected_int_base2_test{ 0, 1, 2, 2, 9};
  const std::vector<std::string> accept_int_base2_test{ "0", "1", "10", "010", "101" };
  const std::vector<std::string> reject_int_base2_test{ "2", "09" };

  const std::vector<int> expected_int_octal_test{ 0, 1, 7, 8};
  const std::vector<std::string> accept_int_octal_test{ "00", "01", "07", "010"};
  const std::vector<std::string> reject_int_octal_test{ "08", "1" };

  const std::vector<int> expected_int_hex_test{ 0, 1, 10, 16};
  const std::vector<std::string> accept_int_hex_test{ "0", "1", "A", "0xF", "0X10"};
  const std::vector<std::string> reject_int_hex_test{ "0x", "0X" };
  */

  for (std::size_t i = 0; i < accept_int_basic_test.size(); ++i)
  {
    const auto& f = accept_int_basic_test[i];
    double result;
    auto answer = fast_float::from_chars(f.data(), f.data() + f.size(), result);
    if (answer.ec != std::errc()) {
      std::cerr << "could not convert to int for input:" << f << std::endl;
      return EXIT_FAILURE;
    }
    else if (result != expected_int_basic_test[i]) {
      std::cerr << "result did not match with expected int for input:" << f << std::endl;
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}