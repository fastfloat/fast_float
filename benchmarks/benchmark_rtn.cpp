// The same parsing loops as findmax_fastfloat64 and findmax_fastfloat32 in
// benchmark.cpp, but compiled with FASTFLOAT_ROUNDS_TO_NEAREST: fast_float
// assumes the default rounding mode and skips its check before the fast path.
//
// We rename the fast_float namespace in this file. Otherwise, the two
// configurations would define the same inline functions differently, and the
// linker could keep either one (an ODR violation).
#include <stdexcept>
#include <string>
#include <vector>

#define FASTFLOAT_ROUNDS_TO_NEAREST
#define fast_float fast_float_rtn
#include "fast_float/fast_float.h"

template <typename CharT>
double findmax_fastfloat64_rtn(std::vector<std::basic_string<CharT>> &s) {
  double answer = 0;
  double x = 0;
  for (auto &st : s) {
    auto [p, ec] = fast_float::from_chars(st.data(), st.data() + st.size(), x);
    if (p == st.data()) {
      throw std::runtime_error("bug in findmax_fastfloat_rtn");
    }
    answer = answer > x ? answer : x;
  }
  return answer;
}

template <typename CharT>
double findmax_fastfloat32_rtn(std::vector<std::basic_string<CharT>> &s) {
  float answer = 0;
  float x = 0;
  for (auto &st : s) {
    auto [p, ec] = fast_float::from_chars(st.data(), st.data() + st.size(), x);
    if (p == st.data()) {
      throw std::runtime_error("bug in findmax_fastfloat_rtn");
    }
    answer = answer > x ? answer : x;
  }
  return answer;
}

template double findmax_fastfloat64_rtn<char>(std::vector<std::string> &);
template double findmax_fastfloat32_rtn<char>(std::vector<std::string> &);
template double
findmax_fastfloat64_rtn<char16_t>(std::vector<std::u16string> &);
template double
findmax_fastfloat32_rtn<char16_t>(std::vector<std::u16string> &);
