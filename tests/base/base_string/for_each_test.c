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

#include "base/base_core.h"
#include "base/base_string.h"

static void test_empty_string(void) {
    struct str8 string = {0};
    const u8* element;
    size_t count = 0;

    for_each_char(element, &string) {
        (void)element;
        ++count;
    }

    assert(count == 0);
}

static void test_visits_each_character(void) {
    const u8 expected[] = "eunha";
    struct str8 string = str8_lit("eunha");
    const u8* element;
    size_t index = 0;

    for_each_char(element, &string) {
        assert(index < sizeof(expected) - 1);
        assert(element == string.data + index);
        assert(*element == expected[index]);
        ++index;
    }

    assert(index == sizeof(expected) - 1);
}

static void test_visits_embedded_null(void) {
    const u8 bytes[] = {'a', 0, 'b'};
    struct str8 string = str8(bytes, sizeof(bytes));
    const u8* element;
    size_t count = 0;

    for_each_char(element, &string) {
        ++count;
    }

    assert(count == sizeof(bytes));
}

static void test_break_and_continue(void) {
    struct str8 string = str8_lit("abcde");
    const u8* element;
    u8 visited[5] = {0};
    size_t index = 0;

    for_each_char(element, &string) {
        if (*element == 'b') {
            continue;
        }

        if (*element == 'd') {
            break;
        }

        visited[index++] = *element;
    }

    assert(index == 2);
    assert(visited[0] == 'a');
    assert(visited[1] == 'c');
}

static void test_nested_loops(void) {
    struct str8 outer = str8_lit("abc");
    struct str8 inner = str8_lit("xy");
    const u8* outer_element;
    const u8* inner_element;
    size_t count = 0;

    for_each_char(outer_element, &outer) {
        for_each_char(inner_element, &inner) {
            assert(*outer_element != '\0');
            assert(*inner_element != '\0');
            ++count;
        }
    }

    assert(count == 6);
}

int main(void) {
    test_empty_string();
    test_visits_each_character();
    test_visits_embedded_null();
    test_break_and_continue();
    test_nested_loops();
    return 0;
}
