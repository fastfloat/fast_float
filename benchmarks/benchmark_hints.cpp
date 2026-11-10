// The parsing loops of benchmark.cpp, with performance hints (see
// findmax_fastfloat_hints in benchmark.cpp).
#include <stdexcept>
#include <string>
#include <vector>

#include "fast_float/fast_float.h"

// With performance hints.
template <fast_float::performance_hints Hints, typename T, typename CharT>
double findmax_fastfloat_hints(std::vector<std::basic_string<CharT>> &s) {
  T answer = 0;
  T x = 0;
  for (auto &st : s) {
    auto [p, ec] =
        fast_float::from_chars<Hints>(st.data(), st.data() + st.size(), x);
    if (p == st.data()) {
      throw std::runtime_error("bug in findmax_fastfloat_hints");
    }
    answer = answer > x ? answer : x;
  }
  return answer;
}

constexpr fast_float::performance_hints unpredictable_sign =
    fast_float::performance_hints::unpredictable_sign;
constexpr fast_float::performance_hints assume_nearest =
    fast_float::performance_hints::assume_round_to_nearest;

template double findmax_fastfloat_hints<unpredictable_sign, double, char>(
    std::vector<std::string> &);
template double findmax_fastfloat_hints<unpredictable_sign, double, char16_t>(
    std::vector<std::u16string> &);
template double findmax_fastfloat_hints<assume_nearest, double, char>(
    std::vector<std::string> &);
template double findmax_fastfloat_hints<assume_nearest, double, char16_t>(
    std::vector<std::u16string> &);
template double findmax_fastfloat_hints<assume_nearest, float, char>(
    std::vector<std::string> &);
template double findmax_fastfloat_hints<assume_nearest, float, char16_t>(
    std::vector<std::u16string> &);
