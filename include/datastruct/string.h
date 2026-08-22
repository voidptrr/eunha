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

#ifndef EUNHA_STRING_H
#define EUNHA_STRING_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "eunha.h"

/*
 * Owned string. data is always NUL-terminated, but length remains authoritative
 * so appended body data may contain embedded NUL bytes.
 */
struct string {
    char* data;
    size_t length;
    size_t capacity;
};

/*
 * Owned result of splitting once. length is one when the delimiter is absent
 * and two when it is present, including when either side is empty.
 */
struct string_split {
    struct string values[2];
    size_t length;
};

/*
 * Initializes a string by copying a NUL-terminated value.
 */
enum eunha_result string_init(struct string* string, const char* initial);

/*
 * Removes current contents without releasing storage.
 */
void string_clear(struct string* string);

/*
 * Appends length bytes copied from data.
 */
enum eunha_result string_append(
    struct string* string, const uint8_t* data, size_t length);

/*
 * Replaces destination ownership with source ownership and empties source.
 */
void string_move(struct string* destination, struct string* source);

/* Returns whether string exactly matches a NUL-terminated value. */
bool string_equals(const struct string* string, const char* expected);

/* Removes horizontal whitespace from both ends in place. */
void string_trim(struct string* string);

/* Splits string at the first delimiter into one or two owned strings. */
enum eunha_result string_split_once(
    const struct string* string, char delimiter, struct string_split* split);

/* Releases every string owned by a split result. */
void string_split_deinit(struct string_split* split);

/*
 * Parses an unsigned decimal value. Returns SIZE_MAX and sets errno when the
 * string is empty, malformed, or outside the supported range.
 */
size_t string_parse_size(const struct string* string);

/*
 * Releases owned storage and resets fields.
 */
void string_deinit(struct string* string);

#endif
