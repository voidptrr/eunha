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
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "utils.h"

/*
 * Returns a non-owning byte view over a C string.
 */
static struct buffer buffer_from_string(const char* string) {
    return (struct buffer){
        .data = (const uint8_t*)string,
        .length = strlen(string),
    };
}

/*
 * Verifies exact byte comparison against a string literal.
 */
static int test_buffer_equals(void) {
    const char* data = "Content-Length";
    struct buffer buffer = buffer_from_string(data);

    assert(buffer_equals(buffer, "Content-Length"));
    assert(!buffer_equals(buffer, "content-length"));
    assert(!buffer_equals(buffer, "Content"));

    buffer.length = 7;
    assert(!buffer_equals(buffer, "Content-Length"));
    return 0;
}

/*
 * Verifies ASCII case-insensitive byte comparison against a string literal.
 */
static int test_buffer_equals_case_insensitive(void) {
    const char* data = "Content-Length";
    struct buffer buffer = buffer_from_string(data);

    assert(buffer_equals_case_insensitive(buffer, "content-length"));
    assert(buffer_equals_case_insensitive(buffer, "CONTENT-LENGTH"));
    assert(!buffer_equals_case_insensitive(buffer, "content-type"));

    buffer.length = 7;
    assert(!buffer_equals_case_insensitive(buffer, "content-length"));
    return 0;
}

/*
 * Verifies whitespace means only space and tab.
 */
static int test_is_whitespace(void) {
    assert(is_whitespace(' '));
    assert(is_whitespace('\t'));
    assert(!is_whitespace('\r'));
    assert(!is_whitespace('\n'));
    assert(!is_whitespace('a'));
    return 0;
}

/*
 * Verifies trimming adjusts the byte slice without changing the source data.
 */
static int test_trim_whitespace(void) {
    const char* padded = " \tContent-Length\t ";
    struct buffer buffer = trim_whitespace(buffer_from_string(padded));

    assert(buffer_equals(buffer, "Content-Length"));
    assert(buffer.data == (const uint8_t*)padded + 2);
    return 0;
}

/*
 * Verifies trimming can produce an empty slice.
 */
static int test_trim_whitespace_empty(void) {
    const char* spaces = " \t ";
    struct buffer buffer = trim_whitespace(buffer_from_string(spaces));

    assert(buffer.length == 0);
    assert(buffer.data == (const uint8_t*)spaces + strlen(spaces));
    return 0;
}

/*
 * Verifies split_once returns the two slices around the first delimiter.
 */
static int test_split_once(void) {
    const char* data = "Content-Type: application/json";
    struct buffer_split split = split_once(buffer_from_string(data), ':');

    assert(buffer_equals(split.before, "Content-Type"));
    assert(buffer_equals(split.after, " application/json"));
    return 0;
}

/*
 * Verifies split_once reports a missing delimiter without losing input.
 */
static int test_split_once_missing(void) {
    const char* data = "Content-Type";
    struct buffer_split split = split_once(buffer_from_string(data), ':');

    assert(buffer_equals(split.before, "Content-Type"));
    assert(split.after.data == NULL);
    assert(split.after.length == 0);
    return 0;
}

/*
 * Verifies split_once accepts empty left and right sides.
 */
static int test_split_once_empty_sides(void) {
    const char* data = ":";
    struct buffer_split split = split_once(buffer_from_string(data), ':');

    assert(split.before.length == 0);
    assert(split.after.length == 0);
    assert(split.after.data == (const uint8_t*)data + 1);
    return 0;
}

/*
 * Verifies decimal byte slices convert to size_t values.
 */
static int test_buffer_to_digit(void) {
    const char* digits = "12345";
    size_t value = buffer_to_digit(buffer_from_string(digits));

    assert(value == 12345);
    return 0;
}

/*
 * Verifies malformed decimal byte slices are rejected.
 */
static int test_buffer_to_digit_errors(void) {
    const char* letters = "12a";
    char overflow[128];

    errno = 0;
    assert(buffer_to_digit((struct buffer){0}) == SIZE_MAX);
    assert(errno == EINVAL);

    errno = 0;
    assert(buffer_to_digit(buffer_from_string(letters)) == SIZE_MAX);
    assert(errno == EINVAL);

    memset(overflow, '9', sizeof(overflow));
    errno = 0;
    assert(buffer_to_digit((struct buffer){
               .data = (const uint8_t*)overflow,
               .length = sizeof(overflow),
           }) == SIZE_MAX);
    assert(errno == EOVERFLOW);
    return 0;
}

/*
 * Runs utility tests.
 */
int main(void) {
    assert(test_buffer_equals() == 0);
    assert(test_buffer_equals_case_insensitive() == 0);
    assert(test_is_whitespace() == 0);
    assert(test_trim_whitespace() == 0);
    assert(test_trim_whitespace_empty() == 0);
    assert(test_split_once() == 0);
    assert(test_split_once_missing() == 0);
    assert(test_split_once_empty_sides() == 0);
    assert(test_buffer_to_digit() == 0);
    assert(test_buffer_to_digit_errors() == 0);
    return 0;
}
