// from_chars_resume must give the same result as from_chars_advanced, whatever
// prefix of digits the caller has already read.
#include "fast_float/fast_float.h"
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <random>
#include <string>
#include <vector>

template <typename T> bool same_value(T a, T b) {
  if (a != a) {
    return b != b;
  }
  return std::memcmp(&a, &b, sizeof(T)) == 0;
}

template <typename T, typename UC>
bool check(std::basic_string<UC> const &s,
           fast_float::parse_options_t<UC> options) {
  UC const *first = s.data();
  UC const *last = s.data() + s.size();
  T expected = T(1.5);
  auto const want =
      fast_float::from_chars_advanced(first, last, expected, options);
  // Every cursor such that [first, cursor) is an optional sign followed by
  // digits, and cursor == first in any case.
  UC const *start = first;
  bool const plus =
      uint64_t(options.format & fast_float::chars_format::allow_leading_plus) &&
      !uint64_t(options.format & fast_float::detail::basic_json_fmt);
  if (start != last && (*start == UC('-') || (plus && *start == UC('+')))) {
    ++start;
  }
  std::vector<UC const *> cursors{first};
  for (UC const *c = start;; ++c) {
    if (c != first) {
      cursors.push_back(c);
    }
    if (c == last || !fast_float::is_integer(*c)) {
      break;
    }
  }
  for (UC const *cursor : cursors) {
    uint64_t prefix = 0;
    for (UC const *c = start; c < cursor; ++c) {
      prefix = 10 * prefix + uint64_t(*c - UC('0'));
    }
    T got = T(1.5);
    auto const r = fast_float::from_chars_resume(first, cursor, last, prefix,
                                                 got, options);
    if (r.ec != want.ec || r.ptr != want.ptr || !same_value(got, expected)) {
      std::cerr << "mismatch on \"";
      for (UC c : s) {
        std::cerr << char(c);
      }
      std::cerr << "\" (format " << uint64_t(options.format) << ", cursor at "
                << (cursor - first) << ", sizeof(T) " << sizeof(T)
                << ", sizeof(UC) " << sizeof(UC) << ")\n";
      return false;
    }
  }
  return true;
}

template <typename UC> bool check_all(std::string const &s) {
  using fast_float::chars_format;
  std::basic_string<UC> const w(s.begin(), s.end());
  chars_format const formats[] = {
      chars_format::general,
      chars_format::fixed,
      chars_format::scientific,
      chars_format::json,
      chars_format::json_or_infnan,
      chars_format::fortran,
      chars_format::javascript,
      chars_format::general | chars_format::allow_leading_plus,
      chars_format::general | chars_format::skip_white_space,
      chars_format::general | chars_format::no_infnan |
          chars_format::allow_leading_plus,
  };
  for (chars_format fmt : formats) {
    for (UC dp : {UC('.'), UC(',')}) {
      fast_float::parse_options_t<UC> const options{fmt, dp};
      if (!check<double>(w, options) || !check<float>(w, options)) {
        return false;
      }
    }
  }
  return true;
}

int main() {
  std::vector<std::string> inputs = {
      "",
      "-",
      "+",
      "0",
      "-0",
      "00",
      "01",
      "0.5",
      "1",
      "123",
      "123456789",
      "1234567890",
      "1700000000000",
      "-1700000000000.5",
      "1.5",
      "-12.25e3",
      "12e",
      "12.",
      ".5",
      "-.5",
      "1,5",
      "3.14159265358979323846264338327950288",
      "123456789012345678901234567890",
      "18446744073709551615",
      "18446744073709551616",
      "99999999999999999999.5",
      "9007199254740993",
      "2.2250738585072011e-308",
      "1e400",
      "-1e400",
      "1e-400",
      "123abc",
      "12 ",
      " 12",
      "  -3.5",
      "+12",
      "-+1",
      "inf",
      "-infinity",
      "nan",
      "12d3",
      "0x10",
      "017",
  };
  std::mt19937_64 rng(435);
  char const alphabet[] = "0123456789000.-+e, ";
  for (int k = 0; k < 20000; k++) {
    std::string s;
    size_t const len = size_t(rng() % 32);
    for (size_t j = 0; j < len; j++) {
      s += alphabet[rng() % (sizeof(alphabet) - 1)];
    }
    inputs.push_back(s);
  }
  for (auto const &s : inputs) {
    if (!check_all<char>(s) || !check_all<char16_t>(s)) {
      return EXIT_FAILURE;
    }
  }
  std::cout << "from_chars_resume: " << inputs.size() << " inputs checked\n";
  return EXIT_SUCCESS;
}
