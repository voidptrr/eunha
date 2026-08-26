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

#ifndef BASE_STRING
#define BASE_STRING

#include <stddef.h>

struct string_t {
    size_t len;
    size_t capacity;
    char* data;
};

/**
 * Allocates an owned string from null-terminated bytes, or an empty string when
 * initialized with NULL. Exits the process if allocation fails.
 */
struct string_t* string_init(const char* initial);

/** Returns the string length in bytes, excluding the null terminator. */
size_t string_len(const struct string_t* string);

/**
 * Appends null-terminated bytes, growing the destination as needed. The source
 * must not point into the destination's backing storage.
 */
void string_append(struct string_t* dst, const char* src);

/**
 * Prepend bytes, growing the destination as needed. The source
 * must not point into the destination's backing storage.
 */
void string_prepend(struct string_t* dst, const char* src);

/**
 * Returns whether the string contains the null-terminated needle. Matching is
 * bytewise and case-sensitive; an empty needle always matches.
 */
bool string_contains(const struct string_t* string, const char* needle);

/**
 * Returns whether the string starts with the given prefix.
 */
bool string_starts_with(const struct string_t* string, const char* prefix);

/** Frees an owned string and its backing storage. */
void string_deinit(struct string_t* string);

#endif
