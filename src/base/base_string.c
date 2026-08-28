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
#include <string.h>

#include "base/base_arena.h"
#include "base/base_core.h"
#include "base/base_string.h"

static bool str8_is_valid(struct str8 str)
{
    return (str.len == 0 || str.data != NULL) != 0;
}

static u8 *str8_push_buffer(struct arena *arena, size_t len)
{
    assert(arena != NULL);

    size_t allocation_size = 0;
    if (ckd_add(&allocation_size, len, (size_t)1)) {
        return NULL;
    }

    return arena_push_array(arena, u8, allocation_size);
}

struct str8 str8(const u8 *data, size_t len)
{
    assert(data != NULL || len == 0);
    return (struct str8){ .data = data, .len = len };
}

struct str8 str8_cstring(const char *cstr)
{
    if (cstr == NULL) {
        return (struct str8){ 0 };
    }

    return str8((const u8 *)cstr, strlen(cstr));
}

size_t str8_len(struct str8 str)
{
    assert(str8_is_valid(str));
    return str.len;
}

struct str8 str8_copy(struct arena *arena, struct str8 source)
{
    assert(str8_is_valid(source));

    u8 *data = str8_push_buffer(arena, source.len);
    if (data == NULL) {
        return (struct str8){ 0 };
    }

    if (source.len > 0) {
        memcpy(data, source.data, source.len);
    }
    data[source.len] = '\0';

    return str8(data, source.len);
}

struct str8 str8_cat(struct arena *arena, struct str8 first, struct str8 second)
{
    assert(str8_is_valid(first));
    assert(str8_is_valid(second));

    size_t len = 0;
    if (ckd_add(&len, first.len, second.len)) {
        return (struct str8){ 0 };
    }

    u8 *data = str8_push_buffer(arena, len);
    if (data == NULL) {
        return (struct str8){ 0 };
    }

    if (first.len > 0) {
        memcpy(data, first.data, first.len);
    }
    if (second.len > 0) {
        memcpy(data + first.len, second.data, second.len);
    }
    data[len] = '\0';

    return str8(data, len);
}

bool str8_contains(struct str8 haystack, struct str8 needle)
{
    assert(str8_is_valid(haystack));
    assert(str8_is_valid(needle));

    if (needle.len > haystack.len) {
        return false;
    }
    if (needle.len == 0) {
        return true;
    }

    const u8 *cursor = haystack.data;
    size_t remaining = haystack.len;
    while (remaining >= needle.len) {
        size_t candidate_count = remaining - needle.len + 1;
        const u8 *candidate = memchr(cursor, needle.data[0], candidate_count);
        if (candidate == NULL) {
            return false;
        }
        if (memcmp(candidate, needle.data, needle.len) == 0) {
            return true;
        }

        size_t consumed = (size_t)(candidate - cursor) + 1;
        cursor += consumed;
        remaining -= consumed;
    }

    return false;
}

bool str8_has_prefix(struct str8 string, struct str8 prefix)
{
    assert(str8_is_valid(string));
    assert(str8_is_valid(prefix));

    return (prefix.len <= string.len &&
            (prefix.len == 0 ||
             memcmp(string.data, prefix.data, prefix.len) == 0)) != 0;
}

void str8_list_push(struct arena *arena, struct str8_list *list,
                    struct str8 str)
{
    assert(arena != NULL);
    assert(list != NULL);
    assert(str8_is_valid(str));

    size_t total_len = 0;
    if (ckd_add(&total_len, list->total_len, str.len)) {
        return;
    }

    struct str8_list_node *node = arena_push_type(arena, struct str8_list_node);
    if (node == NULL) {
        return;
    }

    *node = (struct str8_list_node){ .data = str, .next = NULL };

    if (list->tail != NULL) {
        list->tail->next = node;
    } else {
        list->head = node;
    }

    list->tail = node;
    list->total_len = total_len;
}

struct str8 str8_list_join(struct arena *arena, const struct str8_list *list)
{
    assert(arena != NULL);
    assert(list != NULL);

    u8 *data = str8_push_buffer(arena, list->total_len);
    if (data == NULL) {
        return (struct str8){ 0 };
    }

    u8 *dst = data;
    const struct str8_list_node *current = list->head;
    while (current != NULL) {
        if (current->data.len > 0) {
            memcpy(dst, current->data.data, current->data.len);
        }
        dst += current->data.len;
        current = current->next;
    }
    *dst = '\0';

    return str8(data, list->total_len);
}
