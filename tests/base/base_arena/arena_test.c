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
#include <stdalign.h>
#include <stddef.h>
#include <stdint.h>

#include "base/base_arena.h"
#include "base/base_core.h"

static void test_alignment_and_typed_pushes(void)
{
    struct arena *arena = arena_alloc();
    assert(arena != NULL);

    u8 *byte = arena_push_type(arena, u8);
    u64 *word = arena_push_type(arena, u64);
    void *page_aligned = arena_push(arena, 1, 4096);
    u64 *words = arena_push_array(arena, u64, 8);
    u32 *number = arena_push_type(arena, u32);

    assert(byte != NULL);
    assert(word != NULL);
    assert((uintptr_t)word % alignof(u64) == 0);
    assert(page_aligned != NULL);
    assert((uintptr_t)page_aligned % 4096 == 0);
    assert(words != NULL);
    assert((uintptr_t)words % alignof(u64) == 0);
    assert(number != NULL);
    assert((uintptr_t)number % alignof(u32) == 0);

    *byte = 1;
    *word = 2;
    *(u8 *)page_aligned = 3;
    words[7] = 4;
    *number = 5;

    arena_free(arena);
}

static void test_oversized_push_adds_region(void)
{
    struct arena *arena = arena_alloc();
    assert(arena != NULL);
    assert(arena_push(arena, 1, 1) != NULL);

    struct arena_region *first = arena->current;
    size_t first_position = arena_position(arena);
    void *oversized = arena_push(arena, kb(100), 64);

    assert(oversized != NULL);
    assert((uintptr_t)oversized % 64 == 0);
    assert(arena->current != first);
    assert(arena->current->prev == first);
    assert(arena->current->capacity >= kb(100) + 63);
    assert(first->base_position == 0);
    assert(arena->current->base_position == first->capacity);
    assert(arena_position(arena) ==
           arena->current->base_position + arena->current->offset);
    assert(arena_position(arena) > first_position);

    arena_free(arena);
}

static void test_overflow_does_not_add_region(void)
{
    struct arena *arena = arena_alloc();
    assert(arena != NULL);
    assert(arena_push(arena, 1, 1) != NULL);

    struct arena_region *before_overflow = arena->current;
    assert(arena_push(arena, SIZE_MAX, 2) == NULL);
    assert(arena->current == before_overflow);

    arena_free(arena);
}

static void test_custom_block_capacity_is_reused(void)
{
    struct arena *arena = arena_alloc(.block_capacity = kb(1));
    assert(arena != NULL);
    assert(arena->block_capacity == kb(1));

    assert(arena_push(arena, kb(1), 1) != NULL);
    struct arena_region *first = arena->current;
    assert(first->capacity == kb(1));

    assert(arena_push(arena, 1, 1) != NULL);
    assert(arena->current != first);
    assert(arena->current->capacity == kb(1));

    arena_free(arena);
}

static void test_no_grow_uses_one_region(void)
{
    struct arena *arena = arena_alloc(.flags = NO_CHAIN);
    assert(arena != NULL);
    assert(arena_push(arena, kb(64), 1) != NULL);

    struct arena_region *only_region = arena->current;
    assert(arena_push(arena, 1, 1) == NULL);
    assert(arena->current == only_region);
    assert(arena->current->prev == NULL);

    arena_free(arena);
}

static void test_no_grow_rejects_oversized_first_push(void)
{
    struct arena *arena = arena_alloc(.flags = NO_CHAIN);
    assert(arena != NULL);
    assert(arena_push(arena, kb(64) + 1, 1) == NULL);
    assert(arena->current == NULL);

    assert(arena_push(arena, 1, 1) != NULL);
    assert(arena->current->capacity == kb(64));
    assert(arena->current->prev == NULL);

    arena_free(arena);
}

static void test_empty_arena_position_is_zero(void)
{
    struct arena *arena = arena_alloc();
    assert(arena != NULL);
    assert(arena_position(arena) == 0);

    arena_free(arena);
}

static void test_pop_restores_position_in_current_region(void)
{
    struct arena *arena = arena_alloc();
    assert(arena != NULL);
    assert(arena_push(arena, 32, 1) != NULL);

    size_t position = arena_position(arena);
    void *temporary = arena_push(arena, 16, 1);
    assert(temporary != NULL);

    arena_pop(arena, 16);
    assert(arena_position(arena) == position);
#if BASE_ASAN_ENABLED
    assert(asan_address_is_poisoned(temporary));
#endif

    void *reused = arena_push(arena, 16, 1);
    assert(reused == temporary);
#if BASE_ASAN_ENABLED
    assert(!asan_address_is_poisoned(reused));
#endif
    *(u8 *)reused = 1;

    arena_free(arena);
}

static void test_temp_end_restores_position_across_regions(void)
{
    struct arena *arena = arena_alloc();
    assert(arena != NULL);
    assert(arena_push(arena, 1, 1) != NULL);

    struct arena_region *persistent_region = arena->current;
    struct arena_temp temp = arena_temp_begin(arena);
    assert(temp.arena == arena);
    assert(temp.pos == arena_position(arena));

    assert(arena_push(arena, kb(100), 1) != NULL);
    assert(arena->current != persistent_region);

    arena_temp_end(temp);
    assert(arena->current == persistent_region);
    assert(arena_position(arena) == temp.pos);

    arena_free(arena);
}

int main(void)
{
    test_empty_arena_position_is_zero();
    test_alignment_and_typed_pushes();
    test_oversized_push_adds_region();
    test_overflow_does_not_add_region();
    test_custom_block_capacity_is_reused();
    test_no_grow_uses_one_region();
    test_no_grow_rejects_oversized_first_push();
    test_pop_restores_position_in_current_region();
    test_temp_end_restores_position_across_regions();
    return 0;
}
