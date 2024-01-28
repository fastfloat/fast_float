#include "fast_float/fast_float.h"
#include <fuzzer/FuzzedDataProvider.h>
#include <string>
#include <system_error>

fast_float::chars_format arbitrary_format(FuzzedDataProvider &fdp) {
    using fast_float::chars_format;
    switch (fdp.ConsumeIntegralInRange<int>(0,3)) {
        case 0:
            return chars_format::scientific;
            break;
        case 1:
            return chars_format::fixed;
            break;
        case 2:
            return chars_format::fixed;
            break;
    }
    return chars_format::general;
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
  FuzzedDataProvider fdp(data, size);
  fast_float::chars_format format = arbitrary_format(fdp);
  double result_d = 0.0;
  std::string input_d = fdp.ConsumeRandomLengthString(128);
  auto answer =
      fast_float::from_chars(input_d.data(), input_d.data() + input_d.size(), result_d, format);
  std::string input_f = fdp.ConsumeRandomLengthString(128);
  float result_f = 0.0;
  answer =
      fast_float::from_chars(input_f.data(), input_f.data() + input_f.size(), result_f, format);
  int result_i = 0;
  std::string input_i = fdp.ConsumeRandomLengthString(128);
  answer =
      fast_float::from_chars(input_i.data(), input_i.data() + input_i.size(), result_i);
  return 0;
}