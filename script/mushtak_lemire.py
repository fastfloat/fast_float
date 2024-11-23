#
# Reference :
# Noble Mushtak and Daniel Lemire, Fast Number Parsing Without Fallback (to appear)
#

all_tqs = []

# Generates all possible values of T[q]
# Appendix B of  Number parsing at a gigabyte per second.
# Software: Practice and Experience 2021;51(8):1700–1727.
for q in range(-342, -27):
    power5 = 5 ** -q
    z = 0
    while (1 << z) < power5:
        z += 1
    b = 2 * z + 2 * 64
    c = 2 ** b // power5 + 1
    while c >= (1 << 128):
        c //= 2
    all_tqs.append(c)
for q in range(-27, 0):
    power5 = 5 ** -q
    z = 0
    while (1 << z) < power5:
        z += 1
    b = z + 127
    c = 2 ** b // power5 + 1
    all_tqs.append(c)
for q in range(0, 308 + 1):
    power5 = 5 ** q
    while power5 < (1 << 127):
        power5 *= 2
    while power5 >= (1 << 128):
        power5 //= 2
    all_tqs.append(power5)

# Returns the continued fraction of numer/denom as a list [a0; a1, a2, ..., an]
def continued_fraction(numer, denom):
    # (look at page numbers in top-left, not PDF page numbers)
    cf = []
    while denom != 0:
        quot, rem = divmod(numer, denom)
        cf.append(quot)
        numer, denom = denom, rem
    return cf


# Given a continued fraction [a0; a1, a2, ..., an], returns
# all the convergents of that continued fraction
# as pairs of the form (numer, denom), where numer/denom is
# a convergent of the continued fraction in simple form.
def convergents(cf):
    p_n_minus_2 = 0
    q_n_minus_2 = 1
    p_n_minus_1 = 1
    q_n_minus_1 = 0
    convergents = []
    for a_n in cf:
        p_n = a_n * p_n_minus_1 + p_n_minus_2
        q_n = a_n * q_n_minus_1 + q_n_minus_2
        convergents.append((p_n, q_n))
        p_n_minus_2, q_n_minus_2, p_n_minus_1, q_n_minus_1 = (
            p_n_minus_1,
            q_n_minus_1,
            p_n,
            q_n,
        )
    return convergents


# Enumerate through all the convergents of T[q] / 2^137 with denominators < 2^64
found_solution = False
for j, tq in enumerate(all_tqs):
    for _, w in convergents(continued_fraction(tq, 2 ** 137)):
        if w >= 2 ** 64:
            break
            if (tq * w) % 2 ** 137 > 2 ** 137 - 2 ** 64:
                print(f"SOLUTION: q={j-342} T[q]={tq} w={w}")
                found_solution = True
if not found_solution:
    print("No solutions!")
