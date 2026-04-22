#ifndef __ALIGN_H__
#define __ALIGN_H__

#ifdef alignas
#undef alignas
#endif
#ifdef _Alignas
#undef _Alignas
#endif
#ifdef alignof
#undef alignof
#endif
#ifdef _Alignof
#undef _Alignof
#endif

#if !defined(__cplusplus) || (__cplusplus < 201103L)
#if defined(__GNUC__) || defined(__clang__) || defined(__INTEL_COMPILER)
#define alignas(x) __attribute__((aligned(x)))
#define _Alignas alignas
#define alignof(type) __alignof__(type)
#define _Alignof alignof
#elif defined(_MSC_VER)
#define alignas(x) __declspec(align(x))
#define _Alignas alignas
#pragma message("alignof and _Alignof are not directly supported by MSVC before C++11.")
#define alignof(type)
#define _Alignof alignof
#else
#pragma message("This compiler does not support alignment features.")
#define alignas(x)
#define _Alignas
#define alignof(type)
#define _Alignof
#endif
#else
#if defined(__GNUC__) || defined(__clang__) || defined(__INTEL_COMPILER)
#define alignas(x) alignas(x)
#define _Alignas alignas
#define alignof(type) alignof(type)
#define _Alignof alignof
#elif defined(_MSC_VER)
#define alignas(x) alignas(x)
#define _Alignas alignas
#define alignof(type) alignof(type)
#define _Alignof alignof
#else
#pragma message("This compiler does not support C++11 alignment features.")
#define alignas(x)
#define _Alignas
#define alignof(type)
#define _Alignof
#endif
#endif

#endif /* __ALIGN_H__ */