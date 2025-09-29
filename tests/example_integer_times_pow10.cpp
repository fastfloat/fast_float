#include "fast_float/fast_float.h"

#include <iostream>

void default_overload() {
  const uint64_t W = 12345678901234567;
  const int Q = 23;
  const double result = fast_float::integer_times_pow10(W, Q);
  std::cout.precision(17);
  std::cout << W << " * 10^" << Q << " = " << result << " ("
            << (result == 12345678901234567e23 ? "==" : "!=") << "expected)\n";
}

void double_specialization() {
  const uint64_t W = 12345678901234567;
  const int Q = 23;
  const double result = fast_float::integer_times_pow10<double>(W, Q);
  std::cout.precision(17);
  std::cout << "double: " << W << " * 10^" << Q << " = " << result << " ("
            << (result == 12345678901234567e23 ? "==" : "!=") << "expected)\n";
}

void float_specialization() {
  const uint64_t W = 1234567;
  const int Q = 23;
  const double result = fast_float::integer_times_pow10<float>(W, Q);
  std::cout.precision(7);
  std::cout << "float: " << W << " * 10^" << Q << " = " << result << " ("
            << (result == 1234567e23f ? "==" : "!=") << "expected)\n";
}

int main() {
  default_overload();
  double_specialization();
  float_specialization();
}
