
#include <cstdlib>
#include <iostream>
#include <vector>

// test that this option is ignored
#define FASTFLOAT_ALLOWS_LEADING_PLUS

#include "fast_float/fast_float.h"

int main_readme() {
    const std::string input =  "+.1"; // not valid
    double result;
    fast_float::parse_options options{ fast_float::chars_format::json };
    auto answer = fast_float::from_chars_advanced(input.data(), input.data()+input.size(), result, options);
    if(answer.ec == std::errc()) { std::cerr << "should have failed\n"; return EXIT_FAILURE; }
    return EXIT_SUCCESS;
}


int main_readme2() {
    const std::string input =  "inf"; // not valid in JSON
    double result;
    fast_float::parse_options options{ fast_float::chars_format::json };
    auto answer = fast_float::from_chars_advanced(input.data(), input.data()+input.size(), result, options);
    if(answer.ec == std::errc()) { std::cerr << "should have failed\n"; return EXIT_FAILURE; }
    return EXIT_SUCCESS;
}

int main_readme3() {
    const std::string input =  "inf"; // not valid in JSON but we allow it with json_or_infnan
    double result;
    fast_float::parse_options options{ fast_float::chars_format::json_or_infnan };
    auto answer = fast_float::from_chars_advanced(input.data(), input.data()+input.size(), result, options);
    if(answer.ec != std::errc() || (!std::isinf(result))) { std::cerr << "should have parsed infinity\n"; return EXIT_FAILURE; }
    return EXIT_SUCCESS;
}

int main()
{
  const std::vector<double> expected{ -0.2, 0.02, 0.002, 1., 0., std::numeric_limits<double>::infinity() };
  const std::vector<std::string> accept{ "-0.2", "0.02", "0.002", "1e+0000", "0e-2", "inf" };
  const std::vector<std::string> reject{ "-.2", "00.02", "0.e+1", "00.e+1", ".25", "+0.25", "inf", "nan(snan)" };

  for (std::size_t i = 0; i < accept.size(); ++i)
  {
    const auto& f = accept[i];
    double result;
    auto answer = fast_float::from_chars(f.data(), f.data() + f.size(), result, fast_float::chars_format::json_or_infnan);
    if (answer.ec != std::errc() || result != expected[i]) {
      std::cerr << "json fmt rejected valid json " << f << std::endl;
      return EXIT_FAILURE;
    }
  }

  for (std::size_t i = 0; i < reject.size(); ++i)
  {
    const auto& f = reject[i];
    double result;
    auto answer = fast_float::from_chars(f.data(), f.data() + f.size(), result, fast_float::chars_format::json);
    if (answer.ec == std::errc()) {
      std::cerr << "json fmt accepted invalid json " << f << std::endl;
      return EXIT_FAILURE;
    }
  }

  if(main_readme() != EXIT_SUCCESS) { return EXIT_FAILURE; }
  if(main_readme2() != EXIT_SUCCESS) { return EXIT_FAILURE; }

  return EXIT_SUCCESS;
}