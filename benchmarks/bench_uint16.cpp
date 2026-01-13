#include "counters/bench.h"
#include "fast_float/fast_float.h"
#include <charconv>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <random>
#include <atomic>
#include <string>
#include <vector>

void pretty_print(size_t volume, size_t bytes, std::string name,
                  counters::event_aggregate agg) {
  if (agg.inner_count > 1) {
    printf("# (inner count: %d)\n", agg.inner_count);
  }
  printf("%-40s : ", name.c_str());
  printf(" %5.2f GB/s ", bytes / agg.fastest_elapsed_ns());
  printf(" %5.1f Mip/s ", volume * 1000.0 / agg.fastest_elapsed_ns());
  printf(" %5.2f ns/ip ", agg.fastest_elapsed_ns() / volume);
  if (counters::event_collector().has_events()) {
    printf(" %5.2f GHz ", agg.fastest_cycles() / agg.fastest_elapsed_ns());
    printf(" %5.2f c/ip ", agg.fastest_cycles() / volume);
    printf(" %5.2f i/ip ", agg.fastest_instructions() / volume);
    printf(" %5.2f c/b ", agg.fastest_cycles() / bytes);
    printf(" %5.2f i/b ", agg.fastest_instructions() / bytes);
    printf(" %5.2f i/c ", agg.fastest_instructions() / agg.fastest_cycles());
  }
  printf("\n");
}

enum class parse_method { standard, fast_float };

void validate(const std::string &buffer, const std::vector<uint16_t> &expected,
              char delimiter) {
  const char *p = buffer.data();
  const char *pend = p + buffer.size();

  for (size_t i = 0; i < expected.size(); i++) {
    uint16_t val;
    auto r = fast_float::from_chars(p, pend, val);
    if (r.ec != std::errc() || val != expected[i]) {
      printf("Validation failed at index %zu: expected %u, got %u\n", i,
             expected[i], val);
      std::abort();
    }
    p = r.ptr;
    if (i + 1 < expected.size()) {
      if (p >= pend || *p != delimiter) {
        printf("Validation failed at index %zu: delimiter mismatch\n", i);
        std::abort();
      }
      ++p;
    }
  }

  if (p != pend) {
    printf("Validation failed: trailing bytes remain\n");
    std::abort();
  }
  printf("Validation passed!\n");
}

int main() {
  constexpr size_t N = 500000;
  constexpr char delimiter = ',';
  std::mt19937 rng(1234);
  std::uniform_int_distribution<int> dist(0, 65535);

  std::vector<uint16_t> expected;
  expected.reserve(N);

  std::string buffer;
  buffer.reserve(N * 6); // up to 5 digits + delimiter

  for (size_t i = 0; i < N; ++i) {
    uint16_t val = (uint16_t)dist(rng);
    expected.push_back(val);
    std::string s = std::to_string(val);
    buffer.append(s);
    if (i + 1 < N) {
      buffer.push_back(delimiter);
    }
  }

  size_t total_bytes = buffer.size();

  validate(buffer, expected, delimiter);

  volatile uint64_t sink = 0;

  pretty_print(N, total_bytes, "parse_uint16_std_fromchars",
               counters::bench([&]() {
                 uint64_t sum = 0;
                 const char *p = buffer.data();
                 const char *pend = p + buffer.size();
                 for (size_t i = 0; i < N; ++i) {
                   uint16_t value = 0;
                   auto r = std::from_chars(p, pend, value);
                   if (r.ec != std::errc())
                     std::abort();
                   sum += value;
                   p = r.ptr;
                   if (i + 1 < N) {
                     if (p >= pend || *p != delimiter)
                       std::abort();
                     ++p;
                   }
                 }
                 if (p != pend)
                   std::abort();
                 sink += sum;
               }));

  pretty_print(N, total_bytes, "parse_uint16_fastfloat", counters::bench([&]() {
                 uint64_t sum = 0;
                 const char *p = buffer.data();
                 const char *pend = p + buffer.size();
                 for (size_t i = 0; i < N; ++i) {
                   uint16_t value = 0;
                   auto r = fast_float::from_chars(p, pend, value);
                   if (r.ec != std::errc())
                     std::abort();
                   sum += value;
                   p = r.ptr;
                   if (i + 1 < N) {
                     if (p >= pend || *p != delimiter)
                       std::abort();
                     ++p;
                   }
                 }
                 if (p != pend)
                   std::abort();
                 sink += sum;
               }));

  return EXIT_SUCCESS;
}
