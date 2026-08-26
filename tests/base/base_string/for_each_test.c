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

#include "base/base_string.h"

static void test_empty_string(void) {
    struct string* string = string_alloc(NULL);
    const char* element;
    size_t count = 0;

    for_each_char(element, string) {
        (void)element;
        ++count;
    }

    assert(count == 0);
    string_free(string);
}

static void test_visits_each_character(void) {
    const char expected[] = "eunha";
    struct string* string = string_alloc(expected);
    const char* element;
    size_t index = 0;

    for_each_char(element, string) {
        assert(index < sizeof(expected) - 1);
        assert(element == string->data + index);
        assert(*element == expected[index]);
        ++index;
    }

    assert(index == sizeof(expected) - 1);
    string_free(string);
}

static void test_break_and_continue(void) {
    struct string* string = string_alloc("abcde");
    const char* element;
    char visited[3] = {0};
    size_t index = 0;

    for_each_char(element, string) {
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
    string_free(string);
}

static void test_nested_loops(void) {
    struct string* outer = string_alloc("abc");
    struct string* inner = string_alloc("xy");
    const char* outer_element;
    const char* inner_element;
    size_t count = 0;

    for_each_char(outer_element, outer) {
        for_each_char(inner_element, inner) {
            assert(*outer_element != '\0');
            assert(*inner_element != '\0');
            ++count;
        }
    }

    assert(count == 6);
    string_free(outer);
    string_free(inner);
}

int main(void) {
    test_empty_string();
    test_visits_each_character();
    test_break_and_continue();
    test_nested_loops();
    return 0;
}
