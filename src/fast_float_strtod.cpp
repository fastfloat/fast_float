#include "fast_float/fast_float_strtod.h"
#include "fast_float/fast_float.h"
#include <cerrno>
#include <cstring>
#include <system_error>

extern "C" double fast_float_strtod(const char *nptr, char **endptr) {
  double result = 0.0;

  // Parse the string using fast_float's from_chars function
  auto parse_result = fast_float::from_chars(nptr, nptr + strlen(nptr), result);

  // Check if parsing encountered an error
  if (parse_result.ec != std::errc{}) {
    errno = EINVAL;
  }

  // Update endptr if provided
  if (endptr != nullptr) {
    *endptr = const_cast<char *>(parse_result.ptr);
  }

  return result;
}