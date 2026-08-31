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
#include <eunha/arena.h>
#include <eunha/core.h>
#include <eunha/deque.h>

static void test_initial_capacity(void)
{
    struct arena *arena = arena_alloc();
    struct deque values = deque(.arena = arena, .item_size = sizeof(u64),
                                .item_alignment = alignof(u64),
                                .initial_capacity = 8);

    assert(values.data != NULL);
    assert((uintptr_t)values.data % alignof(u64) == 0);
    assert(values.len == 0);
    assert(values.capacity == 8);

    arena_free(arena);
}

int main(void)
{
    test_initial_capacity();
    return 0;
}
