#include "fast_float/fast_float.h"

#include <iostream>

int main() {
  const uint64_t W = 12345678901234567;
  const int Q = 23;
  const double result = fast_float::integer_times_pow10(W, Q);
  std::cout.precision(17);
  std::cout << W << " * 10^" << Q << " = " << result << " ("
            << (result == 12345678901234567e23 ? "==" : "!=") << "expected)\n";
}
