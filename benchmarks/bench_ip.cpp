#include "counters/event_counter.h"
#include "fast_float/fast_float.h"
#include <charconv>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <random>
#include <atomic>
event_collector collector;

template <class function_type>
event_aggregate bench(const function_type &function, size_t min_repeat = 10,
                      size_t min_time_ns = 1000000000,
                      size_t max_repeat = 1000000) {
  event_aggregate aggregate{};
  size_t N = min_repeat;
  if (N == 0) {
    N = 1;
  }
  for (size_t i = 0; i < N; i++) {
    std::atomic_thread_fence(std::memory_order_acquire);
    collector.start();
    function();
    std::atomic_thread_fence(std::memory_order_release);
    event_count allocate_count = collector.end();
    aggregate << allocate_count;
    if ((i + 1 == N) && (aggregate.total_elapsed_ns() < min_time_ns) &&
        (N < max_repeat)) {
      N *= 10;
    }
  }
  return aggregate;
}

void pretty_print(size_t volume, size_t bytes, std::string name,
                  event_aggregate agg) {
  printf("%-40s : ", name.c_str());
  printf(" %5.2f GB/s ", bytes / agg.fastest_elapsed_ns());
  printf(" %5.1f Ma/s ", volume * 1000.0 / agg.fastest_elapsed_ns());
  printf(" %5.2f ns/d ", agg.fastest_elapsed_ns() / volume);
  if (collector.has_events()) {
    printf(" %5.2f GHz ", agg.fastest_cycles() / agg.fastest_elapsed_ns());
    printf(" %5.2f c/d ", agg.fastest_cycles() / volume);
    printf(" %5.2f i/d ", agg.fastest_instructions() / volume);
    printf(" %5.2f c/b ", agg.fastest_cycles() / bytes);
    printf(" %5.2f i/b ", agg.fastest_instructions() / bytes);
    printf(" %5.2f i/c ", agg.fastest_instructions() / agg.fastest_cycles());
  }
  printf("\n");
}

int parse_u8_fastfloat(const char *&p, const char *pend, uint8_t *out) {
  if (p == pend)
    return 0;
  auto r = fast_float::from_chars(p, pend, *out);
  if (r.ec == std::errc()) {
    p = r.ptr;
    return 1;
  }
  return 0;
}

static inline int parse_u8_fromchars(const char *&p, const char *pend,
                                     uint8_t *out) {
  if (p == pend)
    return 0;
  auto r = std::from_chars(p, pend, *out);
  if (r.ec == std::errc()) {
    p = r.ptr;
    return 1;
  }
  return 0;
}

template <typename Parser>
static inline int parse_ip_line(const char *&p, const char *pend, uint32_t &sum,
                                Parser parse_uint8) {
  uint8_t o = 0;
  for (int i = 0; i < 4; ++i) {
    if (!parse_uint8(p, pend, &o))
      return 0;
    sum += o;
    if (i != 3) {
      if (p == pend || *p != '.')
        return 0;
      ++p;
    }
  }
  // consume optional '\r'
  if (p != pend && *p == '\r')
    ++p;
  // expect '\n' or end
  if (p != pend && *p == '\n')
    ++p;
  return 1;
}

static std::string make_ip_line(uint8_t a, uint8_t b, uint8_t c, uint8_t d) {
  std::string s;
  s.reserve(16);
  s += std::to_string(a);
  s += '.';
  s += std::to_string(b);
  s += '.';
  s += std::to_string(c);
  s += '.';
  s += std::to_string(d);
  s += '\n';
  return s;
}

int main() {
  constexpr size_t N = 500000;
  std::mt19937 rng(1234);
  std::uniform_int_distribution<int> dist(0, 255);

  std::string buf;
  buf.reserve(N * 16);

  for (size_t i = 0; i < N; ++i) {
    uint8_t a = (uint8_t)dist(rng);
    uint8_t b = (uint8_t)dist(rng);
    uint8_t c = (uint8_t)dist(rng);
    uint8_t d = (uint8_t)dist(rng);
    buf += make_ip_line(a, b, c, d);
  }

  // sentinel to allow 4-byte loads at end
  buf.append(4, '\0');

  const size_t bytes = buf.size() - 4; // exclude sentinel from throughput
  const size_t volume = N;

  // validate correctness
  {
    const char *start = buf.data();
    const char *end = buf.data() + bytes;
    const char *p = start;
    const char *pend = end;
    uint32_t sum = 0;
    for (size_t i = 0; i < N; ++i) {
      int ok = parse_ip_line(p, pend, sum, parse_u8_fromchars);
      if (!ok) {
        std::fprintf(stderr, "fromchars parse failed at line %zu\n", i);
        std::abort();
      }
      p = start;
      pend = end;
      ok = parse_ip_line(p, pend, sum, parse_u8_fastfloat);
      if (!ok) {
        std::fprintf(stderr, "fastswar parse failed at line %zu\n", i);
        std::abort();
      }
    }
  }

  uint32_t sink = 0;

  pretty_print(volume, bytes, "parse_ip_std_fromchars", bench([&]() {
                 const char *p = buf.data();
                 const char *pend = buf.data() + bytes;
                 uint32_t sum = 0;
                 int ok = 0;
                 for (size_t i = 0; i < N; ++i) {
                   ok = parse_ip_line(p, pend, sum, parse_u8_fromchars);
                   if (!ok)
                     std::abort();
                 }
                 sink += sum;
               }));

  pretty_print(volume, bytes, "parse_ip_fastfloat", bench([&]() {
                 const char *p = buf.data();
                 const char *pend = buf.data() + bytes;
                 uint32_t sum = 0;
                 int ok = 0;
                 for (size_t i = 0; i < N; ++i) {
                   ok = parse_ip_line(p, pend, sum, parse_u8_fastfloat);
                   if (!ok)
                     std::abort();
                 }
                 sink += sum;
               }));

  std::printf("sink=%u\n", sink);
  return EXIT_SUCCESS;
}