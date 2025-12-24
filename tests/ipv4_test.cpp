
#include <charconv>
#include <cstdint>
#include <iostream>
#include <algorithm>
#include "fast_float/fast_float.h"

char* uint8_to_chars_manual(char* ptr, uint8_t value) {
    if (value == 0) {
        *ptr++ = '0';
        return ptr;
    }
    char* start = ptr;
    while (value > 0) {
        *ptr++ = '0' + (value % 10);
        value /= 10;
    }
    // Reverse the digits written so far
    std::reverse(start, ptr);
    return ptr;
}

void uint32_to_ipv4_string(uint32_t ip, char* buffer) {
    uint8_t octets[4] = {
        static_cast<uint8_t>(ip >> 24),
        static_cast<uint8_t>(ip >> 16),
        static_cast<uint8_t>(ip >> 8),
        static_cast<uint8_t>(ip)
    };

    char* ptr = buffer;

    for (int i = 0; i < 4; ++i) {
        ptr = uint8_to_chars_manual(ptr, octets[i]);

        if (i < 3) {
            *ptr++ = '.';
        }
    }
    *ptr = '\0';
}

fastfloat_really_inline uint32_t ipv4_string_to_uint32(const char* str, const char* end) {
  uint32_t ip = 0;
  const char* current = str;

  for (int i = 0; i < 4; ++i) {
    uint8_t value;
    auto r = fast_float::from_chars(current, end, value);
    if (r.ec != std::errc()) {
      throw std::invalid_argument("Invalid IP address format");
    }
    current = r.ptr;
    ip = (ip << 8) | value;

    if (i < 3) {
      if (current == end || *current++ != '.') {
        throw std::invalid_argument("Invalid IP address format");
      }
    }
  }
  return ip;
}

bool test_all_ipv4_conversions() {
    std::cout << "Testing all IPv4 conversions... 0, 1000, 2000, 3000, 4000, 5000, 6000, 7000, 8000, 9000, ..." << std::endl;
    char buffer[16];
    for (uint64_t ip = 0; ip <= 0xFFFFFFFF; ip+=1000) {
        if(ip % 10000000 == 0) {
            std::cout << "." << std::flush;
        }
        uint32_to_ipv4_string(static_cast<uint32_t>(ip), buffer);
        const char* end = buffer + strlen(buffer);
        uint32_t parsed_ip = ipv4_string_to_uint32(buffer, end);
        if (parsed_ip != ip) {
            std::cerr << "Mismatch: original " << ip << ", parsed " << parsed_ip << std::endl;
            return false;
        }
    }
    std::cout << std::endl;
    return true;
}

int main() {
    if (test_all_ipv4_conversions()) {
        std::cout << "All IPv4 conversions passed!" << std::endl;
        return EXIT_SUCCESS;
    } else {
        std::cerr << "IPv4 conversion test failed!" << std::endl;
        return EXIT_FAILURE;
    }
}