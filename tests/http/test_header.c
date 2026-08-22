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
#include <stdio.h>
#include <string.h>

#include "datastruct/string.h"
#include "eunha.h"
#include "http/header.h"

/* Parses one C string through the owned string-based header API. */
static enum eunha_result headers_parse_c_string(
    struct headers* headers, const char* value) {
    struct string line;
    if (string_init(&line, value) == EUNHA_ERROR) {
        return EUNHA_ERROR;
    }

    enum eunha_result result = headers_parse_line(headers, &line);
    string_deinit(&line);
    return result;
}

/* Verifies empty table initialization and nested cleanup. */
static int test_headers_lifecycle(void) {
    struct headers headers;

    assert(headers_init(&headers) == EUNHA_OK);
    assert(headers.entries != NULL);
    assert(headers.length == 0);
    assert(headers.capacity > 0);
    assert(headers_parse_c_string(&headers, "Host: example.test") == EUNHA_OK);
    assert(headers_parse_c_string(&headers, "X-Trace: abc") == EUNHA_OK);
    assert(headers.length == 2);

    headers_deinit(&headers);
    assert(headers.entries == NULL);
    assert(headers.length == 0);
    assert(headers.capacity == 0);
    return 0;
}

/* Verifies values are copied, trimmed, and found case-insensitively. */
static int test_header_ownership_and_lookup(void) {
    char host[] = "hOsT:\t example.test  ";
    struct headers headers;
    struct string line;

    assert(headers_init(&headers) == EUNHA_OK);
    assert(string_init(&line, host) == EUNHA_OK);
    assert(headers_parse_line(&headers, &line) == EUNHA_OK);
    string_deinit(&line);
    assert(
        headers_parse_c_string(&headers, "X-Request-ID: abc-123") == EUNHA_OK);
    assert(headers_parse_c_string(&headers, "X-Empty:") == EUNHA_OK);
    memset(host, 'x', sizeof(host) - 1);

    const struct string* value = headers_get(&headers, HTTP_HEADER_HOST);
    assert(value != NULL);
    assert(string_equals(value, "example.test"));

    value = headers_get(&headers, "x-request-id");
    assert(value != NULL);
    assert(string_equals(value, "abc-123"));

    value = headers_get(&headers, "x-empty");
    assert(value != NULL);
    assert(value->length == 0);
    assert(headers_get(&headers, "missing") == NULL);

    headers_deinit(&headers);
    return 0;
}

/* Verifies differently-cased duplicate names share ordered values. */
static int test_duplicate_values(void) {
    struct headers headers;

    assert(headers_init(&headers) == EUNHA_OK);
    assert(headers_parse_c_string(&headers, "Content-Type: application/json") ==
           EUNHA_OK);
    assert(headers_parse_c_string(&headers, "content-type: text/plain") ==
           EUNHA_OK);
    assert(headers.length == 1);
    assert(headers_value_count(&headers, HTTP_HEADER_CONTENT_TYPE) == 2);

    const struct string* value = headers_get_at(&headers, "content-type", 0);
    assert(value != NULL);
    assert(string_equals(value, "application/json"));

    value = headers_get_at(&headers, "CONTENT-TYPE", 1);
    assert(value != NULL);
    assert(string_equals(value, "text/plain"));
    assert(headers_get_at(&headers, "content-type", 2) == NULL);
    assert(headers_value_count(&headers, "missing") == 0);

    headers_deinit(&headers);
    return 0;
}

/* Verifies table growth preserves every case-insensitive lookup. */
static int test_header_table_growth(void) {
    struct headers headers;

    assert(headers_init(&headers) == EUNHA_OK);
    for (size_t index = 0; index < 64; index += 1) {
        char line[64];
        int written = snprintf(
            line, sizeof(line), "X-Header-%zu: value-%zu", index, index);
        assert(written > 0);
        assert((size_t)written < sizeof(line));
        assert(headers_parse_c_string(&headers, line) == EUNHA_OK);
    }

    assert(headers.length == 64);
    assert(headers.capacity > 64);

    for (size_t index = 0; index < 64; index += 1) {
        char name[64];
        char expected[64];
        int name_length = snprintf(name, sizeof(name), "x-header-%zu", index);
        int value_length =
            snprintf(expected, sizeof(expected), "value-%zu", index);
        assert(name_length > 0 && (size_t)name_length < sizeof(name));
        assert(value_length > 0 && (size_t)value_length < sizeof(expected));

        const struct string* value = headers_get(&headers, name);
        assert(value != NULL);
        assert(string_equals(value, expected));
    }

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
        assert(headers_parse_c_string(&headers, invalid[index]) == EUNHA_ERROR);
        assert(headers.length == 0);
    }

    headers_deinit(&headers);
    return 0;
}

/* Runs hashed header ownership, duplicate, and growth tests. */
int main(void) {
    assert(test_headers_lifecycle() == 0);
    assert(test_header_ownership_and_lookup() == 0);
    assert(test_duplicate_values() == 0);
    assert(test_header_table_growth() == 0);
    assert(test_invalid_headers() == 0);
    return 0;
}
