// Integer parsing benchmark: fast_float::from_chars on integers of various
// digit counts, either packed in one comma-separated buffer (the parser can
// look past the number) or as separate exact-length strings.
#include "counters/bench.h"
#include "fast_float/fast_float.h"
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <random>
#include <string>
#include <vector>

static void pretty_print(size_t volume, std::string name,
                         counters::event_aggregate agg) {
  printf("%-34s : ", name.c_str());
  printf(" %6.2f ns/int ", agg.fastest_elapsed_ns() / volume);
  if (counters::event_collector().has_events()) {
    printf(" %6.2f c/int ", agg.fastest_cycles() / volume);
    printf(" %6.2f i/int ", agg.fastest_instructions() / volume);
    printf(" %5.2f i/c ", agg.fastest_instructions() / agg.fastest_cycles());
  }
  printf("\n");
}

static volatile uint64_t sink = 0;

template <typename T>
static void bench(std::string name, const std::vector<T> &values) {
  std::string buffer;
  std::vector<std::string> pieces;
  uint64_t expected = 0;
  for (size_t i = 0; i < values.size(); i++) {
    std::string s = std::to_string(values[i]);
    buffer += s;
    buffer.push_back(',');
    pieces.push_back(s);
    expected += uint64_t(values[i]);
  }
  size_t const N = values.size();
  auto packed = [&]() {
    uint64_t sum = 0;
    char const *p = buffer.data();
    char const *pend = p + buffer.size();
    for (size_t i = 0; i < N; ++i) {
      T v;
      auto r = fast_float::from_chars(p, pend, v);
      if (r.ec != std::errc()) {
        std::abort();
      }
      sum += uint64_t(v);
      p = r.ptr + 1;
    }
    if (sum != expected) {
      std::abort();
    }
    sink = sink + sum;
  };
  auto separate = [&]() {
    uint64_t sum = 0;
    for (auto const &s : pieces) {
      T v;
      auto r = fast_float::from_chars(s.data(), s.data() + s.size(), v);
      if (r.ec != std::errc()) {
        std::abort();
      }
      sum += uint64_t(v);
    }
    if (sum != expected) {
      std::abort();
    }
    sink = sink + sum;
  };
  pretty_print(N, name + " packed", counters::bench(packed));
  pretty_print(N, name + " separate", counters::bench(separate));
}

// A value with a uniformly random number of digits in [lo, hi].
static uint64_t random_digits(std::mt19937_64 &rng, int lo, int hi) {
  int d = lo + int(rng() % uint64_t(hi - lo + 1));
  uint64_t low = 1;
  for (int i = 1; i < d; i++) {
    low *= 10;
  }
  if (d == 1) {
    return rng() % 10;
  }
  uint64_t span = (d == 20) ? (UINT64_MAX - low) : 9 * low;
  return low + rng() % span;
}

int main() {
  size_t const N = 200000;
  std::mt19937_64 rng(1234);
  auto make = [&](int lo, int hi) {
    std::vector<uint64_t> v(N);
    for (auto &x : v) {
      x = random_digits(rng, lo, hi);
    }
    return v;
  };
  bench("u64 1-3 digits", make(1, 3));
  bench("u64 1-7 digits", make(1, 7));
  bench("u64 4-7 digits", make(4, 7));
  bench("u64 8-12 digits", make(8, 12));
  bench("u64 1-20 digits", make(1, 20));
  {
    std::vector<uint64_t> v(N);
    for (auto &x : v) {
      x = rng();
    }
    bench("u64 random", v);
  }
  {
    std::vector<uint32_t> v(N);
    for (auto &x : v) {
      x = uint32_t(random_digits(rng, 1, 7));
    }
    bench("u32 1-7 digits", v);
  }
  {
    std::vector<int64_t> v(N);
    for (auto &x : v) {
      int64_t m = int64_t(random_digits(rng, 1, 7));
      x = (rng() & 1) ? -m : m;
    }
    bench("i64 +-1-7 digits", v);
  }
  return EXIT_SUCCESS;
}
