#ifndef FAST_FLOAT_STRTOD_H__
#define FAST_FLOAT_STRTOD_H__

#if defined(__cplusplus)
extern "C" {
#endif
/**
 * @brief Convert a string to a double using the fast_float library. This is
 * a C-compatible wrapper around the fast_float parsing functionality, designed
 * to mimic the behavior of the standard strtod function.
 *
 * This function parses the initial portion of the null-terminated string `nptr`
 * and converts it to a `double`, similar to the standard `strtod` function but
 * utilizing the high-performance fast_float library for parsing.
 *
 * On successful conversion, the result is returned. If parsing fails, errno is
 * set to EINVAL and 0.0 is returned.
 *
 * @param nptr   Pointer to the null-terminated string to be parsed.
 * @param endptr If not NULL, a pointer to store the address of the first
 *               character after the parsed number. If parsing fails, it points
 *               to the beginning of the string.
 * @return       The converted double value on success, or 0.0 on failure.
 */
double fast_float_strtod(const char *in, char **out);

#if defined(__cplusplus)
}
#endif

#endif /* FAST_FLOAT_STRTOD_H__ */