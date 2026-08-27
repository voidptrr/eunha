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
#include <stdint.h>

#include "base/base_arena.h"
#include "base/base_core.h"

static void test_alignment_and_typed_pushes(void) {
    struct arena* arena = arena_alloc(ARENA_DEFAULT);
    assert(arena != NULL);

    uint8_t* byte = arena_push(arena, sizeof(*byte), alignof(uint8_t));
    uint64_t* word = arena_push(arena, sizeof(*word), alignof(uint64_t));
    void* page_aligned = arena_push(arena, 1, 4096);
    uint64_t* words = arena_push_array(arena, uint64_t, 8);
    uint32_t* number = arena_push_struct(arena, uint32_t);

    assert(byte != NULL);
    assert(word != NULL);
    assert((uintptr_t)word % alignof(uint64_t) == 0);
    assert(page_aligned != NULL);
    assert((uintptr_t)page_aligned % 4096 == 0);
    assert(words != NULL);
    assert((uintptr_t)words % alignof(uint64_t) == 0);
    assert(number != NULL);
    assert((uintptr_t)number % alignof(uint32_t) == 0);

    arena_free(arena);
}

static void test_oversized_push_adds_region(void) {
    struct arena* arena = arena_alloc(ARENA_DEFAULT);
    assert(arena != NULL);
    assert(arena_push(arena, 1, 1) != NULL);

    struct arena_region* first = arena->current;
    void* oversized = arena_push(arena, kb(100), 64);

    assert(oversized != NULL);
    assert((uintptr_t)oversized % 64 == 0);
    assert(arena->current != first);
    assert(arena->current->prev == first);
    assert(arena->current->capacity >= kb(100) + 63);

    arena_free(arena);
}

static void test_overflow_does_not_add_region(void) {
    struct arena* arena = arena_alloc(ARENA_DEFAULT);
    assert(arena != NULL);
    assert(arena_push(arena, 1, 1) != NULL);

    struct arena_region* before_overflow = arena->current;
    assert(arena_push(arena, SIZE_MAX, 2) == NULL);
    assert(arena->current == before_overflow);

    arena_free(arena);
}

static void test_no_grow_uses_one_region(void) {
    struct arena* arena = arena_alloc(NO_CHAIN);
    assert(arena != NULL);
    assert(arena_push(arena, kb(64), 1) != NULL);

    struct arena_region* only_region = arena->current;
    assert(arena_push(arena, 1, 1) == NULL);
    assert(arena->current == only_region);
    assert(arena->current->prev == NULL);

    arena_free(arena);
}

static void test_no_grow_rejects_oversized_first_push(void) {
    struct arena* arena = arena_alloc(NO_CHAIN);
    assert(arena != NULL);
    assert(arena_push(arena, kb(64) + 1, 1) == NULL);
    assert(arena->current == NULL);

    assert(arena_push(arena, 1, 1) != NULL);
    assert(arena->current->capacity == kb(64));
    assert(arena->current->prev == NULL);

    arena_free(arena);
}

int main(void) {
    test_alignment_and_typed_pushes();
    test_oversized_push_adds_region();
    test_overflow_does_not_add_region();
    test_no_grow_uses_one_region();
    test_no_grow_rejects_oversized_first_push();
    return 0;
}
