/*
 * MIT License
 *
 * Copyright (c) 2026 Tommaso Bruno
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef EUNHA_CORE_H
#define EUNHA_CORE_H

#include <stddef.h>
#include <stdint.h>

#ifdef __clang__
#define BASE_ASAN_ENABLED __has_feature(address_sanitizer)
#elif defined(__GNUC__) && defined(__SANITIZE_ADDRESS__)
#define BASE_ASAN_ENABLED 1
#else
#define BASE_ASAN_ENABLED 0
#endif

#if BASE_ASAN_ENABLED
#include <sanitizer/asan_interface.h>

/** Marks a memory range inaccessible to AddressSanitizer. */
#define asan_poison_memory_region(address, size) \
    ASAN_POISON_MEMORY_REGION((address), (size))

/** Marks a previously poisoned memory range accessible to AddressSanitizer. */
#define asan_unpoison_memory_region(address, size) \
    ASAN_UNPOISON_MEMORY_REGION((address), (size))

/** Returns whether AddressSanitizer considers address poisoned. */
#define asan_address_is_poisoned(address) \
    (__asan_address_is_poisoned((address)) != 0)
#else
#define asan_poison_memory_region(...) ((void)0)
#define asan_unpoison_memory_region(...) ((void)0)
#define asan_address_is_poisoned(...) (0)
#endif

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;

/** Converts x kibibytes to bytes. */
#define kb(x) ((size_t)(x) << 10)

/** Converts x mebibytes to bytes. */
#define mb(x) ((size_t)(x) << 20)

/** Converts x gibibytes to bytes. */
#define gb(x) ((size_t)(x) << 30)

/** Rounds x up to the next multiple of the power-of-two alignment b. */
#define align_pow2(x, b) (((x) + (b) - 1) & (~((b) - 1)))

/** Returns the padding needed to align x to the power-of-two alignment b. */
#define align_pad_pow2(x, b) ((0 - (x)) & ((b) - 1))

/** Returns whether x is a nonzero power of two. */
#define is_pow2(x) ((x) != 0 && ((x) & ((x) - 1)) == 0)

#endif
