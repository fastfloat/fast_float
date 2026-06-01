#include "fast_float/fast_float.h"

#include <atomic>
#include <cassert>
#include <cmath>
#include <cstdio>
#include <ios>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <thread>
#include <vector>

#if defined(__CYGWIN__) || defined(__MINGW32__) || defined(__MINGW64__)
// Anything at all that is related to cygwin, msys and so forth will
// always use this fallback because we cannot rely on it behaving as normal
// gcc.
#include <locale>
#include <sstream>

// workaround for CYGWIN
double cygwin_strtod_l(char const *start, char **end) {
  double d;
  std::stringstream ss;
  ss.imbue(std::locale::classic());
  ss << start;
  ss >> d;
  if (ss.fail()) {
    *end = nullptr;
  }
  if (ss.eof()) {
    ss.clear();
  }
  auto nread = ss.tellg();
  *end = const_cast<char *>(start) + nread;
  return d;
}

float cygwin_strtof_l(char const *start, char **end) {
  float d;
  std::stringstream ss;
  ss.imbue(std::locale::classic());
  ss << start;
  ss >> d;
  if (ss.fail()) {
    *end = nullptr;
  }
  if (ss.eof()) {
    ss.clear();
  }
  auto nread = ss.tellg();
  *end = const_cast<char *>(start) + nread;
  return d;
}
#endif

template <typename T> char *to_string(T d, char *buffer) {
  auto written = std::snprintf(buffer, 64, "%.*e",
                               std::numeric_limits<T>::max_digits10 - 1, d);
  return buffer + written;
}

void strtof_from_string(char const *st, float &d) {
  char *pr = (char *)st;
#if defined(__CYGWIN__) || defined(__MINGW32__) || defined(__MINGW64__) ||     \
    defined(sun) || defined(__sun)
  d = cygwin_strtof_l(st, &pr);
#elif defined(_WIN32)
  static _locale_t c_locale = _create_locale(LC_ALL, "C");
  d = _strtof_l(st, &pr, c_locale);
#else
  static locale_t c_locale = newlocale(LC_ALL_MASK, "C", NULL);
  d = strtof_l(st, &pr, c_locale);
#endif
  if (pr == st) {
    throw std::runtime_error("bug in strtod_from_string");
  }
}

// Checks a single 32-bit word (interpreted as a float). Returns true if the
// parser agrees with the reference, false (after logging) on a mismatch.
bool check_word(uint32_t word) {
  char buffer[64];
  float v;
  memcpy(&v, &word, sizeof(v));
  if (!std::isfinite(v)) {
    return true;
  }
  float nextf = std::nextafterf(v, INFINITY);
  if (copysign(1, v) != copysign(1, nextf)) {
    return true;
  }
  if (!std::isfinite(nextf)) {
    return true;
  }
  double v1{v};
  assert(float(v1) == v);
  double v2{nextf};
  assert(float(v2) == nextf);
  double midv{v1 + (v2 - v1) / 2};
  float expected_midv = float(midv);

  char const *string_end = to_string(midv, buffer);
  float str_answer;
  strtof_from_string(buffer, str_answer);

  float result_value;
  auto result = fast_float::from_chars(buffer, string_end, result_value);
  // Starting with version 4.0 for fast_float, we return result_out_of_range
  // if the value is either too small (too close to zero) or too large
  // (effectively infinity). So std::errc::result_out_of_range is normal for
  // well-formed input strings.
  if (result.ec != std::errc() && result.ec != std::errc::result_out_of_range) {
    std::cerr << "parsing error ? " << buffer << std::endl;
    return false;
  }
  if (std::isnan(v)) {
    if (!std::isnan(result_value)) {
      std::cerr << "not nan" << buffer << std::endl;
      std::cerr << "v " << std::hexfloat << v << std::endl;
      std::cerr << "v2 " << std::hexfloat << v2 << std::endl;
      std::cerr << "midv " << std::hexfloat << midv << std::endl;
      std::cerr << "expected_midv " << std::hexfloat << expected_midv
                << std::endl;
      return false;
    }
  } else if (copysign(1, result_value) != copysign(1, v)) {
    std::cerr << buffer << std::endl;
    std::cerr << "v " << std::hexfloat << v << std::endl;
    std::cerr << "v2 " << std::hexfloat << v2 << std::endl;
    std::cerr << "midv " << std::hexfloat << midv << std::endl;
    std::cerr << "expected_midv " << std::hexfloat << expected_midv
              << std::endl;
    std::cerr << "I got " << std::hexfloat << result_value
              << " but I was expecting " << v << std::endl;
    return false;
  } else if (result_value != str_answer) {
    std::cerr << "no match ? " << buffer << std::endl;
    std::cerr << "v " << std::hexfloat << v << std::endl;
    std::cerr << "v2 " << std::hexfloat << v2 << std::endl;
    std::cerr << "midv " << std::hexfloat << midv << std::endl;
    std::cerr << "expected_midv " << std::hexfloat << expected_midv
              << std::endl;
    std::cout << "started with " << std::hexfloat << midv << std::endl;
    std::cout << "round down to " << std::hexfloat << str_answer << std::endl;
    std::cout << "got back " << std::hexfloat << result_value << std::endl;
    std::cout << std::dec;
    return false;
  }
  return true;
}

// Sweeps the whole 2^32 float space, split across hardware threads (the values
// are independent). Returns false as soon as any word mismatches.
bool allvalues() {
  unsigned int nthreads = std::thread::hardware_concurrency();
  if (nthreads == 0) {
    nthreads = 1;
  }
  std::atomic<bool> ok{true};
  std::vector<std::thread> workers;
  workers.reserve(nthreads);
  for (unsigned int t = 0; t < nthreads; t++) {
    workers.emplace_back([t, nthreads, &ok]() {
      for (uint64_t w = t;
           w <= 0xFFFFFFFF && ok.load(std::memory_order_relaxed);
           w += nthreads) {
        if (!check_word(uint32_t(w))) {
          ok.store(false, std::memory_order_relaxed);
          return;
        }
      }
    });
  }
  for (std::thread &worker : workers) {
    worker.join();
  }
  return ok.load();
}

inline void Assert(bool Assertion) {
#if defined(__CYGWIN__) || defined(__MINGW32__) || defined(__MINGW64__) ||     \
    defined(sun) || defined(__sun)
  if (!Assertion) {
    std::cerr << "Omitting hard failure on msys/cygwin/sun systems.";
  }
#else
  if (!Assertion) {
    throw std::runtime_error("bug");
  }
#endif
}

int main() {
#if defined(__CYGWIN__) || defined(__MINGW32__) || defined(__MINGW64__) ||     \
    defined(sun) || defined(__sun)
  std::cout << "Warning: msys/cygwin or solaris detected. This particular test "
               "is likely to generate false failures due to our reliance on "
               "the underlying runtime library as a gold standard."
            << std::endl;
#endif
  Assert(allvalues());
  std::cout << std::endl;
  std::cout << "all ok" << std::endl;
  return EXIT_SUCCESS;
}
