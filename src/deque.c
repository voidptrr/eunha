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

static bool deque_is_valid(const struct deque *deque)
{
    if (deque == NULL) {
        return false;
    }

    return (deque->arena != NULL && deque->data != NULL &&
            deque->capacity != 0 && deque->len <= deque->capacity &&
            deque->head < deque->capacity) != 0;
}

static size_t deque_size(size_t capacity, size_t item_size)
{
    size_t size = 0;
    if (ckd_mul(&size, capacity, item_size)) {
        abort();
    }

    return size;
}

static size_t deque_index(const struct deque *deque, size_t index)
{
    size_t remaining = deque->capacity - deque->head;
    return index < remaining ? deque->head + index : index - remaining;
}

static void deque_grow(struct deque *deque)
{
    size_t capacity = 0;
    if (ckd_mul(&capacity, deque->capacity, (size_t)2)) {
        abort();
    }

    size_t size = deque_size(capacity, deque->item_size);
    u8 *data = arena_push(deque->arena, size, deque->item_alignment);

    if (deque->len != 0) {
        size_t first_len = deque->capacity - deque->head;
        if (first_len > deque->len) {
            first_len = deque->len;
        }

        size_t first_size = first_len * deque->item_size;
        memcpy(data, (u8 *)deque->data + (deque->head * deque->item_size),
               first_size);

        size_t second_len = deque->len - first_len;
        memcpy(data + first_size, deque->data, second_len * deque->item_size);
    }

    deque->data = data;
    deque->capacity = capacity;
    deque->head = 0;
}

struct deque deque_with_params(const struct deque_params *params)
{
    assert(params != NULL);
    assert(params->arena != NULL);
    assert(params->item_size != 0);
    assert(params->item_alignment != 0);
    assert(params->item_size % params->item_alignment == 0);

    struct deque result = {
        .arena = params->arena,
        .item_size = params->item_size,
        .item_alignment = params->item_alignment,
    };

    size_t capacity = params->initial_capacity != 0 ? params->initial_capacity :
                                                      DEQUE_DEFAULT_CAPACITY;
    size_t size = deque_size(capacity, params->item_size);
    result.data = arena_push(params->arena, size, params->item_alignment);
    result.capacity = capacity;
    return result;
}

void deque_push(struct deque *deque, const void *item)
{
    assert(deque_is_valid(deque));
    assert(item != NULL);

    if (deque->len == deque->capacity) {
        deque_grow(deque);
    }

    size_t tail = deque_index(deque, deque->len);
    u8 *dst = (u8 *)deque->data + (tail * deque->item_size);
    memmove(dst, item, deque->item_size);
    deque->len += 1;
}

void deque_pushfront(struct deque *deque, const void *item)
{
    assert(deque_is_valid(deque));
    assert(item != NULL);

    if (deque->len == deque->capacity) {
        deque_grow(deque);
    }

    deque->head = deque->head == 0 ? deque->capacity - 1 : deque->head - 1;
    u8 *dst = (u8 *)deque->data + (deque->head * deque->item_size);
    memmove(dst, item, deque->item_size);
    deque->len += 1;
}

void *deque_popback(struct deque *deque)
{
    assert(deque_is_valid(deque));

    if (deque->len == 0) {
        return NULL;
    }

    size_t tail = deque_index(deque, deque->len - 1);
    void *item = (u8 *)deque->data + (tail * deque->item_size);
    deque->len -= 1;
    if (deque->len == 0) {
        deque->head = 0;
    }

    return item;
}

void *deque_popfront(struct deque *deque)
{
    assert(deque_is_valid(deque));

    if (deque->len == 0) {
        return NULL;
    }

    void *item = (u8 *)deque->data + (deque->head * deque->item_size);
    deque->head = deque->head == deque->capacity - 1 ? 0 : deque->head + 1;
    deque->len -= 1;
    if (deque->len == 0) {
        deque->head = 0;
    }

    return item;
}

size_t deque_len(const struct deque *deque)
{
    assert(deque_is_valid(deque));
    return deque->len;
}
