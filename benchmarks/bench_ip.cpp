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

void pretty_print(size_t volume, size_t bytes, std::string name,
                  counters::event_aggregate agg) {
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

const char *seek_ip_end(const char *p, const char *pend) {
  const char *current = p;
  size_t count = 0;
  for (; current != pend; ++current) {
    if (*current == '.') {
      count++;
      if (count == 3) {
        ++current;
        break;
      }
    }
  }
  while (current != pend) {
    if (*current <= '9' && *current >= '0') {
      ++current;
    } else {
      break;
    }
  }
  return current;
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
  if (p == pend) {
    return 0;
  }
  auto r = std::from_chars(p, pend, *out);
  if (r.ec == std::errc()) {
    p = r.ptr;
    return 1;
  }
  return 0;
}

template <typename Parser>
std::pair<bool, uint32_t> simple_parse_ip_line(const char *p, const char *pend,
                                               Parser parse_uint8) {
  uint8_t v1;
  if (!parse_uint8(p, pend, &v1)) {
    return {false, 0};
  }
  if (p == pend || *p++ != '.') {
    return {false, 0};
  }
  uint8_t v2;
  if (!parse_uint8(p, pend, &v2)) {
    return {false, 0};
  }
  if (p == pend || *p++ != '.') {
    return {false, 0};
  }
  uint8_t v3;
  if (!parse_uint8(p, pend, &v3)) {
    return {false, 0};
  }
  if (p == pend || *p++ != '.') {
    return {false, 0};
  }
  uint8_t v4;
  if (!parse_uint8(p, pend, &v4)) {
    return {false, 0};
  }
  return {true, (uint32_t(v1) << 24) | (uint32_t(v2) << 16) |
                    (uint32_t(v3) << 8) | uint32_t(v4)};
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
  constexpr size_t N = 15000;
  std::mt19937 rng(1234);
  std::uniform_int_distribution<int> dist(0, 255);

  std::string buf;
  constexpr size_t ip_size = 16;
  buf.reserve(N * ip_size);

  for (size_t i = 0; i < N; ++i) {
    uint8_t a = (uint8_t)dist(rng);
    uint8_t b = (uint8_t)dist(rng);
    uint8_t c = (uint8_t)dist(rng);
    uint8_t d = (uint8_t)dist(rng);
    std::string ip_line = make_ip_line(a, b, c, d);
    ip_line.resize(ip_size, ' '); // pad to fixed size
    buf.append(ip_line);
  }

  // sentinel to allow 4-byte loads at end
  buf.append(4, '\0');

  const size_t bytes = buf.size() - 4; // exclude sentinel from throughput
  const size_t volume = N;

  volatile uint32_t sink = 0;
  std::string buffer(ip_size * N, ' ');

  pretty_print(volume, bytes, "memcpy baseline",
               counters::bench([&]() {
                std::memcpy((char *)buffer.data(), buf.data(), bytes);
               }));


  pretty_print(volume, bytes, "just_seek_ip_end (no parse)",
               counters::bench([&]() {
                 const char *p = buf.data();
                 const char *pend = buf.data() + bytes;
                 uint32_t sum = 0;
                 int ok = 0;
                 for (size_t i = 0; i < N; ++i) {
                   const char *q = seek_ip_end(p, pend);
                   sum += (uint32_t)(q - p);
                   p += ip_size;
                 }
                 sink += sum;
               }));

  pretty_print(volume, bytes, "parse_ip_std_fromchars", counters::bench([&]() {
                 const char *p = buf.data();
                 const char *pend = buf.data() + bytes;
                 uint32_t sum = 0;
                 int ok = 0;
                 for (size_t i = 0; i < N; ++i) {
                   auto [ok, ip] =
                       simple_parse_ip_line(p, pend, parse_u8_fromchars);
                   sum += ip;
                   if (!ok) {
                     std::abort();
                   }
                   p += ip_size;
                 }
                 sink += sum;
               }));

  pretty_print(volume, bytes, "parse_ip_fastfloat", counters::bench([&]() {
                 const char *p = buf.data();
                 const char *pend = buf.data() + bytes;
                 uint32_t sum = 0;
                 int ok = 0;
                 for (size_t i = 0; i < N; ++i) {
                   auto [ok, ip] =
                       simple_parse_ip_line(p, pend, parse_u8_fastfloat);
                   sum += ip;
                   if (!ok) {
                     std::abort();
                   }
                   p += ip_size;
                 }
                 sink += sum;
               }));

  return EXIT_SUCCESS;
}