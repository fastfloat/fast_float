#ifndef FASTFLOAT_CONSTEXPR_FEATURE_DETECT_H
#define FASTFLOAT_CONSTEXPR_FEATURE_DETECT_H

#ifdef _MSVC_LANG
#define FASTFLOAT_CPLUSPLUS _MSVC_LANG
#else
#define FASTFLOAT_CPLUSPLUS __cplusplus
#endif

// Detect __has_*.
#ifdef __has_feature
#define FASTFLOAT_HAS_FEATURE(x) __has_feature(x)
#else
#define FASTFLOAT_HAS_FEATURE(x) 0
#endif
#ifdef __has_include
#define FASTFLOAT_HAS_INCLUDE(x) __has_include(x)
#else
#define FASTFLOAT_HAS_INCLUDE(x) 0
#endif
#ifdef __has_builtin
#define FASTFLOAT_HAS_BUILTIN(x) __has_builtin(x)
#else
#define FASTFLOAT_HAS_BUILTIN(x) 0
#endif
#ifdef __has_cpp_attribute
#define FASTFLOAT_HAS_CPP_ATTRIBUTE(x) __has_cpp_attribute(x)
#else
#define FASTFLOAT_HAS_CPP_ATTRIBUTE(x) 0
#endif

#if FASTFLOAT_HAS_INCLUDE(<version>)
#include <version>
#endif

// C++14 constexpr
#if defined(__cpp_constexpr) && __cpp_constexpr >= 201304L
#define FASTFLOAT_CONSTEXPR14 constexpr
#elif FASTFLOAT_CPLUSPLUS >= 201402L
#define FASTFLOAT_CONSTEXPR14 constexpr
#elif defined(_MSC_VER) && _MSC_VER >= 1910 && _MSVC_LANG >= 201402L
#define FASTFLOAT_CONSTEXPR14 constexpr
#else
#define FASTFLOAT_CONSTEXPR14
#endif

// C++14 variable templates
#if defined(__cpp_variable_templates) && __cpp_variable_templates >= 201304L
#define FASTFLOAT_HAS_VARIABLE_TEMPLATES 1
#elif FASTFLOAT_CPLUSPLUS >= 201402L
#define FASTFLOAT_HAS_VARIABLE_TEMPLATES 1
#else
#define FASTFLOAT_HAS_VARIABLE_TEMPLATES 0
#endif

// C++17 if constexpr
#if defined(__cpp_if_constexpr) && __cpp_if_constexpr >= 201606L
#define FASTFLOAT_CONSTEXPR17 constexpr
#elif defined(__cpp_constexpr) && __cpp_constexpr >= 201603L
#define FASTFLOAT_CONSTEXPR17 constexpr
#elif FASTFLOAT_CPLUSPLUS >= 201703L
#define FASTFLOAT_CONSTEXPR17 constexpr
#else
#define FASTFLOAT_CONSTEXPR17
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

// Before C++17 constexpr variables may need an out-of-class definition.
#if FASTFLOAT_CPLUSPLUS >= 201703L
#define FASTFLOAT_DETAIL_MUST_DEFINE_CONSTEXPR_VARIABLE 0
#else
#define FASTFLOAT_DETAIL_MUST_DEFINE_CONSTEXPR_VARIABLE 1
#endif

// C++20 std::bit_cast
#if defined(__cpp_lib_bit_cast) && __cpp_lib_bit_cast >= 201806L
#define FASTFLOAT_HAS_BIT_CAST 1
#else
#define FASTFLOAT_HAS_BIT_CAST 0
#endif

// C++20 constexpr library support, including std::is_constant_evaluated(),
// std::bit_cast(), std::copy and std::fill, and constexpr algorithms. Therefore
// all required library feature macros must be present.
#if defined(__cpp_lib_is_constant_evaluated) &&                                \
    __cpp_lib_is_constant_evaluated >= 201811L && defined(__cpp_consteval) &&  \
    __cpp_consteval >= 201811L && FASTFLOAT_HAS_BIT_CAST
#define FASTFLOAT_CONSTEVAL consteval
#define FASTFLOAT_CONSTEXPR20 constexpr
#define FASTFLOAT_IS_CONSTEXPR 1
#define FASTFLOAT_HAS_IS_CONSTANT_EVALUATED 1
#else
#define FASTFLOAT_CONSTEVAL constexpr
#define FASTFLOAT_CONSTEXPR20
#define FASTFLOAT_IS_CONSTEXPR 0
#define FASTFLOAT_HAS_IS_CONSTANT_EVALUATED 0
#endif

// C++20 std::byteswap
#if defined(__cpp_lib_byteswap) && __cpp_lib_byteswap >= 202110L
#define FASTFLOAT_HAS_BYTESWAP 1
#else
#define FASTFLOAT_HAS_BYTESWAP 0
#endif

#ifdef FASTFLOAT_ASSUME
// Use the provided definition.
#elif FASTFLOAT_HAS_CPP_ATTRIBUTE(assume) && !defined(__GNUC__)
#define FASTFLOAT_ASSUME(expr) [[assume(expr)]]
#else
#define FASTFLOAT_ASSUME(expr)
#endif

#ifdef FASTFLOAT_NO_UNIQUE_ADDRESS
// Use the provided definition.
#elif FASTFLOAT_HAS_CPP_ATTRIBUTE(no_unique_address)
#define FASTFLOAT_NO_UNIQUE_ADDRESS [[no_unique_address]]
// VS2019 v16.10 and later except clang-cl (https://reviews.llvm.org/D110485).
#elif defined(_MSC_VER) && _MSC_VER >= 1929 && !defined(__clang__)
#define FASTFLOAT_NO_UNIQUE_ADDRESS [[msvc::no_unique_address]]
#else
#define FASTFLOAT_NO_UNIQUE_ADDRESS
#endif

#endif // FASTFLOAT_CONSTEXPR_FEATURE_DETECT_H
