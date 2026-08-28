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

#include "base/base_arena.h"
#include "base/base_core.h"
#include "base/base_string.h"

static void expect_str8(struct str8 string, const char* expected) {
    struct str8 expected_string = str8_cstring(expected);

    assert(str8_len(string) == expected_string.len);
    if (string.len > 0) {
        assert(memcmp(string.data, expected_string.data, string.len) == 0);
    }
}

static void test_views(void) {
    struct str8 zero = str8_cstring(NULL);
    struct str8 empty = str8_cstring("");
    struct str8 literal = str8_lit("eunha");

    assert(zero.data == NULL);
    assert(str8_len(zero) == 0);
    assert(empty.data != NULL);
    assert(str8_len(empty) == 0);
    expect_str8(literal, "eunha");
}

static void test_byte_view_can_contain_null(void) {
    const u8 bytes[] = {'a', 0, 'b'};
    const u8 needle_bytes[] = {0, 'b'};
    struct str8 string = str8(bytes, sizeof(bytes));
    struct str8 needle = str8(needle_bytes, sizeof(needle_bytes));

    assert(str8_len(string) == 3);
    assert(str8_contains(string, needle));
}

static void test_arena_copy_owns_its_bytes(void) {
    struct arena* arena = arena_alloc(ARENA_DEFAULT);
    u8 source_bytes[] = {'h', 'e', 'l', 'l', 'o'};
    struct str8 source = str8(source_bytes, sizeof(source_bytes));
    struct str8 copy = str8_copy(arena, source);

    expect_str8(copy, "hello");
    assert(copy.data != source.data);
    assert(copy.data[copy.len] == 0);

    source_bytes[0] = 'j';
    expect_str8(source, "jello");
    expect_str8(copy, "hello");

    arena_free(arena);
}

static void test_arena_copy_of_empty_string(void) {
    struct arena* arena = arena_alloc(ARENA_DEFAULT);
    struct str8 copy = str8_copy(arena, (struct str8){0});

    assert(copy.data != NULL);
    assert(copy.len == 0);
    assert(copy.data[0] == 0);

    arena_free(arena);
}

static void test_cat(void) {
    struct arena* arena = arena_alloc(ARENA_DEFAULT);
    struct str8 first = str8_lit("hello, ");
    struct str8 second = str8_lit("world");
    struct str8 result = str8_cat(arena, first, second);

    expect_str8(first, "hello, ");
    expect_str8(second, "world");
    expect_str8(result, "hello, world");
    assert(result.data[result.len] == 0);

    arena_free(arena);
}

static void test_contains_at_each_position(void) {
    struct str8 string = str8_lit("prefix-middle-suffix");

    assert(str8_contains(string, str8_lit("prefix")));
    assert(str8_contains(string, str8_lit("middle")));
    assert(str8_contains(string, str8_lit("suffix")));
    assert(str8_contains(string, string));
    assert(!str8_contains(string, str8_lit("missing")));
    assert(!str8_contains(string, str8_lit("prefix-middle-suffix-extra")));
}

static void test_contains_is_case_sensitive(void) {
    struct str8 string = str8_lit("Eunha");

    assert(str8_contains(string, str8_lit("Eunha")));
    assert(!str8_contains(string, str8_lit("eunha")));
}

static void test_contains_empty_needle(void) {
    struct str8 empty = {0};
    struct str8 nonempty = str8_lit("eunha");

    assert(str8_contains(empty, empty));
    assert(str8_contains(nonempty, empty));
    assert(!str8_contains(empty, nonempty));
}

static void test_has_prefix(void) {
    struct str8 string = str8_lit("prefix-middle-suffix");

    assert(str8_has_prefix(string, str8_lit("prefix")));
    assert(str8_has_prefix(string, string));
    assert(!str8_has_prefix(string, str8_lit("middle")));
    assert(!str8_has_prefix(string, str8_lit("suffix")));
    assert(!str8_has_prefix(string, str8_lit("prefix-middle-suffix-extra")));
}

static void test_has_prefix_is_case_sensitive(void) {
    struct str8 string = str8_lit("Eunha");

    assert(str8_has_prefix(string, str8_lit("Eun")));
    assert(!str8_has_prefix(string, str8_lit("eun")));
}

static void test_has_prefix_empty_prefix(void) {
    struct str8 empty = {0};
    struct str8 nonempty = str8_lit("eunha");

    assert(str8_has_prefix(empty, empty));
    assert(str8_has_prefix(nonempty, empty));
    assert(!str8_has_prefix(empty, nonempty));
}

int main(void) {
    test_views();
    test_byte_view_can_contain_null();
    test_arena_copy_owns_its_bytes();
    test_arena_copy_of_empty_string();
    test_cat();
    test_contains_at_each_position();
    test_contains_is_case_sensitive();
    test_contains_empty_needle();
    test_has_prefix();
    test_has_prefix_is_case_sensitive();
    test_has_prefix_empty_prefix();
    return 0;
}
