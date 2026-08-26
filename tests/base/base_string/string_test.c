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

static void expect_string(const struct string* string, const char* expected,
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

static void test_empty_allocation(void) {
    struct string* from_null = string_alloc(NULL);
    struct string* from_empty = string_alloc("");

    expect_string(from_null, "", DEFAULT_CAPACITY);
    expect_string(from_empty, "", DEFAULT_CAPACITY);

    string_free(from_null);
    string_free(from_empty);
}

static void test_initial_content_and_capacity(void) {
    struct string* short_string = string_alloc("hello");
    struct string* exact_string = string_alloc("12345678");
    struct string* long_string = string_alloc("123456789");

    expect_string(short_string, "hello", DEFAULT_CAPACITY);
    expect_string(exact_string, "12345678", DEFAULT_CAPACITY);
    expect_string(long_string, "123456789", 9);

    string_free(short_string);
    string_free(exact_string);
    string_free(long_string);
}

static void test_empty_append(void) {
    struct string* string = string_alloc("hello");

    string_append(string, "");
    expect_string(string, "hello", DEFAULT_CAPACITY);

    string_free(string);
}

static void test_append_without_growth(void) {
    char source[] = "def";
    struct string* string = string_alloc("abc");

    string_append(string, source);
    expect_string(string, "abcdef", DEFAULT_CAPACITY);
    assert(strcmp(source, "def") == 0);

    string_free(string);
}

static void test_append_at_capacity_boundary(void) {
    struct string* string = string_alloc("1234");

    string_append(string, "5678");
    expect_string(string, "12345678", DEFAULT_CAPACITY);

    string_free(string);
}

static void test_append_with_growth(void) {
    struct string* string = string_alloc("12345678");

    string_append(string, "9");
    expect_string(string, "123456789", 18);

    string_free(string);
}

static void test_repeated_append_growth_boundaries(void) {
    struct string* string = string_alloc(NULL);

    string_append(string, "12345678");
    expect_string(string, "12345678", DEFAULT_CAPACITY);

    string_append(string, "9");
    expect_string(string, "123456789", 18);

    string_append(string, "abcdefghi");
    expect_string(string, "123456789abcdefghi", 18);

    string_append(string, "j");
    expect_string(string, "123456789abcdefghij", 38);

    string_free(string);
}

static void test_large_append(void) {
    char initial[257];
    char suffix[257];
    memset(initial, 'a', sizeof(initial) - 1);
    memset(suffix, 'b', sizeof(suffix) - 1);
    initial[sizeof(initial) - 1] = '\0';
    suffix[sizeof(suffix) - 1] = '\0';

    struct string* string = string_alloc(initial);
    expect_string(string, initial, 256);

    string_append(string, suffix);
    assert(string_len(string) == 512);
    assert(string->capacity == 1024);
    assert(memcmp(string->data, initial, 256) == 0);
    assert(memcmp(string->data + 256, suffix, 257) == 0);

    string_free(string);
}

static void test_empty_prepend(void) {
    char source[] = "";
    struct string* string = string_alloc("hello");

    string_prepend(string, source);
    expect_string(string, "hello", DEFAULT_CAPACITY);

    string_free(string);
}

static void test_prepend_to_empty_string(void) {
    char source[] = "hello";
    struct string* string = string_alloc(NULL);

    string_prepend(string, source);
    expect_string(string, "hello", DEFAULT_CAPACITY);

    string_free(string);
}

static void test_prepend_without_growth(void) {
    char source[] = "abc";
    struct string* string = string_alloc("def");

    string_prepend(string, source);
    expect_string(string, "abcdef", DEFAULT_CAPACITY);
    assert(strcmp(source, "abc") == 0);

    string_free(string);
}

static void test_prepend_at_capacity_boundary(void) {
    char source[] = "1234";
    struct string* string = string_alloc("5678");

    string_prepend(string, source);
    expect_string(string, "12345678", DEFAULT_CAPACITY);

    string_free(string);
}

static void test_prepend_with_growth(void) {
    char source[] = "1";
    struct string* string = string_alloc("23456789");

    string_prepend(string, source);
    expect_string(string, "123456789", 18);

    string_free(string);
}

static void test_repeated_prepend_growth_boundaries(void) {
    char first[] = "12345678";
    char second[] = "9";
    char third[] = "abcdefghi";
    char fourth[] = "j";
    struct string* string = string_alloc(NULL);

    string_prepend(string, first);
    expect_string(string, "12345678", DEFAULT_CAPACITY);

    string_prepend(string, second);
    expect_string(string, "912345678", 18);

    string_prepend(string, third);
    expect_string(string, "abcdefghi912345678", 18);

    string_prepend(string, fourth);
    expect_string(string, "jabcdefghi912345678", 38);

    string_free(string);
}

static void test_large_prepend(void) {
    char initial[257];
    char prefix[257];
    memset(initial, 'a', sizeof(initial) - 1);
    memset(prefix, 'b', sizeof(prefix) - 1);
    initial[sizeof(initial) - 1] = '\0';
    prefix[sizeof(prefix) - 1] = '\0';

    struct string* string = string_alloc(initial);
    string_prepend(string, prefix);

    assert(string_len(string) == 512);
    assert(string->capacity == 1024);
    assert(memcmp(string->data, prefix, 256) == 0);
    assert(memcmp(string->data + 256, initial, 257) == 0);

    string_free(string);
}

static void test_contains_at_each_position(void) {
    struct string* string = string_alloc("prefix-middle-suffix");

    assert(string_contains(string, "prefix"));
    assert(string_contains(string, "middle"));
    assert(string_contains(string, "suffix"));
    assert(string_contains(string, "prefix-middle-suffix"));
    assert(!string_contains(string, "missing"));
    assert(!string_contains(string, "prefix-middle-suffix-extra"));

    string_free(string);
}

static void test_contains_is_case_sensitive(void) {
    struct string* string = string_alloc("Eunha");

    assert(string_contains(string, "Eunha"));
    assert(!string_contains(string, "eunha"));

    string_free(string);
}

static void test_contains_empty_needle(void) {
    struct string* empty = string_alloc(NULL);
    struct string* nonempty = string_alloc("eunha");

    assert(string_contains(empty, ""));
    assert(string_contains(nonempty, ""));
    assert(!string_contains(empty, "eunha"));

    string_free(empty);
    string_free(nonempty);
}

static void test_has_prefix(void) {
    struct string* string = string_alloc("prefix-middle-suffix");

    assert(string_has_prefix(string, "prefix"));
    assert(string_has_prefix(string, "prefix-middle-suffix"));
    assert(!string_has_prefix(string, "middle"));
    assert(!string_has_prefix(string, "suffix"));
    assert(!string_has_prefix(string, "prefix-middle-suffix-extra"));

    string_free(string);
}

static void test_has_prefix_is_case_sensitive(void) {
    struct string* string = string_alloc("Eunha");

    assert(string_has_prefix(string, "Eun"));
    assert(!string_has_prefix(string, "eun"));

    string_free(string);
}

static void test_has_prefix_empty_prefix(void) {
    struct string* empty = string_alloc(NULL);
    struct string* nonempty = string_alloc("eunha");

    assert(string_has_prefix(empty, ""));
    assert(string_has_prefix(nonempty, ""));
    assert(!string_has_prefix(empty, "eunha"));

    string_free(empty);
    string_free(nonempty);
}

int main(void) {
    test_empty_allocation();
    test_initial_content_and_capacity();
    test_empty_append();
    test_append_without_growth();
    test_append_at_capacity_boundary();
    test_append_with_growth();
    test_repeated_append_growth_boundaries();
    test_large_append();
    test_empty_prepend();
    test_prepend_to_empty_string();
    test_prepend_without_growth();
    test_prepend_at_capacity_boundary();
    test_prepend_with_growth();
    test_repeated_prepend_growth_boundaries();
    test_large_prepend();
    test_contains_at_each_position();
    test_contains_is_case_sensitive();
    test_contains_empty_needle();
    test_has_prefix();
    test_has_prefix_is_case_sensitive();
    test_has_prefix_empty_prefix();
    return 0;
}
