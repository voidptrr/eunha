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

#include "datastruct/vector.h"

/*
 * Verifies vector_init allocates storage and vector_deinit clears ownership.
 */
static int test_vector_init_deinit(void) {
    struct vector vector;

    assert(vector_init(&vector, sizeof(int)) == 0);
    assert(vector.data != NULL);
    assert(vector.item_size == sizeof(int));
    assert(vector_len(&vector) == 0);
    assert(vector.capacity > 0);

    vector_deinit(&vector);
    assert(vector.data == NULL);
    assert(vector.item_size == 0);
    assert(vector_len(&vector) == 0);
    assert(vector.capacity == 0);
    return 0;
}

/*
 * Verifies single element append and indexed access.
 */
static int test_vector_append_get(void) {
    struct vector vector;
    int first = 10;
    int second = 20;

    assert(vector_init(&vector, sizeof(int)) == 0);
    assert(vector_append(&vector, &first) == 0);
    assert(vector_append(&vector, &second) == 0);

    assert(vector_len(&vector) == 2);
    assert(*(int*)vector_get(&vector, 0) == first);
    assert(*(int*)vector_get(&vector, 1) == second);

    vector_deinit(&vector);
    return 0;
}

/*
 * Verifies zero-count extend is accepted and does not change the vector.
 */
static int test_vector_extend_empty(void) {
    struct vector vector;

    assert(vector_init(&vector, sizeof(int)) == 0);
    assert(vector_extend(&vector, NULL, 0) == 0);
    assert(vector_len(&vector) == 0);

    vector_deinit(&vector);
    return 0;
}

/*
 * Verifies vector_extend appends many elements and grows capacity.
 */
static int test_vector_extend_grow(void) {
    struct vector vector;
    int items[20];

    for (size_t index = 0; index < 20; index += 1) {
        items[index] = (int)(index * 3);
    }

    assert(vector_init(&vector, sizeof(int)) == 0);
    assert(vector_extend(&vector, items, 20) == 0);
    assert(vector_len(&vector) == 20);

    for (size_t index = 0; index < 20; index += 1) {
        assert(*(int*)vector_get(&vector, index) == items[index]);
    }

    vector_deinit(&vector);
    return 0;
}

/*
 * Runs vector unit tests.
 */
int main(void) {
    assert(test_vector_init_deinit() == 0);
    assert(test_vector_append_get() == 0);
    assert(test_vector_extend_empty() == 0);
    assert(test_vector_extend_grow() == 0);
    return 0;
}
