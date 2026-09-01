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
#include <stddef.h>
#include <eunha/arena.h>
#include <eunha/core.h>
#include <eunha/vector.h>

static void test_push(void)
{
    struct arena *arena = arena_alloc();
    struct vector values = vector(u64, .arena = arena);
    u64 item = 10;

    assert(values.capacity == VECTOR_DEFAULT_CAPACITY);
    vector_push(&values, &item);
    assert(vector_len(&values) == 1);
    assert(*(u64 *)vector_at(&values, 0) == item);
    assert(vector_at(&values, 1) == NULL);

    arena_free(arena);
}

static void test_resize(void)
{
    struct arena *arena = arena_alloc();
    struct vector values = vector(u64, .arena = arena, .initial_capacity = 2);
    u64 items[] = { 10, 20, 30 };

    assert(values.capacity == 2);
    vector_push(&values, &items[0]);
    vector_push(&values, &items[1]);
    vector_push(&values, &items[2]);

    assert(vector_len(&values) == 3);
    assert(*(u64 *)vector_at(&values, 0) == items[0]);
    assert(*(u64 *)vector_at(&values, 1) == items[1]);
    assert(*(u64 *)vector_at(&values, 2) == items[2]);

    arena_free(arena);
}

static void test_pop(void)
{
    struct arena *arena = arena_alloc();
    struct vector values = vector(u64, .arena = arena);
    u64 items[] = { 10, 20 };

    vector_push(&values, &items[0]);
    vector_push(&values, &items[1]);

    assert(*(u64 *)vector_pop(&values) == items[1]);
    assert(*(u64 *)vector_pop(&values) == items[0]);
    assert(vector_pop(&values) == NULL);

    arena_free(arena);
}

static void test_reserve(void)
{
    struct arena *arena = arena_alloc();
    struct vector values = vector(u64, .arena = arena);
    u64 item = 10;

    vector_push(&values, &item);
    vector_reserve(&values, 16);

    assert(values.capacity >= 16);
    assert(vector_len(&values) == 1);
    assert(*(u64 *)vector_at(&values, 0) == item);

    arena_free(arena);
}

static void test_clear(void)
{
    struct arena *arena = arena_alloc();
    struct vector values = vector(u64, .arena = arena);
    u64 items[] = { 10, 20 };

    vector_push(&values, &items[0]);
    vector_push(&values, &items[1]);
    vector_clear(&values);

    assert(vector_len(&values) == 0);
    assert(vector_pop(&values) == NULL);

    arena_free(arena);
}

int main(void)
{
    test_push();
    test_resize();
    test_pop();
    test_reserve();
    test_clear();
    return 0;
}
