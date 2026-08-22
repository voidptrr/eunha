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

#include "datastruct/vector.h"

#define VECTOR_DEFAULT_CAPACITY 8

/*
 * Grows capacity geometrically while guarding the capacity * item_size
 * multiplication from overflow.
 */
static enum eunha_result vector_grow(struct vector* vector) {
    assert(vector != NULL);
    assert(vector->item_size != 0);

    /*
     * Capacities above this value cannot be converted safely into byte sizes.
     */
    size_t max_capacity = SIZE_MAX / vector->item_size;
    size_t next_capacity = VECTOR_DEFAULT_CAPACITY;

    if (vector->capacity != 0) {
        /* Saturate when doubling would exceed the largest safe capacity. */
        if (vector->capacity > max_capacity / 2) {
            next_capacity = max_capacity;
        } else {
            next_capacity = vector->capacity * 2;
        }
    }

    /* No larger representable allocation means the vector cannot grow. */
    if (next_capacity <= vector->capacity || next_capacity > max_capacity) {
        errno = ENOMEM;
        return EUNHA_ERROR;
    }

    void* next_data = realloc(vector->data, next_capacity * vector->item_size);
    if (next_data == NULL) {
        return EUNHA_ERROR;
    }

    vector->data = next_data;
    vector->capacity = next_capacity;
    return EUNHA_OK;
}

/*
 * Leaves a deinitialized vector in an obvious empty state for debugging.
 */
static void vector_reset(struct vector* vector) {
    vector->data = NULL;
    vector->item_size = 0;
    vector->length = 0;
    vector->capacity = 0;
}

/*
 * Allocates the initial backing storage for elements of item_size bytes.
 */
enum eunha_result vector_init(struct vector* vector, size_t item_size) {
    assert(vector != NULL);
    assert(item_size != 0);

    /* Validate element-to-byte conversion before the initial allocation. */
    if (VECTOR_DEFAULT_CAPACITY > SIZE_MAX / item_size) {
        errno = ENOMEM;
        return EUNHA_ERROR;
    }

    void* data = malloc(VECTOR_DEFAULT_CAPACITY * item_size);
    if (data == NULL) {
        return EUNHA_ERROR;
    }

    vector->data = data;
    vector->item_size = item_size;
    vector->length = 0;
    vector->capacity = VECTOR_DEFAULT_CAPACITY;
    return EUNHA_OK;
}

/*
 * Returns the current number of initialized elements.
 */
size_t vector_len(const struct vector* vector) {
    assert(vector != NULL);

    return vector->length;
}

/*
 * Returns a mutable pointer to one element inside the backing storage.
 */
void* vector_get(struct vector* vector, size_t index) {
    assert(vector != NULL);
    assert(index < vector->length);

    return (uint8_t*)vector->data + (index * vector->item_size);
}

/*
 * Appends count contiguous elements, growing storage before copying.
 */
enum eunha_result vector_extend(
    struct vector* vector, const void* items, size_t count) {
    assert(vector != NULL);
    assert(vector->item_size != 0);

    if (count == 0) {
        return EUNHA_OK;
    }

    assert(items != NULL);

    /* Check element-count addition before calculating the new length. */
    if (count > SIZE_MAX - vector->length) {
        errno = ENOMEM;
        return EUNHA_ERROR;
    }

    size_t required_length = vector->length + count;

    while (required_length > vector->capacity) {
        if (vector_grow(vector) == EUNHA_ERROR) {
            return EUNHA_ERROR;
        }
    }

    /*
     * Capacity guarantees both the destination offset and copy size are safe.
     */
    uint8_t* next_item =
        (uint8_t*)vector->data + (vector->length * vector->item_size);
    memcpy(next_item, items, count * vector->item_size);
    vector->length = required_length;
    return EUNHA_OK;
}

/*
 * Appends a single element by reusing the bulk append path.
 */
enum eunha_result vector_append(struct vector* vector, const void* item) {
    assert(vector != NULL);
    assert(vector->item_size != 0);
    assert(item != NULL);

    return vector_extend(vector, item, 1);
}

/*
 * Frees vector-owned storage and clears the fields.
 */
void vector_deinit(struct vector* vector) {
    assert(vector != NULL);

    free(vector->data);
    vector_reset(vector);
}
