#include "fast_float/fast_float.h"

#include <iomanip>

inline void Assert(bool Assertion) {
  if (!Assertion)
    throw std::runtime_error("bug");
}

template <typename T> std::string to_string(T d) {
  std::string s(64, '\0');
  auto written = std::snprintf(&s[0], s.size(), "%.*e",
                               std::numeric_limits<T>::max_digits10 - 1, d);
  s.resize(written);
  return s;
}

bool demo32(std::string vals) {
  float result_value;
  auto result = fast_float::from_chars(vals.data(), vals.data() + vals.size(),
                                      result_value);
  if (result.ec != std::errc()) {
    std::cerr << " I could not parse " << vals << std::endl;
    return false;
  }

  std::cout << result_value << std::endl;
  return true;
}

bool demo32(std::string vals, float val) {
  float result_value;
  auto result = fast_float::from_chars(vals.data(), vals.data() + vals.size(),
                                      result_value);
  if (result.ec != std::errc()) {
    std::cerr << " I could not parse " << vals << std::endl;
    return false;
  }
  if (std::isnan(val)) {
    if (!std::isnan(result_value)) {
      std::cerr << "not nan" << result_value << std::endl;
      return false;
    }
  } else if (result_value != val) {
    std::cerr << "I got " << std::setprecision(15) << result_value << " but I was expecting " << val
              << std::endl;
    uint32_t word;
    memcpy(&word, &result_value, sizeof(word));
    std::cout << "got mantissa = " << (word & ((1<<23)-1)) << std::endl;
    memcpy(&word, &val, sizeof(word));
    std::cout << "wanted mantissa = " << (word & ((1<<23)-1)) << std::endl;
    std::cerr << "string: " << vals << std::endl;
    return false;
  }
  std::cout << result_value << " == " << val << std::endl;
  return true;
}

bool demo32(float val) {
  std::string vals = to_string(val);
  return demo32(vals, val);
}

bool demo64(std::string vals, double val) {
  double result_value;
  auto result = fast_float::from_chars(vals.data(), vals.data() + vals.size(),
                                      result_value);
  if (result.ec != std::errc()) {
    std::cerr << " I could not parse " << vals << std::endl;
    return false;
  }
  if (std::isnan(val)) {
    if (!std::isnan(result_value)) {
      std::cerr << "not nan" << result_value << std::endl;
      return false;
    }
  } else if (result_value != val) {
    std::cerr << "I got " << std::setprecision(15) << result_value << " but I was expecting " << val
              << std::endl;
    std::cerr << "string: " << vals << std::endl;
    return false;
  }
  std::cout << result_value << " == " << val << std::endl;

  return true;
}
bool demo64(double val) {
  std::string vals = to_string(val);
  return demo64(vals, val);
}

int main() {
  std::cout << "32 bits " << std::endl;
  Assert(demo64("+1", 1));
  Assert(demo64("2e3000", std::numeric_limits<float>::infinity()));
  Assert(demo32("3.5028234666e38", std::numeric_limits<float>::infinity()));
  Assert(demo32("7.0060e-46", 0));
  Assert(demo32(1.00000006e+09f));
  Assert(demo32(1.4012984643e-45f));
  Assert(demo32(1.1754942107e-38f));
  Assert(demo32(1.1754943508e-45f));
  Assert(demo32(3.4028234664e38f));
  Assert(demo32(3.4028234665e38f));
  Assert(demo32(3.4028234666e38f));
  std::cout << std::endl;

  std::cout << "64 bits " << std::endl;
  Assert(demo64("+1", 1));
  Assert(demo64("2e3000", std::numeric_limits<double>::infinity()));
  Assert(demo64("1.9e308", std::numeric_limits<double>::infinity()));
  Assert(demo64(3e-324));
  Assert(demo32(1.00000006e+09f));
  Assert(demo64(4.9406564584124653e-324));
  Assert(demo64(4.9406564584124654e-324));
  Assert(demo64(2.2250738585072009e-308));
  Assert(demo64(2.2250738585072014e-308));
  Assert(demo64(1.7976931348623157e308));
  Assert(demo64(1.7976931348623158e308));
  std::cout << std::endl;
  std::cout << "All ok" << std::endl;
  return EXIT_SUCCESS;
}
