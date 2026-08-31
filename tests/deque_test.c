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
#include <eunha/deque.h>

static void test_push(void)
{
    struct arena *arena = arena_alloc();
    struct deque values = deque(u64, .arena = arena, .initial_capacity = 2);
    u64 item = 10;

    deque_push(&values, &item);

    assert(deque_len(&values) == 1);
    assert(*(u64 *)deque_popback(&values) == item);

    arena_free(arena);
}

static void test_resize(void)
{
    struct arena *arena = arena_alloc();
    struct deque values = deque(u64, .arena = arena, .initial_capacity = 2);
    u64 items[] = { 10, 20, 30 };

    deque_push(&values, &items[0]);
    deque_push(&values, &items[1]);
    deque_push(&values, &items[2]);

    assert(*(u64 *)deque_popfront(&values) == items[0]);
    assert(*(u64 *)deque_popfront(&values) == items[1]);
    assert(*(u64 *)deque_popfront(&values) == items[2]);

    arena_free(arena);
}

static void test_pushfront(void)
{
    struct arena *arena = arena_alloc();
    struct deque values = deque(u64, .arena = arena, .initial_capacity = 2);
    u64 items[] = { 10, 20, 30 };

    deque_push(&values, &items[1]);
    deque_push(&values, &items[2]);
    deque_pushfront(&values, &items[0]);

    assert(*(u64 *)deque_popfront(&values) == items[0]);
    assert(*(u64 *)deque_popfront(&values) == items[1]);
    assert(*(u64 *)deque_popfront(&values) == items[2]);

    arena_free(arena);
}

static void test_popback(void)
{
    struct arena *arena = arena_alloc();
    struct deque values = deque(u64, .arena = arena);
    u64 items[] = { 10, 20 };

    deque_push(&values, &items[1]);
    deque_pushfront(&values, &items[0]);

    assert(*(u64 *)deque_popback(&values) == items[1]);
    assert(*(u64 *)deque_popback(&values) == items[0]);
    assert(deque_popback(&values) == NULL);
    assert(deque_len(&values) == 0);

    arena_free(arena);
}

static void test_popfront(void)
{
    struct arena *arena = arena_alloc();
    struct deque values = deque(u64, .arena = arena);
    u64 items[] = { 10, 20 };

    deque_push(&values, &items[0]);
    deque_push(&values, &items[1]);

    assert(*(u64 *)deque_popfront(&values) == items[0]);
    assert(*(u64 *)deque_popfront(&values) == items[1]);
    assert(deque_popfront(&values) == NULL);
    assert(deque_len(&values) == 0);

    arena_free(arena);
}

static void test_resize_wrapped_buffer(void)
{
    struct arena *arena = arena_alloc();
    struct deque values = deque(u64, .arena = arena, .initial_capacity = 3);
    u64 items[] = { 10, 20, 30, 40, 50, 60 };

    deque_push(&values, &items[0]);
    deque_push(&values, &items[1]);
    deque_push(&values, &items[2]);
    (void)deque_popfront(&values);
    (void)deque_popfront(&values);
    deque_push(&values, &items[3]);
    deque_push(&values, &items[4]);
    deque_push(&values, &items[5]);

    assert(*(u64 *)deque_popfront(&values) == items[2]);
    assert(*(u64 *)deque_popfront(&values) == items[3]);
    assert(*(u64 *)deque_popfront(&values) == items[4]);
    assert(*(u64 *)deque_popfront(&values) == items[5]);

    arena_free(arena);
}

int main(void)
{
    test_push();
    test_resize();
    test_pushfront();
    test_popback();
    test_popfront();
    test_resize_wrapped_buffer();
    return 0;
}
