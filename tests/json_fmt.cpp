
#include <cstdlib>
#include <iostream>
#include <vector>

// test that this option is ignored
#define FASTFLOAT_ALLOWS_LEADING_PLUS

#include "fast_float/fast_float.h"

int main()
{
  const std::vector<double> expected{ -0.2, 0.02, 0.002, 1., 0. };
  const std::vector<std::string> accept{ "-0.2", "0.02", "0.002", "1e+0000", "0e-2" };
  const std::vector<std::string> reject{ "-.2", "00.02", "0.e+1", "00.e+1", ".25", "+0.25"};
  const auto fmt = fast_float::chars_format::json;

  for (std::size_t i = 0; i < accept.size(); ++i)
  {
    const auto& f = accept[i];
    double result;
    auto answer = fast_float::from_chars(f.data(), f.data() + f.size(), result, fmt);
    if (answer.ec != std::errc() || result != expected[i]) {
      std::cerr << "json fmt rejected valid json " << f << std::endl;
      return EXIT_FAILURE;
    }
  }

  for (std::size_t i = 0; i < reject.size(); ++i)
  {
    const auto& f = reject[i];
    double result;
    auto answer = fast_float::from_chars(f.data(), f.data() + f.size(), result, fmt);
    if (answer.ec == std::errc()) {
      std::cerr << "json fmt accepted invalid json " << f << std::endl;
      return EXIT_FAILURE;
    }
  }

  return EXIT_SUCCESS;
}