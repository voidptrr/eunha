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
#include <eunha/arena.h>
#include <eunha/core.h>
#include <eunha/vector.h>

static bool vector_is_valid(const struct vector *vector)
{
    if (vector == NULL) {
        return false;
    }

    return (vector->arena != NULL && vector->data != NULL &&
            vector->capacity != 0 && vector->len <= vector->capacity) != 0;
}

static size_t vector_size(size_t capacity, size_t item_size)
{
    size_t size = 0;
    if (ckd_mul(&size, capacity, item_size)) {
        abort();
    }

    return size;
}

static u8 *vector_item(struct vector *vector, size_t index)
{
    return (u8 *)vector->data + (index * vector->item_size);
}

static void vector_resize(struct vector *vector, size_t capacity)
{
    size_t size = vector_size(capacity, vector->item_size);
    u8 *data = arena_push(vector->arena, size, vector->item_alignment);
    if (vector->len != 0) {
        memcpy(data, vector->data, vector_size(vector->len, vector->item_size));
    }

    vector->data = data;
    vector->capacity = capacity;
}

static void vector_grow(struct vector *vector)
{
    size_t capacity = 0;
    if (ckd_mul(&capacity, vector->capacity, (size_t)2)) {
        abort();
    }

    vector_resize(vector, capacity);
}

struct vector vector_with_params(const struct vector_params *params)
{
    assert(params != NULL);
    assert(params->arena != NULL);
    assert(params->item_size != 0);
    assert(params->item_alignment != 0);
    assert(params->item_size % params->item_alignment == 0);

    struct vector result = {
        .arena = params->arena,
        .item_size = params->item_size,
        .item_alignment = params->item_alignment,
    };

    size_t capacity = params->initial_capacity != 0 ? params->initial_capacity :
                                                      VECTOR_DEFAULT_CAPACITY;
    vector_resize(&result, capacity);

    return result;
}

void vector_push(struct vector *vector, const void *item)
{
    assert(vector_is_valid(vector));
    assert(item != NULL);

    if (vector->len == vector->capacity) {
        vector_grow(vector);
    }

    u8 *dst = vector_item(vector, vector->len);
    memmove(dst, item, vector->item_size);
    vector->len += 1;
}

void *vector_pop(struct vector *vector)
{
    assert(vector_is_valid(vector));

    if (vector->len == 0) {
        return NULL;
    }

    vector->len -= 1;
    return vector_item(vector, vector->len);
}

void *vector_at(struct vector *vector, size_t index)
{
    assert(vector_is_valid(vector));

    if (index >= vector->len) {
        return NULL;
    }

    return vector_item(vector, index);
}

void vector_reserve(struct vector *vector, size_t capacity)
{
    assert(vector_is_valid(vector));

    if (capacity > vector->capacity) {
        vector_resize(vector, capacity);
    }
}

void vector_clear(struct vector *vector)
{
    assert(vector_is_valid(vector));
    vector->len = 0;
}

size_t vector_len(const struct vector *vector)
{
    assert(vector_is_valid(vector));
    return vector->len;
}
