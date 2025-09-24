#ifndef FASTFLOAT_DIGIT_COMPARISON_H
#define FASTFLOAT_DIGIT_COMPARISON_H

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <iterator>

#include "float_common.h"
#include "bigint.h"
#include "ascii_number.h"

namespace fast_float {

// 1e0 to 1e19
constexpr static uint64_t powers_of_ten_uint64[] = {1UL,
                                                    10UL,
                                                    100UL,
                                                    1000UL,
                                                    10000UL,
                                                    100000UL,
                                                    1000000UL,
                                                    10000000UL,
                                                    100000000UL,
                                                    1000000000UL,
                                                    10000000000UL,
                                                    100000000000UL,
                                                    1000000000000UL,
                                                    10000000000000UL,
                                                    100000000000000UL,
                                                    1000000000000000UL,
                                                    10000000000000000UL,
                                                    100000000000000000UL,
                                                    1000000000000000000UL,
                                                    10000000000000000000UL};

// calculate the exponent, in scientific notation, of the number.
// this algorithm is not even close to optimized, but it has no practical
// effect on performance: in order to have a faster algorithm, we'd need
// to slow down performance for faster algorithms, and this is still fast.
template <typename UC>
fastfloat_really_inline FASTFLOAT_CONSTEXPR14 am_pow_t
scientific_exponent(parsed_number_string_t<UC> const &num) noexcept {
  am_mant_t mantissa = num.mantissa;
  am_pow_t exponent = num.exponent;
  while (mantissa >= 10000) {
    mantissa /= 10000;
    exponent += 4;
  }
  while (mantissa >= 100) {
    mantissa /= 100;
    exponent += 2;
  }
  while (mantissa >= 10) {
    mantissa /= 10;
    exponent += 1;
  }
  return exponent;
}

// this converts a native floating-point number to an extended-precision float.
template <typename T>
fastfloat_really_inline FASTFLOAT_CONSTEXPR20 adjusted_mantissa
to_extended(T const &value) noexcept {
  using equiv_uint = equiv_uint_t<T>;
  constexpr equiv_uint exponent_mask = binary_format<T>::exponent_mask();
  constexpr equiv_uint mantissa_mask = binary_format<T>::mantissa_mask();
  constexpr equiv_uint hidden_bit_mask = binary_format<T>::hidden_bit_mask();

  adjusted_mantissa am;
  constexpr am_pow_t bias = binary_format<T>::mantissa_explicit_bits() -
                            binary_format<T>::minimum_exponent();
  equiv_uint bits;
#if FASTFLOAT_HAS_BIT_CAST
  bits =
#if FASTFLOAT_HAS_BIT_CAST == 1
      std::
#endif
          bit_cast<equiv_uint>(value);
#else
  ::memcpy(&bits, &value, sizeof(T));
#endif
  if ((bits & exponent_mask) == 0) {
    // denormal
    am.power2 = 1 - bias;
    am.mantissa = bits & mantissa_mask;
  } else {
    // normal
    am.power2 = am_pow_t((bits & exponent_mask) >>
                         binary_format<T>::mantissa_explicit_bits());
    am.power2 -= bias;
    am.mantissa = (bits & mantissa_mask) | hidden_bit_mask;
  }

  return am;
}

// get the extended precision value of the halfway point between b and b+u.
// we are given a native float that represents b, so we need to adjust it
// halfway between b and b+u.
template <typename T>
fastfloat_really_inline FASTFLOAT_CONSTEXPR20 adjusted_mantissa
to_extended_halfway(T const &value) noexcept {
  adjusted_mantissa am = to_extended(value);
  am.mantissa <<= 1;
  am.mantissa += 1;
  am.power2 -= 1;
  return am;
}

// round an extended-precision float to the nearest machine float.
template <typename T, typename callback>
fastfloat_really_inline FASTFLOAT_CONSTEXPR14 void round(adjusted_mantissa &am,
                                                         callback cb) noexcept {
  constexpr am_pow_t mantissa_shift =
      64 - binary_format<T>::mantissa_explicit_bits() - 1;
  if (-am.power2 >= mantissa_shift) {
    // have a denormal float
    am_pow_t shift = -am.power2 + 1;
    cb(am, std::min<am_pow_t>(shift, 64));
    // check for round-up: if rounding-nearest carried us to the hidden bit.
    am.power2 = (am.mantissa <
                 (am_mant_t(1) << binary_format<T>::mantissa_explicit_bits()))
                    ? 0
                    : 1;
    return;
  }

  // have a normal float, use the default shift.
  cb(am, mantissa_shift);

  // check for carry
  if (am.mantissa >=
      (am_mant_t(2) << binary_format<T>::mantissa_explicit_bits())) {
    am.mantissa = (am_mant_t(1) << binary_format<T>::mantissa_explicit_bits());
    ++am.power2;
  }

  // check for infinite: we could have carried to an infinite power
  am.mantissa &= ~(am_mant_t(1) << binary_format<T>::mantissa_explicit_bits());
  if (am.power2 >= binary_format<T>::infinite_power()) {
    am.power2 = binary_format<T>::infinite_power();
    am.mantissa = 0;
  }
}

template <typename callback>
fastfloat_really_inline FASTFLOAT_CONSTEXPR14 void
round_nearest_tie_even(adjusted_mantissa &am, am_pow_t shift,
                       callback cb) noexcept {
  am_mant_t const mask =
      (shift == 64) ? UINT64_MAX : (am_mant_t(1) << shift) - 1;
  am_mant_t const halfway = (shift == 0) ? 0 : am_mant_t(1) << (shift - 1);
  am_mant_t truncated_bits = am.mantissa & mask;
  bool is_above = truncated_bits > halfway;
  bool is_halfway = truncated_bits == halfway;

  // shift digits into position
  if (shift == 64) {
    am.mantissa = 0;
  } else {
    am.mantissa >>= shift;
  }
  am.power2 += shift;

  bool is_odd = (am.mantissa & 1) == 1;
  am.mantissa += am_mant_t(cb(is_odd, is_halfway, is_above));
}

fastfloat_really_inline FASTFLOAT_CONSTEXPR14 void
round_down(adjusted_mantissa &am, am_pow_t shift) noexcept {
  if (shift == 64) {
    am.mantissa = 0;
  } else {
    am.mantissa >>= shift;
  }
  am.power2 += shift;
}

template <typename UC>
fastfloat_really_inline FASTFLOAT_CONSTEXPR20 void
skip_zeros(UC const *&first, UC const *last) noexcept {
  uint64_t val;
  while (!cpp20_and_in_constexpr() &&
         std::distance(first, last) >= int_cmp_len<UC>()) {
    ::memcpy(&val, first, sizeof(uint64_t));
    if (val != int_cmp_zeros<UC>()) {
      break;
    }
    first += int_cmp_len<UC>();
  }
  while (first != last) {
    if (*first != UC('0')) {
      break;
    }
    first++;
  }
}

// determine if any non-zero digits were truncated.
// all characters must be valid digits.
template <typename UC>
fastfloat_really_inline FASTFLOAT_CONSTEXPR20 bool
is_truncated(UC const *first, UC const *last) noexcept {
  // do 8-bit optimizations, can just compare to 8 literal 0s.
  uint64_t val;
  while (!cpp20_and_in_constexpr() &&
         std::distance(first, last) >= int_cmp_len<UC>()) {
    ::memcpy(&val, first, sizeof(uint64_t));
    if (val != int_cmp_zeros<UC>()) {
      return true;
    }
    first += int_cmp_len<UC>();
  }
  while (first != last) {
    if (*first != UC('0')) {
      return true;
    }
    ++first;
  }
  return false;
}

template <typename UC>
fastfloat_really_inline FASTFLOAT_CONSTEXPR20 bool
is_truncated(span<UC const> s) noexcept {
  return is_truncated(s.ptr, s.ptr + s.len());
}

template <typename UC>
fastfloat_really_inline FASTFLOAT_CONSTEXPR20 void
parse_eight_digits(UC const *&p, limb &value, am_digits &counter,
                   am_digits &count) noexcept {
  value = value * 100000000 + parse_eight_digits_unrolled(p);
  p += 8;
  counter += 8;
  count += 8;
}

template <typename UC>
fastfloat_really_inline FASTFLOAT_CONSTEXPR14 void
parse_one_digit(UC const *&p, limb &value, am_digits &counter,
                am_digits &count) noexcept {
  value = value * 10 + limb(*p - UC('0'));
  ++p;
  ++counter;
  ++count;
}

fastfloat_really_inline FASTFLOAT_CONSTEXPR20 void
add_native(bigint &big, limb power, limb value) noexcept {
  big.mul(power);
  big.add(value);
}

fastfloat_really_inline FASTFLOAT_CONSTEXPR20 void
round_up_bigint(bigint &big, am_digits &count) noexcept {
  // need to round-up the digits, but need to avoid rounding
  // ....9999 to ...10000, which could cause a false halfway point.
  add_native(big, 10, 1);
  ++count;
}

// parse the significant digits into a big integer
template <typename T, typename UC>
inline FASTFLOAT_CONSTEXPR20 am_digits
parse_mantissa(bigint &result, const parsed_number_string_t<UC> &num) noexcept {
  // try to minimize the number of big integer and scalar multiplication.
  // therefore, try to parse 8 digits at a time, and multiply by the largest
  // scalar value (9 or 19 digits) for each step.
  constexpr am_digits max_digits = binary_format<T>::max_digits();
  am_digits counter = 0;
  am_digits digits = 0;
  limb value = 0;
#ifdef FASTFLOAT_64BIT_LIMB
  am_digits const step = 19;
#else
  am_digits const step = 9;
#endif

  // process all integer digits.
  UC const *p = num.integer.ptr;
  UC const *pend = p + num.integer.len();
  skip_zeros(p, pend);
  // process all digits, in increments of step per loop
  while (p != pend) {
    while ((std::distance(p, pend) >= 8) && (step - counter >= 8) &&
           (max_digits - digits >= 8)) {
      parse_eight_digits<UC>(p, value, counter, digits);
    }
    while (counter < step && p != pend && digits < max_digits) {
      parse_one_digit<UC>(p, value, counter, digits);
    }
    if (digits == max_digits) {
      // add the temporary value, then check if we've truncated any digits
      add_native(result, limb(powers_of_ten_uint64[counter]), value);
      bool truncated = is_truncated(p, pend);
      if (num.fraction.ptr != nullptr) {
        truncated |= is_truncated(num.fraction);
      }
      if (truncated) {
        round_up_bigint(result, digits);
      }
      return digits;
    } else {
      add_native(result, limb(powers_of_ten_uint64[counter]), value);
      counter = 0;
      value = 0;
    }
  }

  // add our fraction digits, if they're available.
  if (num.fraction.ptr != nullptr) {
    p = num.fraction.ptr;
    pend = p + num.fraction.len();
    if (digits == 0) {
      skip_zeros(p, pend);
    }
    // process all digits, in increments of step per loop
    while (p != pend) {
      while ((std::distance(p, pend) >= 8) && (step - counter >= 8) &&
             (max_digits - digits >= 8)) {
        parse_eight_digits<UC>(p, value, counter, digits);
      }
      while (counter < step && p != pend && digits < max_digits) {
        parse_one_digit<UC>(p, value, counter, digits);
      }
      if (digits == max_digits) {
        // add the temporary value, then check if we've truncated any digits
        add_native(result, limb(powers_of_ten_uint64[counter]), value);
        bool truncated = is_truncated(p, pend);
        if (truncated) {
          round_up_bigint(result, digits);
        }
        return digits;
      } else {
        add_native(result, limb(powers_of_ten_uint64[counter]), value);
        counter = 0;
        value = 0;
      }
    }
  }

  if (counter != 0) {
    add_native(result, limb(powers_of_ten_uint64[counter]), value);
  }
  return digits;
}

template <typename T>
inline FASTFLOAT_CONSTEXPR20 adjusted_mantissa positive_digit_comp(
    bigint &bigmant, adjusted_mantissa am, am_pow_t const exponent) noexcept {
  FASTFLOAT_ASSERT(bigmant.pow10(exponent));
  bool truncated;
  am.mantissa = bigmant.hi64(truncated);
  constexpr am_pow_t bias = binary_format<T>::mantissa_explicit_bits() -
                            binary_format<T>::minimum_exponent();
  am.power2 =
      static_cast<fast_float::am_pow_t>(bigmant.bit_length() - 64 + bias);

  round<T>(am, [truncated](adjusted_mantissa &a, am_pow_t shift) {
    round_nearest_tie_even(
        a, shift,
        [truncated](bool is_odd, bool is_halfway, bool is_above) -> bool {
          return is_above || (is_halfway && truncated) ||
                 (is_odd && is_halfway);
        });
  });

  return am;
}

// the scaling here is quite simple: we have, for the real digits `m * 10^e`,
// and for the theoretical digits `n * 2^f`. Since `e` is always negative,
// to scale them identically, we do `n * 2^f * 5^-f`, so we now have `m * 2^e`.
// we then need to scale by `2^(f- e)`, and then the two significant digits
// are of the same magnitude.
template <typename T>
inline FASTFLOAT_CONSTEXPR20 adjusted_mantissa negative_digit_comp(
    bigint &bigmant, adjusted_mantissa am, am_pow_t const exponent) noexcept {
  bigint &real_digits = bigmant;
  am_pow_t const &real_exp = exponent;

  T b;
  {
    // get the value of `b`, rounded down, and get a bigint representation of
    // b+h
    adjusted_mantissa am_b = am;
    // gcc7 bug: use a lambda to remove the noexcept qualifier bug with
    // -Wnoexcept-type.
    round<T>(am_b, [](adjusted_mantissa &a, am_pow_t shift) {
      round_down(a, shift);
    });
    to_float(
#ifndef FASTFLOAT_ONLY_POSITIVE_C_NUMBER_WO_INF_NAN
        false,
#endif
        am_b, b);
  }
  adjusted_mantissa theor = to_extended_halfway(b);
  bigint theor_digits(theor.mantissa);
  am_pow_t theor_exp = theor.power2;

  // scale real digits and theor digits to be same power.
  am_pow_t pow2_exp = theor_exp - real_exp;
  am_pow_t pow5_exp = -real_exp;
  if (pow5_exp != 0) {
    FASTFLOAT_ASSERT(theor_digits.pow5(pow5_exp));
  }
  if (pow2_exp > 0) {
    FASTFLOAT_ASSERT(theor_digits.pow2(pow2_exp));
  } else if (pow2_exp < 0) {
    FASTFLOAT_ASSERT(real_digits.pow2(-pow2_exp));
  }

  // compare digits, and use it to director rounding
  int ord = real_digits.compare(theor_digits);
  round<T>(am, [ord](adjusted_mantissa &a, am_pow_t shift) {
    round_nearest_tie_even(
        a, shift, [ord](bool is_odd, bool _, bool __) -> bool {
          (void)_;  // not needed, since we've done our comparison
          (void)__; // not needed, since we've done our comparison
          if (ord > 0) {
            return true;
          } else if (ord < 0) {
            return false;
          } else {
            return is_odd;
          }
        });
  });

  return am;
}

// parse the significant digits as a big integer to unambiguously round the
// the significant digits. here, we are trying to determine how to round
// an extended float representation close to `b+h`, halfway between `b`
// (the float rounded-down) and `b+u`, the next positive float. this
// algorithm is always correct, and uses one of two approaches. when
// the exponent is positive relative to the significant digits (such as
// 1234), we create a big-integer representation, get the high 64-bits,
// determine if any lower bits are truncated, and use that to direct
// rounding. in case of a negative exponent relative to the significant
// digits (such as 1.2345), we create a theoretical representation of
// `b` as a big-integer type, scaled to the same binary exponent as
// the actual digits. we then compare the big integer representations
// of both, and use that to direct rounding.
template <typename T, typename UC>
inline FASTFLOAT_CONSTEXPR20 adjusted_mantissa digit_comp(
    parsed_number_string_t<UC> const &num, adjusted_mantissa am) noexcept {
  // remove the invalid exponent bias
  am.power2 -= invalid_am_bias;

  bigint bigmant;
  am_pow_t const sci_exp = scientific_exponent(num);

  am_digits const digits = parse_mantissa<T, UC>(bigmant, num);
  // can't underflow, since digits is at most max_digits.
  am_pow_t const exponent =
      static_cast<am_pow_t>(sci_exp + 1 - static_cast<am_pow_t>(digits));
  if (exponent >= 0) {
    return positive_digit_comp<T>(bigmant, am, exponent);
  } else {
    return negative_digit_comp<T>(bigmant, am, exponent);
  }
}

} // namespace fast_float

#endif
