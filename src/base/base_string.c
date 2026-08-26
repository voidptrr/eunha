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
#include <stdckdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "base_string.h"

#define DEFAULT_CAPACITY 8

static int string_grow(struct string* dst, size_t final_len) {
    if (ckd_mul(&dst->capacity, final_len, 2)) {
        return 1;
    }

    char* tmp = realloc(dst->data, dst->capacity + 1);
    if (tmp == NULL) {
        return 1;
    }

    dst->data = tmp;

    return 0;
}

struct string* string_alloc(const char* initial) {
    size_t initial_len = initial != NULL ? strlen(initial) : 0;
    struct string* str = malloc(sizeof(struct string));
    if (str == NULL) {
        exit(EXIT_FAILURE);
    }

    str->capacity =
        initial_len > DEFAULT_CAPACITY ? initial_len : DEFAULT_CAPACITY;
    str->len = initial_len;

    char* initial_data = malloc((sizeof(char) * str->capacity) + 1);
    if (initial_data == NULL) {
        exit(EXIT_FAILURE);
    }

    str->data = initial_data;

    if (initial_len > 0) {
        memcpy(str->data, initial, initial_len);
    }

    str->data[initial_len] = '\0';
    return str;
}

size_t string_len(const struct string* str) {
    assert(str != NULL);
    return str->len;
}

void string_append(struct string* dst, const char* src) {
    assert(dst != NULL);
    assert(src != NULL);

    size_t dst_len = string_len(dst);
    size_t src_len = strlen(src);

    size_t final_len = 0;
    if (ckd_add(&final_len, dst_len, src_len)) {
        string_free(dst);
        exit(EXIT_FAILURE);
    }

    if (final_len > dst->capacity && string_grow(dst, final_len)) {
        string_free(dst);
        exit(EXIT_FAILURE);
    }

    memmove(dst->data + dst_len, src, src_len);
    dst->len = final_len;
    dst->data[final_len] = '\0';
}

void string_prepend(struct string* dst, const char* src) {
    assert(dst != NULL);
    assert(src != NULL);

    size_t src_len = strlen(src);
    size_t dst_len = string_len(dst);
    size_t final_len = 0;
    if (ckd_add(&final_len, dst_len, src_len)) {
        string_free(dst);
        exit(EXIT_FAILURE);
    }

    if (final_len > dst->capacity && string_grow(dst, final_len)) {
        string_free(dst);
        exit(EXIT_FAILURE);
    }

    memmove(dst->data + src_len, dst->data, dst_len + 1);
    memcpy(dst->data, src, src_len);
    dst->len = final_len;
}

bool string_contains(const struct string* haystack, const char* needle) {
    assert(haystack != NULL);
    assert(needle != NULL);

    return strstr(haystack->data, needle) != NULL;
}

bool string_has_prefix(const struct string* str, const char* prefix) {
    assert(str != NULL);
    assert(prefix != NULL);

    size_t prefix_len = strlen(prefix);
    return (string_len(str) >= prefix_len &&
            memcmp(str->data, prefix, prefix_len) == 0) != 0;
}

void string_free(struct string* str) {
    assert(str != NULL);
    if (str->data != NULL) {
        free(str->data);
    }

    free(str);
}
