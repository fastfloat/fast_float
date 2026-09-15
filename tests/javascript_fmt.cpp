/*
 * Exercise the JavaScript (ECMAScript DecimalLiteral) conversion option.
 * https://tc39.es/ecma262/#prod-DecimalLiteral
 */
#include <cstdlib>
#include <iostream>
#include <vector>
#include "fast_float/fast_float.h"

int main_readme() {
  std::string const input = "01"; // not valid: leading zero
  double result;
  fast_float::parse_options options{fast_float::chars_format::javascript};
  auto answer = fast_float::from_chars_advanced(
      input.data(), input.data() + input.size(), result, options);
  if (answer.ec == std::errc()) {
    std::cerr << "should have failed\n";
    return EXIT_FAILURE;
  }
  return EXIT_SUCCESS;
}

int main_readme2() {
  std::string const input = ".5"; // valid in JavaScript, not in JSON
  double result;
  fast_float::parse_options options{fast_float::chars_format::javascript};
  auto answer = fast_float::from_chars_advanced(
      input.data(), input.data() + input.size(), result, options);
  if (answer.ec != std::errc() || result != 0.5) {
    std::cerr << "should have parsed 0.5\n";
    return EXIT_FAILURE;
  }
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
  std::vector<AcceptedValue> const accept{
      {"0", {0., ""}},
      {"-0", {-0., ""}},
      {"0.5", {0.5, ""}},
      {"0.0", {0., ""}},
      {"0e5", {0., ""}},
      {"0e-2", {0., ""}},
      {"0.5e-1", {0.05, ""}},
      {"-0.2", {-0.2, ""}},
      {"0.02", {0.02, ""}},
      {"1e+0000", {1., ""}},
      {"123", {123., ""}},
      {"123.456", {123.456, ""}},
      {"123.456e7", {1234560000., ""}},
      {"1E5", {100000., ""}},
      // Unlike JSON, the integer part may be empty...
      {".5", {0.5, ""}},
      {"-.5", {-0.5, ""}},
      {".5e1", {5., ""}},
      {".5E-1", {0.05, ""}},
      // ... and so may the fractional part.
      {"5.", {5., ""}},
      {"-5.", {-5., ""}},
      {"5.e3", {5000., ""}},
      {"0.", {0., ""}},
      {"0.e1", {0., ""}},
      // An incomplete exponent is trailing junk, as in the other formats.
      {"1e", {1., "e"}},
      {"1e+", {1., "e+"}},
      {"5.e", {5., "e"}},
      // Trailing junk.
      {"0x1", {0., "x1"}},
      {"1n", {1., "n"}},
      {"1_000", {1., "_000"}},
  };
  std::vector<RejectedValue> const reject{
      {"00", {fast_float::parse_error::leading_zeros_in_integer_part, 0}},
      {"01", {fast_float::parse_error::leading_zeros_in_integer_part, 0}},
      {"007", {fast_float::parse_error::leading_zeros_in_integer_part, 0}},
      {"00.5", {fast_float::parse_error::leading_zeros_in_integer_part, 0}},
      {"00.0e-1", {fast_float::parse_error::leading_zeros_in_integer_part, 0}},
      {"0775e-1", {fast_float::parse_error::leading_zeros_in_integer_part, 0}},
      {"-01", {fast_float::parse_error::leading_zeros_in_integer_part, 1}},
      {".", {fast_float::parse_error::no_digits_in_mantissa, 1}},
      {".e5", {fast_float::parse_error::no_digits_in_mantissa, 1}},
      {"-.", {fast_float::parse_error::no_digits_in_mantissa, 2}},
      {"-", {fast_float::parse_error::missing_integer_or_dot_after_sign, 1}},
      {"-e5", {fast_float::parse_error::missing_integer_or_dot_after_sign, 1}},
      {"e5", {fast_float::parse_error::no_digits_in_mantissa, 0}},
      // A leading plus sign needs allow_leading_plus.
      {"+1", {fast_float::parse_error::no_digits_in_mantissa, 0}},
      // No inf/nan.
      {"inf", {fast_float::parse_error::no_digits_in_mantissa, 0}},
      {"Infinity", {fast_float::parse_error::no_digits_in_mantissa, 0}},
      {"nan", {fast_float::parse_error::no_digits_in_mantissa, 0}},
  };

  for (std::size_t i = 0; i < accept.size(); ++i) {
    auto const &s = accept[i].input;
    auto const &expected = accept[i].expected;
    double result;
    auto answer = fast_float::from_chars(s.data(), s.data() + s.size(), result,
                                         fast_float::chars_format::javascript);
    if (answer.ec != std::errc()) {
      std::cerr << "javascript fmt rejected valid javascript " << s
                << std::endl;
      return EXIT_FAILURE;
    }
    if (result != expected.value) {
      std::cerr << "javascript fmt gave wrong result " << s << " (expected "
                << expected.value << " got " << result << ")" << std::endl;
      return EXIT_FAILURE;
    }
    if (std::string(answer.ptr) != expected.junk_chars) {
      std::cerr << "javascript fmt has wrong trailing characters " << s
                << " (expected " << expected.junk_chars << " got " << answer.ptr
                << ")" << std::endl;
      return EXIT_FAILURE;
    }
  }

  for (std::size_t i = 0; i < reject.size(); ++i) {
    auto const &s = reject[i].input;
    double result;
    auto answer = fast_float::from_chars(s.data(), s.data() + s.size(), result,
                                         fast_float::chars_format::javascript);
    if (answer.ec == std::errc()) {
      std::cerr << "javascript fmt accepted invalid javascript " << s
                << std::endl;
      return EXIT_FAILURE;
    }
  }

  for (std::size_t i = 0; i < reject.size(); ++i) {
    auto const &f = reject[i].input;
    auto const &expected_reason = reject[i].reason;
    auto answer = fast_float::parse_number_string<false>(
        f.data(), f.data() + f.size(),
        fast_float::parse_options(fast_float::chars_format::javascript));
    if (answer.valid) {
      std::cerr << "javascript parse accepted invalid javascript " << f
                << std::endl;
      return EXIT_FAILURE;
    }
    if (answer.error != expected_reason.error) {
      std::cerr << "javascript parse failure had invalid error reason " << f
                << std::endl;
      return EXIT_FAILURE;
    }
    intptr_t error_location = answer.lastmatch - f.data();
    if (error_location != expected_reason.location_offset) {
      std::cerr << "javascript parse failure had invalid error location " << f
                << " (expected " << expected_reason.location_offset << " got "
                << error_location << ")" << std::endl;
      return EXIT_FAILURE;
    }
  }

  // With allow_leading_plus, a leading plus sign is accepted (unlike JSON,
  // where it is ignored).
  {
    std::vector<std::string> const plus{"+1", "+.5", "+5.", "+0.5e1"};
    std::vector<double> const expected{1., 0.5, 5., 5.};
    fast_float::parse_options const options{
        fast_float::chars_format::javascript |
        fast_float::chars_format::allow_leading_plus};
    for (std::size_t i = 0; i < plus.size(); ++i) {
      auto const &s = plus[i];
      double result;
      auto answer = fast_float::from_chars_advanced(
          s.data(), s.data() + s.size(), result, options);
      if (answer.ec != std::errc() || result != expected[i]) {
        std::cerr << "javascript fmt with allow_leading_plus failed on " << s
                  << std::endl;
        return EXIT_FAILURE;
      }
    }
    std::string const s = "+01";
    double result;
    auto answer = fast_float::from_chars_advanced(s.data(), s.data() + s.size(),
                                                  result, options);
    if (answer.ec == std::errc()) {
      std::cerr << "javascript fmt with allow_leading_plus accepted " << s
                << std::endl;
      return EXIT_FAILURE;
    }
  }

  if (main_readme() != EXIT_SUCCESS) {
    return EXIT_FAILURE;
  }
  if (main_readme2() != EXIT_SUCCESS) {
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
