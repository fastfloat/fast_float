#if defined(__linux__) || (__APPLE__ && __aarch64__)
#define USING_COUNTERS
#endif
#include "counters/event_counter.h"
#include <algorithm>
#include <array>
#include "fast_float/fast_float.h"
#include <chrono>
#include <climits>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <ctype.h>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <random>
#include <sstream>
#include <stdio.h>
#include <string>
#include <vector>
#include <locale.h>

template <typename CharT>
double findmax_fastfloat64(std::vector<std::basic_string<CharT>> &s) {
  double answer = 0;
  double x = 0;
  for (auto &st : s) {
    auto [p, ec] = fast_float::from_chars(st.data(), st.data() + st.size(), x);
    if (p == st.data()) {
      throw std::runtime_error("bug in findmax_fastfloat");
    }
    answer = answer > x ? answer : x;
  }
  return answer;
}

template <typename CharT>
double findmax_fastfloat32(std::vector<std::basic_string<CharT>> &s) {
  float answer = 0;
  float x = 0;
  for (auto &st : s) {
    auto [p, ec] = fast_float::from_chars(st.data(), st.data() + st.size(), x);
    if (p == st.data()) {
      throw std::runtime_error("bug in findmax_fastfloat");
    }
    answer = answer > x ? answer : x;
  }
  return answer;
}

counters::event_collector collector{};

#ifdef USING_COUNTERS
template <class T, class CharT>
std::vector<counters::event_count>
time_it_ns(std::vector<std::basic_string<CharT>> &lines, T const &function,
           size_t repeat) {
  std::vector<counters::event_count> aggregate;
  bool printed_bug = false;
  for (size_t i = 0; i < repeat; i++) {
    collector.start();
    double ts = function(lines);
    if (ts == 0 && !printed_bug) {
      printf("bug\n");
      printed_bug = true;
    }
    aggregate.push_back(collector.end());
  }
  return aggregate;
}

void pretty_print(double volume, size_t number_of_floats, std::string name,
                  std::vector<counters::event_count> events) {
  double volumeMB = volume / (1024. * 1024.);
  double average_ns{0};
  double min_ns{DBL_MAX};
  double cycles_min{DBL_MAX};
  double instructions_min{DBL_MAX};
  double cycles_avg{0};
  double instructions_avg{0};
  double branches_min{0};
  double branches_avg{0};
  double branch_misses_min{0};
  double branch_misses_avg{0};
  for (counters::event_count e : events) {
    double ns = e.elapsed_ns();
    average_ns += ns;
    min_ns = min_ns < ns ? min_ns : ns;

    double cycles = e.cycles();
    cycles_avg += cycles;
    cycles_min = cycles_min < cycles ? cycles_min : cycles;

    double instructions = e.instructions();
    instructions_avg += instructions;
    instructions_min =
        instructions_min < instructions ? instructions_min : instructions;

    double branches = e.branches();
    branches_avg += branches;
    branches_min = branches_min < branches ? branches_min : branches;

    double branch_misses = e.branch_misses();
    branch_misses_avg += branch_misses;
    branch_misses_min =
        branch_misses_min < branch_misses ? branch_misses_min : branch_misses;
  }
  cycles_avg /= events.size();
  instructions_avg /= events.size();
  average_ns /= events.size();
  branches_avg /= events.size();
  branch_misses_avg /= events.size();
  printf("%-40s: %8.2f MB/s (+/- %.1f %%) ", name.data(),
         volumeMB * 1000000000 / min_ns,
         (average_ns - min_ns) * 100.0 / average_ns);
  printf("%8.2f Mfloat/s  ", number_of_floats * 1000 / min_ns);
  if (instructions_min > 0) {
    printf(" %8.2f i/B %8.2f i/f (+/- %.1f %%) ", instructions_min / volume,
           instructions_min / number_of_floats,
           (instructions_avg - instructions_min) * 100.0 / instructions_avg);

    printf(" %8.2f c/B %8.2f c/f (+/- %.1f %%) ", cycles_min / volume,
           cycles_min / number_of_floats,
           (cycles_avg - cycles_min) * 100.0 / cycles_avg);
    printf(" %8.2f i/c ", instructions_min / cycles_min);
    printf(" %8.2f b/f ", branches_avg / number_of_floats);
    printf(" %8.2f bm/f ", branch_misses_avg / number_of_floats);
    printf(" %8.2f GHz ", cycles_min / min_ns);
  }
  printf("\n");
}
#else
template <class T, class CharT>
std::pair<double, double>
time_it_ns(std::vector<std::basic_string<CharT>> &lines, T const &function,
           size_t repeat) {
  std::chrono::high_resolution_clock::time_point t1, t2;
  double average = 0;
  double min_value = DBL_MAX;
  bool printed_bug = false;
  for (size_t i = 0; i < repeat; i++) {
    t1 = std::chrono::high_resolution_clock::now();
    double ts = function(lines);
    if (ts == 0 && !printed_bug) {
      printf("bug\n");
      printed_bug = true;
    }
    t2 = std::chrono::high_resolution_clock::now();
    double dif =
        std::chrono::duration_cast<std::chrono::nanoseconds>(t2 - t1).count();
    average += dif;
    min_value = min_value < dif ? min_value : dif;
  }
  average /= repeat;
  return std::make_pair(min_value, average);
}

void pretty_print(double volume, size_t number_of_floats, std::string name,
                  std::pair<double, double> result) {
  double volumeMB = volume / (1024. * 1024.);
  printf("%-40s: %8.2f MB/s (+/- %.1f %%) ", name.data(),
         volumeMB * 1000000000 / result.first,
         (result.second - result.first) * 100.0 / result.second);
  printf("%8.2f Mfloat/s  ", number_of_floats * 1000 / result.first);
  printf(" %8.2f ns/f \n", double(result.first) / number_of_floats);
}
#endif

// this is okay, all chars are ASCII
inline std::u16string widen(std::string line) {
  std::u16string u16line;
  u16line.resize(line.size());
  for (size_t i = 0; i < line.size(); ++i) {
    u16line[i] = char16_t(line[i]);
  }
  return u16line;
}

std::vector<std::u16string> widen(const std::vector<std::string> &lines) {
  std::vector<std::u16string> u16lines;
  u16lines.reserve(lines.size());
  for (auto const &line : lines) {
    u16lines.push_back(widen(line));
  }
  return u16lines;
}

void process(std::vector<std::string> &lines, size_t volume) {
  size_t repeat = 1000;
  double volumeMB = volume / (1024. * 1024.);
  std::cout << "ASCII volume = " << volumeMB << " MB " << std::endl;
  pretty_print(volume, lines.size(), "fastfloat (64)",
               time_it_ns(lines, findmax_fastfloat64<char>, repeat));
  pretty_print(volume, lines.size(), "fastfloat (32)",
               time_it_ns(lines, findmax_fastfloat32<char>, repeat));

  std::vector<std::u16string> lines16 = widen(lines);
  volume = 2 * volume;
  volumeMB = volume / (1024. * 1024.);
  std::cout << "UTF-16 volume = " << volumeMB << " MB " << std::endl;
  pretty_print(volume, lines.size(), "fastfloat (64)",
               time_it_ns(lines16, findmax_fastfloat64<char16_t>, repeat));
  pretty_print(volume, lines.size(), "fastfloat (32)",
               time_it_ns(lines16, findmax_fastfloat32<char16_t>, repeat));
}

void fileload(std::string filename) {
  std::ifstream inputfile(filename);
  if (!inputfile) {
    std::cerr << "can't open " << filename << std::endl;
    return;
  }
  std::cout << "#### " << std::endl;
  std::cout << "# reading " << filename << std::endl;
  std::cout << "#### " << std::endl;
  std::string line;
  std::vector<std::string> lines;
  lines.reserve(10000); // let us reserve plenty of memory.
  size_t volume = 0;
  while (getline(inputfile, line)) {
    volume += line.size();
    lines.push_back(line);
  }
  std::cout << "# read " << lines.size() << " lines " << std::endl;
  process(lines, volume);
}

namespace {

constexpr size_t truncated_fraction_integer_digits =
    fast_float::binary_format<double>::max_digits() + 1;
constexpr size_t truncated_fraction_max_digits =
    fast_float::binary_format<double>::max_digits();
constexpr size_t truncated_fraction_digits = 4 * 1024 * 1024;
constexpr size_t truncated_fraction_batches = 9;
constexpr size_t truncated_fraction_zero_batches = 17;
constexpr size_t truncated_fraction_direct_iterations = 2048;
constexpr size_t truncated_fraction_direct_zero_iterations = 8192;
constexpr size_t truncated_fraction_from_chars_iterations = 256;
constexpr size_t truncated_fraction_from_chars_zero_iterations = 512;
constexpr double truncated_fraction_expected_value = 0x0.607b00a417628p-1022;

#if defined(_MSC_VER)
#define FASTFLOAT_BENCH_NOINLINE __declspec(noinline)
#elif defined(__GNUC__) || defined(__clang__)
#define FASTFLOAT_BENCH_NOINLINE __attribute__((noinline))
#else
#define FASTFLOAT_BENCH_NOINLINE
#endif

struct truncated_fraction_input {
  std::string text{};
  fast_float::parsed_number_string parsed{};
};

[[noreturn]] void truncated_fraction_fail(char const *message) {
  std::fputs(message, stderr);
  std::fputc('\n', stderr);
  std::exit(EXIT_FAILURE);
}

void truncated_fraction_usage() {
  std::fputs("usage: realbenchmark --truncated-fraction "
             "{parse_mantissa|from_chars} {nonzero|zero}\n",
             stderr);
}

std::string make_truncated_fraction_input(char final_integer_digit) {
  std::string result = "8385788696668661046";
  result.append(truncated_fraction_integer_digits - result.size() - 1, '0');
  result.push_back(final_integer_digit);
  result.push_back('.');
  result.append(truncated_fraction_digits, '0');
  result += "e-1078";
  return result;
}

void initialize_truncated_fraction_input(truncated_fraction_input &input,
                                         char final_integer_digit) {
  input.text = make_truncated_fraction_input(final_integer_digit);
  fast_float::parse_options options;
  input.parsed = fast_float::parse_number_string<false>(
      input.text.data(), input.text.data() + input.text.size(), options, true);
  if (!input.parsed.valid || !input.parsed.too_many_digits ||
      input.parsed.integer.len() != truncated_fraction_integer_digits ||
      input.parsed.fraction.len() != truncated_fraction_digits) {
    truncated_fraction_fail("unexpected parsed input");
  }

  double parsed_value = 0;
  auto const parsed = fast_float::from_chars(
      input.text.data(), input.text.data() + input.text.size(), parsed_value);
  if (parsed.ec != std::errc() ||
      parsed.ptr != input.text.data() + input.text.size() ||
      parsed_value != truncated_fraction_expected_value) {
    truncated_fraction_fail("unexpected conversion result");
  }
}

FASTFLOAT_BENCH_NOINLINE uint64_t
parse_mantissa_once(truncated_fraction_input const &input) {
  fast_float::parsed_number_string number = input.parsed;
  fast_float::bigint result;
  size_t digits = 0;
  fast_float::parse_mantissa(result, number, truncated_fraction_max_digits,
                             digits);
  bool truncated = false;
  return result.hi64(truncated) ^ (uint64_t(digits) << 1) ^ uint64_t(truncated);
}

FASTFLOAT_BENCH_NOINLINE uint64_t
from_chars_once(truncated_fraction_input const &input) {
  double result = 0;
  auto const parsed = fast_float::from_chars(
      input.text.data(), input.text.data() + input.text.size(), result);
  if (parsed.ec != std::errc() ||
      parsed.ptr != input.text.data() + input.text.size()) {
    truncated_fraction_fail("unexpected conversion result");
  }

  uint64_t bits = 0;
  static_assert(sizeof(bits) == sizeof(result), "unexpected double size");
  std::memcpy(&bits, &result, sizeof(bits));
  return bits;
}

inline void do_not_optimize(uint64_t value) {
#if defined(__GNUC__) || defined(__clang__)
  asm volatile("" : : "r"(value) : "memory");
#else
  volatile uint64_t sink = value;
  (void)sink;
#endif
}

template <typename Function>
double measure_truncated_fraction(
    std::array<truncated_fraction_input, 2> const &inputs, size_t iterations,
    size_t batches, Function function) {
  std::array<double, truncated_fraction_zero_batches> samples{};
  for (size_t batch = 0; batch < batches; ++batch) {
    uint64_t sink = 0;
    auto const start = std::chrono::steady_clock::now();
    for (size_t index = 0; index < iterations; ++index) {
      sink += function(inputs[index & 1]);
    }
    auto const finish = std::chrono::steady_clock::now();
    do_not_optimize(sink);
    samples[batch] =
        double(
            std::chrono::duration_cast<std::chrono::nanoseconds>(finish - start)
                .count()) /
        double(iterations);
  }
  std::sort(samples.begin(), samples.begin() + batches);
  return samples[batches / 2];
}

void print_truncated_fraction_measurement(double nanoseconds_per_operation) {
  std::printf("{\"metric\":\"ns/op\",\"value\":%.17g}\n",
              nanoseconds_per_operation);
}

int run_truncated_fraction_benchmark(char const *operation,
                                     char const *integer_suffix) {
  bool direct = false;
  if (std::strcmp(operation, "parse_mantissa") == 0) {
    direct = true;
  } else if (std::strcmp(operation, "from_chars") != 0) {
    truncated_fraction_usage();
    return EXIT_FAILURE;
  }

  bool zero_suffix = false;
  if (std::strcmp(integer_suffix, "zero") == 0) {
    zero_suffix = true;
  } else if (std::strcmp(integer_suffix, "nonzero") != 0) {
    truncated_fraction_usage();
    return EXIT_FAILURE;
  }

  std::array<truncated_fraction_input, 2> inputs{};
  initialize_truncated_fraction_input(inputs[0], zero_suffix ? '0' : '1');
  initialize_truncated_fraction_input(inputs[1], zero_suffix ? '0' : '2');

  // The zero-suffix route retains the long fractional scan. Use larger batches
  // and more independent samples there so that its unchanged path has a precise
  // check.
  size_t const iterations =
      zero_suffix ? (direct ? truncated_fraction_direct_zero_iterations
                            : truncated_fraction_from_chars_zero_iterations)
                  : (direct ? truncated_fraction_direct_iterations
                            : truncated_fraction_from_chars_iterations);
  size_t const batches = zero_suffix ? truncated_fraction_zero_batches
                                     : truncated_fraction_batches;

  if (direct) {
    print_truncated_fraction_measurement(measure_truncated_fraction(
        inputs, iterations, batches, [](truncated_fraction_input const &input) {
          return parse_mantissa_once(input);
        }));
  } else {
    print_truncated_fraction_measurement(measure_truncated_fraction(
        inputs, iterations, batches, [](truncated_fraction_input const &input) {
          return from_chars_once(input);
        }));
  }
  return EXIT_SUCCESS;
}

#undef FASTFLOAT_BENCH_NOINLINE

} // namespace

int main(int argc, char **argv) {
  if (argc > 1 && std::strcmp(argv[1], "--truncated-fraction") == 0) {
    if (argc != 4) {
      truncated_fraction_usage();
      return EXIT_FAILURE;
    }
    return run_truncated_fraction_benchmark(argv[2], argv[3]);
  }

  if (collector.has_events()) {
    std::cout << "# Using hardware counters" << std::endl;
  } else {
#if defined(__linux__) || (__APPLE__ && __aarch64__)
    std::cout << "# Hardware counters not available, try to run in privileged "
                 "mode (e.g., sudo)."
              << std::endl;
#endif
  }
  if (argc > 1) {
    fileload(argv[1]);
    return EXIT_SUCCESS;
  }
  fileload(std::string(BENCHMARK_DATA_DIR) + "/canada.txt");
  fileload(std::string(BENCHMARK_DATA_DIR) + "/mesh.txt");
  return EXIT_SUCCESS;
}
