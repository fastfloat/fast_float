// Benchmark for hexadecimal floating-point parsing (chars_format::hex).
//
// We turn decimal numbers (from a file, or random doubles) into hexadecimal
// strings with printf("%a"), and we parse them back with fast_float,
// std::from_chars (when the standard library supports it) and std::strtod.
//
// Usage: bench_hex [file]   (without a file: canada.txt, mesh.txt, random)
#if defined(__linux__) || (__APPLE__ && __aarch64__)
#define USING_COUNTERS
#endif
#include "counters/event_counter.h"
#include "fast_float/fast_float.h"

#include <cfloat>
#include <cmath>
#include <charconv>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

#if defined(__cpp_lib_to_chars) && __cpp_lib_to_chars >= 201611L
#define HAVE_STD_FROM_CHARS_FLOAT 1
#endif

counters::event_collector collector{};

template <typename F>
void bench(std::string const &name, std::vector<std::string> const &lines,
           F const &function, size_t repeat = 200) {
  size_t volume = 0;
  for (auto const &l : lines) {
    volume += l.size();
  }
  std::vector<counters::event_count> events;
  for (size_t i = 0; i < repeat; i++) {
    collector.start();
    double ts = function(lines);
    events.push_back(collector.end());
    if (ts == 0) {
      std::printf("bug\n");
    }
  }
  double min_ns = DBL_MAX, cycles = DBL_MAX, instructions = DBL_MAX;
  for (auto const &e : events) {
    min_ns = std::min(min_ns, double(e.elapsed_ns()));
    cycles = std::min(cycles, double(e.cycles()));
    instructions = std::min(instructions, double(e.instructions()));
  }
  double const n = double(lines.size());
  std::printf("%-28s: %8.2f MB/s %8.2f Mfloat/s %7.2f ns/f", name.c_str(),
              double(volume) * 1000 / min_ns / 1.048576, n * 1000 / min_ns,
              min_ns / n);
  if (collector.has_events()) {
    std::printf(" %8.2f i/f %8.2f c/f %6.2f i/c", instructions / n, cycles / n,
                instructions / cycles);
  }
  std::printf("\n");
}

template <typename T>
double findmax_fastfloat(std::vector<std::string> const &lines) {
  T answer = 0;
  T x = 0;
  for (auto const &st : lines) {
    auto r = fast_float::from_chars(st.data(), st.data() + st.size(), x,
                                    fast_float::chars_format::hex);
    if (r.ptr != st.data() + st.size()) {
      throw std::runtime_error("bug in findmax_fastfloat");
    }
    answer = answer > x ? answer : x;
  }
  return double(answer);
}

#ifdef HAVE_STD_FROM_CHARS_FLOAT
template <typename T>
double findmax_from_chars(std::vector<std::string> const &lines) {
  T answer = 0;
  T x = 0;
  for (auto const &st : lines) {
    auto r = std::from_chars(st.data(), st.data() + st.size(), x,
                             std::chars_format::hex);
    if (r.ptr != st.data() + st.size()) {
      throw std::runtime_error("bug in findmax_from_chars");
    }
    answer = answer > x ? answer : x;
  }
  return double(answer);
}
#endif

// strtod and strtof need the "0x" prefix: they get their own strings.
double findmax_strtod(std::vector<std::string> const &lines) {
  double answer = 0;
  for (auto const &st : lines) {
    char *end;
    double x = std::strtod(st.c_str(), &end);
    if (end != st.data() + st.size()) {
      throw std::runtime_error("bug in findmax_strtod");
    }
    answer = answer > x ? answer : x;
  }
  return answer;
}

double findmax_strtof(std::vector<std::string> const &lines) {
  float answer = 0;
  for (auto const &st : lines) {
    char *end;
    float x = std::strtof(st.c_str(), &end);
    if (end != st.data() + st.size()) {
      throw std::runtime_error("bug in findmax_strtof");
    }
    answer = answer > x ? answer : x;
  }
  return double(answer);
}

// x must be finite: printf("%a") writes inf and nan without the "0x".
std::string to_hex(double x) {
  char buffer[64];
  std::snprintf(buffer, sizeof(buffer), "%a", x);
  std::string s = buffer;
  size_t const at = s.find('x');
  return s.substr(0, at - 1) + s.substr(at + 1); // drop the "0x"
}

void process(std::vector<double> const &values) {
  std::vector<std::string> hex, prefixed;
  size_t volume = 0;
  for (double v : values) {
    if (!std::isfinite(v)) {
      continue;
    }
    std::string s = to_hex(v);
    volume += s.size();
    hex.push_back(s);
    std::string p = s;
    p.insert(p[0] == '-' ? 1 : 0, "0x");
    prefixed.push_back(p);
  }
  std::printf("# %zu numbers, %.1f bytes/number, e.g., %s\n", hex.size(),
              double(volume) / double(hex.size()), hex[hex.size() / 2].c_str());
  bench("fastfloat (64)", hex, findmax_fastfloat<double>);
#ifdef HAVE_STD_FROM_CHARS_FLOAT
  bench("std::from_chars (64)", hex, findmax_from_chars<double>);
#endif
  bench("strtod (64)", prefixed, findmax_strtod);
  bench("fastfloat (32)", hex, findmax_fastfloat<float>);
#ifdef HAVE_STD_FROM_CHARS_FLOAT
  bench("std::from_chars (32)", hex, findmax_from_chars<float>);
#endif
  bench("strtof (32)", prefixed, findmax_strtof);
}

void fileload(std::string const &filename) {
  std::ifstream inputfile(filename);
  if (!inputfile) {
    std::cerr << "can't open " << filename << std::endl;
    return;
  }
  std::printf("# reading %s\n", filename.c_str());
  std::vector<double> values;
  std::string line;
  while (std::getline(inputfile, line)) {
    double x;
    auto r = fast_float::from_chars(line.data(), line.data() + line.size(), x);
    if (r.ec == std::errc()) {
      values.push_back(x);
    }
  }
  process(values);
}

void random_values(size_t n) {
  std::printf("# random doubles in [0,1)\n");
  std::mt19937_64 rng(42);
  std::uniform_real_distribution<double> dis(0, 1);
  std::vector<double> values;
  for (size_t i = 0; i < n; i++) {
    values.push_back(dis(rng));
  }
  process(values);
}

int main(int argc, char **argv) {
  if (collector.has_events()) {
    std::printf("# Using hardware counters\n");
  }
#ifndef HAVE_STD_FROM_CHARS_FLOAT
  std::printf("# std::from_chars for floats is not available\n");
#endif
  if (argc > 1) {
    fileload(argv[1]);
    return EXIT_SUCCESS;
  }
  fileload(std::string(BENCHMARK_DATA_DIR) + "/canada.txt");
  fileload(std::string(BENCHMARK_DATA_DIR) + "/mesh.txt");
  random_values(100000);
  return EXIT_SUCCESS;
}
