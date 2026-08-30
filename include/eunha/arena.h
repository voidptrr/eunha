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

#ifndef EUNHA_ARENA_H
#define EUNHA_ARENA_H

#include <stdalign.h>
#include <stddef.h>
#include <eunha/core.h>

enum arena_flags {
    NO_CHAIN = 1 << 0,
};

/**
 * struct arena_params - Optional parameters used to create an arena.
 * @flags: Behavior flags; zero enables normal region chaining.
 * @block_capacity: Preferred region size; zero selects the 64 KiB default.
 */
struct arena_params {
    enum arena_flags flags;
    size_t block_capacity;
};

/**
 * struct arena_region - One backing-memory region in an arena chain.
 * @prev: Previous region in the chain, or NULL for the first region.
 * @base_position: Logical arena position at the start of this region.
 * @capacity: Number of bytes available in data.
 * @offset: Number of bytes consumed, including alignment gaps.
 * @data: Flexible byte buffer owned by this region.
 */
struct arena_region {
    struct arena_region *prev;
    size_t base_position;
    size_t capacity;
    size_t offset;
    u8 data[];
};

/**
 * struct arena - Chained linear allocator state.
 * @current: Region used for new pushes, or NULL before the first push.
 * @flags: Behavior flags selected when the arena was created.
 * @block_capacity: Preferred capacity for newly allocated regions.
 */
struct arena {
    struct arena_region *current;
    enum arena_flags flags;
    size_t block_capacity;
};

/**
 * struct arena_temp - Saved arena position for a temporary allocation scope.
 * @arena: Arena restored when the temporary scope ends.
 * @pos: Logical arena position saved at the beginning of the scope.
 */
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

/**
 * Creates an arena from optional designated arena_params fields. With no
 * fields, the arena grows by chaining lazily allocated regions.
 */
#define arena_alloc(...) \
    arena_alloc_params(&(struct arena_params){ __VA_ARGS__ })

/** Creates an arena using the supplied parameter structure. */
struct arena *arena_alloc_params(const struct arena_params *params);

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
