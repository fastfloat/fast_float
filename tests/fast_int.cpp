#include <cstdlib>
#include <iostream>
#include <vector>
#include "fast_float/fast_float.h"

int main()
{
// all tests are assuming int and unsigned is size 32 bits

// int basic tests
  const std::vector<int> int_basic_test_expected {0, 10, -40, 1001, 9, 2147483647, -2147483648};
  const std::vector<std::string> int_basic_test {"0", "10", "-40", "1001 with text", "9.999", "2147483647 ", "-2147483648"};

  for (std::size_t i = 0; i < int_basic_test.size(); ++i)
  {
    const auto& f = int_basic_test[i];
    int result;
    auto answer = fast_float::from_chars(f.data(), f.data() + f.size(), result);
    if (answer.ec != std::errc()) {
      std::cerr << "could not convert to int for input:" << f << std::endl;
      return EXIT_FAILURE;
    }
    else if (result != int_basic_test_expected[i]) {
      std::cerr << "result did not match with expected int for input:" << f << std::endl;
      return EXIT_FAILURE;
    }
  }

// invalid error test
  const std::vector<std::string> int_invalid_argument_test{ "text" , "text with 1002", "+50" " 50"};

  for (std::size_t i = 0; i < int_invalid_argument_test.size(); ++i)
  {
    const auto& f = int_invalid_argument_test[i];
    int result;
    auto answer = fast_float::from_chars(f.data(), f.data() + f.size(), result);
    if (answer.ec != std::errc::invalid_argument) {
      std::cerr << "expected error should be 'invalid_argument'" << f << std::endl;
      return EXIT_FAILURE;
    }
  }

  // out of range test
  const std::vector<std::string> int_out_of_range_test{ "2147483648", "-2147483649" };

  for (std::size_t i = 0; i < int_out_of_range_test.size(); ++i)
  {
    const auto& f = int_out_of_range_test[i];
    int result;
    auto answer = fast_float::from_chars(f.data(), f.data() + f.size(), result);
    if (answer.ec != std::errc::result_out_of_range) {
      std::cerr << "expected error should be 'result_out_of_range'" << f << std::endl;
      return EXIT_FAILURE;
    }
  }

  // base 2 test
  const std::vector<int> int_base_2_test_expected {0, 1, 4, 2, -1};
  const std::vector<std::string> int_base_2_test {"0", "1", "100", "010", "-1"};

  for (std::size_t i = 0; i < int_base_2_test.size(); ++i)
  {
    const auto& f = int_base_2_test[i];
    int result;
    auto answer = fast_float::from_chars(f.data(), f.data() + f.size(), result, 2);
    if (answer.ec != std::errc()) {
      std::cerr << "could not convert to int for input:" << f << std::endl;
      return EXIT_FAILURE;
    }
    else if (result != int_base_2_test_expected[i]) {
      std::cerr << "result did not match with expected int for input:" << f << std::endl;
      return EXIT_FAILURE;
    }
  }

  // invalid error base 2 test
  const std::vector<std::string> int_invalid_argument_base_2_test{ "2", "A", "-2" };

  for (std::size_t i = 0; i < int_invalid_argument_base_2_test.size(); ++i)
  {
    const auto& f = int_invalid_argument_base_2_test[i];
    int result;
    auto answer = fast_float::from_chars(f.data(), f.data() + f.size(), result, 2);
    if (answer.ec != std::errc::invalid_argument) {
      std::cerr << "expected error should be 'invalid_argument'" << f << std::endl;
      return EXIT_FAILURE;
    }
  }

  // octal test
  const std::vector<int> int_base_octal_test_expected {0, 1, 7, 8, 9};
  const std::vector<std::string> int_base_octal_test {"0", "1", "07", "010", "0011"};

  for (std::size_t i = 0; i < int_base_octal_test.size(); ++i)
  {
    const auto& f = int_base_octal_test[i];
    int result;
    auto answer = fast_float::from_chars(f.data(), f.data() + f.size(), result, 8);
    if (answer.ec != std::errc()) {
      std::cerr << "could not convert to int for input:" << f << std::endl;
      return EXIT_FAILURE;
    }
    else if (result != int_base_octal_test_expected[i]) {
      std::cerr << "result did not match with expected int for input:" << f << std::endl;
      return EXIT_FAILURE;
    }
  }

  // hex test
  const std::vector<int> int_base_hex_test_expected { 0, 1, 15, 16, 0, 16};
  const std::vector<std::string> int_base_hex_test { "0", "1", "F", "010", "0x11", "10X11"};

  for (std::size_t i = 0; i < int_base_hex_test.size(); ++i)
  {
    const auto& f = int_base_hex_test[i];
    int result;
    auto answer = fast_float::from_chars(f.data(), f.data() + f.size(), result, 16);
    if (answer.ec != std::errc()) {
      std::cerr << "could not convert to int for input:" << f << std::endl;
      return EXIT_FAILURE;
    }
    else if (result != int_base_hex_test_expected[i]) {
      std::cerr << "result did not match with expected int for input:" << f << std::endl;
      return EXIT_FAILURE;
    }
  }


  return EXIT_SUCCESS;
}