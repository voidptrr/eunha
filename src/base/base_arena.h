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

#ifndef BASE_ARENA_H
#define BASE_ARENA_H

#include <stdalign.h>
#include <stddef.h>

#include "base/base_core.h"

enum arena_flags {
    NO_CHAIN = 1 << 0,
};

struct arena_region {
    struct arena_region *prev;
    size_t base_position;
    size_t capacity;
    size_t offset;
    u8 data[];
};

struct arena {
    struct arena_region *current;
    enum arena_flags flags;
    size_t block_capacity;
};

struct arena_temp {
    struct arena *arena;
    size_t pos;
};

/** Pushes uninitialized storage for one object of type. */
#define arena_push_type(arena, type) \
    ((type *)arena_push((arena), sizeof(type), alignof(type)))

/**
 * Pushes an uninitialized array of item_count objects of type. The caller must
 * ensure sizeof(type) * item_count is representable by size_t.
 */
#define arena_push_array(arena, type, item_count) \
    ((type *)arena_push((arena), sizeof(type) * (item_count), alignof(type)))

/** Creates a growable arena whose regions are allocated lazily on first use. */
struct arena *arena_alloc(void);

/** Creates an arena with the requested optional behavior flags. */
struct arena *arena_alloc_with_flags(enum arena_flags flags);

/**
 * Returns size bytes of uninitialized arena storage beginning at an address
 * divisible by alignment. Alignment must be a nonzero power of two. Returns
 * NULL when the request cannot be satisfied; NO_CHAIN limits the arena to one
 * region of block_capacity bytes.
 */
void *arena_push(struct arena *arena, size_t size, size_t alignment);

/** Returns the arena's current logical position across its region chain. */
size_t arena_position(const struct arena *arena);

/** Restores a position previously returned by arena_position. */
void arena_pop_to(struct arena *arena, size_t position);

/** Moves backward by amount in the arena's logical position space. */
void arena_pop(struct arena *arena, size_t amount);

/** Begins a temporary scope by saving the arena's current position. */
struct arena_temp arena_temp_begin(struct arena *arena);

/** Ends a temporary scope and invalidates every push made within it. */
void arena_temp_end(struct arena_temp temp);

/** Releases every backing region and the arena itself. */
void arena_free(struct arena *arena);

#endif
