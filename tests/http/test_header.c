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
#include "eunha.h"
#include "http/header.h"

/* Verifies empty collection initialization and nested cleanup. */
static int test_headers_lifecycle(void) {
    struct headers headers;
    const char first[] = "Host: example.test";
    const char second[] = "X-Trace: abc";

    assert(headers_init(&headers) == EUNHA_OK);
    assert(vector_len(&headers.fields) == 0);
    assert(headers_parse_line(&headers, (const uint8_t*)first, strlen(first)) ==
           EUNHA_OK);
    assert(headers_parse_line(
               &headers, (const uint8_t*)second, strlen(second)) == EUNHA_OK);

    headers_deinit(&headers);
    assert(headers.fields.data == NULL);
    assert(headers.fields.item_size == 0);
    assert(headers.fields.length == 0);
    assert(headers.fields.capacity == 0);
    return 0;
}

/* Verifies every name/value pair is copied, trimmed, and searchable. */
static int test_header_ownership_and_lookup(void) {
    char host[] = "hOsT:\t example.test  ";
    const char custom[] = "X-Request-ID: abc-123";
    const char empty[] = "X-Empty:";
    struct headers headers;

    assert(headers_init(&headers) == EUNHA_OK);
    assert(headers_parse_line(&headers, (const uint8_t*)host, strlen(host)) ==
           EUNHA_OK);
    assert(headers_parse_line(
               &headers, (const uint8_t*)custom, strlen(custom)) == EUNHA_OK);
    assert(headers_parse_line(&headers, (const uint8_t*)empty, strlen(empty)) ==
           EUNHA_OK);
    memset(host, 'x', sizeof(host) - 1);

    const struct header* header = headers_get(&headers, "HOST");
    assert(header != NULL);
    assert(strcmp((const char*)header->name.data, "hOsT") == 0);
    assert(strcmp((const char*)header->value.data, "example.test") == 0);

    header = headers_get(&headers, "x-request-id");
    assert(header != NULL);
    assert(strcmp((const char*)header->name.data, "X-Request-ID") == 0);
    assert(strcmp((const char*)header->value.data, "abc-123") == 0);

    header = headers_get(&headers, "x-empty");
    assert(header != NULL);
    assert(header->value.length == 0);
    assert(headers_get(&headers, "missing") == NULL);

    headers_deinit(&headers);
    return 0;
}

/* Verifies duplicate fields are retained in their original wire order. */
static int test_duplicates_and_iteration(void) {
    const char first[] = "Content-Type: application/json";
    const char second[] = "content-type: text/plain";
    struct headers headers;

    assert(headers_init(&headers) == EUNHA_OK);
    assert(headers_parse_line(&headers, (const uint8_t*)first, strlen(first)) ==
           EUNHA_OK);
    assert(headers_parse_line(
               &headers, (const uint8_t*)second, strlen(second)) == EUNHA_OK);
    assert(vector_len(&headers.fields) == 2);
    assert(headers_get(&headers, "CONTENT-TYPE") != NULL);

    struct header_iterator iterator = {
        .headers = &headers,
        .index = 0,
    };
    const struct header* header = header_iterator_next(&iterator);
    assert(header != NULL);
    assert(strcmp((const char*)header->name.data, "Content-Type") == 0);
    assert(strcmp((const char*)header->value.data, "application/json") == 0);

    header = header_iterator_next(&iterator);
    assert(header != NULL);
    assert(strcmp((const char*)header->name.data, "content-type") == 0);
    assert(strcmp((const char*)header->value.data, "text/plain") == 0);
    assert(header_iterator_next(&iterator) == NULL);

    headers_deinit(&headers);
    return 0;
}

/* Verifies storage rejects only malformed generic HTTP field syntax. */
static int test_invalid_headers(void) {
    static const char* invalid[] = {
        "Host example.test",
        "Bad Name: value",
        ": value",
        "Name: bad\x01value",
    };
    struct headers headers;

    assert(headers_init(&headers) == EUNHA_OK);
    for (size_t index = 0; index < sizeof(invalid) / sizeof(invalid[0]);
        index += 1) {
        assert(headers_parse_line(&headers, (const uint8_t*)invalid[index],
                   strlen(invalid[index])) == EUNHA_ERROR);
        assert(vector_len(&headers.fields) == 0);
    }

    headers_deinit(&headers);
    return 0;
}

/* Runs generic ordered header ownership tests. */
int main(void) {
    assert(test_headers_lifecycle() == 0);
    assert(test_header_ownership_and_lookup() == 0);
    assert(test_duplicates_and_iteration() == 0);
    assert(test_invalid_headers() == 0);
    return 0;
}
