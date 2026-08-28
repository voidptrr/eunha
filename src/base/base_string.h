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

#ifndef BASE_STRING_H
#define BASE_STRING_H

#include <stddef.h>

#include "base/base_arena.h"
#include "base/base_core.h"

struct str8 {
    const u8* data;
    size_t len;
};

#define str8_lit(value) str8((const u8*)(value), sizeof(value) - 1)

/** Iterates a caller-owned byte cursor over the string's contents. */
#define for_each_char(pos, str) \
    for ((pos) = (str)->data;   \
         (pos) != NULL && (pos) < (str)->data + (str)->len; ++(pos))

/** Creates a non-owning UTF-8 view over len bytes. */
struct str8 str8(const u8* data, size_t len);

/** Creates a non-owning UTF-8 view over a C string, or a zero view for NULL. */
struct str8 str8_cstring(const char* cstr);

/** Returns the string length in bytes, excluding the null terminator. */
size_t str8_len(struct str8 str);

/**
 * Copies a string and a trailing null byte into the arena. Returns an empty
 * zero view when the allocation cannot be satisfied.
 */
struct str8 str8_copy(struct arena* arena, struct str8 source);

/**
 * Returns a new arena-owned concatenation of first and second. The inputs
 * remain unchanged. Returns a zero view on allocation failure.
 */
struct str8 str8_cat(struct arena* arena, struct str8 first,
                     struct str8 second);

/**
 * Returns whether haystack contains needle. Matching is bytewise and
 * case-sensitive; an empty needle always matches.
 */
bool str8_contains(struct str8 haystack, struct str8 needle);

/**
 * Returns whether string has prefix. Matching is bytewise and case-sensitive;
 * an empty prefix always matches.
 */
bool str8_has_prefix(struct str8 string, struct str8 prefix);

#endif
