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

#include "datastruct/string.h"
#include "eunha.h"

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
 * Verifies initialization copies both empty and non-empty C strings.
 */
static int test_string_init_deinit(void) {
    struct string string;

    assert(string_init(&string, "authorization") == EUNHA_OK);
    assert(string.data != NULL);
    assert_string_equals(&string, "authorization");

    string_deinit(&string);
    assert(string.data == NULL);
    assert(string.length == 0);
    assert(string.capacity == 0);
    return 0;
}

/*
 * Verifies string_append grows storage and preserves existing bytes.
 */
static int test_string_append(void) {
    struct string string;
    const char* prefix = "grant";
    const char* suffix = "=client_credentials";

    assert(string_init(&string, "") == EUNHA_OK);
    assert(string_append(&string, (const uint8_t*)prefix, strlen(prefix)) ==
           EUNHA_OK);
    assert(string_append(&string, (const uint8_t*)suffix, strlen(suffix)) ==
           EUNHA_OK);
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

    assert(string_init(&string, value) == EUNHA_OK);

    capacity = string.capacity;
    string_clear(&string);
    assert(string.length == 0);
    assert(string.capacity == capacity);
    assert(string.data[0] == 0);

    string_deinit(&string);
    return 0;
}

/* Verifies ownership moves without copying and leaves source deinitializable.
 */
static int test_string_move(void) {
    struct string source;
    struct string destination;

    assert(string_init(&source, "moved") == EUNHA_OK);
    assert(string_init(&destination, "old") == EUNHA_OK);
    char* allocation = source.data;

    string_move(&destination, &source);
    assert(destination.data == allocation);
    assert_string_equals(&destination, "moved");
    assert(source.data == NULL);
    assert(source.length == 0);
    assert(source.capacity == 0);

    string_deinit(&source);
    string_deinit(&destination);
    return 0;
}

/* Verifies horizontal whitespace is removed from both ends in place. */
static int test_string_trim(void) {
    struct string string;

    assert(string_init(&string, " \tContent-Length\t ") == EUNHA_OK);
    string_trim(&string);
    assert_string_equals(&string, "Content-Length");

    string_clear(&string);
    assert(string_append(&string, (const uint8_t*)" \t ", 3) == EUNHA_OK);
    string_trim(&string);
    assert_string_equals(&string, "");

    string_deinit(&string);
    return 0;
}

/* Verifies split results own one or two strings, including empty sides. */
static int test_string_split_once(void) {
    struct string string;
    struct string_split split;

    assert(string_init(&string, "Content-Type: application/json") == EUNHA_OK);
    assert(string_split_once(&string, ':', &split) == EUNHA_OK);
    assert(split.length == 2);
    assert_string_equals(&split.values[0], "Content-Type");
    assert_string_equals(&split.values[1], " application/json");
    string_split_deinit(&split);
    string_deinit(&string);

    assert(string_init(&string, "Content-Type") == EUNHA_OK);
    assert(string_split_once(&string, ':', &split) == EUNHA_OK);
    assert(split.length == 1);
    assert_string_equals(&split.values[0], "Content-Type");
    assert(split.values[1].data == NULL);
    string_split_deinit(&split);
    string_deinit(&string);

    assert(string_init(&string, ":") == EUNHA_OK);
    assert(string_split_once(&string, ':', &split) == EUNHA_OK);
    assert(split.length == 2);
    assert_string_equals(&split.values[0], "");
    assert_string_equals(&split.values[1], "");
    string_split_deinit(&split);
    string_deinit(&string);
    return 0;
}

/* Verifies checked decimal parsing and every malformed result. */
static int test_string_parse_size(void) {
    struct string string;
    char overflow[129];

    assert(string_init(&string, "12345") == EUNHA_OK);
    assert(string_parse_size(&string) == 12345);
    string_deinit(&string);

    assert(string_init(&string, "") == EUNHA_OK);
    errno = 0;
    assert(string_parse_size(&string) == SIZE_MAX);
    assert(errno == EINVAL);
    string_deinit(&string);

    assert(string_init(&string, "12a") == EUNHA_OK);
    errno = 0;
    assert(string_parse_size(&string) == SIZE_MAX);
    assert(errno == EINVAL);
    string_deinit(&string);

    memset(overflow, '9', sizeof(overflow) - 1);
    overflow[sizeof(overflow) - 1] = 0;
    assert(string_init(&string, overflow) == EUNHA_OK);
    errno = 0;
    assert(string_parse_size(&string) == SIZE_MAX);
    assert(errno == EOVERFLOW);
    string_deinit(&string);
    return 0;
}

/* Verifies length-aware equality rejects an embedded NUL prefix match. */
static int test_string_embedded_nul(void) {
    static const uint8_t suffix[] = {0, 'x'};
    struct string string;

    assert(string_init(&string, "GET") == EUNHA_OK);
    assert(string_append(&string, suffix, sizeof(suffix)) == EUNHA_OK);
    assert(!string_equals(&string, "GET"));
    assert(string.length == 5);
    assert(string.data[string.length] == 0);
    string_deinit(&string);
    return 0;
}

/*
 * Runs string unit tests.
 */
int main(void) {
    assert(test_string_init_deinit() == 0);
    assert(test_string_append() == 0);
    assert(test_string_clear() == 0);
    assert(test_string_move() == 0);
    assert(test_string_trim() == 0);
    assert(test_string_split_once() == 0);
    assert(test_string_parse_size() == 0);
    assert(test_string_embedded_nul() == 0);
    return 0;
}
