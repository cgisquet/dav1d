/*
 * Copyright © 2018, VideoLAN and dav1d authors
 * Copyright © 2018, Two Orioles, LLC
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this
 *    list of conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice,
 *    this list of conditions and the following disclaimer in the documentation
 *    and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR
 * ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef __DAV1D_COMMON_ATTRIBUTES_H__
#define __DAV1D_COMMON_ATTRIBUTES_H__

#include "config.h"

#include <stddef.h>

#ifdef __GNUC__
#define ATTR_ALIAS __attribute__((may_alias))
#else
#define ATTR_ALIAS
#endif

#if ARCH_X86
#define ALIGN_32_VAL 32
#define ALIGN_16_VAL 16
#elif ARCH_ARM || ARCH_AARCH64
// ARM doesn't benefit from anything more than 16 byte alignment.
#define ALIGN_32_VAL 16
#define ALIGN_16_VAL 16
#else
// No need for extra alignment on platforms without assembly.
#define ALIGN_32_VAL 8
#define ALIGN_16_VAL 8
#endif

/*
 * API for variables, struct members (ALIGN()) like:
 * uint8_t var[1][2][3][4]
 * becomes:
 * ALIGN(uint8_t var[1][2][3][4], alignment).
 */
#ifdef _MSC_VER
#define ALIGN(ll, a) \
    __declspec(align(a)) ll
#else
#define ALIGN(line, align) \
    line __attribute__((aligned(align)))
#endif

/*
 * API for stack alignment (ALIGN_STK_$align()) of variables like:
 * uint8_t var[1][2][3][4]
 * becomes:
 * ALIGN_STK_$align(uint8_t, var, 1, [2][3][4])
 */
#define ALIGN_STK_32(type, var, sz1d, sznd) \
    ALIGN(type var[sz1d]sznd, ALIGN_32_VAL)
// as long as stack is itself 16-byte aligned, this works (win64, gcc)
#define ALIGN_STK_16(type, var, sz1d, sznd) \
    ALIGN(type var[sz1d]sznd, ALIGN_16_VAL)

/*
 * Forbid inlining of a function:
 * static NOINLINE void func() {}
 */
#ifdef _MSC_VER
#define NOINLINE __declspec(noinline)
#else /* !_MSC_VER */
#define NOINLINE __attribute__((noinline))
#endif /* !_MSC_VER */

#if defined(__GNUC__) && !defined(__INTEL_COMPILER) && !defined(__clang__)
#    define dav1d_uninit(x) x=x
#else
#    define dav1d_uninit(x) x
#endif

#if defined (__GNUC__) || defined (__clang__)
#    define likely(p)   __builtin_expect(!!(p), 1)
#    define unlikely(p) __builtin_expect(!!(p), 0)
#else
#    define likely(p)   (!!(p))
#    define unlikely(p) (!!(p))
#endif

#if defined(__has_attribute)
#if __has_attribute(no_sanitize)
#define NO_SANITIZE(what) __attribute__((no_sanitize(what)))
#endif
#endif
#if !defined(NO_SANITIZE)
#define NO_SANITIZE(what)
#endif

 #ifdef _MSC_VER
 #include <intrin.h>

static inline int ctz(const unsigned int mask) {
    unsigned long idx;
    _BitScanForward(&idx, mask);
    return idx;
}

static inline int clz(const unsigned int mask) {
    unsigned long leading_zero = 0;
    _BitScanReverse(&leading_zero, mask);
    return (31 - leading_zero);
}

#ifdef _WIN64
static inline int clzll(const unsigned long long mask) {
    unsigned long leading_zero = 0;
    _BitScanReverse64(&leading_zero, mask);
    return (63 - leading_zero);
}
#else /* _WIN64 */
static inline int clzll(const unsigned long long mask) {
    if (mask >> 32)
        return clz((unsigned)(mask >> 32));
    else
        return clz((unsigned)mask) + 32;
}
#endif /* _WIN64 */
#else /* !_MSC_VER */
static inline int ctz(const unsigned int mask) {
    return __builtin_ctz(mask);
}

static inline int clz(const unsigned int mask) {
    return __builtin_clz(mask);
}

static inline int clzll(const unsigned long long mask) {
    return __builtin_clzll(mask);
}
#endif /* !_MSC_VER */

#include <stdint.h>
#ifdef _MSC_VER
#include <stdlib.h>

static inline uint16_t bswap16(const uint16_t val) {
    return _byteswap_ushort(val);
}

static inline uint32_t bswap32(const uint32_t val) {
    return _byteswap_ulong(val);
}

static inline uint64_t bswap64(const uint64_t val) {
    return _byteswap_uint64(val);
}
#else
static inline uint16_t bswap16(const uint16_t val) {
    return __builtin_bswap16(val);
}

static inline uint32_t bswap32(const uint32_t val) {
    return __builtin_bswap32(val);
}

static inline uint64_t bswap64(const uint64_t val) {
    return __builtin_bswap64(val);
}
#endif

static inline uint32_t hton32(const uint32_t val) {
#if BIG_ENDIAN
  return val;
#else
  return bswap32(val);
#endif
}

static inline uint64_t hton64(const uint64_t val) {
#if BIG_ENDIAN
  return val;
#else
  return bswap64(val);
#endif
}

#endif /* __DAV1D_COMMON_ATTRIBUTES_H__ */
