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
#include <stdint.h>
#include <string.h>

#include "datastruct/vector.h"
#include "http/header.h"

/* Verifies empty collection initialization and cleanup. */
static int test_headers_init(void) {
    struct headers headers;

    assert(headers_init(&headers) == 0);
    assert(vector_len(&headers.values) == 0);
    headers_deinit(&headers);
    return 0;
}

/* Verifies generic fields are normalized, trimmed, and searchable. */
static int test_headers_append(void) {
    struct headers headers;
    const char host[] = "Host:\t example.test  ";
    const char custom[] = "X-Request-ID: abc-123";
    const char empty[] = "X-Empty:";

    assert(headers_init(&headers) == 0);
    assert(headers_append(&headers, (const uint8_t*)host, strlen(host)) == 0);
    assert(
        headers_append(&headers, (const uint8_t*)custom, strlen(custom)) == 0);
    assert(headers_append(&headers, (const uint8_t*)empty, strlen(empty)) == 0);
    assert(vector_len(&headers.values) == 3);

    const struct header* header = headers_get(&headers, "HOST");
    assert(header != NULL);
    assert(strcmp((const char*)header->name.data, "host") == 0);
    assert(strcmp((const char*)header->value.data, "example.test") == 0);

    header = headers_get(&headers, "x-request-id");
    assert(header != NULL);
    assert(strcmp((const char*)header->value.data, "abc-123") == 0);

    header = headers_get(&headers, "x-empty");
    assert(header != NULL);
    assert(header->value.length == 0);
    assert(headers_get(&headers, "missing") == NULL);

    headers_deinit(&headers);
    return 0;
}

/* Verifies the iterator yields every field in wire order. */
static int test_header_iterator(void) {
    const char first[] = "X-First: one";
    const char second[] = "X-Second: two";
    struct headers headers;

    assert(headers_init(&headers) == 0);
    assert(headers_append(&headers, (const uint8_t*)first, strlen(first)) == 0);
    assert(
        headers_append(&headers, (const uint8_t*)second, strlen(second)) == 0);

    struct header_iterator iterator = {
        .headers = &headers,
        .index = 0,
    };
    const struct header* header = header_next(&iterator);
    assert(header != NULL);
    assert(strcmp((const char*)header->name.data, "x-first") == 0);

    header = header_next(&iterator);
    assert(header != NULL);
    assert(strcmp((const char*)header->name.data, "x-second") == 0);
    assert(header_next(&iterator) == NULL);

    headers_deinit(&headers);
    return 0;
}

/* Verifies syntax errors do not append partial fields. */
static int test_invalid_headers(void) {
    static const char* invalid[] = {
        "Host example.test",
        "Bad Name: value",
        ": value",
        "Name: bad\x01value",
    };
    struct headers headers;

    assert(headers_init(&headers) == 0);

    for (size_t index = 0; index < sizeof(invalid) / sizeof(invalid[0]);
        index += 1) {
        assert(headers_append(&headers, (const uint8_t*)invalid[index],
                   strlen(invalid[index])) == -1);
        assert(vector_len(&headers.values) == 0);
    }

    headers_deinit(&headers);
    return 0;
}

/* Runs generic header collection tests. */
int main(void) {
    assert(test_headers_init() == 0);
    assert(test_headers_append() == 0);
    assert(test_header_iterator() == 0);
    assert(test_invalid_headers() == 0);
    return 0;
}
