#ifndef FASTFLOAT_BINARY_TO_DECIMAL_H
#define FASTFLOAT_BINARY_TO_DECIMAL_H

#include "float_common.h"
#include "fast_table.h"
#include "decimal_to_binary.h"

#include <cfloat>
#include <cinttypes>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <iostream>

namespace fast_float {

namespace detail {
typedef struct {
    uint64_t mantissa_cutoff;
    int16_t power;
} power_conversion;

static const power_conversion power_conversions[] = {
    {0xffffffffffffffff, 16},
    {0xffffffffffffffff, 16},
    {0xffffffffffffffff, 16},
    {0x0014000000000000, 16},
    {0xffffffffffffffff, 15},
    {0xffffffffffffffff, 15},
    {0x0019000000000000, 15},
    {0xffffffffffffffff, 14},
    {0xffffffffffffffff, 14},
    {0x001f400000000000, 14},
    {0xffffffffffffffff, 13},
    {0xffffffffffffffff, 13},
    {0xffffffffffffffff, 13},
    {0x0013880000000000, 13},
    {0xffffffffffffffff, 12},
    {0xffffffffffffffff, 12},
    {0x00186a0000000000, 12},
    {0xffffffffffffffff, 11},
    {0xffffffffffffffff, 11},
    {0x001e848000000000, 11},
    {0xffffffffffffffff, 10},
    {0xffffffffffffffff, 10},
    {0xffffffffffffffff, 10},
    {0x001312d000000000, 10},
    {0xffffffffffffffff, 9},
    {0xffffffffffffffff, 9},
    {0x0017d78400000000, 9},
    {0xffffffffffffffff, 8},
    {0xffffffffffffffff, 8},
    {0x001dcd6500000000, 8},
    {0xffffffffffffffff, 7},
    {0xffffffffffffffff, 7},
    {0xffffffffffffffff, 7},
    {0x0012a05f20000000, 7},
    {0xffffffffffffffff, 6},
    {0xffffffffffffffff, 6},
    {0x00174876e8000000, 6},
    {0xffffffffffffffff, 5},
    {0xffffffffffffffff, 5},
    {0x001d1a94a2000000, 5},
    {0xffffffffffffffff, 4},
    {0xffffffffffffffff, 4},
    {0xffffffffffffffff, 4},
    {0x0012309ce5400000, 4},
    {0xffffffffffffffff, 3},
    {0xffffffffffffffff, 3},
    {0x0016bcc41e900000, 3},
    {0xffffffffffffffff, 2},
    {0xffffffffffffffff, 2},
    {0x001c6bf526340000, 2},
    {0xffffffffffffffff, 1},
    {0xffffffffffffffff, 1},
    {0xffffffffffffffff, 1},
    {0x0011c37937e08000, 1},
    {0xffffffffffffffff, 0},
    {0xffffffffffffffff, 0},
    {0x0016345785d8a000, 0},
    {0xffffffffffffffff, -1},
    {0xffffffffffffffff, -1},
    {0x001bc16d674ec800, -1},
    {0xffffffffffffffff, -2},
    {0xffffffffffffffff, -2},
    {0xffffffffffffffff, -2},
    {0x001158e460913d00, -2},
    {0xffffffffffffffff, -3},
    {0xffffffffffffffff, -3},
    {0x0015af1d78b58c40, -3},
    {0xffffffffffffffff, -4},
    {0xffffffffffffffff, -4},
    {0x001b1ae4d6e2ef50, -4},
    {0xffffffffffffffff, -5},
    {0xffffffffffffffff, -5},
    {0xffffffffffffffff, -5},
    {0x0010f0cf064dd592, -5},
    {0xffffffffffffffff, -6},
    {0xffffffffffffffff, -6},
    {0x00152d02c7e14af6, -6},
    {0xffffffffffffffff, -7},
    {0xffffffffffffffff, -7},
    {0x001a784379d99db4, -7},
    {0xffffffffffffffff, -8},
    {0xffffffffffffffff, -8},
    {0xffffffffffffffff, -8},
    {0x00108b2a2c280291, -8},
    {0xffffffffffffffff, -9},
    {0xffffffffffffffff, -9},
    {0x0014adf4b7320335, -9},
    {0xffffffffffffffff, -10},
    {0xffffffffffffffff, -10},
    {0x0019d971e4fe8402, -10},
    {0xffffffffffffffff, -11},
    {0xffffffffffffffff, -11},
    {0xffffffffffffffff, -11},
    {0x001027e72f1f1281, -11},
    {0xffffffffffffffff, -12},
    {0xffffffffffffffff, -12},
    {0x001431e0fae6d721, -12},
    {0xffffffffffffffff, -13},
    {0xffffffffffffffff, -13},
    {0x00193e5939a08cea, -13},
    {0xffffffffffffffff, -14},
    {0xffffffffffffffff, -14},
    {0x001f8def8808b024, -14},
    {0xffffffffffffffff, -15},
    {0xffffffffffffffff, -15},
    {0xffffffffffffffff, -15},
    {0x0013b8b5b5056e17, -15},
    {0xffffffffffffffff, -16},
    {0xffffffffffffffff, -16},
    {0x0018a6e32246c99c, -16},
    {0xffffffffffffffff, -17},
    {0xffffffffffffffff, -17},
    {0x001ed09bead87c03, -17},
    {0xffffffffffffffff, -18},
    {0xffffffffffffffff, -18},
    {0xffffffffffffffff, -18},
    {0x0013426172c74d82, -18},
    {0xffffffffffffffff, -19},
    {0xffffffffffffffff, -19},
    {0x001812f9cf7920e3, -19},
    {0xffffffffffffffff, -20},
    {0xffffffffffffffff, -20},
    {0x001e17b84357691b, -20},
    {0xffffffffffffffff, -21},
    {0xffffffffffffffff, -21},
    {0xffffffffffffffff, -21},
    {0x0012ced32a16a1b1, -21},
    {0xffffffffffffffff, -22},
    {0xffffffffffffffff, -22},
    {0x00178287f49c4a1d, -22},
    {0xffffffffffffffff, -23},
    {0xffffffffffffffff, -23},
    {0x001d6329f1c35ca5, -23},
    {0xffffffffffffffff, -24},
    {0xffffffffffffffff, -24},
    {0xffffffffffffffff, -24},
    {0x00125dfa371a19e7, -24},
    {0xffffffffffffffff, -25},
    {0xffffffffffffffff, -25},
    {0x0016f578c4e0a061, -25},
    {0xffffffffffffffff, -26},
    {0xffffffffffffffff, -26},
    {0x001cb2d6f618c879, -26},
    {0xffffffffffffffff, -27},
    {0xffffffffffffffff, -27},
    {0xffffffffffffffff, -27},
    {0x0011efc659cf7d4c, -27},
    {0xffffffffffffffff, -28},
    {0xffffffffffffffff, -28},
    {0x00166bb7f0435c9e, -28},
    {0xffffffffffffffff, -29},
    {0xffffffffffffffff, -29},
    {0x001c06a5ec5433c6, -29},
    {0xffffffffffffffff, -30},
    {0xffffffffffffffff, -30},
    {0xffffffffffffffff, -30},
    {0x00118427b3b4a05c, -30},
    {0xffffffffffffffff, -31},
    {0xffffffffffffffff, -31},
    {0x0015e531a0a1c873, -31},
    {0xffffffffffffffff, -32},
    {0xffffffffffffffff, -32},
    {0x001b5e7e08ca3a8f, -32},
    {0xffffffffffffffff, -33},
    {0xffffffffffffffff, -33},
    {0xffffffffffffffff, -33},
    {0x00111b0ec57e649a, -33},
    {0xffffffffffffffff, -34},
    {0xffffffffffffffff, -34},
    {0x001561d276ddfdc0, -34},
    {0xffffffffffffffff, -35},
    {0xffffffffffffffff, -35},
    {0x001aba4714957d30, -35},
    {0xffffffffffffffff, -36},
    {0xffffffffffffffff, -36},
    {0xffffffffffffffff, -36},
    {0x0010b46c6cdd6e3e, -36},
    {0xffffffffffffffff, -37},
    {0xffffffffffffffff, -37},
    {0x0014e1878814c9ce, -37},
    {0xffffffffffffffff, -38},
    {0xffffffffffffffff, -38},
    {0x001a19e96a19fc41, -38},
    {0xffffffffffffffff, -39},
    {0xffffffffffffffff, -39},
    {0xffffffffffffffff, -39},
    {0x00105031e2503da9, -39},
    {0xffffffffffffffff, -40},
    {0xffffffffffffffff, -40},
    {0x0014643e5ae44d13, -40},
    {0xffffffffffffffff, -41},
    {0xffffffffffffffff, -41},
    {0x00197d4df19d6057, -41},
    {0xffffffffffffffff, -42},
    {0xffffffffffffffff, -42},
    {0x001fdca16e04b86d, -42},
    {0xffffffffffffffff, -43},
    {0xffffffffffffffff, -43},
    {0xffffffffffffffff, -43},
    {0x0013e9e4e4c2f344, -43},
    {0xffffffffffffffff, -44}
};

// do better! (todo)
void fast_four_digits(uint16_t value, char *buffer) {
    buffer[0] = char(value / 1000) + '0';
    value %= 1000;
    buffer[1] = char(value / 100) + '0';
    value %= 100;
    buffer[2] = char(value / 10) + '0';
    value %= 10;
    buffer[3] = char(value) + '0';
}
char * fast_dynamic_three_digits(uint16_t value, char *buffer) {
    if(value >= 100) {
        *(buffer++) = char(value / 100)  + '0';
        value %= 100;
        *(buffer++) = char(value / 10)  + '0';
        value %= 10;
        *(buffer++) =char(value) + '0';
    } else if(value >= 10) {
        *(buffer++) = char(value / 10)  + '0';
        value %= 10;
        *(buffer++) =char(value) + '0';
    } else {
        *(buffer++) =char(value) + '0';
    }
    return buffer;
}
} // detail


void serialize(double d, char *buffer) {
    char* orig = buffer;
    // TODO: include integer fast path?
    // Decompose double
    uint64_t bits{0};
    ::memcpy(&bits, &d, sizeof(d));
    uint64_t mantissa = (bits & ((~0ULL) >> 12)) | (1ULL << 52);
    int exp = ((bits >> 52) & 0x7FF) - 1023;

    // Handle uncommon cases here (todo)

    // Handle if negative
    bool isNegative = bits >> 63;
    if (isNegative) {
        buffer[0] = '-';
        buffer++;
    }
    // e.g., 0.232 = 8358680908399641*2**-(55)
    std::cout << "we have "<< mantissa << " * 2^ " << exp << std::endl;

    // Compute product
    detail::power_conversion conversion =  detail::power_conversions[exp];
    int16_t pow10 = conversion.power;
    //pow10+=53-35;
    if (mantissa > conversion.mantissa_cutoff) { pow10--; }
    std::cout << "we have  pow10= "<< pow10 << std::endl;

    // todo: the <64> is weird and probably wrong.
    value128 total_product = compute_product_approximation<64>(pow10, mantissa);
    // todo : handle error
    uint64_t product = total_product.high;
    std::cout << "product is "<< product << std::endl;
    // Write out digits
    uint64_t first = product / 10000000000000000ULL;
    std::cout << "first:" <<first << std::endl;

    product -= first * 10000000000000000ULL;
    std::cout << "first:" <<char('0' + char(first)) << std::endl;
    (*buffer++) = '0' + char(first);
    (*buffer++) = '.';
        std::cout <<"buffer comma="<<orig<<std::endl;

    uint32_t lo8 = (uint32_t)(product % 100000000);
    uint32_t hi8 = (uint32_t)(product / 100000000);
    uint16_t lolo4 = uint16_t(lo8 % 10000);
    uint16_t lohi4 = uint16_t(lo8 / 10000);
    uint16_t hilo4 = uint16_t(hi8 % 10000);
    uint16_t hihi4 = uint16_t(hi8 / 10000);
    std::cout <<"hihi4="<<hihi4<<std::endl;
    detail::fast_four_digits(hihi4, buffer);
    std::cout <<"hihi4text="<<orig<<std::endl;

    std::cout <<"hilo4="<<hilo4<<std::endl;
    detail::fast_four_digits(hilo4, buffer + 4);
    std::cout <<"lohi4="<<lohi4<<std::endl;
    detail::fast_four_digits(lohi4, buffer + 8);
    std::cout <<"lolo4="<<lohi4<<std::endl;
    detail::fast_four_digits(lolo4, buffer + 12);
    std::cout <<"hbufferihi4="<<orig<<std::endl;

    // Remove trailing zeros
    char *mantissa_end = buffer - 1; // Remove '.' if unnecessary
    for (int i = 15; i >= 0; i--) {
        if (buffer[i] != '0') {
            mantissa_end = buffer + i + 1;
            break;
        }
    }
    std::cout << "digits = " <<(mantissa_end - buffer) << std::endl;
    buffer = mantissa_end; // Just overwrite what we had

    // Add exponent
    int32_t target = -pow10 + 17 - 1 /* account for digit to left of decimal */;
    if (target == 0) {
        *buffer = '\0';
        return;
    }
    (*buffer++) = 'e';
    if (target < 0) {
        (*buffer++) = '-';
        target = -target;
    }
    std::cout << "target = " << target << std::endl;
    buffer = detail::fast_dynamic_three_digits(uint16_t(target),buffer);
    *buffer = '\0';

}

} // namespace fast_float

#endif