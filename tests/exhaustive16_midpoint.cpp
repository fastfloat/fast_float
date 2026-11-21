// For std::float16_t and std::bfloat16_t, parse the exact decimal expansion
// of every finite value and of every midpoint between adjacent values, plus
// strings just above and below each midpoint and midpoints padded past 19
// digits. The expected bits follow from the bit pattern that generated the
// string, so no C library reference is needed.
#include "fast_float/fast_float.h"

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <stdfloat>
#include <string>
#include <system_error>

#ifdef __STDCPP_FLOAT16_T__

namespace {

// Decimal digit strings, most significant digit first, no leading zeros.
std::string times(std::string const &digits, unsigned factor) {
  std::string out;
  unsigned carry = 0;
  for (size_t i = digits.size(); i-- > 0;) {
    unsigned v = static_cast<unsigned>(digits[i] - '0') * factor + carry;
    out.push_back(static_cast<char>('0' + v % 10));
    carry = v / 10;
  }
  while (carry != 0) {
    out.push_back(static_cast<char>('0' + carry % 10));
    carry /= 10;
  }
  return std::string(out.rbegin(), out.rend());
}

std::string minus_one(std::string digits) {
  size_t i = digits.size();
  while (i-- > 0) {
    if (digits[i] != '0') {
      digits[i] = static_cast<char>(digits[i] - 1);
      break;
    }
    digits[i] = '9';
  }
  return digits;
}

// num * 2^exp2 written as "<digits>e<exponent>" with an exact expansion.
struct Decimal {
  std::string digits;
  int exponent;

  std::string str() const { return digits + "e" + std::to_string(exponent); }

  Decimal above() const { return {digits + "1", exponent - 1}; }

  Decimal below() const { return {minus_one(digits) + "9", exponent - 1}; }

  Decimal padded(size_t count) const {
    return {digits + std::string(count, '0'),
            exponent - static_cast<int>(count)};
  }
};

Decimal exact_decimal(uint64_t num, int exp2) {
  std::string digits = std::to_string(num);
  if (exp2 >= 0) {
    for (int i = 0; i < exp2; i++) {
      digits = times(digits, 2);
    }
    return {digits, 0};
  }
  for (int i = 0; i < -exp2; i++) {
    digits = times(digits, 5);
  }
  return {digits, exp2};
}

template <typename T> struct Layout;

template <> struct Layout<std::float16_t> {
  static constexpr char const *name = "float16";
  static constexpr int p = 10;
  static constexpr int bias = 25; // value = mantissa * 2^(field - bias)
  static constexpr uint16_t infinity = 0x7C00;
};

template <> struct Layout<std::bfloat16_t> {
  static constexpr char const *name = "bfloat16";
  static constexpr int p = 7;
  static constexpr int bias = 134;
  static constexpr uint16_t infinity = 0x7F80;
};

struct Failure {
  long count = 0;
  long checked = 0;
};

template <typename T>
void expect(Failure &f, std::string const &s, uint16_t want) {
  T value{};
  auto r = fast_float::from_chars(s.data(), s.data() + s.size(), value);
  uint16_t got = 0;
  std::memcpy(&got, &value, sizeof(got));
  bool zero_or_inf = want == 0 || want == Layout<T>::infinity;
  std::errc want_ec =
      zero_or_inf ? std::errc::result_out_of_range : std::errc();
  f.checked++;
  if (got != want || r.ec != want_ec || r.ptr != s.data() + s.size()) {
    f.count++;
    if (f.count <= 20) {
      std::printf(
          "%s: \"%s\" parsed to 0x%04x (ec %d), expected 0x%04x (ec %d)\n",
          Layout<T>::name, s.c_str(), got, static_cast<int>(r.ec), want,
          static_cast<int>(want_ec));
    }
  }
}

template <typename T> Failure sweep() {
  Failure f;
  constexpr int p = Layout<T>::p;
  constexpr uint16_t mantissa_mask = static_cast<uint16_t>((1u << p) - 1);
  for (uint32_t b = 0; b < Layout<T>::infinity; b++) {
    uint16_t bits = static_cast<uint16_t>(b);
    int field = bits >> p;
    uint64_t num = bits & mantissa_mask;
    int exp2 = (field == 0 ? 1 : field) - Layout<T>::bias;
    if (field != 0) {
      num |= uint64_t(1) << p;
    }
    if (num != 0) {
      Decimal d = exact_decimal(num, exp2);
      expect<T>(f, d.str(), bits);
      expect<T>(f, d.padded(21).str(), bits);
    }
    // The tie between bits and bits + 1 is (2 num + 1) * 2^(exp2 - 1); the
    // last one (bits + 1 == infinity) is the overflow threshold.
    Decimal tie = exact_decimal(2 * num + 1, exp2 - 1);
    uint16_t lower = bits;
    uint16_t upper = static_cast<uint16_t>(bits + 1);
    uint16_t even = (bits & 1) ? upper : lower;
    expect<T>(f, tie.str(), even);
    expect<T>(f, tie.padded(21).str(), even);
    expect<T>(f, tie.above().str(), upper);
    expect<T>(f, tie.below().str(), lower);
  }
  return f;
}

} // namespace

#endif

int main() {
#ifdef __STDCPP_FLOAT16_T__
  Failure f16 = sweep<std::float16_t>();
  std::printf("float16: %ld strings, %ld failures\n", f16.checked, f16.count);
  Failure bf16 = sweep<std::bfloat16_t>();
  std::printf("bfloat16: %ld strings, %ld failures\n", bf16.checked,
              bf16.count);
  if (f16.count != 0 || bf16.count != 0) {
    return EXIT_FAILURE;
  }
#else
  std::printf("float16x type unsupported, so this test isn't applied\n");
#endif
  std::printf("all ok\n");
  return EXIT_SUCCESS;
}
