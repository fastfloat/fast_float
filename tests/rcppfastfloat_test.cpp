/**
 * See https://github.com/eddelbuettel/rcppfastfloat/issues/4
 */

#define FASTFLOAT_ALLOWS_LEADING_PLUS 1
#define FASTFLOAT_SKIP_WHITE_SPACE 1 // important !
#include "fast_float/fast_float.h"
#include <iostream>
#include <string>
#include <vector>

bool eddelbuettel() {
  std::vector<std::string> inputs = {"infinity",
                                     " \r\n\t\f\v3.16227766016838 \r\n\t\f\v",
                                     " \r\n\t\f\v3 \r\n\t\f\v",
                                     "  1970-01-01",
                                     "-NaN",
                                     "-inf",
                                     " \r\n\t\f\v2.82842712474619 \r\n\t\f\v",
                                     "nan",
                                     " \r\n\t\f\v2.44948974278318 \r\n\t\f\v",
                                     "Inf",
                                     " \r\n\t\f\v2 \r\n\t\f\v",
                                     "-infinity",
                                     " \r\n\t\f\v0 \r\n\t\f\v",
                                     " \r\n\t\f\v1.73205080756888 \r\n\t\f\v",
                                     " \r\n\t\f\v1 \r\n\t\f\v",
                                     " \r\n\t\f\v1.4142135623731 \r\n\t\f\v",
                                     " \r\n\t\f\v2.23606797749979 \r\n\t\f\v",
                                     "1970-01-02  ",
                                     " \r\n\t\f\v2.64575131106459 \r\n\t\f\v",
                                     "inf",
                                     "-nan",
                                     "NaN",
                                     "",
                                     "-Inf",
                                     "+2.2",
                                     "1d+4",
                                     "1d-1"};
  std::vector<std::pair<bool, double>> expected_results = {
      {true, std::numeric_limits<double>::infinity()},
      {true, 3.16227766016838},
      {true, 3},
      {false, -1},
      {true, std::numeric_limits<double>::quiet_NaN()},
      {true, -std::numeric_limits<double>::infinity()},
      {true, 2.82842712474619},
      {true, std::numeric_limits<double>::quiet_NaN()},
      {true, 2.44948974278318},
      {true, std::numeric_limits<double>::infinity()},
      {true, 2},
      {true, -std::numeric_limits<double>::infinity()},
      {true, 0},
      {true, 1.73205080756888},
      {true, 1},
      {true, 1.4142135623731},
      {true, 2.23606797749979},
      {false, -1},
      {true, 2.64575131106459},
      {true, std::numeric_limits<double>::infinity()},
      {true, std::numeric_limits<double>::quiet_NaN()},
      {true, std::numeric_limits<double>::quiet_NaN()},
      {false, -1},
      {true, -std::numeric_limits<double>::infinity()},
      {true, 2.2},
      {false, -1},
      {false, -1}};
  for (size_t i = 0; i < inputs.size(); i++) {
    std::string &input = inputs[i];
    std::pair<bool, double> expected = expected_results[i];
    double result;
    // answer contains a error code and a pointer to the end of the
    // parsed region (on success).
    auto answer = fast_float::from_chars(input.data(),
                                         input.data() + input.size(), result);
    if (answer.ec != std::errc()) {
      std::cout << "could not parse" << std::endl;
      if (expected.first) {
        return false;
      }
      continue;
    }
    bool non_space_trailing_content = false;
    if (answer.ptr != input.data() + input.size()) {
      // check that there is no content left
      for (const char *leftover = answer.ptr;
           leftover != input.data() + input.size(); leftover++) {
        if (!fast_float::is_space(uint8_t(*leftover))) {
          non_space_trailing_content = true;
          break;
        }
      }
    }
    if (non_space_trailing_content) {
      std::cout << "found trailing content " << std::endl;
    }

    if (non_space_trailing_content) {
      if (!expected.first) {
        continue;
      } else {
        return false;
      }
    }
    std::cout << "parsed " << result << std::endl;
    if (!expected.first) {
      return false;
    }
    if (result != expected.second) {
      if (std::isnan(result) && std::isnan(expected.second)) {
        continue;
      }
      std::cout << "results do not match. Expected "<<  expected.second << std::endl;
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
