#include <cstdlib>
#include <iostream>
#include <vector>
#include <iomanip>
#include <cstring>
#include "fast_float/fast_float.h"

/*
all tests conducted are to check fast_float::from_chars functionality with int and unsigned
test cases include:
int basic tests - numbers only, numbers with strings behind, decimals, negative numbers
unsigned basic tests - numbers only, numbers with strings behind, decimals
int invalid tests - strings only, strings with numbers behind, space in front of number, plus sign in front of number
unsigned invalid tests - strings only, strings with numbers behind, space in front of number, plus/minus sign in front of number
int out of range tests - numbers exceeding int bit size (Note: out of range errors for 8, 16, 32, and 64 bits have not been tested)
unsigned out of range tests - numbers exceeding unsigned bit size (Note: out of range errors for 8, 16, 32, and 64 bits have not been tested)
int pointer tests - points to first character that is not recognized as int
unsigned pointer tests - points to first character that is not recognized as unsigned
int/unsigned base 2 tests - numbers are converted from binary to decimal
octal tests - numbers are converted from octal to decimal
hex tests - numbers are converted from hex to decimal (Note: 0x and 0X are considered invalid)
invalid base tests - everything is invalid
out of range base tests - should still work even with a base greater than 36
*/

int main()
{

  // int basic test
  const std::vector<int> int_basic_test_expected { 0, 10, -40, 1001, 9 };
  const std::vector<std::string> int_basic_test { "0", "10 ", "-40", "1001 with text", "9.999" };

  for (std::size_t i = 0; i < int_basic_test.size(); ++i)
  {
    const auto& f = int_basic_test[i];
    int result;
    auto answer = fast_float::from_chars(f.data(), f.data() + f.size(), result);
    if (answer.ec != std::errc()) {
      std::cerr << "could not convert to int for input: " << std::quoted(f) << std::endl;
      return EXIT_FAILURE;
    }
    else if (result != int_basic_test_expected[i]) {
      std::cerr << "result "  << std::quoted(f) << " did not match with expected int: " << int_basic_test_expected[i] << std::endl;
      return EXIT_FAILURE;
    }
  }

  // unsigned basic test
  const std::vector<unsigned> unsigned_basic_test_expected { 0, 10, 1001, 9 };
  const std::vector<std::string> unsigned_basic_test { "0", "10 ", "1001 with text", "9.999" };

  for (std::size_t i = 0; i < unsigned_basic_test.size(); ++i)
  {
    const auto& f = unsigned_basic_test[i];
    unsigned result;
    auto answer = fast_float::from_chars(f.data(), f.data() + f.size(), result);
    if (answer.ec != std::errc()) {
      std::cerr << "could not convert to unsigned for input: " << std::quoted(f) << std::endl;
      return EXIT_FAILURE;
    }
    else if (result != unsigned_basic_test_expected[i]) {
      std::cerr << "result "  << std::quoted(f) << " did not match with expected unsigned: " << unsigned_basic_test_expected[i] << std::endl;
      return EXIT_FAILURE;
    }
  }

  // int invalid error test
  const std::vector<std::string> int_invalid_argument_test{ "text", "text with 1002", "+50", " 50" };

  for (std::size_t i = 0; i < int_invalid_argument_test.size(); ++i)
  {
    const auto& f = int_invalid_argument_test[i];
    int result;
    auto answer = fast_float::from_chars(f.data(), f.data() + f.size(), result);
    if (answer.ec != std::errc::invalid_argument) {
      std::cerr << "expected error should be 'invalid_argument' for: " << std::quoted(f) << std::endl;
      return EXIT_FAILURE;
    }
  }

  // unsigned invalid error test
  const std::vector<std::string> unsigned_invalid_argument_test{ "text", "text with 1002", "+50", " 50", "-50" };

  for (std::size_t i = 0; i < unsigned_invalid_argument_test.size(); ++i)
  {
    const auto& f = unsigned_invalid_argument_test[i];
    unsigned result;
    auto answer = fast_float::from_chars(f.data(), f.data() + f.size(), result);
    if (answer.ec != std::errc::invalid_argument) {
      std::cerr << "expected error should be 'invalid_argument' for: " << std::quoted(f) << std::endl;
      return EXIT_FAILURE;
    }
  }

  // int out of range error test 
  const std::vector<std::string> int_out_of_range_test{ "2000000000000000000000" };

  for (std::size_t i = 0; i < int_out_of_range_test.size(); ++i)
  {
    const auto& f = int_out_of_range_test[i];
    int result;
    auto answer = fast_float::from_chars(f.data(), f.data() + f.size(), result);
    if (answer.ec != std::errc::result_out_of_range) {
      std::cerr << "expected error for should be 'result_out_of_range': " << std::quoted(f) << std::endl;
      return EXIT_FAILURE;
    }
  }

  // unsigned out of range error test 
  const std::vector<std::string> unsigned_out_of_range_test{ "2000000000000000000000" };

  for (std::size_t i = 0; i < unsigned_out_of_range_test.size(); ++i)
  {
    const auto& f = unsigned_out_of_range_test[i];
    unsigned result;
    auto answer = fast_float::from_chars(f.data(), f.data() + f.size(), result);
    if (answer.ec != std::errc::result_out_of_range) {
      std::cerr << "expected error for should be 'result_out_of_range': " << std::quoted(f) << std::endl;
      return EXIT_FAILURE;
    }
  }

  // int pointer test #1 (only numbers)
  const std::vector<std::string> int_pointer_test_1 { "0", "010", "-40" };

  for (std::size_t i = 0; i < int_pointer_test_1.size(); ++i)
  {
    const auto& f = int_pointer_test_1[i];
    int result;
    auto answer = fast_float::from_chars(f.data(), f.data() + f.size(), result);
    if (answer.ec != std::errc()) {
      std::cerr << "could not convert to int for input: " << std::quoted(f) << std::endl;
      return EXIT_FAILURE;
    }
    else if (strcmp(answer.ptr, "") != 0) {
      std::cerr << "ptr of result "  << std::quoted(f) << " did not match with expected ptr: " << std::quoted("") << std::endl;
      return EXIT_FAILURE;
    }
  }

  // int pointer test #2 (string behind numbers)
  const std::vector<std::string> int_pointer_test_2 { "1001 with text" };

  for (std::size_t i = 0; i < int_pointer_test_2.size(); ++i)
  {
    const auto& f = int_pointer_test_2[i];
    int result;
    auto answer = fast_float::from_chars(f.data(), f.data() + f.size(), result);
    if (answer.ec != std::errc()) {
      std::cerr << "could not convert to int for input: " << std::quoted(f) << std::endl;
      return EXIT_FAILURE;
    }
    else if (strcmp(answer.ptr, " with text") != 0) {
      std::cerr << "ptr of result "  << std::quoted(f) << " did not match with expected ptr: " << std::quoted(" with text") << std::endl;
      return EXIT_FAILURE;
    }
  }

  // int pointer test #3 (string with newline behind numbers)
  const std::vector<std::string> int_pointer_test_3 { "1001 with text\n" };

  for (std::size_t i = 0; i < int_pointer_test_3.size(); ++i)
  {
    const auto& f = int_pointer_test_3[i];
    int result;
    auto answer = fast_float::from_chars(f.data(), f.data() + f.size(), result);
    if (answer.ec != std::errc()) {
      std::cerr << "could not convert to int for input: " << std::quoted(f) << std::endl;
      return EXIT_FAILURE;
    }
    else if (strcmp(answer.ptr, " with text\n") != 0) {
      std::cerr << "ptr of result "  << std::quoted(f) << " did not match with expected ptr: " << std::quoted(" with text") << std::endl;
      return EXIT_FAILURE;
    }
  }

  // int pointer test #4 (float)
  const std::vector<std::string> int_pointer_test_4 { "9.999" };

  for (std::size_t i = 0; i < int_pointer_test_4.size(); ++i)
  {
    const auto& f = int_pointer_test_4[i];
    int result;
    auto answer = fast_float::from_chars(f.data(), f.data() + f.size(), result);
    if (answer.ec != std::errc()) {
      std::cerr << "could not convert to int for input: " << std::quoted(f) << std::endl;
      return EXIT_FAILURE;
    }
    else if (strcmp(answer.ptr, ".999") != 0) {
      std::cerr << "ptr of result "  << std::quoted(f) << " did not match with expected ptr: " << std::quoted(".999") << std::endl;
      return EXIT_FAILURE;
    }
  }

  // int pointer test #5 (invalid int)
  const std::vector<std::string> int_pointer_test_5 { "+50" };

  for (std::size_t i = 0; i < int_pointer_test_5.size(); ++i)
  {
    const auto& f = int_pointer_test_5[i];
    int result;
    auto answer = fast_float::from_chars(f.data(), f.data() + f.size(), result);
    if (strcmp(answer.ptr, "+50") != 0) {
      std::cerr << "ptr of result "  << std::quoted(f) << " did not match with expected ptr: " << std::quoted("+50") << std::endl;
      return EXIT_FAILURE;
    }
  }

  // unsigned pointer test #1 (string behind numbers)
  const std::vector<std::string> unsigned_pointer_test_1 { "1001 with text" };

  for (std::size_t i = 0; i < unsigned_pointer_test_1.size(); ++i)
  {
    const auto& f = unsigned_pointer_test_1[i];
    unsigned result;
    auto answer = fast_float::from_chars(f.data(), f.data() + f.size(), result);
    if (answer.ec != std::errc()) {
      std::cerr << "could not convert to unsigned for input: " << std::quoted(f) << std::endl;
      return EXIT_FAILURE;
    }
    if (strcmp(answer.ptr, " with text") != 0) {
      std::cerr << "ptr of result "  << std::quoted(f) << " did not match with expected ptr: " << std::quoted(" with text") << std::endl;
      return EXIT_FAILURE;
    }
  }

  // unsigned pointer test #2 (invalid unsigned)
  const std::vector<std::string> unsigned_pointer_test_2 { "-50" };

  for (std::size_t i = 0; i < unsigned_pointer_test_2.size(); ++i)
  {
    const auto& f = unsigned_pointer_test_2[i];
    unsigned result;
    auto answer = fast_float::from_chars(f.data(), f.data() + f.size(), result);
    if (strcmp(answer.ptr, "-50") != 0) {
      std::cerr << "ptr of result "  << std::quoted(f) << " did not match with expected ptr: " << std::quoted("-50") << std::endl;
      return EXIT_FAILURE;
    }
  }

  // int base 2 test
  const std::vector<int> int_base_2_test_expected { 0, 1, 4, 2, -1 };
  const std::vector<std::string> int_base_2_test { "0", "1", "100", "010", "-1" };

  for (std::size_t i = 0; i < int_base_2_test.size(); ++i)
  {
    const auto& f = int_base_2_test[i];
    int result;
    auto answer = fast_float::from_chars(f.data(), f.data() + f.size(), result, 2);
    if (answer.ec != std::errc()) {
      std::cerr << "could not convert to int for input: " << std::quoted(f) << std::endl;
      return EXIT_FAILURE;
    }
    else if (result != int_base_2_test_expected[i]) {
      std::cerr << "result "  << std::quoted(f) << " did not match with expected int: " << int_base_2_test_expected[i] << std::endl;
      return EXIT_FAILURE;
    }
  }

  // unsigned base 2 test
  const std::vector<unsigned> unsigned_base_2_test_expected { 0, 1, 4, 2 };
  const std::vector<std::string> unsigned_base_2_test { "0", "1", "100", "010" };

  for (std::size_t i = 0; i < unsigned_base_2_test.size(); ++i)
  {
    const auto& f = unsigned_base_2_test[i];
    unsigned result;
    auto answer = fast_float::from_chars(f.data(), f.data() + f.size(), result, 2);
    if (answer.ec != std::errc()) {
      std::cerr << "could not convert to unsigned for input: " << std::quoted(f) << std::endl;
      return EXIT_FAILURE;
    }
    else if (result != unsigned_base_2_test_expected[i]) {
      std::cerr << "result "  << std::quoted(f) << " did not match with expected unsigned: " << unsigned_base_2_test_expected[i] << std::endl;
      return EXIT_FAILURE;
    }
  }

  // int invalid error base 2 test
  const std::vector<std::string> int_invalid_argument_base_2_test{ "2", "A", "-2" };

  for (std::size_t i = 0; i < int_invalid_argument_base_2_test.size(); ++i)
  {
    const auto& f = int_invalid_argument_base_2_test[i];
    int result;
    auto answer = fast_float::from_chars(f.data(), f.data() + f.size(), result, 2);
    if (answer.ec != std::errc::invalid_argument) {
      std::cerr << "expected error should be 'invalid_argument' for: " << std::quoted(f) << std::endl;
      return EXIT_FAILURE;
    }
  }

  // unsigned invalid error base 2 test
  const std::vector<std::string> unsigned_invalid_argument_base_2_test{ "2", "A", "-1", "-2" };

  for (std::size_t i = 0; i < unsigned_invalid_argument_base_2_test.size(); ++i)
  {
    const auto& f = unsigned_invalid_argument_base_2_test[i];
    unsigned result;
    auto answer = fast_float::from_chars(f.data(), f.data() + f.size(), result, 2);
    if (answer.ec != std::errc::invalid_argument) {
      std::cerr << "expected error should be 'invalid_argument' for: " << std::quoted(f) << std::endl;
      return EXIT_FAILURE;
    }
  }

  // octal test
  const std::vector<int> base_octal_test_expected {0, 1, 7, 8, 9};
  const std::vector<std::string> base_octal_test { "0", "1", "07", "010", "0011" };

  for (std::size_t i = 0; i < base_octal_test.size(); ++i)
  {
    const auto& f = base_octal_test[i];
    int result;
    auto answer = fast_float::from_chars(f.data(), f.data() + f.size(), result, 8);
    if (answer.ec != std::errc()) {
      std::cerr << "could not convert to int for input: " << std::quoted(f) << std::endl;
      return EXIT_FAILURE;
    }
    else if (result != base_octal_test_expected[i]) {
      std::cerr << "result "  << std::quoted(f) << " did not match with expected int: " << base_octal_test_expected[i] << std::endl;
      return EXIT_FAILURE;
    }
  }

  // hex test
  const std::vector<int> base_hex_test_expected { 0, 1, 15, 16, 0, 16};
  const std::vector<std::string> base_hex_test { "0", "1", "F", "01f", "0x11", "10X11" };

  for (std::size_t i = 0; i < base_hex_test.size(); ++i)
  {
    const auto& f = base_hex_test[i];
    int result;
    auto answer = fast_float::from_chars(f.data(), f.data() + f.size(), result, 16);
    if (answer.ec != std::errc()) {
      std::cerr << "could not convert to int for input: " << std::quoted(f) << std::endl;
      return EXIT_FAILURE;
    }
    else if (result != base_hex_test_expected[i]) {
      std::cerr << "result "  << std::quoted(f) << " did not match with expected int: " << base_hex_test_expected[i] << std::endl;
      return EXIT_FAILURE;
    }
  }

  // invalid base test (-1) 
  const std::vector<std::string> invalid_base_test { "0", "1", "-1", "F", "10Z" };

  for (std::size_t i = 0; i < invalid_base_test.size(); ++i)
  {
    const auto& f = invalid_base_test[i];
    int result;
    auto answer = fast_float::from_chars(f.data(), f.data() + f.size(), result, -1);
    if (answer.ec != std::errc::invalid_argument) {
      std::cerr << "expected error should be 'invalid_argument' for: " << std::quoted(f) << std::endl;
      return EXIT_FAILURE;
    }
  }

  // out of range base test (100)
  const std::vector<int> base_out_of_range_test_expected { 0, 1, 15, 35, 10035 };
  const std::vector<std::string> base_out_of_range_test { "0", "1", "F", "Z", "10Z" };

  for (std::size_t i = 0; i < base_out_of_range_test.size(); ++i)
  {
    const auto& f = base_out_of_range_test[i];
    int result;
    auto answer = fast_float::from_chars(f.data(), f.data() + f.size(), result, 100);
    if (answer.ec != std::errc()) {
      std::cerr << "could not convert to int for input: " << std::quoted(f) << std::endl;
      return EXIT_FAILURE;
    }
    else if (result != base_out_of_range_test_expected[i]) {
      std::cerr << "result "  << std::quoted(f) << " did not match with expected int: " << base_out_of_range_test_expected[i] << std::endl;
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}