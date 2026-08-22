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

#ifndef EUNHA_UTILS_H
#define EUNHA_UTILS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/*
 * Byte slice used by parsers. It is not null-terminated.
 */
struct buffer {
    const uint8_t* data;
    size_t length;
};

/*
 * Two non-owning slices around a delimiter. A null after.data means the
 * delimiter was absent.
 */
struct buffer_split {
    struct buffer before;
    struct buffer after;
};

/*
 * Returns whether a byte slice matches a null-terminated ASCII string exactly.
 */
bool buffer_equals(struct buffer buffer, const char* expected);

/*
 * Returns whether a byte slice matches a null-terminated ASCII string while
 * ignoring ASCII case.
 */
bool buffer_equals_case_insensitive(struct buffer buffer, const char* expected);

/*
 * Returns whether byte is horizontal whitespace: space or tab.
 */
bool is_whitespace(uint8_t byte);

/*
 * Trims horizontal whitespace from both ends of a byte slice.
 */
struct buffer trim_whitespace(struct buffer buffer);

/*
 * Splits data once at delimiter. after.data is null when delimiter is absent.
 */
struct buffer_split split_once(struct buffer buffer, uint8_t delimiter);

/*
 * Parses a decimal byte slice into a size_t. Returns SIZE_MAX and sets errno
 * when the input is empty, malformed, or outside the supported range.
 */
size_t buffer_to_digit(struct buffer buffer);

#endif
