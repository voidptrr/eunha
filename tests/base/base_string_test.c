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
#include <string.h>

#include "base/base_string.h"

#define DEFAULT_CAPACITY 8

static void expect_string(const struct string_t* string, const char* expected,
                          size_t expected_capacity) {
    size_t expected_len = strlen(expected);

    assert(string != NULL);
    assert(string_len(string) == expected_len);
    assert(string->len == expected_len);
    assert(string->capacity == expected_capacity);
    assert(string->capacity >= string->len);
    assert(memcmp(string->data, expected, expected_len + 1) == 0);
    assert(string->data[string->len] == '\0');
}

static void test_empty_initialization(void) {
    struct string_t* from_null = string_init(NULL);
    struct string_t* from_empty = string_init("");

    expect_string(from_null, "", DEFAULT_CAPACITY);
    expect_string(from_empty, "", DEFAULT_CAPACITY);

    string_deinit(from_null);
    string_deinit(from_empty);
}

static void test_initial_content_and_capacity(void) {
    struct string_t* short_string = string_init("hello");
    struct string_t* exact_string = string_init("12345678");
    struct string_t* long_string = string_init("123456789");

    expect_string(short_string, "hello", DEFAULT_CAPACITY);
    expect_string(exact_string, "12345678", DEFAULT_CAPACITY);
    expect_string(long_string, "123456789", 9);

    string_deinit(short_string);
    string_deinit(exact_string);
    string_deinit(long_string);
}

static void test_empty_append(void) {
    struct string_t* string = string_init("hello");

    string_append(string, "");
    expect_string(string, "hello", DEFAULT_CAPACITY);

    string_deinit(string);
}

static void test_append_without_growth(void) {
    char source[] = "def";
    struct string_t* string = string_init("abc");

    string_append(string, source);
    expect_string(string, "abcdef", DEFAULT_CAPACITY);
    assert(strcmp(source, "def") == 0);

    string_deinit(string);
}

static void test_append_at_capacity_boundary(void) {
    struct string_t* string = string_init("1234");

    string_append(string, "5678");
    expect_string(string, "12345678", DEFAULT_CAPACITY);

    string_deinit(string);
}

static void test_append_with_growth(void) {
    struct string_t* string = string_init("12345678");

    string_append(string, "9");
    expect_string(string, "123456789", 18);

    string_deinit(string);
}

static void test_repeated_growth_boundaries(void) {
    struct string_t* string = string_init(NULL);

    string_append(string, "12345678");
    expect_string(string, "12345678", DEFAULT_CAPACITY);

    string_append(string, "9");
    expect_string(string, "123456789", 18);

    string_append(string, "abcdefghi");
    expect_string(string, "123456789abcdefghi", 18);

    string_append(string, "j");
    expect_string(string, "123456789abcdefghij", 38);

    string_deinit(string);
}

static void test_large_string(void) {
    char initial[257];
    char suffix[257];
    memset(initial, 'a', sizeof(initial) - 1);
    memset(suffix, 'b', sizeof(suffix) - 1);
    initial[sizeof(initial) - 1] = '\0';
    suffix[sizeof(suffix) - 1] = '\0';

    struct string_t* string = string_init(initial);
    expect_string(string, initial, 256);

    string_append(string, suffix);
    assert(string_len(string) == 512);
    assert(string->capacity == 1024);
    assert(memcmp(string->data, initial, 256) == 0);
    assert(memcmp(string->data + 256, suffix, 257) == 0);

    string_deinit(string);
}

int main(void) {
    test_empty_initialization();
    test_initial_content_and_capacity();
    test_empty_append();
    test_append_without_growth();
    test_append_at_capacity_boundary();
    test_append_with_growth();
    test_repeated_growth_boundaries();
    test_large_string();
    return 0;
}
