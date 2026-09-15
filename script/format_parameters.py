#
# Checks the binary_format<T> constants in include/fast_float/float_common.h
# with exact integer arithmetic, for double, float, std::float16_t and
# std::bfloat16_t. Every string is parsed as w * 10^q with w < 10^19, so each
# constant has a condition on (w, q) that it must satisfy.
#
# References:
# Daniel Lemire, Number Parsing at a Gigabyte per Second,
# Software: Practice and Experience 51 (8), 2021 https://arxiv.org/abs/2101.11408
# Noble Mushtak and Daniel Lemire, Fast Number Parsing Without Fallback,
# Software: Practice and Experience 53 (6), 2023 https://arxiv.org/abs/2212.06644
#
# Usage: python3 script/format_parameters.py [--types float16,bfloat16]
#        [--sweep-width 3] [--jobs N]
#

import argparse
import multiprocessing
import os
import re
import sys

HEADER = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..",
                      "include", "fast_float", "float_common.h")
MASK64 = (1 << 64) - 1
MAX_W = 10**19
CXX_TYPES = {"double": "double", "float": "float",
             "float16": "std::float16_t", "bfloat16": "std::bfloat16_t"}


class Format:
    def __init__(self, name):
        self.name = name
        cxx = CXX_TYPES[name]
        with open(HEADER) as f:
            text = f.read()
        pattern = r"binary_format<" + re.escape(cxx) + r">::(\w+)\(\)\s*\{(.*?)\n\}"
        for fn, body in re.findall(pattern, text, re.S):
            returns = re.findall(r"return\s+([^;]+);", body)
            # the last return is the FLT_EVAL_METHOD 0/1 branch
            if returns and re.fullmatch(r"-?(0x[0-9A-Fa-f]+|\d+)", returns[-1].strip()):
                setattr(self, fn, int(returns[-1], 0))
        tables = re.search(r"binary_format_lookup_tables<" + re.escape(cxx) +
                           r", U>\s*\{(.*?)\n\};", text, re.S).group(1)
        powers = re.search(r"powers_of_ten\[\]\s*=\s*\{(.*?)\}", tables, re.S).group(1)
        self.powers_of_ten = [float(x) for x in
                              re.sub(r"(bf16|f16|f)\b", "", powers).split(",") if x.strip()]
        mant = re.search(r"max_mantissa\[\]\s*=\s*\{(.*?)\}", tables, re.S).group(1)
        mant = re.sub(r"//[^\n]*", "", mant).replace("/", "//").replace("\n", " ")
        self.max_mantissa = [eval(x.strip(), {"constant_55555": 5**5})
                             for x in mant.split(",") if x.strip()]
        self.p = self.mantissa_explicit_bits
        self.bias = self.p - self.minimum_exponent
        self.denorm_min_exp = 1 - self.bias  # smallest subnormal is 2^denorm_min_exp
        self.emin = self.minimum_exponent + 1  # leading bit of the smallest normal
        self.emax = self.infinite_power - 1 + self.minimum_exponent

    def grid(self):
        """Every finite value and every midpoint as (num, exp2, kind) with
        kind in value, tie_down (even lower neighbour), tie_up, overflow."""
        out = []
        two_p = 1 << self.p
        d = self.denorm_min_exp
        for m in range(1, two_p):
            out.append((m, d, "value"))
        for m in range(two_p):
            out.append((2 * m + 1, d - 1, "tie_down" if m % 2 == 0 else "tie_up"))
        for e in range(self.emin, self.emax + 1):
            for f in range(two_p):
                out.append((two_p + f, e - self.p, "value"))
                kind = "tie_down" if f % 2 == 0 else "tie_up"
                if e == self.emax and f == two_p - 1:
                    kind = "overflow"
                out.append((2 * (two_p + f) + 1, e - self.p - 1, kind))
        return out


def ratio(num, exp2):
    return (num << exp2, 1) if exp2 >= 0 else (num, 1 << -exp2)


def ratio10(w, q):
    return (w * 10**q, 1) if q >= 0 else (w, 10**-q)


def le(n1, d1, n2, d2):
    return n1 * d2 <= n2 * d1


def floor_log2(n, d):
    e = n.bit_length() - d.bit_length()
    if (d << e if e >= 0 else d) > (n if e >= 0 else n << -e):
        e -= 1
    return e


def round_ties_even(n, d):
    m, r = divmod(n, d)
    if 2 * r > d or (2 * r == d and m & 1):
        m += 1
    return m


def reference_bits(fmt, n, d):
    """Correctly rounded bit pattern of n/d >= 0."""
    if n == 0:
        return 0
    e = floor_log2(n, d)
    s = fmt.denorm_min_exp if e < fmt.emin else e - fmt.p
    m = round_ties_even(n, d << s) if s >= 0 else round_ties_even(n << -s, d)
    if e < fmt.emin:
        return m
    if m == 2 << fmt.p:
        m >>= 1
        e += 1
    if e > fmt.emax:
        return fmt.infinite_power << fmt.p
    return ((e - fmt.minimum_exponent) << fmt.p) | (m - (1 << fmt.p))


def ceil_log2(x):
    z = 0
    while (1 << z) < x:
        z += 1
    return z


def table_entry(q):
    """T[q] as stored in fast_table.h (see table_generation.py)."""
    if q < 0:
        power5 = 5**-q
        z = ceil_log2(power5)
        b = z + 127 if q >= -27 else 2 * z + 128
        c = 2**b // power5 + 1
        while c >= 1 << 128:
            c //= 2
        return c
    power5 = 5**q
    while power5 < 1 << 127:
        power5 *= 2
    while power5 >= 1 << 128:
        power5 //= 2
    return power5


TABLE = {q: table_entry(q) for q in range(-342, 309)}


def compute_product_approximation(q, w, bit_precision):
    t = TABLE[q]
    first = w * (t >> 64)
    high, low = first >> 64, first & MASK64
    if (high & (MASK64 >> bit_precision)) == MASK64 >> bit_precision:
        second_high = (w * (t & MASK64)) >> 64
        low = (low + second_high) & MASK64
        if second_high > low:
            high += 1
    return high, low


def compute_float(fmt, q, w, subnormal_tie_test=True):
    """Model of compute_float in decimal_to_binary.h. Returns
    (mantissa, power2, truncated mantissa, k) with truncated == floor(w 10^q 2^k)."""
    if w == 0 or q < fmt.smallest_power_of_ten:
        return 0, 0, None, None
    if q > fmt.largest_power_of_ten:
        return 0, fmt.infinite_power, None, None
    lz = 64 - w.bit_length()
    w <<= lz
    high, low = compute_product_approximation(q, w, fmt.p + 3)
    upperbit = high >> 63
    shift = upperbit + 64 - fmt.p - 3
    mantissa = high >> shift
    power2 = ((152170 + 65536) * q >> 16) + 63 + upperbit - lz - fmt.minimum_exponent
    tie_q = fmt.min_exponent_round_to_even <= q <= fmt.max_exponent_round_to_even
    if power2 <= 0:
        if -power2 + 1 >= 64:
            return 0, 0, None, None
        extra = -power2 + 1
        mantissa >>= extra
        truncated, k = mantissa, fmt.bias
        if (subnormal_tie_test and low <= 1 and tie_q and (mantissa & 3) == 1
                and ((mantissa << extra) << shift) == high):
            mantissa &= ~1
        mantissa += mantissa & 1
        mantissa >>= 1
        return mantissa, int(mantissa >= 1 << fmt.p), truncated, k
    truncated, k = mantissa, 1 + fmt.bias - power2
    if low <= 1 and tie_q and (mantissa & 3) == 1 and (mantissa << shift) == high:
        mantissa &= ~1
    mantissa += mantissa & 1
    mantissa >>= 1
    if mantissa >= 2 << fmt.p:
        mantissa = 1 << fmt.p
        power2 += 1
    mantissa &= ~(1 << fmt.p)
    if power2 >= fmt.infinite_power:
        return 0, fmt.infinite_power, truncated, k
    return mantissa, power2, truncated, k


def parsed_bits(fmt, w, q, subnormal_tie_test=True):
    """from_chars on the digits w * 10^q. Clinger's fast path is one correctly
    rounded operation on exact operands, so it equals the reference."""
    if fmt.min_exponent_fast_path <= q <= fmt.max_exponent_fast_path and w <= 2 << fmt.p:
        return reference_bits(fmt, *ratio10(w, q)), None, None
    mantissa, power2, truncated, k = compute_float(fmt, q, w, subnormal_tie_test)
    return mantissa | (power2 << fmt.p), truncated, k


failures = 0


def check(ok, text):
    global failures
    print(("  ok    " if ok else "  FAIL  ") + text)
    failures += not ok


def note(text):
    print("  note  " + text)


def continued_fraction(numer, denom):
    cf = []
    while denom:
        quot, rem = divmod(numer, denom)
        cf.append(quot)
        numer, denom = denom, rem
    return cf


def convergents(cf):
    p2, q2, p1, q1 = 0, 1, 1, 0
    for a in cf:
        p1, p2, q1, q2 = a * p1 + p2, p1, a * q1 + q2, q1
        yield p1, q1


def check_constants(fmt):
    half_denorm = ratio(1, fmt.denorm_min_exp - 1)
    q = fmt.smallest_power_of_ten
    check(le(*ratio10(MAX_W - 1, q - 1), *half_denorm),
          "smallest_power_of_ten=%d: q < %d rounds to zero" % (q, q))
    check(not le(*ratio10(MAX_W - 1, q), *half_denorm),
          "smallest_power_of_ten is tight")
    threshold = ratio((4 << fmt.p) - 1, fmt.emax - fmt.p - 1)  # max + ulp/2
    q = fmt.largest_power_of_ten
    check(le(*threshold, *ratio10(1, q + 1)),
          "largest_power_of_ten=%d: q > %d overflows" % (q, q))
    check(not le(*threshold, *ratio10(1, q)), "largest_power_of_ten is tight")

    two_p1 = 2 << fmt.p
    q = fmt.max_exponent_fast_path
    check(5**q <= two_p1 < 5 ** (q + 1),
          "max_exponent_fast_path=%d: 10^q exact in T iff q <= %d" % (q, q))
    check(fmt.powers_of_ten[: q + 1] == [10.0**i for i in range(q + 1)], "powers_of_ten")
    check(fmt.max_mantissa[: q + 1] == [two_p1 // 5**i for i in range(q + 1)],
          "max_mantissa[q] == floor(2^%d / 5^q)" % (fmt.p + 1))
    overflow = not le(two_p1 * 10**q, 1, *ratio(two_p1 - 1, fmt.emax - fmt.p))
    note("fast path product 2^%d * 10^%d %s the largest finite value"
         % (fmt.p + 1, q, "exceeds" if overflow else "is below"))
    k = -fmt.min_exponent_fast_path
    check(0 <= k <= fmt.max_exponent_fast_path, "min_exponent_fast_path=%d" % -k)
    if fmt.p <= 10:
        # double rounding: w / 10^k rounded to `wide` bits, then to T
        for wide in (24, 53, 64):
            bad = 0
            for kk in range(1, fmt.max_exponent_fast_path + 1):
                for w in range(1, two_p1 + 1):
                    e = floor_log2(w, 10**kk)
                    s = e - wide + 1
                    m = round_ties_even(w, 10**kk << s) if s >= 0 else round_ties_even(w << -s, 10**kk)
                    if reference_bits(fmt, *ratio(m, s)) != reference_bits(fmt, w, 10**kk):
                        bad += 1
            check(bad == 0, "w / 10^k, k <= %d, via %d-bit intermediate never double-rounds"
                  % (fmt.max_exponent_fast_path, wide))


def check_ties(fmt):
    """A tie is m * 2^e with m odd. It is w * 10^q, w < 10^19, iff q <= e and
    w = m 5^-q 2^(e-q) (q <= 0) or 5^q | m (q > 0). Ties whose lower neighbour
    is even (m == 1 mod 4) must fall in the round-to-even range."""
    p = fmt.p
    lo, hi = fmt.min_exponent_round_to_even, fmt.max_exponent_round_to_even
    q = 0
    while ((1 << (p + 1)) + 1) * 5 ** (q + 1) < MAX_W:
        q += 1
    normal_lo = -q
    q = 0
    while 5 ** (q + 1) < 1 << (p + 2):
        q += 1
    while True:
        c = 1
        while 5**q * c <= 1 << (p + 1):
            c += 4
        if 5**q * c < 1 << (p + 2) and q <= fmt.emax - p - 1:
            break
        q -= 1
    normal_hi = q
    e = fmt.denorm_min_exp - 1
    sub = []
    q = e
    while 5**-q << (e - q) < MAX_W:
        sub.append(q)
        q -= 1
    check(lo <= normal_lo and hi >= normal_hi,
          "round_to_even range [%d, %d] covers normal ties [%d, %d]" % (lo, hi, normal_lo, normal_hi))
    if sub:
        check(lo <= min(sub), "and the subnormal ties at q in [%d, %d]" % (min(sub), max(sub)))
    else:
        note("no subnormal tie has fewer than 20 digits")
    longest = len(str(((1 << (p + 2)) - 1) * 5**fmt.bias))
    check(fmt.max_digits >= longest, "max_digits=%d >= %d, the longest tie" % (fmt.max_digits, longest))


def check_table(fmt):
    """Facts behind the tie test (product.low <= 1 and dropped bits zero)."""
    q_lo, q_hi = max(fmt.smallest_power_of_ten, -27), -1
    check(fmt.min_exponent_round_to_even >= -27, "tie test stays where 5^-q < 2^64")
    check(all(TABLE[q] & MASK64 for q in range(q_lo, q_hi + 1)),
          "T[q] low word nonzero for %d <= q <= %d" % (q_lo, q_hi))
    fake = False
    for q in range(q_lo, q_hi + 1):
        t_hi = TABLE[q] >> 64
        v2 = (t_hi & -t_hi).bit_length() - 1
        z = ceil_log2(5**-q)
        for shift in (61 - fmt.p, 62 - fmt.p):
            # skipped second product: w * t_hi == 0 or 1 mod 2^(shift+64)
            if shift + 64 - v2 <= 63:
                fake = True
            if v2 == 0 and (1 << 63) <= pow(t_hi, -1, 1 << (shift + 64)) < 1 << 64:
                fake = True
            # computed second product: a non-tie within 2^-62 of a rational
            # with denominator 2^(z-1-shift) is impossible
            if z - 1 - shift > 62:
                fake = True
    check(not fake, "no false tie for %d <= q <= %d" % (q_lo, q_hi))
    bad = False
    for q in range(fmt.smallest_power_of_ten, fmt.largest_power_of_ten + 1):
        for _, w in convergents(continued_fraction(TABLE[q], 2**137)):
            if w >= 2**64:
                break
            if (TABLE[q] * w) % 2**137 > 2**137 - 2**64:
                bad = True
    check(not bad, "Mushtak-Lemire condition for %d <= q <= %d"
          % (fmt.smallest_power_of_ten, fmt.largest_power_of_ten))


def sweep_q(args):
    fmt, grid, q, width = args
    cands = set()
    for num, exp2, _ in grid:
        n, d = ratio(num, exp2)
        if q >= 0:
            d *= 10**q
        else:
            n *= 10**-q
        cands.update(w for w in range(n // d - width, n // d + width + 2) if 1 <= w < MAX_W)
    wrong, wrong_without, bad_trunc = [], 0, 0
    for w in cands:
        n, d = ratio10(w, q)
        want = reference_bits(fmt, n, d)
        got, truncated, k = parsed_bits(fmt, w, q)
        if got != want:
            wrong.append((w, q, got, want))
        if truncated is not None:
            exact = (n << k) // d if k >= 0 else n // (d << -k)
            bad_trunc += truncated != exact
        if parsed_bits(fmt, w, q, False)[0] != want:
            wrong_without += 1
    return len(cands), wrong, wrong_without, bad_trunc


def sweep(fmt, width, jobs):
    """Compare the model with the reference on every w within `width` of
    G / 10^q for every value, tie and the overflow threshold G."""
    grid = fmt.grid()
    qs = range(fmt.smallest_power_of_ten - 1, fmt.largest_power_of_ten + 2)
    tasks = [(fmt, grid, q, width) for q in qs]
    with multiprocessing.Pool(jobs) as pool:
        results = pool.map(sweep_q, tasks)
    wrong = [x for r in results for x in r[1]]
    note("swept %d pairs (w, q)" % sum(r[0] for r in results))
    check(sum(r[3] for r in results) == 0, "truncated mantissa is exact")
    check(not wrong, "compute_float matches the reference")
    for w, q, got, want in sorted(wrong)[:20]:
        print("        w=%d q=%d got 0x%04x want 0x%04x" % (w, q, got, want))
    note("without the subnormal tie test: %d wrong" % sum(r[2] for r in results))


def main():
    ap = argparse.ArgumentParser()
    ap.add_argument("--types", default="double,float,float16,bfloat16")
    ap.add_argument("--sweep-width", type=int, default=3)
    ap.add_argument("--jobs", type=int, default=os.cpu_count() or 1)
    args = ap.parse_args()
    for name in args.types.split(","):
        fmt = Format(name.strip())
        print("== %s" % fmt.name)
        check_constants(fmt)
        check_table(fmt)
        check_ties(fmt)
        if fmt.p <= 10 and args.sweep_width > 0:
            sweep(fmt, args.sweep_width, args.jobs)
    print("%d failure(s)" % failures)
    return 1 if failures else 0


if __name__ == "__main__":
    sys.exit(main())
