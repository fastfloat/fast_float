// Integer parsing reads eight bytes at once when they are available and has a
// separate path for 0 to 7 digits. Compare fast_float::from_chars with
// std::from_chars on every digit count, on every byte that may follow the
// digits, and on every truncation of the input, so that both the eight-byte
// path and the byte-by-byte path are exercised at each length.
#include "fast_float/fast_float.h"

#include <charconv>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <random>
#include <string>
#include <system_error>

static size_t checks = 0;

template <typename T> static void check(std::string const &s) {
  char const *const first = s.data();
  for (size_t len = 0; len <= s.size(); len++) {
    char const *const last = first + len;
    T a = 7, b = 7;
    auto const ra = fast_float::from_chars(first, last, a);
    auto const rb = std::from_chars(first, last, b);
    checks++;
    bool const ok = ra.ec == rb.ec && ra.ptr == rb.ptr &&
                    (ra.ec == std::errc() ? a == b : a == T(7));
    if (!ok) {
      std::printf("mismatch: sizeof(T)=%zu signed=%d input='%.*s'\n"
                  "  fast_float: ec=%d ptr=%td value=%lld\n"
                  "  std:        ec=%d ptr=%td value=%lld\n",
                  sizeof(T), int(std::is_signed<T>::value), int(len), first,
                  int(ra.ec), ra.ptr - first, static_cast<long long>(a),
                  int(rb.ec), rb.ptr - first, static_cast<long long>(b));
      std::exit(EXIT_FAILURE);
    }
  }
}

static void check_all_types(std::string const &s) {
  check<uint64_t>(s);
  check<int64_t>(s);
  check<uint32_t>(s);
  check<int32_t>(s);
  check<uint16_t>(s);
  check<int16_t>(s);
  check<uint8_t>(s);
  check<int8_t>(s);
}

int main() {
  char const *const suffixes[] = {"",     ",",    "]",  ".5",       "e3",
                                  "/",    ":",    " ",  "x",        "\x80",
                                  "\xff", "\x7f", "/:", "123456789"};
  char const *const prefixes[] = {"", "-", "+", "0", "-0", "00"};
  char const *const patterns[] = {
      "12345678901234567890123", "99999999999999999999999",
      "00000000000000000000000", "10000000000000000000000",
      "18446744073709551615999", "92233720368547758079999"};
  for (char const *pattern : patterns) {
    for (size_t n = 0; n <= 23; n++) {
      for (char const *suffix : suffixes) {
        for (char const *prefix : prefixes) {
          check_all_types(std::string(prefix) + std::string(pattern, n) +
                          suffix + "zzzzzzzz");
        }
      }
    }
  }
  // Every byte value right after 0 to 9 digits.
  for (size_t k = 0; k <= 9; k++) {
    for (int c = 0; c < 256; c++) {
      std::string s("98765432109", k);
      s.push_back(static_cast<char>(c));
      s += "1234567";
      check_all_types(s);
    }
  }
  std::mt19937_64 rng(42);
  char const alphabet[] = "0123456789-+,.e";
  for (int r = 0; r < 100000; r++) {
    std::string s;
    size_t const len = size_t(rng() % 14);
    for (size_t j = 0; j < len; j++) {
      s.push_back(alphabet[rng() % (sizeof(alphabet) - 1)]);
    }
    check_all_types(s);
  }
  std::printf("int_swar_test: %zu checks passed\n", checks);
  return EXIT_SUCCESS;
}
