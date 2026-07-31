#include "fast_float/fast_float.h"

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iomanip>
#include <iostream>
#include <string>
#include <system_error>
#include <vector>

namespace {

struct input_case {
  std::string input;
  std::string canonical;
};

struct target_case {
  fast_float::parsed_number_string number;
  fast_float::adjusted_mantissa am;

  target_case() : number(), am() {}
};

volatile uint64_t sink = 0;

std::string nonzero_tail(size_t length) {
  std::string result(length, '1');
  result.front() = '7';
  result.back() = '7';
  return result;
}

bool make_target_case(std::string const &input, target_case &target) {
  fast_float::parse_options options;
  fast_float::parsed_number_string const parsed =
      fast_float::parse_number_string<false, char>(
          input.data(), input.data() + input.size(), options, true);
  fast_float::adjusted_mantissa am =
      fast_float::compute_float<fast_float::binary_format<double>>(
          parsed.exponent, parsed.mantissa);
  if (!parsed.valid || !parsed.too_many_digits || am.power2 < 0 ||
      am == fast_float::compute_float<fast_float::binary_format<double>>(
                parsed.exponent, parsed.mantissa + 1)) {
    return false;
  }
  am = fast_float::compute_error<fast_float::binary_format<double>>(
      parsed.exponent, parsed.mantissa);
  if (am.power2 >= 0) {
    return false;
  }
  target.number = parsed;
  target.am = am;
  return true;
}

bool takes_digit_comp(std::string const &input) {
  target_case target;
  return make_target_case(input, target);
}

uint64_t bits(double value) {
  uint64_t result;
  ::memcpy(&result, &value, sizeof(result));
  return result;
}

void parse(std::string const &input, double &value, std::errc &error,
           size_t &parsed_length) {
  fast_float::from_chars_result const result = fast_float::from_chars(
      input.data(), input.data() + input.size(), value);
  error = result.ec;
  parsed_length = size_t(result.ptr - input.data());
}

bool verify_case(input_case const &test) {
  double input_value = 0;
  double canonical_value = 0;
  std::errc input_error;
  std::errc canonical_error;
  size_t input_length = 0;
  size_t canonical_length = 0;
  parse(test.input, input_value, input_error, input_length);
  parse(test.canonical, canonical_value, canonical_error, canonical_length);
  return takes_digit_comp(test.input) &&
         input_length + 1 == test.input.size() &&
         canonical_length + 1 == test.canonical.size() &&
         input_error == canonical_error &&
         bits(input_value) == bits(canonical_value);
}

std::vector<input_case> make_cases(std::vector<size_t> const &zero_counts) {
  // This prefix makes compute_float(m) and compute_float(m + 1) differ at
  // exponent -18, so public from_chars reaches digit_comp after parsing a
  // coefficient longer than 19 digits.
  std::string const prefix = "6497987825129815764";
  std::vector<input_case> result;
  for (size_t core_length : {size_t(20), size_t(30), size_t(120)}) {
    std::string const tail = nonzero_tail(core_length - prefix.size());
    for (size_t zero_count : zero_counts) {
      std::string const zeroes(zero_count, '0');
      std::string const integer_exponent =
          std::to_string(-18 - int(tail.size()) - int(zero_count));

      // Put a non-digit marker after each number so verification also checks
      // the public from_chars pointer result.
      result.push_back(input_case{prefix + tail + zeroes + "e" +
                                      integer_exponent + "x",
                                  prefix + tail + "e" +
                                      std::to_string(-18 - int(tail.size())) +
                                      "x"});
      result.push_back(input_case{"0." + prefix + tail + zeroes + "e1x",
                                  "0." + prefix + tail + "e1x"});
      result.push_back(input_case{prefix.substr(0, 1) + "." +
                                      prefix.substr(1) + tail + zeroes + "e0x",
                                  prefix.substr(0, 1) + "." +
                                      prefix.substr(1) + tail + "e0x"});
    }
  }
  return result;
}

bool verify(std::vector<input_case> const &cases) {
  for (input_case const &test : cases) {
    if (!verify_case(test)) {
      return false;
    }
  }
  return true;
}

bool make_target_cases(std::vector<input_case> const &cases,
                       std::vector<target_case> &targets) {
  targets.clear();
  targets.reserve(cases.size());
  for (input_case const &test : cases) {
    target_case target;
    if (!make_target_case(test.input, target)) {
      return false;
    }
    targets.push_back(target);
  }
  return true;
}

void parse_all(std::vector<input_case> const &cases, size_t iterations) {
  uint64_t local_sink = 0;
  for (size_t iteration = 0; iteration < iterations; ++iteration) {
    for (input_case const &test : cases) {
      double value = 0;
      std::errc error;
      size_t parsed_length = 0;
      parse(test.input, value, error, parsed_length);
      local_sink += bits(value) + uint64_t(parsed_length) + uint64_t(error);
    }
  }
  sink += local_sink;
}

double benchmark(std::vector<input_case> const &cases) {
  // Keep input construction and correctness validation out of the measured
  // parse operation, as callers normally own the input buffers already.
  parse_all(cases, 1);
  size_t const iterations = 2000;
  std::chrono::steady_clock::time_point const start =
      std::chrono::steady_clock::now();
  parse_all(cases, iterations);
  std::chrono::steady_clock::duration const elapsed =
      std::chrono::steady_clock::now() - start;
  double const operations = double(cases.size()) * double(iterations);
  return std::chrono::duration<double, std::nano>(elapsed).count() /
         operations;
}

void digit_comp_all(std::vector<target_case> &targets, size_t iterations) {
  uint64_t local_sink = 0;
  for (size_t iteration = 0; iteration < iterations; ++iteration) {
    for (target_case &target : targets) {
      fast_float::adjusted_mantissa const answer =
          fast_float::digit_comp<double>(target.number, target.am);
      local_sink += answer.mantissa + uint64_t(answer.power2);
    }
  }
  sink += local_sink;
}

double benchmark_digit_comp(std::vector<target_case> &targets) {
  digit_comp_all(targets, 1);
  size_t const iterations = 20000;
  std::chrono::steady_clock::time_point const start =
      std::chrono::steady_clock::now();
  digit_comp_all(targets, iterations);
  std::chrono::steady_clock::duration const elapsed =
      std::chrono::steady_clock::now() - start;
  double const operations = double(targets.size()) * double(iterations);
  return std::chrono::duration<double, std::nano>(elapsed).count() /
         operations;
}

} // namespace

int main(int argc, char **argv) {
  if (argc != 2) {
    std::cerr << "usage: trailing_zero_benchmark "
                 "--verify|--benchmark|--target-benchmark\n";
    return EXIT_FAILURE;
  }

  std::string const mode(argv[1]);
  if (mode == "--verify") {
    std::vector<input_case> const cases = make_cases(
        {size_t(0), size_t(1), size_t(8), size_t(15), size_t(16), size_t(17),
         size_t(64), size_t(700), size_t(769), size_t(1000), size_t(4096)});
    if (!verify(cases)) {
      std::cerr << "trailing-zero verification failed\n";
      return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
  }

  if (mode == "--benchmark") {
    std::vector<input_case> const cases = make_cases(
        {size_t(64), size_t(700), size_t(769), size_t(4096)});
    if (!verify(cases)) {
      std::cerr << "trailing-zero verification failed\n";
      return EXIT_FAILURE;
    }
    std::cout << "{\"metric\":\"ns_per_parse\",\"value\":"
              << std::fixed << std::setprecision(3) << benchmark(cases)
              << "}\n";
    return EXIT_SUCCESS;
  }

  if (mode == "--target-benchmark") {
    std::vector<input_case> const cases = make_cases(
        {size_t(64), size_t(700), size_t(769), size_t(4096)});
    std::vector<target_case> targets;
    if (!verify(cases) || !make_target_cases(cases, targets)) {
      std::cerr << "trailing-zero verification failed\n";
      return EXIT_FAILURE;
    }
    std::cout << "{\"metric\":\"ns_per_digit_comp\",\"value\":"
              << std::fixed << std::setprecision(3)
              << benchmark_digit_comp(targets) << "}\n";
    return EXIT_SUCCESS;
  }

  std::cerr << "unknown mode: " << mode << '\n';
  return EXIT_FAILURE;
}
