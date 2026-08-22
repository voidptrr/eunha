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

#include <assert.h>
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "utils.h"

/*
 * Returns the ASCII lowercase form for case-insensitive comparisons.
 */
static uint8_t ascii_lower(uint8_t byte) {
    if (byte >= 'A' && byte <= 'Z') {
        return (uint8_t)(byte + ('a' - 'A'));
    }

    return byte;
}

/*
 * Returns whether a byte slice matches a null-terminated ASCII string exactly.
 */
bool buffer_equals(struct buffer buffer, const char* expected) {
    assert(buffer.data != NULL || buffer.length == 0);
    assert(expected != NULL);

    size_t expected_length = strlen(expected);

    if (buffer.length != expected_length) {
        return false;
    }

    if (buffer.length == 0) {
        return true;
    }

    return memcmp(buffer.data, expected, buffer.length) == 0;
}

/*
 * Returns whether a byte slice matches a null-terminated ASCII string while
 * ignoring ASCII case.
 */
bool buffer_equals_case_insensitive(
    struct buffer buffer, const char* expected) {
    assert(buffer.data != NULL || buffer.length == 0);
    assert(expected != NULL);

    size_t expected_length = strlen(expected);
    if (buffer.length != expected_length) {
        return false;
    }

    for (size_t index = 0; index < buffer.length; index += 1) {
        if (ascii_lower(buffer.data[index]) !=
            ascii_lower((uint8_t)expected[index])) {
            return false;
        }
    }

    return true;
}

/*
 * Returns whether byte is horizontal whitespace: space or tab.
 */
bool is_whitespace(uint8_t byte) {
    return byte == ' ' || byte == '\t';
}

/*
 * Trims horizontal whitespace from both ends of a byte slice.
 */
struct buffer trim_whitespace(struct buffer buffer) {
    assert(buffer.data != NULL || buffer.length == 0);

    size_t start = 0;
    size_t end = buffer.length;

    while (start < end && is_whitespace(buffer.data[start])) {
        start += 1;
    }

    while (end > start && is_whitespace(buffer.data[end - 1])) {
        end -= 1;
    }

    return (struct buffer){
        .data = buffer.data == NULL ? NULL : buffer.data + start,
        .length = end - start,
    };
}

/*
 * Splits data once at delimiter. after.data is null when delimiter is absent.
 */
struct buffer_split split_once(struct buffer buffer, uint8_t delimiter) {
    assert(buffer.data != NULL || buffer.length == 0);

    struct buffer_split split = {
        .before = buffer,
        .after = {0},
    };

    if (buffer.length == 0) {
        return split;
    }

    const uint8_t* match = memchr(buffer.data, delimiter, buffer.length);
    if (match == NULL) {
        return split;
    }

    split.before.length = (size_t)(match - buffer.data);
    split.after.data = match + 1;
    split.after.length = buffer.length - split.before.length - 1;
    return split;
}

/*
 * Parses a decimal byte slice into a size_t. SIZE_MAX is reserved for errors.
 */
size_t buffer_to_digit(struct buffer buffer) {
    assert(buffer.data != NULL || buffer.length == 0);

    if (buffer.length == 0) {
        errno = EINVAL;
        return SIZE_MAX;
    }

    size_t parsed = 0;

    for (size_t index = 0; index < buffer.length; index += 1) {
        uint8_t byte = buffer.data[index];

        if (byte < '0' || byte > '9') {
            errno = EINVAL;
            return SIZE_MAX;
        }

        size_t digit = (size_t)(byte - '0');

        if (parsed > (SIZE_MAX - digit) / 10) {
            errno = EOVERFLOW;
            return SIZE_MAX;
        }

        parsed = (parsed * 10) + digit;
    }

    if (parsed == SIZE_MAX) {
        errno = EOVERFLOW;
        return SIZE_MAX;
    }

    return parsed;
}
