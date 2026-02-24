#include "fast_float/fast_float_strtod.h"
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

int main() {
    // Test successful conversion
    const char *str1 = "3.14159";
    char *end1;
    errno = 0;
    double d1 = fast_float_strtod(str1, &end1);
    printf("Input: %s\n", str1);
    printf("Converted: %f\n", d1);
    printf("End pointer: %s\n", end1);
    printf("errno: %d\n", errno);

    // Test invalid input
    const char *str2 = "invalid";
    char *end2;
    errno = 0;
    double d2 = fast_float_strtod(str2, &end2);
    printf("\nInput: %s\n", str2);
    printf("Converted: %f\n", d2);
    printf("End pointer: %s\n", end2);
    printf("errno: %d\n", errno);

    return 0;
}