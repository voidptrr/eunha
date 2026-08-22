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

#include <stddef.h>
#include <stdint.h>

#include "eunha.h"

/*
 * Owned byte string. data is always NUL-terminated for convenience, but length
 * is authoritative and data may contain non-text bytes before the terminator.
 */
struct string {
    uint8_t* data;
    size_t length;
    size_t capacity;
};

/*
 * Initializes an empty string with owned storage.
 */
enum eunha_result string_init(struct string* string);

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
 * Replaces current contents with length bytes copied from data.
 */
enum eunha_result string_set(
    struct string* string, const uint8_t* data, size_t length);

/*
 * Releases owned storage and resets fields.
 */
void string_deinit(struct string* string);

#endif
