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
#include <eunha/deque.h>

static size_t deque_allocation_size(size_t capacity, size_t item_size)
{
    size_t result = 0;
    if (ckd_mul(&result, capacity, item_size)) {
        abort();
    }

    return result;
}

static size_t deque_physical_index(const struct deque *deque,
                                   size_t logical_index)
{
    assert(deque != NULL);
    assert(deque->capacity != 0);
    assert(deque->head < deque->capacity);
    assert(logical_index < deque->capacity);

    size_t slots_to_end = deque->capacity - deque->head;
    return logical_index < slots_to_end ? deque->head + logical_index :
                                          logical_index - slots_to_end;
}

static void deque_grow(struct deque *deque)
{
    assert(deque != NULL);
    assert(deque->len == deque->capacity);

    size_t new_capacity = DEQUE_DEFAULT_CAPACITY;
    if (deque->capacity != 0 &&
        ckd_mul(&new_capacity, deque->capacity, (size_t)2)) {
        abort();
    }

    size_t allocation_size =
        deque_allocation_size(new_capacity, deque->item_size);
    u8 *new_data =
        arena_push(deque->arena, allocation_size, deque->item_alignment);

    if (deque->len != 0) {
        size_t first_count = deque->capacity - deque->head;
        if (first_count > deque->len) {
            first_count = deque->len;
        }

        size_t first_size = first_count * deque->item_size;
        memcpy(new_data, (u8 *)deque->data + (deque->head * deque->item_size),
               first_size);

        size_t second_count = deque->len - first_count;
        memcpy(new_data + first_size, deque->data,
               second_count * deque->item_size);
    }

    deque->data = new_data;
    deque->capacity = new_capacity;
    deque->head = 0;
}

struct deque deque_with_params(const struct deque_params *params)
{
    assert(params != NULL);
    assert(params->arena != NULL);
    assert(params->item_size != 0);
    assert(is_pow2(params->item_alignment));
    assert(params->item_size % params->item_alignment == 0);

    struct deque result = {
        .arena = params->arena,
        .item_size = params->item_size,
        .item_alignment = params->item_alignment,
    };

    if (params->initial_capacity == 0) {
        return result;
    }

    size_t allocation_size =
        deque_allocation_size(params->initial_capacity, params->item_size);
    result.data =
        arena_push(params->arena, allocation_size, params->item_alignment);
    result.capacity = params->initial_capacity;
    return result;
}

void deque_push(struct deque *deque, const void *item)
{
    assert(deque != NULL);
    assert(item != NULL);
    assert(deque->len <= deque->capacity);

    if (deque->len == deque->capacity) {
        deque_grow(deque);
    }

    size_t tail = deque_physical_index(deque, deque->len);
    u8 *destination = (u8 *)deque->data + (tail * deque->item_size);
    memmove(destination, item, deque->item_size);
    deque->len += 1;
}

void deque_pushfront(struct deque *deque, const void *item)
{
    assert(deque != NULL);
    assert(item != NULL);
    assert(deque->len <= deque->capacity);

    if (deque->len == deque->capacity) {
        deque_grow(deque);
    }

    deque->head = deque->head == 0 ? deque->capacity - 1 : deque->head - 1;
    u8 *destination = (u8 *)deque->data + (deque->head * deque->item_size);
    memmove(destination, item, deque->item_size);
    deque->len += 1;
}

void *deque_popback(struct deque *deque)
{
    assert(deque != NULL);
    assert(deque->len <= deque->capacity);

    if (deque->len == 0) {
        return NULL;
    }

    size_t tail = deque_physical_index(deque, deque->len - 1);
    void *result = (u8 *)deque->data + (tail * deque->item_size);
    deque->len -= 1;
    if (deque->len == 0) {
        deque->head = 0;
    }

    return result;
}

void *deque_popfront(struct deque *deque)
{
    assert(deque != NULL);
    assert(deque->len <= deque->capacity);

    if (deque->len == 0) {
        return NULL;
    }

    void *result = (u8 *)deque->data + (deque->head * deque->item_size);
    deque->head = deque->head == deque->capacity - 1 ? 0 : deque->head + 1;
    deque->len -= 1;
    if (deque->len == 0) {
        deque->head = 0;
    }

    return result;
}

size_t deque_len(const struct deque *deque)
{
    assert(deque != NULL);
    return deque->len;
}
