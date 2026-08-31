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
#include <eunha/arena.h>
#include <eunha/core.h>
#include <eunha/deque.h>

struct deque deque_with_params(const struct deque_params *params)
{
    assert(params != NULL);
    assert(params->arena != NULL);
    assert(params->item_size != 0);
    assert(is_pow2(params->item_alignment));
    assert(params->item_size % params->item_alignment == 0);

    struct deque result = {
        .arena = params->arena,
        .item_size = params->item_size,
        .item_alignment = params->item_alignment,
    };

    if (params->initial_capacity == 0) {
        return result;
    }

    size_t allocation_size = 0;
    if (ckd_mul(&allocation_size, params->initial_capacity,
                params->item_size)) {
        abort();
    }

    result.data =
        arena_push(params->arena, allocation_size, params->item_alignment);
    result.capacity = params->initial_capacity;
    return result;
}
