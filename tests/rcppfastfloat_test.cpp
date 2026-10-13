/**
 * See https://github.com/eddelbuettel/rcppfastfloat/issues/4
 */

#include "fast_float/fast_float.h"
#include <iostream>
#include <string>
#include <vector>

struct test_data {
  const std::string input;
  const bool expected_success;
  const double expected_result;
};

bool eddelbuettel() {
  std::vector<test_data> const test_datas = {
      {"infinity", true, std::numeric_limits<double>::infinity()},
      {" \r\n\t\f\v3.16227766016838 \r\n\t\f\v", true, 3.16227766016838},
      {" \r\n\t\f\v3 \r\n\t\f\v", true, 3.0},
      {"  1970-01-01", false, 0.0},
      {"-NaN", true, std::numeric_limits<double>::quiet_NaN()},
      {"-inf", true, -std::numeric_limits<double>::infinity()},
      {" \r\n\t\f\v2.82842712474619 \r\n\t\f\v", true, 2.82842712474619},
      {"nan", true, std::numeric_limits<double>::quiet_NaN()},
      {" \r\n\t\f\v2.44948974278318 \r\n\t\f\v", true, 2.44948974278318},
      {"Inf", true, std::numeric_limits<double>::infinity()},
      {" \r\n\t\f\v2 \r\n\t\f\v", true, 2.0},
      {"-infinity", true, -std::numeric_limits<double>::infinity()},
      {" \r\n\t\f\v0 \r\n\t\f\v", true, 0.0},
      {" \r\n\t\f\v1.73205080756888 \r\n\t\f\v", true, 1.73205080756888},
      {" \r\n\t\f\v1 \r\n\t\f\v", true, 1.0},
      {" \r\n\t\f\v1.4142135623731 \r\n\t\f\v", true, 1.4142135623731},
      {" \r\n\t\f\v2.23606797749979 \r\n\t\f\v", true, 2.23606797749979},
      {"1970-01-02  ", false, 0.0},
      {" \r\n\t\f\v2.64575131106459 \r\n\t\f\v", true, 2.64575131106459},
      {"inf", true, std::numeric_limits<double>::infinity()},
      {"-nan", true, std::numeric_limits<double>::quiet_NaN()},
      {"NaN", true, std::numeric_limits<double>::quiet_NaN()},
      {"", false, 0.0},
      {"-Inf", true, -std::numeric_limits<double>::infinity()},
      {"+2.2", true, 2.2},
      {"1d+4", false, 0.0},
      {"1d-1", false, 0.0},
      {"0.", true, 0.0},
      {"-.1", true, -0.1},
      {"+.1", true, 0.1},
      {"1e+1", true, 10.0},
      {"+1e1", true, 10.0},
      {"-+0", false, 0.0},
      {"-+inf", false, 0.0},
      {"-+nan", false, 0.0},
  };
  for (const auto &i : test_datas) {
    auto const &input = i.input;
    auto const expected_success = i.expected_success;
    auto const expected_result = i.expected_result;
    double result;
    // answer contains a error code and a pointer to the end of the
    // parsed region (on success).
    auto const answer = fast_float::from_chars(
        input.data(), input.data() + input.size(), result,
        fast_float::chars_format::general |
            fast_float::chars_format::allow_leading_plus |
            fast_float::chars_format::skip_white_space);
    if (answer.ec != std::errc()) {
      std::cout << "could not parse" << std::endl;
      if (expected_success) {
        return false;
      }
      continue;
    }
    bool non_space_trailing_content = false;
    if (answer.ptr != input.data() + input.size()) {
      // check that there is no content left
      for (char const *leftover = answer.ptr;
           leftover != input.data() + input.size(); leftover++) {
        if (!fast_float::is_space(*leftover)) {
          non_space_trailing_content = true;
          break;
        }
      }
    }
    if (non_space_trailing_content) {
      std::cout << "found trailing content " << std::endl;
      if (!expected_success) {
        continue;
      } else {
        return false;
      }
    }
    std::cout << "parsed " << result << std::endl;
    if (!expected_success) {
      return false;
    }
    if (result != expected_result &&
        !(std::isnan(result) && std::isnan(expected_result))) {
      std::cout << "results do not match. Expected " << expected_result
                << std::endl;
      return false;
    }
  }
  return true;
}

int main() {
  if (!eddelbuettel()) {
    printf("Bug.\n");
    return EXIT_FAILURE;
  }
  printf("All ok.\n");
  return EXIT_SUCCESS;
}
