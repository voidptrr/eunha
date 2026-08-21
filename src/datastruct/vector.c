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

#include "vector.h"

#define VECTOR_DEFAULT_CAPACITY 8

/*
 * Grows capacity geometrically while guarding the capacity * item_size
 * multiplication from overflow.
 */
static int vector_grow(struct vector* vector) {
    assert(vector != NULL);
    assert(vector->item_size != 0);

    size_t max_capacity = SIZE_MAX / vector->item_size;
    size_t next_capacity = VECTOR_DEFAULT_CAPACITY;

    if (vector->capacity != 0) {
        if (vector->capacity > max_capacity / 2) {
            next_capacity = max_capacity;
        } else {
            next_capacity = vector->capacity * 2;
        }
    }

    if (next_capacity <= vector->capacity || next_capacity > max_capacity) {
        errno = ENOMEM;
        return -1;
    }

    void* next_data = realloc(vector->data, next_capacity * vector->item_size);
    if (next_data == NULL) {
        return -1;
    }

    vector->data = next_data;
    vector->capacity = next_capacity;
    return 0;
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
int vector_init(struct vector* vector, size_t item_size) {
    assert(vector != NULL);
    assert(item_size != 0);

    if (VECTOR_DEFAULT_CAPACITY > SIZE_MAX / item_size) {
        errno = ENOMEM;
        return -1;
    }

    void* data = malloc(VECTOR_DEFAULT_CAPACITY * item_size);
    if (data == NULL) {
        return -1;
    }

    vector->data = data;
    vector->item_size = item_size;
    vector->length = 0;
    vector->capacity = VECTOR_DEFAULT_CAPACITY;
    return 0;
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
 * Appends a single element by reusing the bulk append path.
 */
int vector_append(struct vector* vector, const void* item) {
    assert(vector != NULL);
    assert(vector->item_size != 0);
    assert(item != NULL);

    return vector_extend(vector, item, 1);
}

/*
 * Appends count contiguous elements, growing storage before copying.
 */
int vector_extend(struct vector* vector, const void* items, size_t count) {
    assert(vector != NULL);
    assert(vector->item_size != 0);

    if (count == 0) {
        return 0;
    }

    assert(items != NULL);

    if (count > SIZE_MAX - vector->length) {
        errno = ENOMEM;
        return -1;
    }

    size_t required_length = vector->length + count;

    while (required_length > vector->capacity) {
        if (vector_grow(vector) == -1) {
            return -1;
        }
    }

    uint8_t* next_item =
        (uint8_t*)vector->data + (vector->length * vector->item_size);
    memcpy(next_item, items, count * vector->item_size);
    vector->length = required_length;
    return 0;
}

/*
 * Frees vector-owned storage and clears the fields.
 */
void vector_deinit(struct vector* vector) {
    assert(vector != NULL);

    free(vector->data);
    vector_reset(vector);
}
