#ifndef FASTFLOAT_CONSTEXPR_FEATURE_DETECT_H
#define FASTFLOAT_CONSTEXPR_FEATURE_DETECT_H

#ifdef __has_include
#if __has_include(<version>)
#include <version>
#endif
#endif

// C++14 constexpr
#if defined(__cpp_constexpr) && __cpp_constexpr >= 201304L
#define FASTFLOAT_CONSTEXPR14 constexpr
#elif __cplusplus >= 201402L
#define FASTFLOAT_CONSTEXPR14 constexpr
#elif defined(_MSC_VER) && _MSC_VER >= 1910 && _MSVC_LANG >= 201402L
#define FASTFLOAT_CONSTEXPR14 constexpr
#else
#define FASTFLOAT_CONSTEXPR14
#endif

// C++14 variable templates
#if defined(__cpp_variable_templates) && __cpp_variable_templates >= 201304L
#define FASTFLOAT_HAS_VARIABLE_TEMPLATES 1
#elif __cplusplus >= 201402L
#define FASTFLOAT_HAS_VARIABLE_TEMPLATES 1
#elif defined(_MSC_FULL_VER) && _MSC_FULL_VER >= 190023918L &&                 \
    _MSVC_LANG >= 201402L
#define FASTFLOAT_HAS_VARIABLE_TEMPLATES 1
#else
#define FASTFLOAT_HAS_VARIABLE_TEMPLATES 0
#endif

// C++20 std::bit_cast
#if defined(__cpp_lib_bit_cast) && __cpp_lib_bit_cast >= 201806L
#define FASTFLOAT_HAS_BIT_CAST 1
#else
#define FASTFLOAT_HAS_BIT_CAST 0
#endif

#if defined(__cpp_lib_is_constant_evaluated) &&                                \
    __cpp_lib_is_constant_evaluated >= 201811L
#define FASTFLOAT_HAS_IS_CONSTANT_EVALUATED 1
#define FASTFLOAT_CONSTEVAL consteval
#else
#define FASTFLOAT_HAS_IS_CONSTANT_EVALUATED 0
#define FASTFLOAT_CONSTEVAL FASTFLOAT_CONSTEXPR14
#endif

#if defined(__cpp_lib_byteswap)
#define FASTFLOAT_HAS_BYTESWAP 1
#else
#define FASTFLOAT_HAS_BYTESWAP 0
#endif

// C++17 if constexpr
#if defined(__cpp_if_constexpr) && __cpp_if_constexpr >= 201606L
#define FASTFLOAT_IF_CONSTEXPR17(x) if constexpr (x)
#elif defined(__cpp_constexpr) && __cpp_constexpr >= 201603L
#define FASTFLOAT_IF_CONSTEXPR17(x) if constexpr (x)
#elif __cplusplus >= 201703L
#define FASTFLOAT_IF_CONSTEXPR17(x) if constexpr (x)
#elif defined(_MSC_VER) && _MSC_VER >= 1911 && _MSVC_LANG >= 201703L
#define FASTFLOAT_IF_CONSTEXPR17(x) if constexpr (x)
#else
#define FASTFLOAT_IF_CONSTEXPR17(x) if (x)
#endif

// C++17 inline variables
#if defined(__cpp_inline_variables) && __cpp_inline_variables >= 201606L
#define FASTFLOAT_INLINE_VARIABLE inline constexpr
#elif __cplusplus >= 201703L
#define FASTFLOAT_INLINE_VARIABLE inline constexpr
#elif defined(_MSC_VER) && _MSC_VER >= 1912 && _MSVC_LANG >= 201703L
#define FASTFLOAT_INLINE_VARIABLE inline constexpr
#else
#define FASTFLOAT_INLINE_VARIABLE static constexpr
#endif

// Testing for relevant C++20 constexpr library features
#if FASTFLOAT_HAS_IS_CONSTANT_EVALUATED && FASTFLOAT_HAS_BIT_CAST &&           \
    defined(__cpp_lib_constexpr_algorithms) &&                                 \
    __cpp_lib_constexpr_algorithms >= 201806L /*For std::copy and std::fill*/
#define FASTFLOAT_CONSTEXPR20 constexpr
#define FASTFLOAT_IS_CONSTEXPR 1
#else
#define FASTFLOAT_CONSTEXPR20
#define FASTFLOAT_IS_CONSTEXPR 0
#endif

#if __cplusplus >= 201703L || (defined(_MSVC_LANG) && _MSVC_LANG >= 201703L)
#define FASTFLOAT_DETAIL_MUST_DEFINE_CONSTEXPR_VARIABLE 0
#else
#define FASTFLOAT_DETAIL_MUST_DEFINE_CONSTEXPR_VARIABLE 1
#endif

#if defined(__has_builtin)
#define FASTFLOAT_HAS_BUILTIN(x) __has_builtin(x)
#else
#define FASTFLOAT_HAS_BUILTIN(x) false
#endif

// For support attribute [[assume]] is declared in P1774
#if defined(__cpp_attrubute_assume)
#define FASTFLOAT_ASSUME(expr) [[assume(expr)]]
#else
#define FASTFLOAT_ASSUME(expr)
#endif

#endif // FASTFLOAT_CONSTEXPR_FEATURE_DETECT_H
