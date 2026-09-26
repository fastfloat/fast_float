#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"

#include "fast_float/fast_float.h"

#include <cstdint>
#include <cstring>
#include <string>
#include <system_error>
#include <vector>

namespace {

struct parse_result {
  uint64_t value_bits;
  size_t parsed_length;
  std::errc error;
};

std::string nonzero_tail(size_t length) {
  std::string result(length, '1');
  result.front() = '7';
  result.back() = '7';
  return result;
}

uint64_t bits(double value) {
  uint64_t result;
  ::memcpy(&result, &value, sizeof(result));
  return result;
}

parse_result parse(std::string const &input) {
  double value = 0;
  fast_float::from_chars_result const result = fast_float::from_chars(
      input.data(), input.data() + input.size(), value);
  return parse_result{bits(value), size_t(result.ptr - input.data()), result.ec};
}

fast_float::parsed_number_string parse_number(std::string const &input) {
  fast_float::parse_options options;
  return fast_float::parse_number_string<false, char>(
      input.data(), input.data() + input.size(), options, true);
}

bool takes_digit_comp(std::string const &input) {
  fast_float::parse_options options;
  fast_float::parsed_number_string const parsed =
      fast_float::parse_number_string<false, char>(
          input.data(), input.data() + input.size(), options, true);
  fast_float::adjusted_mantissa am =
      fast_float::compute_float<fast_float::binary_format<double>>(
          parsed.exponent, parsed.mantissa);
  if (parsed.too_many_digits && am.power2 >= 0 &&
      am != fast_float::compute_float<fast_float::binary_format<double>>(
                parsed.exponent, parsed.mantissa + 1)) {
    am = fast_float::compute_error<fast_float::binary_format<double>>(
        parsed.exponent, parsed.mantissa);
  }
  return parsed.valid && am.power2 < 0;
}

void check_equivalent(std::string const &padded, std::string const &canonical) {
  std::string const padded_with_marker = padded + "x";
  std::string const canonical_with_marker = canonical + "x";
  REQUIRE(takes_digit_comp(padded_with_marker));

  parse_result const padded_result = parse(padded_with_marker);
  parse_result const canonical_result = parse(canonical_with_marker);
  CHECK(padded_result.parsed_length == padded.size());
  CHECK(canonical_result.parsed_length == canonical.size());
  CHECK(padded_result.error == canonical_result.error);
  CHECK(padded_result.value_bits == canonical_result.value_bits);
}

template <typename UC> std::basic_string<UC> widen(std::string const &input) {
  return std::basic_string<UC>(input.begin(), input.end());
}

template <typename UC> parse_result parse_wide(std::string const &input) {
  std::basic_string<UC> const wide = widen<UC>(input);
  double value = 0;
  fast_float::from_chars_result_t<UC> const result = fast_float::from_chars(
      wide.data(), wide.data() + wide.size(), value);
  return parse_result{bits(value), size_t(result.ptr - wide.data()), result.ec};
}

template <typename UC>
void check_equivalent_wide(std::string const &padded,
                           std::string const &canonical) {
  std::string const padded_with_marker = padded + "x";
  std::string const canonical_with_marker = canonical + "x";
  parse_result const padded_result = parse_wide<UC>(padded_with_marker);
  parse_result const canonical_result = parse_wide<UC>(canonical_with_marker);
  CHECK(padded_result.parsed_length == padded.size());
  CHECK(canonical_result.parsed_length == canonical.size());
  CHECK(padded_result.error == canonical_result.error);
  CHECK(padded_result.value_bits == canonical_result.value_bits);
}

} // namespace

TEST_CASE("long trailing zero coefficients preserve public from_chars results") {
  // The prefix is an ambiguous 19-digit mantissa at exponent -18. Adding a
  // twentieth digit makes public from_chars use the digit comparison fallback.
  std::string const prefix = "6497987825129815764";
  std::vector<size_t> const zero_counts = {
      0, 1, 8, 15, 16, 17, 32, 63, 64, 65, 700, 769, 1000, 4096};
  for (size_t core_length :
       {size_t(20), size_t(30), size_t(120), size_t(720), size_t(769)}) {
    std::string const tail = nonzero_tail(core_length - prefix.size());
    for (size_t zero_count : zero_counts) {
      std::string const zeroes(zero_count, '0');
      std::string const integer_exponent =
          std::to_string(-18 - int(tail.size()) - int(zero_count));

      // Integer suffixes need a compensating explicit exponent. Fractional
      // suffixes do not, because the decimal point already fixes their scale.
      check_equivalent(prefix + tail + zeroes + "e" + integer_exponent,
                       prefix + tail + "e" +
                           std::to_string(-18 - int(tail.size())));
      check_equivalent("-" + prefix + tail + zeroes + "e" + integer_exponent,
                       "-" + prefix + tail + "e" +
                           std::to_string(-18 - int(tail.size())));
      check_equivalent("0." + prefix + tail + zeroes + "e1",
                       "0." + prefix + tail + "e1");
      check_equivalent(prefix.substr(0, 1) + "." + prefix.substr(1) + tail +
                           zeroes + "e0",
                       prefix.substr(0, 1) + "." + prefix.substr(1) + tail +
                           "e0");
    }
  }
}

TEST_CASE("trailing zero fallback accepts inputs without explicit exponents") {
  // This prefix is ambiguous at exponent -19, which is the corrected exponent
  // for a fraction-only input without an explicit exponent.
  std::string const prefix = "2686910556586236953";
  for (size_t zero_count :
       {size_t(0), size_t(15), size_t(16), size_t(63), size_t(64),
        size_t(65), size_t(700), size_t(4096)}) {
    std::string const zeroes(zero_count, '0');
    check_equivalent("0." + prefix + "7" + zeroes,
                     "0." + prefix + "7");
  }
}

TEST_CASE("trailing zero fallback supports wide character input") {
  std::string const prefix = "6497987825129815764";
  std::string const tail = nonzero_tail(101);
  std::string const zeroes(769, '0');
  std::string const padded = "0." + prefix + tail + zeroes + "e1";
  std::string const canonical = "0." + prefix + tail + "e1";
  check_equivalent_wide<char16_t>(padded, canonical);
  check_equivalent_wide<wchar_t>(padded, canonical);
  check_equivalent_wide<char32_t>(padded, canonical);
}

TEST_CASE("all-zero coefficients retain their public result") {
  for (size_t zero_count : {size_t(0), size_t(16), size_t(700), size_t(4096)}) {
    std::string const integer = "0" + std::string(zero_count, '0') + "e10x";
    std::string const fraction = "0." + std::string(zero_count, '0') + "e-10x";
    parse_result const integer_result = parse(integer);
    parse_result const fraction_result = parse(fraction);
    CHECK(integer_result.parsed_length + 1 == integer.size());
    CHECK(fraction_result.parsed_length + 1 == fraction.size());
    CHECK(integer_result.error == std::errc());
    CHECK(fraction_result.error == std::errc());
    CHECK(integer_result.value_bits == 0);
    CHECK(fraction_result.value_bits == 0);
  }
}

TEST_CASE("trailing zero normalization shortens only logical spans") {
  std::string const zeroes64(64, '0');

  std::string const fraction_input = "120.3" + zeroes64;
  fast_float::parsed_number_string fraction = parse_number(fraction_input);
  fast_float::parsed_number_string trimmed;
  REQUIRE(fast_float::has_minimum_trailing_zeroes(fraction));
  REQUIRE(fast_float::trim_trailing_zeros(fraction, trimmed));
  CHECK(fraction.integer.len() == 3);
  CHECK(fraction.fraction.len() == 65);
  CHECK(trimmed.integer.len() == 3);
  CHECK(trimmed.fraction.len() == 1);
  CHECK(std::string(trimmed.integer.ptr, trimmed.integer.len()) == "120");
  CHECK(std::string(trimmed.fraction.ptr, trimmed.fraction.len()) == "3");

  std::string const across_point_input = "120." + zeroes64;
  fast_float::parsed_number_string across_point =
      parse_number(across_point_input);
  REQUIRE(fast_float::has_minimum_trailing_zeroes(across_point));
  REQUIRE(fast_float::trim_trailing_zeros(across_point, trimmed));
  CHECK(trimmed.integer.len() == 2);
  CHECK(trimmed.fraction.len() == 0);
  CHECK(std::string(trimmed.integer.ptr, trimmed.integer.len()) == "12");

  std::string const integer_input = "123" + zeroes64;
  fast_float::parsed_number_string integer_only = parse_number(integer_input);
  REQUIRE(fast_float::has_minimum_trailing_zeroes(integer_only));
  REQUIRE(fast_float::trim_trailing_zeros(integer_only, trimmed));
  CHECK(trimmed.integer.len() == 3);
  CHECK(trimmed.fraction.len() == 0);
  CHECK(std::string(trimmed.integer.ptr, trimmed.integer.len()) == "123");

  std::string const short_suffix_input = "120.3" + std::string(15, '0');
  fast_float::parsed_number_string short_suffix =
      parse_number(short_suffix_input);
  CHECK_FALSE(fast_float::has_minimum_trailing_zeroes(short_suffix));

  std::string const cutoff_input = "120.3" + std::string(16, '0');
  fast_float::parsed_number_string cutoff = parse_number(cutoff_input);
  REQUIRE(fast_float::has_minimum_trailing_zeroes(cutoff));
  REQUIRE(fast_float::trim_trailing_zeros(cutoff, trimmed));
  CHECK(trimmed.integer.len() == 3);
  CHECK(trimmed.fraction.len() == 1);

  std::string const cross_span_input =
      "123" + std::string(15, '0') + ".0";
  fast_float::parsed_number_string cross_span = parse_number(cross_span_input);
  REQUIRE(fast_float::has_minimum_trailing_zeroes(cross_span));
  REQUIRE(fast_float::trim_trailing_zeros(cross_span, trimmed));
  CHECK(trimmed.integer.len() == 3);
  CHECK(trimmed.fraction.len() == 0);
  CHECK(std::string(trimmed.integer.ptr, trimmed.integer.len()) == "123");

  std::string const all_zero_input = "0." + zeroes64;
  fast_float::parsed_number_string all_zero = parse_number(all_zero_input);
  REQUIRE(fast_float::has_minimum_trailing_zeroes(all_zero));
  CHECK_FALSE(fast_float::trim_trailing_zeros(all_zero, trimmed));
}
