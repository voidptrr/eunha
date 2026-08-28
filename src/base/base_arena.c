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
#include <stdint.h>
#include <stdlib.h>

#include "base/base_arena.h"
#include "base/base_core.h"

static void *arena_region_push(struct arena_region *region, size_t size,
                               size_t alignment)
{
    assert(region != NULL);
    assert(region->offset <= region->capacity);

    /* The flexible array may not begin at the requested alignment, so align
     * the actual address rather than the region-relative offset. */
    uintptr_t current_address = (uintptr_t)(region->data + region->offset);
    size_t padding = (size_t)align_pad_pow2(current_address, alignment);
    size_t available = region->capacity - region->offset;

    /* Test padding first to keep the following unsigned subtraction safe. */
    if (padding > available || size > available - padding) {
        return NULL;
    }

    void *result = region->data + region->offset + padding;
    region->offset += padding + size;
    asan_unpoison_memory_region(result, size);
    return result;
}

static struct arena_region *arena_region_alloc(size_t capacity)
{
    /* Allocate the header and flexible byte buffer together. */
    size_t allocation_size = 0;
    if (ckd_add(&allocation_size, sizeof(struct arena_region), capacity)) {
        return NULL;
    }

    struct arena_region *region = malloc(allocation_size);
    if (region == NULL) {
        return NULL;
    }

    region->prev = NULL;
    region->base_position = 0;
    region->capacity = capacity;
    region->offset = 0;
    asan_poison_memory_region(region->data, region->capacity);
    return region;
}

struct arena *arena_alloc_params(const struct arena_params *params)
{
    assert(params != NULL);

    struct arena *arena = malloc(sizeof(struct arena));
    arena->block_capacity = kb(64);
    arena->flags = params->flags;
    arena->current = NULL;

    return arena;
}

void *arena_push(struct arena *arena, size_t size, size_t alignment)
{
    assert(arena != NULL);
    assert(is_pow2(alignment));

    /* The current region is the only region used for new pushes. */
    struct arena_region *region = arena->current;
    void *result = region != NULL ? arena_region_push(region, size, alignment) :
                                    NULL;
    if (result != NULL) {
        return result;
    }

    if (region != NULL && (arena->flags & NO_CHAIN) != 0) {
        return NULL;
    }

    /* A new region must accommodate the request and worst-case alignment
     * padding, whose maximum is alignment - 1 bytes. */
    size_t required_capacity = 0;
    if (ckd_add(&required_capacity, size, alignment - (size_t)1)) {
        return NULL;
    }

    /* Chained arenas give unusually large pushes a dedicated larger region. */
    size_t region_capacity = arena->block_capacity;
    if ((arena->flags & NO_CHAIN) == 0 && required_capacity > region_capacity) {
        region_capacity = required_capacity;
    }

    /* Each region owns a non-overlapping range in the arena's logical
     * position space. Unused bytes at the end of a region remain a gap. */
    size_t base_position = 0;
    if (region != NULL &&
        ckd_add(&base_position, region->base_position, region->capacity)) {
        return NULL;
    }
    if (region_capacity > SIZE_MAX - base_position) {
        return NULL;
    }

    struct arena_region *new_region = arena_region_alloc(region_capacity);
    if (new_region == NULL) {
        return NULL;
    }
    new_region->base_position = base_position;

    /* Do not link the new region until its first push succeeds, leaving the
     * arena unchanged if a fixed-size region cannot satisfy the request. */
    result = arena_region_push(new_region, size, alignment);
    if (result == NULL) {
        free(new_region);
        return NULL;
    }

    new_region->prev = region;
    arena->current = new_region;
    return result;
}

size_t arena_position(const struct arena *arena)
{
    assert(arena != NULL);

    if (arena->current == NULL) {
        return 0;
    }

    assert(arena->current->offset <= arena->current->capacity);
    assert(arena->current->offset <= SIZE_MAX - arena->current->base_position);
    return arena->current->base_position + arena->current->offset;
}

void arena_pop_to(struct arena *arena, size_t position)
{
    assert(arena != NULL);
    assert(position <= arena_position(arena));

    /* Find and validate the region containing position before changing the
     * chain, so an invalid position cannot partially mutate the arena. */
    struct arena_region *target = arena->current;
    while (target != NULL && target->base_position >= position) {
        target = target->prev;
    }

    size_t target_offset = 0;
    if (target != NULL) {
        target_offset = position - target->base_position;
        assert(target_offset <= target->offset);
    } else {
        assert(position == 0);
    }

    /* Regions newer than the target contain only invalidated allocations. */
    while (arena->current != target) {
        struct arena_region *region = arena->current;
        arena->current = region->prev;
        free(region);
    }

    if (target != NULL) {
        asan_poison_memory_region(target->data + target_offset,
                                  target->offset - target_offset);
        target->offset = target_offset;
    }
}

void arena_pop(struct arena *arena, size_t amount)
{
    assert(arena != NULL);

    size_t position = arena_position(arena);
    arena_pop_to(arena, amount < position ? position - amount : 0);
}

struct arena_temp arena_temp_begin(struct arena *arena)
{
    assert(arena != NULL);

    return (struct arena_temp){
        .arena = arena,
        .pos = arena_position(arena),
    };
}

void arena_temp_end(struct arena_temp temp)
{
    assert(temp.arena != NULL);
    arena_pop_to(temp.arena, temp.pos);
}

void arena_free(struct arena *arena)
{
    assert(arena != NULL);

    /* Regions form a stack from newest to oldest through prev. */
    while (arena->current != NULL) {
        struct arena_region *region = arena->current;
        arena->current = region->prev;
        free(region);
    }
    free(arena);
}
