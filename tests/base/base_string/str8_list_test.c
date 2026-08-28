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

static void test_push(void) {
    struct arena* arena = arena_alloc(ARENA_DEFAULT);
    struct str8 first = str8_lit("hello");
    struct str8 second = str8_lit(", ");
    struct str8 third = str8_lit("world");
    struct str8_list list = {0};

    assert(list.head == NULL);
    assert(list.tail == NULL);
    assert(list.total_len == 0);

    str8_list_push(arena, &list, first);
    str8_list_push(arena, &list, second);
    str8_list_push(arena, &list, third);

    assert(list.head != NULL);
    assert(list.tail != NULL);
    assert(list.head->data.data == first.data);
    assert(list.head->next->data.data == second.data);
    assert(list.head->next->next == list.tail);
    assert(list.tail->data.data == third.data);
    assert(list.tail->next == NULL);
    assert(list.total_len == first.len + second.len + third.len);

    arena_free(arena);
}

static void test_join(void) {
    struct arena* arena = arena_alloc(ARENA_DEFAULT);
    u8 first_bytes[] = {'h', 'e', 'l', 'l', 'o'};
    struct str8 first = str8(first_bytes, sizeof(first_bytes));
    struct str8_list list = {0};

    str8_list_push(arena, &list, first);
    str8_list_push(arena, &list, (struct str8){0});
    str8_list_push(arena, &list, str8_lit(", world"));

    struct str8 joined = str8_list_join(arena, &list);

    expect_str8(joined, "hello, world");
    assert(joined.data != first.data);
    assert(joined.data[joined.len] == 0);

    first_bytes[0] = 'j';
    expect_str8(joined, "hello, world");

    arena_free(arena);
}

static void test_join_empty(void) {
    struct arena* arena = arena_alloc(ARENA_DEFAULT);
    struct str8_list list = {0};
    struct str8 joined = str8_list_join(arena, &list);

    assert(joined.data != NULL);
    assert(joined.len == 0);
    assert(joined.data[0] == 0);

    arena_free(arena);
}

int main(void) {
    test_push();
    test_join();
    test_join_empty();
    return 0;
}
