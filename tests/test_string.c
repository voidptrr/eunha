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
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "datastruct/string.h"

/*
 * Verifies a string matches a byte string literal.
 */
static void assert_string_equals(
    const struct string* string, const char* expected) {
    size_t expected_length = strlen(expected);

    assert(string->length == expected_length);
    assert(memcmp(string->data, expected, expected_length) == 0);
    assert(string->data[string->length] == 0);
}

/*
 * Verifies string_init allocates an empty NUL-terminated string.
 */
static int test_string_init_deinit(void) {
    struct string string;

    assert(string_init(&string) == 0);
    assert(string.data != NULL);
    assert(string.length == 0);
    assert(string.capacity > 0);
    assert(string.data[0] == 0);

    string_deinit(&string);
    assert(string.data == NULL);
    assert(string.length == 0);
    assert(string.capacity == 0);
    return 0;
}

/*
 * Verifies string_set replaces previous contents.
 */
static int test_string_set(void) {
    struct string string;
    const char* first = "hello";
    const char* second = "authorization";

    assert(string_init(&string) == 0);
    assert(string_set(&string, (const uint8_t*)first, strlen(first)) == 0);
    assert_string_equals(&string, "hello");

    assert(string_set(&string, (const uint8_t*)second, strlen(second)) == 0);
    assert_string_equals(&string, "authorization");

    string_deinit(&string);
    return 0;
}

/*
 * Verifies string_append grows storage and preserves existing bytes.
 */
static int test_string_append(void) {
    struct string string;
    const char* prefix = "grant";
    const char* suffix = "=client_credentials";

    assert(string_init(&string) == 0);
    assert(string_append(&string, (const uint8_t*)prefix, strlen(prefix)) == 0);
    assert(string_append(&string, (const uint8_t*)suffix, strlen(suffix)) == 0);
    assert_string_equals(&string, "grant=client_credentials");

    string_deinit(&string);
    return 0;
}

/*
 * Verifies string_clear keeps storage reusable.
 */
static int test_string_clear(void) {
    struct string string;
    const char* value = "token";
    size_t capacity = 0;

    assert(string_init(&string) == 0);
    assert(string_set(&string, (const uint8_t*)value, strlen(value)) == 0);

    capacity = string.capacity;
    string_clear(&string);
    assert(string.length == 0);
    assert(string.capacity == capacity);
    assert(string.data[0] == 0);

    string_deinit(&string);
    return 0;
}

/*
 * Runs string unit tests.
 */
int main(void) {
    assert(test_string_init_deinit() == 0);
    assert(test_string_set() == 0);
    assert(test_string_append() == 0);
    assert(test_string_clear() == 0);
    return 0;
}
