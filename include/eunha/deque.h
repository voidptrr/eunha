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

#ifndef EUNHA_DEQUE_H
#define EUNHA_DEQUE_H

#include <stddef.h>
#include <eunha/arena.h>

/**
 * struct deque_params - Parameters used to create a deque.
 * @arena: Borrowed arena used to allocate backing buffers.
 * @item_size: Size of one stored item in bytes.
 * @item_alignment: Required alignment of each stored item.
 * @initial_capacity: Number of items to reserve initially; zero grows lazily.
 */
struct deque_params {
    struct arena *arena;
    size_t item_size;
    size_t item_alignment;
    size_t initial_capacity;
};

/**
 * struct deque - Generic double-ended queue.
 * @arena: Borrowed arena used to allocate backing buffers.
 * @data: Current circular backing buffer, or NULL before the first push.
 * @len: Number of items currently stored.
 * @capacity: Number of items that fit in the current backing buffer.
 * @head: Index of the first item in the circular backing buffer.
 * @item_size: Size of one stored item in bytes.
 * @item_alignment: Required alignment of each stored item.
 */
struct deque {
    struct arena *arena;
    void *data;
    size_t len;
    size_t capacity;
    size_t head;
    size_t item_size;
    size_t item_alignment;
};

/** Creates a deque from designated deque_params fields. */
#define deque(...) deque_with_params(&(struct deque_params){ __VA_ARGS__ })

/**
 * Creates an empty deque using params. arena must be non-NULL, item_size must
 * be nonzero and a multiple of item_alignment, and item_alignment must be a
 * nonzero power of two.
 */
struct deque deque_with_params(const struct deque_params *params);

/** Copies item to the back of deque, growing its backing storage when needed. */
void deque_push(struct deque *deque, const void *item);

/** Copies item to the front of deque, growing its backing storage when needed. */
void deque_pushfront(struct deque *deque, const void *item);

/**
 * Removes and returns the item at the back of deque, or NULL when the deque is
 * empty. The returned pointer may be overwritten by a later deque mutation.
 */
void *deque_popback(struct deque *deque);

/**
 * Removes and returns the item at the front of deque, or NULL when the deque is
 * empty. The returned pointer may be overwritten by a later deque mutation.
 */
void *deque_popfront(struct deque *deque);

/** Returns the number of items currently stored in deque. */
size_t deque_len(const struct deque *deque);

#endif
