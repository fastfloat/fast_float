
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

struct ExpectedResult {
  double value;
  std::string junk_chars;
};
struct AcceptedValue {
  std::string input;
  ExpectedResult expected;
};

struct RejectReason {
  fast_float::parse_error error;
  intptr_t location_offset;
};
struct RejectedValue {
  std::string input;
  RejectReason reason;
};

int main() {
  const std::vector<AcceptedValue> accept{
      {"-0.2", {-0.2, ""}},
      {"0.02", {0.02, ""}},
      {"0.002", {0.002, ""}},
      {"1e+0000", {1., ""}},
      {"0e-2", {0., ""}},
      {"1e", {1., "e"}},
      {"1e+", {1., "e+"}},
      {"inf", {std::numeric_limits<double>::infinity(), ""}}};
  const std::vector<RejectedValue> reject{
      {"-.2", {fast_float::parse_error::missing_integer_after_sign, 1}},
      {"00.02", {fast_float::parse_error::leading_zeros_in_integer_part, 0}},
      {"0.e+1", {fast_float::parse_error::no_digits_in_fractional_part, 2}},
      {"00.e+1", {fast_float::parse_error::leading_zeros_in_integer_part, 0}},
      {".25", {fast_float::parse_error::no_digits_in_integer_part, 0}},
      // The following cases already start as invalid JSON, so they are
      // handled as trailing junk and the error is for not having digits in the
      // empty string before the invalid token.
      {"+0.25", {fast_float::parse_error::no_digits_in_integer_part, 0}},
      {"inf", {fast_float::parse_error::no_digits_in_integer_part, 0}},
      {"nan(snan)", {fast_float::parse_error::no_digits_in_integer_part, 0}}};

  for (std::size_t i = 0; i < accept.size(); ++i)
  {
    const auto& s = accept[i].input;
    const auto& expected = accept[i].expected;
    double result;
    auto answer = fast_float::from_chars(s.data(), s.data() + s.size(), result, fast_float::chars_format::json_or_infnan);
    if (answer.ec != std::errc()) {
      std::cerr << "json fmt rejected valid json " << s << std::endl;
      return EXIT_FAILURE;
    }
    if (result != expected.value) {
      std::cerr << "json fmt gave wrong result " << s << " (expected " << expected.value << " got " << result << ")" << std::endl;
      return EXIT_FAILURE;
    }
    if (std::string(answer.ptr) != expected.junk_chars) {
      std::cerr << "json fmt has wrong trailing characters " << s << " (expected " << expected.junk_chars << " got " << answer.ptr << ")" << std::endl;
      return EXIT_FAILURE;
    }
  }

  for (std::size_t i = 0; i < reject.size(); ++i)
  {
    const auto& s = reject[i].input;
    double result;
    auto answer = fast_float::from_chars(s.data(), s.data() + s.size(), result, fast_float::chars_format::json);
    if (answer.ec == std::errc()) {
      std::cerr << "json fmt accepted invalid json " << s << std::endl;
      return EXIT_FAILURE;
    }
  }

  for (std::size_t i = 0; i < reject.size(); ++i)
  {
    const auto& f = reject[i].input;
    const auto& expected_reason = reject[i].reason;
    auto answer = fast_float::parse_number_string(
        f.data(), f.data() + f.size(),
        fast_float::parse_options(fast_float::chars_format::json));
    if (answer.valid) {
      std::cerr << "json parse accepted invalid json " << f << std::endl;
      return EXIT_FAILURE;
    }
    if (answer.error != expected_reason.error) {
      std::cerr << "json parse failure had invalid error reason " << f
                << std::endl;
      return EXIT_FAILURE;
    }
    intptr_t error_location = answer.lastmatch - f.data();
    if (error_location != expected_reason.location_offset) {
      std::cerr << "json parse failure had invalid error location " << f
                << " (expected " << expected_reason.location_offset << " got "
                << error_location << ")" << std::endl;
      return EXIT_FAILURE;
    }
  }

  if(main_readme() != EXIT_SUCCESS) { return EXIT_FAILURE; }
  if(main_readme2() != EXIT_SUCCESS) { return EXIT_FAILURE; }

  return EXIT_SUCCESS;
}