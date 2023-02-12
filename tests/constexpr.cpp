#include "fast_float/fast_float.h"

constexpr double zero = [] {
  double ret = 0;

  const char *str = "0";

  fast_float::from_chars(str, str + 1, ret);

  return ret;
}();

static_assert(zero == 0.);

constexpr double pi = [] {
  double ret = 0;

  const char *str = "3.1415";

  fast_float::from_chars(str, str + 6, ret);

  return ret;
}();

static_assert(pi == 3.1415);

constexpr double googol = [] {
  double ret = 0;

  const char *str = "1e100";

  fast_float::from_chars(str, str + 5, ret);

  return ret;
}();

static_assert(googol == 1e100);

int main() { return 0; }
