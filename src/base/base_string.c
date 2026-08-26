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

static int string_grow(struct string_t* dst, size_t final_len) {
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

struct string_t* string_init(const char* initial) {
    size_t initial_len = initial != NULL ? strlen(initial) : 0;
    struct string_t* string = malloc(sizeof(struct string_t));
    if (string == NULL) {
        exit(EXIT_FAILURE);
    }

    string->capacity =
        initial_len > DEFAULT_CAPACITY ? initial_len : DEFAULT_CAPACITY;
    string->len = initial_len;

    char* initial_data = malloc((sizeof(char) * string->capacity) + 1);
    if (initial_data == NULL) {
        exit(EXIT_FAILURE);
    }

    string->data = initial_data;

    if (initial_len > 0) {
        memcpy(string->data, initial, initial_len);
    }

    string->data[initial_len] = '\0';
    return string;
}

size_t string_len(const struct string_t* string) {
    assert(string != NULL);
    return string->len;
}

void string_append(struct string_t* dst, const char* src) {
    assert(dst != NULL);
    assert(src != NULL);

    size_t dst_len = string_len(dst);
    size_t src_len = strlen(src);

    size_t final_len = 0;
    if (ckd_add(&final_len, dst_len, src_len)) {
        string_deinit(dst);
        exit(EXIT_FAILURE);
    }

    if (final_len > dst->capacity && string_grow(dst, final_len)) {
        string_deinit(dst);
        exit(EXIT_FAILURE);
    }

    memmove(dst->data + dst_len, src, src_len);
    dst->len = final_len;
    dst->data[final_len] = '\0';
}

void string_prepend(struct string_t* dst, const char* src) {
    assert(dst != NULL);
    assert(src != NULL);

    size_t src_len = strlen(src);
    size_t dst_len = string_len(dst);
    size_t final_len = 0;
    if (ckd_add(&final_len, dst_len, src_len)) {
        string_deinit(dst);
        exit(EXIT_FAILURE);
    }

    if (final_len > dst->capacity && string_grow(dst, final_len)) {
        string_deinit(dst);
        exit(EXIT_FAILURE);
    }

    memmove(dst->data + src_len, dst->data, dst_len + 1);
    memcpy(dst->data, src, src_len);
    dst->len = final_len;
}

bool string_contains(const struct string_t* string, const char* needle) {
    assert(string != NULL);
    assert(needle != NULL);

    return strstr(string->data, needle) != NULL;
}

bool string_starts_with(const struct string_t* string, const char* prefix) {
    assert(string != NULL);
    assert(prefix != NULL);

    size_t prefix_len = strlen(prefix);
    return (string_len(string) >= prefix_len &&
            memcmp(string->data, prefix, prefix_len) == 0) != 0;
}

void string_deinit(struct string_t* string) {
    assert(string != NULL);
    if (string->data != NULL) {
        free(string->data);
    }

    free(string);
}
