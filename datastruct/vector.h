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

#include <stddef.h>

/*
 * Generic contiguous storage. The vector owns data and stores length/capacity
 * in elements, not bytes.
 */
struct vector {
    void* data;
    size_t item_size;
    size_t length;
    size_t capacity;
};

/*
 * Initializes vector with storage for elements of item_size bytes.
 */
int vector_init(struct vector* vector, size_t item_size);

/*
 * Returns the number of elements stored in vector.
 */
size_t vector_len(const struct vector* vector);

/*
 * Returns a pointer to the element at index. The returned pointer is owned by
 * the vector and can be invalidated by vector_append or vector_extend.
 */
void* vector_get(struct vector* vector, size_t index);

/*
 * Appends one element by copying item_size bytes from item. Pointers returned
 * by vector_get are invalidated after a growing append.
 */
int vector_append(struct vector* vector, const void* item);

/*
 * Appends count elements by copying count * item_size bytes from items.
 */
int vector_extend(struct vector* vector, const void* items, size_t count);

/*
 * Releases vector storage and resets the struct to an empty state.
 */
void vector_deinit(struct vector* vector);

#endif
