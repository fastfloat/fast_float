#include "fast_float/fast_float.h"


#include <cassert>
#include <cmath>

template <typename T> char *to_string(T d, char *buffer) {
  auto written = std::snprintf(buffer, 64, "%.*e",
                               std::numeric_limits<T>::max_digits10 - 1, d);
  return buffer + written;
}

static __uint128_t g_lehmer64_state;

/**
 * D. H. Lehmer, Mathematical methods in large-scale computing units.
 * Proceedings of a Second Symposium on Large Scale Digital Calculating
 * Machinery;
 * Annals of the Computation Laboratory, Harvard Univ. 26 (1951), pp. 141-146.
 *
 * P L'Ecuyer,  Tables of linear congruential generators of different sizes and
 * good lattice structure. Mathematics of Computation of the American
 * Mathematical
 * Society 68.225 (1999): 249-260.
 */

static inline void lehmer64_seed(uint64_t seed) { g_lehmer64_state = seed; }

static inline uint64_t lehmer64() {
  g_lehmer64_state *= UINT64_C(0xda942042e4dd58b5);
  return uint64_t(g_lehmer64_state >> 64);
}

size_t errors;

void random_values(size_t N) {
  char buffer[64];
  lehmer64_seed(N);
  for (size_t t = 0; t < N; t++) {
    if ((t % 1048576) == 0) {
      std::cout << ".";
      std::cout.flush();
    }
    uint64_t word = lehmer64();
    double v;
    memcpy(&v, &word, sizeof(v));
    // if (!std::isnormal(v))
    {
      const char *string_end = to_string(v, buffer);
      double result_value;
      auto result = fast_float::from_chars(buffer, string_end, result_value);
      if (result.ec != std::errc()) {
        std::cerr << "parsing error ? " << buffer << std::endl;
        errors++;
        if (errors > 10) {
          abort();
        }
      }
      if (std::isnan(v)) {
        if (!std::isnan(result_value)) {
          std::cerr << "not nan" << buffer << std::endl;
          errors++;
          if (errors > 10) {
            abort();
          }
        }
      } else if (result_value != v) {
        std::cerr << "no match ? " << buffer << std::endl;
        std::cout << "started with " << std::hexfloat << v << std::endl;
        std::cout << "got back " << std::hexfloat << result_value << std::endl; 
        std::cout << std::dec;
        errors++;
        if (errors > 10) {
          abort();
        }
      }
    }
  }
  std::cout << std::endl;
}

int main() {
  errors = 0;
  size_t N = size_t(1) << 32;
  random_values(N);
  if (errors == 0) {
    std::cout << std::endl;
    std::cout << "all ok" << std::endl;
    return EXIT_SUCCESS;
  }
  std::cerr << std::endl;
  std::cerr << "errors were encountered" << std::endl;
  return EXIT_FAILURE;
}
