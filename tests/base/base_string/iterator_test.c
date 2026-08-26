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

static void test_iterator_init(void) {
    struct string_t* string = string_init("eunha");
    struct string_iterator_t iterator = string_iterator_init(string);

    assert(iterator.current_index == 0);
    assert(iterator.len == string_len(string));
    assert(iterator.data == string->data);

    string_deinit(string);
}

static void test_empty_iterator(void) {
    struct string_t* string = string_init(NULL);
    struct string_iterator_t iterator = string_iterator_init(string);
    char element = 'x';

    assert(!string_iterator_next(&iterator, &element));
    assert(element == 'x');
    assert(iterator.current_index == 0);

    string_deinit(string);
}

static void test_iterator_visits_each_character(void) {
    const char expected[] = "eunha";
    struct string_t* string = string_init(expected);
    struct string_iterator_t iterator = string_iterator_init(string);
    size_t index = 0;
    char element = '\0';

    while (string_iterator_next(&iterator, &element)) {
        assert(index < sizeof(expected) - 1);
        assert(element == expected[index]);
        ++index;
    }

    assert(index == sizeof(expected) - 1);
    assert(iterator.current_index == index);
    assert(!string_iterator_next(&iterator, &element));
    assert(iterator.current_index == index);

    string_deinit(string);
}

static void test_iterators_advance_independently(void) {
    struct string_t* string = string_init("abc");
    struct string_iterator_t first = string_iterator_init(string);
    struct string_iterator_t second = string_iterator_init(string);
    char element = '\0';

    assert(string_iterator_next(&first, &element));
    assert(element == 'a');
    assert(string_iterator_next(&first, &element));
    assert(element == 'b');

    assert(string_iterator_next(&second, &element));
    assert(element == 'a');

    string_deinit(string);
}

int main(void) {
    test_iterator_init();
    test_empty_iterator();
    test_iterator_visits_each_character();
    test_iterators_advance_independently();
    return 0;
}
