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
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "datastruct/string.h"

#define STRING_DEFAULT_CAPACITY 16

/*
 * Grows storage to hold required_capacity bytes plus the trailing NUL byte.
 */
static enum eunha_result string_reserve(
    struct string* string, size_t required_capacity) {
    assert(string != NULL);

    if (required_capacity <= string->capacity) {
        return EUNHA_OK;
    }

    size_t next_capacity = STRING_DEFAULT_CAPACITY;
    if (string->capacity != 0) {
        next_capacity = string->capacity;
    }

    while (next_capacity < required_capacity) {
        /*
         * Saturate at the requested size when another doubling could overflow.
         */
        if (next_capacity > SIZE_MAX / 2) {
            next_capacity = required_capacity;
            break;
        }

        next_capacity *= 2;
    }

    /* One additional byte must remain representable for the NUL terminator. */
    if (next_capacity == SIZE_MAX) {
        errno = ENOMEM;
        return EUNHA_ERROR;
    }

    /* realloc leaves the original allocation owned by string on failure. */
    char* next_data = realloc(string->data, next_capacity + 1);
    if (next_data == NULL) {
        return EUNHA_ERROR;
    }

    string->data = next_data;
    string->capacity = next_capacity;
    return EUNHA_OK;
}

/*
 * Leaves a deinitialized string in an obvious empty state for debugging.
 */
static void string_reset(struct string* string) {
    string->data = NULL;
    string->length = 0;
    string->capacity = 0;
}

/* Releases storage for public cleanup and internal ownership rollback. */
static void string_release(struct string* string) {
    free(string->data);
    string_reset(string);
}

/*
 * Initializes an owned string from a range that may contain embedded NULs.
 */
static enum eunha_result string_init_range(
    struct string* string, const char* data, size_t length) {
    assert(string != NULL);
    assert(data != NULL || length == 0);

    size_t capacity =
        length > STRING_DEFAULT_CAPACITY ? length : STRING_DEFAULT_CAPACITY;
    if (capacity == SIZE_MAX) {
        errno = ENOMEM;
        return EUNHA_ERROR;
    }

    string->data = malloc(capacity + 1);
    if (string->data == NULL) {
        return EUNHA_ERROR;
    }

    if (length != 0) {
        memcpy(string->data, data, length);
    }

    string->data[length] = 0;
    string->length = length;
    string->capacity = capacity;
    return EUNHA_OK;
}

/* Initializes an owned string by copying a C string. */
enum eunha_result string_init(struct string* string, const char* initial) {
    assert(string != NULL);
    assert(initial != NULL);

    return string_init_range(string, initial, strlen(initial));
}

/*
 * Removes current contents without releasing storage.
 */
void string_clear(struct string* string) {
    assert(string != NULL);
    assert(string->data != NULL);

    string->length = 0;
    string->data[0] = 0;
}

/*
 * Appends length bytes copied from data.
 */
enum eunha_result string_append(
    struct string* string, const uint8_t* data, size_t length) {
    assert(string != NULL);
    assert(string->data != NULL);
    assert(data != NULL || length == 0);

    if (length == 0) {
        return EUNHA_OK;
    }

    /* Check the addition before calculating the required length. */
    if (length > SIZE_MAX - string->length) {
        errno = ENOMEM;
        return EUNHA_ERROR;
    }

    size_t next_length = string->length + length;
    if (string_reserve(string, next_length) == EUNHA_ERROR) {
        return EUNHA_ERROR;
    }

    memcpy(string->data + string->length, data, length);
    string->length = next_length;
    /* Preserve the C-string convenience invariant after every append. */
    string->data[string->length] = 0;
    return EUNHA_OK;
}

/* Transfers source storage into an initialized destination. */
void string_move(struct string* destination, struct string* source) {
    assert(destination != NULL);
    assert(source != NULL);
    assert(destination != source);

    string_release(destination);
    *destination = *source;
    string_reset(source);
}

/* Compares length before bytes so embedded NULs cannot truncate a match. */
bool string_equals(const struct string* string, const char* expected) {
    assert(string != NULL);
    assert(string->data != NULL);
    assert(expected != NULL);

    size_t expected_length = strlen(expected);
    return string->length == expected_length &&
           memcmp(string->data, expected, expected_length) == 0;
}

/* Removes optional horizontal whitespace while preserving owned storage. */
void string_trim(struct string* string) {
    assert(string != NULL);
    assert(string->data != NULL);

    size_t start = 0;
    size_t end = string->length;

    while (start < end &&
           (string->data[start] == ' ' || string->data[start] == '\t')) {
        start += 1;
    }

    while (end > start &&
           (string->data[end - 1] == ' ' || string->data[end - 1] == '\t')) {
        end -= 1;
    }

    size_t trimmed_length = end - start;
    if (start != 0 && trimmed_length != 0) {
        memmove(string->data, string->data + start, trimmed_length);
    }

    string->length = trimmed_length;
    string->data[trimmed_length] = 0;
}

/* Splits into owned strings so callers never manage borrowed byte slices. */
enum eunha_result string_split_once(
    const struct string* string, char delimiter, struct string_split* split) {
    assert(string != NULL);
    assert(string->data != NULL);
    assert(split != NULL);

    *split = (struct string_split){0};
    const char* match = memchr(string->data, delimiter, string->length);

    if (match == NULL) {
        if (string_init_range(&split->values[0], string->data,
                string->length) == EUNHA_ERROR) {
            return EUNHA_ERROR;
        }

        split->length = 1;
        return EUNHA_OK;
    }

    size_t first_length = (size_t)(match - string->data);
    size_t second_length = string->length - first_length - 1;

    if (string_init_range(&split->values[0], string->data, first_length) ==
        EUNHA_ERROR) {
        return EUNHA_ERROR;
    }

    if (string_init_range(&split->values[1], match + 1, second_length) ==
        EUNHA_ERROR) {
        string_release(&split->values[0]);
        return EUNHA_ERROR;
    }

    split->length = 2;
    return EUNHA_OK;
}

/* Releases both fixed slots so partially initialized results are also safe. */
void string_split_deinit(struct string_split* split) {
    assert(split != NULL);

    string_release(&split->values[1]);
    string_release(&split->values[0]);
    split->length = 0;
}

/* Parses decimal digits while reserving SIZE_MAX as the error sentinel. */
size_t string_parse_size(const struct string* string) {
    assert(string != NULL);
    assert(string->data != NULL);

    if (string->length == 0) {
        errno = EINVAL;
        return SIZE_MAX;
    }

    size_t parsed = 0;

    for (size_t index = 0; index < string->length; index += 1) {
        char byte = string->data[index];
        if (byte < '0' || byte > '9') {
            errno = EINVAL;
            return SIZE_MAX;
        }

        size_t digit = (size_t)(byte - '0');
        if (parsed > (SIZE_MAX - digit) / 10) {
            errno = EOVERFLOW;
            return SIZE_MAX;
        }

        parsed = (parsed * 10) + digit;
    }

    if (parsed == SIZE_MAX) {
        errno = EOVERFLOW;
        return SIZE_MAX;
    }

    return parsed;
}

/*
 * Releases owned storage and resets fields.
 */
void string_deinit(struct string* string) {
    assert(string != NULL);

    string_release(string);
}
