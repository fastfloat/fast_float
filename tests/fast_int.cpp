#include <cstdlib>
#include <iostream>
#include <vector>
#include <iomanip>
#include <cstring>
#include "fast_float/fast_float.h"
#include <cstdint>

/*
all tests conducted are to check fast_float::from_chars functionality with int and unsigned
test cases include:
int basic tests - numbers only, numbers with strings behind, decimals, negative numbers
unsigned basic tests - numbers only, numbers with strings behind, decimals
int invalid tests - strings only, strings with numbers behind, space in front of number, plus sign in front of number
unsigned invalid tests - strings only, strings with numbers behind, space in front of number, plus/minus sign in front of number
int out of range tests - numbers exceeding int bit size for 8, 16, 32, and 64 bits
unsigned out of range tests - numbers exceeding unsigned bit size 8, 16, 32, and 64 bits
int pointer tests - points to first character that is not recognized as int
unsigned pointer tests - points to first character that is not recognized as unsigned
int/unsigned base 2 tests - numbers are converted from binary to decimal
octal tests - numbers are converted from octal to decimal
hex tests - numbers are converted from hex to decimal (Note: 0x and 0X are considered invalid)
invalid base tests - any base not within 2-36 is invalid
out of range base tests - numbers exceeding int/unsigned bit size after converted from base (Note: only 64 bit int and unsigned are tested)
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

  // int out of range error test #1 (8 bit)
  const std::vector<std::string> int_out_of_range_test_1{ "2000000000000000000000", "128", "-129"};

  for (std::size_t i = 0; i < int_out_of_range_test_1.size(); ++i)
  {
    const auto& f = int_out_of_range_test_1[i];
    int8_t result;
    auto answer = fast_float::from_chars(f.data(), f.data() + f.size(), result);
    if (answer.ec != std::errc::result_out_of_range) {
      std::cerr << "expected error for should be 'result_out_of_range': " << std::quoted(f) << std::endl;
      return EXIT_FAILURE;
    }
  }

  // int out of range error test #2 (16 bit)
  const std::vector<std::string> int_out_of_range_test_2{ "2000000000000000000000", "32768", "-32769"};

  for (std::size_t i = 0; i < int_out_of_range_test_2.size(); ++i)
  {
    const auto& f = int_out_of_range_test_2[i];
    int16_t result;
    auto answer = fast_float::from_chars(f.data(), f.data() + f.size(), result);
    if (answer.ec != std::errc::result_out_of_range) {
      std::cerr << "expected error for should be 'result_out_of_range': " << std::quoted(f) << std::endl;
      return EXIT_FAILURE;
    }
  }

  // int out of range error test #3 (32 bit)
  const std::vector<std::string> int_out_of_range_test_3{ "2000000000000000000000", "2147483648", "-2147483649"};

  for (std::size_t i = 0; i < int_out_of_range_test_3.size(); ++i)
  {
    const auto& f = int_out_of_range_test_3[i];
    int32_t result;
    auto answer = fast_float::from_chars(f.data(), f.data() + f.size(), result);
    if (answer.ec != std::errc::result_out_of_range) {
      std::cerr << "expected error for should be 'result_out_of_range': " << std::quoted(f) << std::endl;
      return EXIT_FAILURE;
    }
  }

  // int out of range error test #4 (64 bit)
  const std::vector<std::string> int_out_of_range_test_4{ "2000000000000000000000", "9223372036854775808", "-9223372036854775809"};

  for (std::size_t i = 0; i < int_out_of_range_test_4.size(); ++i)
  {
    const auto& f = int_out_of_range_test_4[i];
    int64_t result;
    auto answer = fast_float::from_chars(f.data(), f.data() + f.size(), result);
    if (answer.ec != std::errc::result_out_of_range) {
      std::cerr << "expected error for should be 'result_out_of_range': " << std::quoted(f) << std::endl;
      return EXIT_FAILURE;
    }
  }

  // unsigned out of range error test #1 (8 bit)
  const std::vector<std::string> unsigned_out_of_range_test_1{ "2000000000000000000000", "255" };

  for (std::size_t i = 0; i < unsigned_out_of_range_test_1.size(); ++i)
  {
    const auto& f = unsigned_out_of_range_test_1[i];
    uint8_t result;
    auto answer = fast_float::from_chars(f.data(), f.data() + f.size(), result);
    if (answer.ec != std::errc::result_out_of_range) {
      std::cerr << "expected error for should be 'result_out_of_range': " << std::quoted(f) << std::endl;
      return EXIT_FAILURE;
    }
  }

  // unsigned out of range error test #2 (16 bit)
  const std::vector<std::string> unsigned_out_of_range_test_2{ "2000000000000000000000", "65536" };

  for (std::size_t i = 0; i < unsigned_out_of_range_test_2.size(); ++i)
  {
    const auto& f = unsigned_out_of_range_test_2[i];
    uint16_t result;
    auto answer = fast_float::from_chars(f.data(), f.data() + f.size(), result);
    if (answer.ec != std::errc::result_out_of_range) {
      std::cerr << "expected error for should be 'result_out_of_range': " << std::quoted(f) << std::endl;
      return EXIT_FAILURE;
    }
  }

  // unsigned out of range error test #3 (32 bit)
  const std::vector<std::string> unsigned_out_of_range_test_3{ "2000000000000000000000", "4294967296" };

  for (std::size_t i = 0; i < unsigned_out_of_range_test_3.size(); ++i)
  {
    const auto& f = unsigned_out_of_range_test_3[i];
    uint32_t result;
    auto answer = fast_float::from_chars(f.data(), f.data() + f.size(), result);
    if (answer.ec != std::errc::result_out_of_range) {
      std::cerr << "expected error for should be 'result_out_of_range': " << std::quoted(f) << std::endl;
      return EXIT_FAILURE;
    }
  }

  // unsigned out of range error test #4 (64 bit)
  const std::vector<std::string> unsigned_out_of_range_test_4{ "2000000000000000000000", "18446744073709551616" };

  for (std::size_t i = 0; i < unsigned_out_of_range_test_4.size(); ++i)
  {
    const auto& f = unsigned_out_of_range_test_4[i];
    uint64_t result;
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
  const std::string int_pointer_test_2 = "1001 with text";

  const auto& f2 = int_pointer_test_2;
  int result2;
  auto answer2 = fast_float::from_chars(f2.data(), f2.data() + f2.size(), result2);
  if (strcmp(answer2.ptr, " with text") != 0) {
    std::cerr << "ptr of result "  << std::quoted(f2) << " did not match with expected ptr: " << std::quoted(" with text") << std::endl;
    return EXIT_FAILURE;
  }

  // int pointer test #3 (string with newline behind numbers)
  const std::string int_pointer_test_3 = "1001 with text\n";

  const auto& f3 = int_pointer_test_3;
  int result3;
  auto answer3 = fast_float::from_chars(f3.data(), f3.data() + f3.size(), result3);
  if (strcmp(answer3.ptr, " with text\n") != 0) {
    std::cerr << "ptr of result "  << std::quoted(f3) << " did not match with expected ptr: " << std::quoted(" with text") << std::endl;
    return EXIT_FAILURE;
  }

  // int pointer test #4 (float)
  const std::string int_pointer_test_4 = "9.999";

  const auto& f4 = int_pointer_test_4;
  int result4;
  auto answer4 = fast_float::from_chars(f4.data(), f4.data() + f4.size(), result4);
  if (strcmp(answer4.ptr, ".999") != 0) {
    std::cerr << "ptr of result "  << std::quoted(f4) << " did not match with expected ptr: " << std::quoted(".999") << std::endl;
    return EXIT_FAILURE;
  }

  // int pointer test #5 (invalid int)
  const std::string int_pointer_test_5 = "+50";

  const auto& f5 = int_pointer_test_5;
  int result5;
  auto answer5 = fast_float::from_chars(f5.data(), f5.data() + f5.size(), result5);
  if (strcmp(answer5.ptr, "+50") != 0) {
    std::cerr << "ptr of result "  << std::quoted(f5) << " did not match with expected ptr: " << std::quoted("+50") << std::endl;
    return EXIT_FAILURE;
  }

  // unsigned pointer test #2 (string behind numbers)
  const std::string unsigned_pointer_test_1 = "1001 with text";

  const auto& f6 = unsigned_pointer_test_1;
  unsigned result6;
  auto answer6 = fast_float::from_chars(f6.data(), f6.data() + f6.size(), result6);
  if (strcmp(answer6.ptr, " with text") != 0) {
    std::cerr << "ptr of result "  << std::quoted(f6) << " did not match with expected ptr: " << std::quoted(" with text") << std::endl;
    return EXIT_FAILURE;
  }

  // unsigned pointer test #2 (invalid unsigned)
  const std::string unsigned_pointer_test_2 = "-50";

  const auto& f7 = unsigned_pointer_test_2;
  unsigned result7;
  auto answer7 = fast_float::from_chars(f7.data(), f7.data() + f7.size(), result7);
  if (strcmp(answer7.ptr, "-50") != 0) {
    std::cerr << "ptr of result "  << std::quoted(f7) << " did not match with expected ptr: " << std::quoted("-50") << std::endl;
    return EXIT_FAILURE;
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

  // invalid base test #1 (-1) 
  const std::vector<std::string> invalid_base_test_1 { "0", "1", "-1", "F", "10Z" };

  for (std::size_t i = 0; i < invalid_base_test_1.size(); ++i)
  {
    const auto& f = invalid_base_test_1[i];
    int result;
    auto answer = fast_float::from_chars(f.data(), f.data() + f.size(), result, -1);
    if (answer.ec != std::errc::invalid_argument) {
      std::cerr << "expected error should be 'invalid_argument' for: " << std::quoted(f) << std::endl;
      return EXIT_FAILURE;
    }
  }

  // invalid base test #2 (37)
  const std::vector<std::string> invalid_base_test_2 { "0", "1", "F", "Z", "10Z" };

  for (std::size_t i = 0; i < invalid_base_test_2.size(); ++i)
  {
    const auto& f = invalid_base_test_2[i];
    int result;
    auto answer = fast_float::from_chars(f.data(), f.data() + f.size(), result, 37);
    if (answer.ec != std::errc::invalid_argument) {
      std::cerr << "expected error should be 'invalid_argument' for: " << std::quoted(f) << std::endl;
      return EXIT_FAILURE;
    }
  }

  // int out of range error base test (64 bit)
  const std::vector<std::string> int_out_of_range_base_test { "1000000000000000000000000000000000000000000000000000000000000000",
                                                              "-1000000000000000000000000000000000000000000000000000000000000001",
                                                              "2021110011022210012102010021220101220222",
                                                              "-2021110011022210012102010021220101221000",
                                                              "20000000000000000000000000000000",
                                                              "-20000000000000000000000000000001",
                                                              "1104332401304422434310311213",
                                                              "-1104332401304422434310311214",
                                                              "1540241003031030222122212",
                                                              "-1540241003031030222122213"
                                                              "22341010611245052052301",
                                                              "-22341010611245052052302"
                                                              "1000000000000000000000",
                                                              "-1000000000000000000001",
                                                              "67404283172107811828",
                                                              "-67404283172107811830",
                                                              "9223372036854775808",
                                                              "-9223372036854775809",
                                                              "1728002635214590698",
                                                              "-1728002635214590699",
                                                              "41A792678515120368",
                                                              "-41A792678515120369",
                                                              "10B269549075433C38",
                                                              "-10B269549075433C39",
                                                              "4340724C6C71DC7A8",
                                                              "-4340724C6C71DC7A9",
                                                              "160E2AD3246366808",
                                                              "-160E2AD3246366809",
                                                              "8000000000000000",
                                                              "-8000000000000001",
                                                              "33D3D8307B214009",
                                                              "-33D3D8307B21400A",
                                                              "16AGH595DF825FA8",
                                                              "-16AGH595DF825FA9",
                                                              "BA643DCI0FFEEHI",
                                                              "-BA643DCI0FFEEI0"
                                                              "5CBFJIA3FH26JA8",
                                                              "-5CBFJIA3FH26JA9",
                                                              "2HEICIIIE82DH98",
                                                              "-2HEICIIIE82DH99",
                                                              "1ADAIBB21DCKFA8",
                                                              "-1ADAIBB21DCKFA9",
                                                              "I6K448CF4192C3",
                                                              "-I6K448CF4192C4",
                                                              "ACD772JNC9L0L8",
                                                              "-ACD772JNC9L0L9",
                                                              "64IE1FOCNN5G78",
                                                              "-64IE1FOCNN5G79",
                                                              "3IGOECJBMCA688",
                                                              "-3IGOECJBMCA689",
                                                              "27C48L5B37OAOQ",
                                                              "-27C48L5B37OAP0",
                                                              "1BK39F3AH3DMQ8",
                                                              "-1BK39F3AH3DMQ9",
                                                              "Q1SE8F0M04ISC",
                                                              "-Q1SE8F0M04ISD",
                                                              "HAJPPBC1FC208",
                                                              "-HAJPPBC1FC209",
                                                              "BM03I95HIA438",
                                                              "-BM03I95HIA439",
                                                              "8000000000000",
                                                              "-8000000000001"
                                                              "5HG4CK9JD4U38",
                                                              "-5HG4CK9JD4U39",
                                                              "3TDTK1V8J6TPQ",
                                                              "-3TDTK1V8J6TPR",
                                                              "2PIJMIKEXRXP8",
                                                              "-2PIJMIKEXRXP9",
                                                              "1Y2P0IJ32E8E8",
                                                              "-1Y2P0IJ32E8E9" };
  int base_int = 2;
  int counter = 0;
  for (std::size_t i = 0; i < int_out_of_range_base_test.size(); ++i)
  {
    const auto& f = int_out_of_range_base_test[i];
    int64_t result;
    auto answer = fast_float::from_chars(f.data(), f.data() + f.size(), result, base_int);
    if (answer.ec != std::errc::result_out_of_range) {
      std::cerr << "expected error for should be 'result_out_of_range': " << std::quoted(f) << std::endl;
      return EXIT_FAILURE;
    }
    if (!(counter)) {
      ++counter;    
    }
    else {
      ++base_int;
      ++counter;
    }
  }

  // unsigned out of range error base test (64 bit)
  const std::vector<std::string> unsigned_out_of_range_base_test { "10000000000000000000000000000000000000000000000000000000000000000",
                                                                   "11112220022122120101211020120210210211221",
                                                                   "100000000000000000000000000000000",
                                                                   "2214220303114400424121122431",
                                                                   "3520522010102100444244424",
                                                                   "45012021522523134134602",
                                                                   "2000000000000000000000",
                                                                   "145808576354216723757",
                                                                   "18446744073709551616",
                                                                   "335500516A429071285",
                                                                   "839365134A2A240714",
                                                                   "219505A9511A867B73",
                                                                   "8681049ADB03DB172",
                                                                   "2C1D56B648C6CD111",
                                                                   "10000000000000000",
                                                                   "67979G60F5428011",
                                                                   "2D3FGB0B9CG4BD2G",
                                                                   "141C8786H1CCAAGH",
                                                                   "B53BJH07BE4DJ0G",
                                                                   "5E8G4GGG7G56DIG",
                                                                   "2L4LF104353J8KG",
                                                                   "1DDH88H2782I516",
                                                                   "L12EE5FN0JI1IG",
                                                                   "C9C336O0MLB7EG",
                                                                   "7B7N2PCNIOKCGG",
                                                                   "4EO8HFAM6FLLMP",
                                                                   "2NC6J26L66RHOG",
                                                                   "1N3RSH11F098RO",
                                                                   "14L9LKMO30O40G",
                                                                   "ND075IB45K86G",
                                                                   "G000000000000",
                                                                   "B1W8P7J5Q9R6G",
                                                                   "7ORP63SH4DPHI",
                                                                   "5G24A25TWKWFG",
                                                                   "3W5E11264SGSG" };
  int base_unsigned = 2;
  for (std::size_t i = 0; i < unsigned_out_of_range_base_test.size(); ++i)
  {
    const auto& f = unsigned_out_of_range_base_test[i];
    uint64_t result;
    auto answer = fast_float::from_chars(f.data(), f.data() + f.size(), result, base_unsigned);
    if (answer.ec != std::errc::result_out_of_range) {
      std::cerr << "expected error for should be 'result_out_of_range': " << std::quoted(f) << std::endl;
      return EXIT_FAILURE;
    }
    ++base_unsigned;
  }

  return EXIT_SUCCESS;
}