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
    uint8_t* next_data = realloc(string->data, next_capacity + 1);
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

/*
 * Initializes an empty owned string.
 */
enum eunha_result string_init(struct string* string) {
    assert(string != NULL);

    string->data = malloc(STRING_DEFAULT_CAPACITY + 1);
    if (string->data == NULL) {
        return EUNHA_ERROR;
    }

    string->data[0] = 0;
    string->length = 0;
    string->capacity = STRING_DEFAULT_CAPACITY;
    return EUNHA_OK;
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

/*
 * Replaces current contents with length bytes copied from data.
 */
enum eunha_result string_set(
    struct string* string, const uint8_t* data, size_t length) {
    assert(string != NULL);
    assert(data != NULL || length == 0);

    string_clear(string);
    return string_append(string, data, length);
}

/*
 * Releases owned storage and resets fields.
 */
void string_deinit(struct string* string) {
    assert(string != NULL);

    free(string->data);
    string_reset(string);
}
