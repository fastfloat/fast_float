#ifndef FASTFLOAT_FAST_FLOAT_H
#define FASTFLOAT_FAST_FLOAT_H

#include <system_error>

#include "constexpr_feature_detect.h"

namespace fast_float {
enum chars_format {
    scientific = 1<<0,
    fixed = 1<<2,
    hex = 1<<3,
    general = fixed | scientific
};

enum parse_rules {
    std_rules,
    json_rules,
};

template <typename CharT>
struct from_chars_result {
  const CharT *ptr;
  std::errc ec;
};

struct parse_options {
  constexpr explicit parse_options(
    chars_format fmt = chars_format::general,
    parse_rules rules = parse_rules::std_rules, 
    char dot = '.', bool allow_inf_nan = true)
    : format(fmt), rules(rules), allow_inf_nan(allow_inf_nan), decimal_point(dot) {}

  /** Which number formats are accepted */
  chars_format format;
  /** Which parsing rules to use */
  parse_rules rules;
  /** Whether to allow inf and nan */
  bool allow_inf_nan;
  /** The character used as decimal point */
  char decimal_point;
};

struct preparsed_parse_options {
  constexpr explicit preparsed_parse_options(
    bool allow_inf_nan = true)
    : allow_inf_nan(allow_inf_nan) {}

  constexpr preparsed_parse_options(
    const parse_options& options)
    : allow_inf_nan(options.allow_inf_nan) {}

  /** Whether to allow inf and nan */
  bool allow_inf_nan;
};

/**
 * This function parses the character sequence [first,last) for a number. It parses floating-point numbers expecting
 * a locale-indepent format equivalent to what is used by std::strtod in the default ("C") locale.
 * The resulting floating-point value is the closest floating-point values (using either float or double),
 * using the "round to even" convention for values that would otherwise fall right in-between two values.
 * That is, we provide exact parsing according to the IEEE standard.
 *
 * Given a successful parse, the pointer (`ptr`) in the returned value is set to point right after the
 * parsed number, and the `value` referenced is set to the parsed value. In case of error, the returned
 * `ec` contains a representative error, otherwise the default (`std::errc()`) value is stored.
 *
 * The implementation does not throw and does not allocate memory (e.g., with `new` or `malloc`).
 *
 * Like the C++17 standard, the `fast_float::from_chars` functions take an optional last argument of
 * the type `fast_float::chars_format`. It is a bitset value: we check whether
 * `fmt & fast_float::chars_format::fixed` and `fmt & fast_float::chars_format::scientific` are set
 * to determine whether we allow the fixed point and scientific notation respectively.
 * The default is  `fast_float::chars_format::general` which allows both `fixed` and `scientific`.
 */
template<typename T, typename CharT>
FASTFLOAT_CONSTEXPR20
from_chars_result<CharT> from_chars(const CharT *first, const CharT *last,
                             T &value, chars_format fmt = chars_format::general)  noexcept;

/**
 * Like from_chars, but accepts an `options` argument to govern number parsing.
 */
template<typename T, typename CharT>
FASTFLOAT_CONSTEXPR20
from_chars_result<CharT> from_chars_advanced(const CharT *first, const CharT *last,
                                      T &value, parse_options options)  noexcept;

} 

#include "ascii_number.h" // parsed_number_string

namespace fast_float {
template <typename T, typename CharT>
FASTFLOAT_CONSTEXPR20
from_chars_result<CharT> from_chars_preparsed(parsed_number_string<CharT> parsed, 
    const CharT* first, const CharT* last, T& value, preparsed_parse_options options) noexcept;
}

// namespace fast_float
#include "parse_number.h"
#endif // FASTFLOAT_FAST_FLOAT_H
