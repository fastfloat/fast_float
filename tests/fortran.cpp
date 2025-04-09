/*
 * Exercise the Fortran conversion option.
 */
#include <cstdlib>
#include <iostream>
#include <vector>
#include "fast_float/fast_float.h"

int main_readme() {
  std::string const input = "1d+4";
  double result;
  auto answer = fast_float::from_chars_advanced(
      input.data(), input.data() + input.size(), result,
      fast_float::parse_options {
      fast_float::chars_format::fortran |
      fast_float::chars_format::allow_leading_plus});
  if ((answer.ec != std::errc()) || ((result != 10000))) {
    std::cerr << "parsing failure\n" << result << "\n";
    return EXIT_FAILURE;
  }
  std::cout << "parsed the number " << result << std::endl;
  return EXIT_SUCCESS;
}

int main() {
  std::vector<double> const expected{10000, 1000, 100,  10,   1,
                                     .1,    .01,  .001, .0001};
  std::vector<std::string> const fmt1{"1+4", "1+3", "1+2", "1+1", "1+0",
                                      "1-1", "1-2", "1-3", "1-4"};
  std::vector<std::string> const fmt2{"1d+4", "1d+3", "1d+2", "1d+1", "1d+0",
                                      "1d-1", "1d-2", "1d-3", "1d-4"};
  std::vector<std::string> const fmt3{"+1+4", "+1+3", "+1+2", "+1+1", "+1+0",
                                      "+1-1", "+1-2", "+1-3", "+1-4"};

  for (auto const &f : fmt1) {
    auto d{std::distance(&fmt1[0], &f)};
    double result;
    auto answer{fast_float::from_chars_advanced(f.data(), f.data() + f.size(),
                                                result,
      fast_float::parse_options {
      fast_float::chars_format::fortran |
      fast_float::chars_format::allow_leading_plus})};
    if (answer.ec != std::errc() || result != expected[std::size_t(d)]) {
      std::cerr << "parsing failure on " << f << std::endl;
      return EXIT_FAILURE;
    }
  }

  for (auto const &f : fmt2) {
    auto d{std::distance(&fmt2[0], &f)};
    double result;
    auto answer{fast_float::from_chars_advanced(f.data(), f.data() + f.size(),
                                                result,
      fast_float::parse_options {
      fast_float::chars_format::fortran |
      fast_float::chars_format::allow_leading_plus})};
    if (answer.ec != std::errc() || result != expected[std::size_t(d)]) {
      std::cerr << "parsing failure on " << f << std::endl;
      return EXIT_FAILURE;
    }
  }

  for (auto const &f : fmt3) {
    auto d{std::distance(&fmt3[0], &f)};
    double result;
    auto answer{fast_float::from_chars_advanced(f.data(), f.data() + f.size(),
                                                result,
      fast_float::parse_options {
      fast_float::chars_format::fortran |
      fast_float::chars_format::allow_leading_plus})};
    if (answer.ec != std::errc() || result != expected[std::size_t(d)]) {
      std::cerr << "parsing failure on " << f << std::endl;
      return EXIT_FAILURE;
    }
  }
  if (main_readme() != EXIT_SUCCESS) {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
