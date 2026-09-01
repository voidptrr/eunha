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

#ifndef EUNHA_VECTOR_H
#define EUNHA_VECTOR_H

#include <stdalign.h>
#include <stddef.h>
#include <eunha/arena.h>

/**
 * struct vector_params - Parameters used to create a vector.
 * @arena: Borrowed arena used to allocate backing buffers.
 * @item_size: Size of one stored item in bytes.
 * @item_alignment: Required alignment of each stored item.
 * @initial_capacity: Number of items to reserve; zero selects the default.
 */
struct vector_params {
    struct arena *arena;
    size_t item_size;
    size_t item_alignment;
    size_t initial_capacity;
};

/**
 * struct vector - Generic arena-backed contiguous array.
 * @arena: Borrowed arena used to allocate backing buffers.
 * @data: Current backing buffer.
 * @len: Number of items currently stored.
 * @capacity: Number of items that fit in the current backing buffer.
 * @item_size: Size of one stored item in bytes.
 * @item_alignment: Required alignment of each stored item.
 */
struct vector {
    struct arena *arena;
    void *data;
    size_t len;
    size_t capacity;
    size_t item_size;
    size_t item_alignment;
};

/** Default number of items reserved by a vector. */
#define VECTOR_DEFAULT_CAPACITY 8

/** Creates a vector for type from optional designated vector_params fields. */
#define vector(type, ...)                                         \
    vector_with_params(                                           \
        &(struct vector_params){ .item_size = sizeof(type),       \
                                 .item_alignment = alignof(type), \
                                 __VA_ARGS__ })

/**
 * Creates an empty vector using params. A zero initial_capacity selects
 * VECTOR_DEFAULT_CAPACITY. arena must be non-NULL, item_size must be nonzero
 * and a multiple of item_alignment, and item_alignment must be a nonzero power
 * of two.
 */
struct vector vector_with_params(const struct vector_params *params);

/** Copies item to the back of vector, growing its backing storage when needed. */
void vector_push(struct vector *vector, const void *item);

/**
 * Removes and returns the final item, or NULL when vector is empty. The
 * returned pointer may be overwritten by a later vector mutation.
 */
void *vector_pop(struct vector *vector);

/**
 * Returns the item at index, or NULL when index is out of bounds. The returned
 * pointer must not be retained across an operation that may grow vector.
 */
void *vector_at(struct vector *vector, size_t index);

/** Ensures vector can hold at least capacity items without growing. */
void vector_reserve(struct vector *vector, size_t capacity);

/** Removes all items without releasing the current backing buffer. */
void vector_clear(struct vector *vector);

/** Returns the number of items currently stored in vector. */
size_t vector_len(const struct vector *vector);

#endif
