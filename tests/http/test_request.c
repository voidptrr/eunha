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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "datastruct/vector.h"
#include "http/header.h"
#include "http/parser.h"
#include "http/request.h"

/* Verifies standalone request storage initialization and cleanup. */
static int test_request_init(void) {
    struct request request;

    assert(request_init(&request) == 0);
    assert(request.method == UNKNOWN);
    assert(request.version == HTTP_VERSION_UNKNOWN);
    assert(request.target.length == 0);
    assert(request.body.length == 0);
    assert(vector_len(&request.headers.values) == 0);
    request_deinit(&request);
    return 0;
}

/* Verifies a complete request populates the generic request representation. */
static int test_complete_request(void) {
    const char message[] = "POST /oauth2/token HTTP/1.1\r\n"
                           "Host: auth.example\r\n"
                           "Content-Type: application/json\r\n"
                           "X-Request-ID: test-1\r\n"
                           "Content-Length: 5\r\n"
                           "\r\n"
                           "hello";
    struct parser parser;

    assert(parser_init(&parser) == 0);
    assert(parser_feed(&parser, (const uint8_t*)message, sizeof(message) - 1) ==
           PARSER_STATUS_COMPLETE);
    assert(parser.request.method == POST);
    assert(parser.request.version == HTTP_1_1);
    assert(
        strcmp((const char*)parser.request.target.data, "/oauth2/token") == 0);
    assert(parser.request.body.length == 5);
    assert(memcmp(parser.request.body.data, "hello", 5) == 0);
    assert(vector_len(&parser.request.headers.values) == 4);

    const struct header* content_type =
        headers_get(&parser.request.headers, "content-type");
    assert(content_type != NULL);
    assert(
        strcmp((const char*)content_type->value.data, "application/json") == 0);

    parser_deinit(&parser);
    return 0;
}

/* Verifies callers can feed each byte exactly once. */
static int test_byte_at_a_time(void) {
    const char message[] = "GET / HTTP/1.1\r\nHost: example.test\r\n\r\n";
    size_t length = sizeof(message) - 1;
    struct parser parser;

    assert(parser_init(&parser) == 0);

    for (size_t index = 0; index < length; index += 1) {
        enum parser_status expected = index + 1 == length
                                          ? PARSER_STATUS_COMPLETE
                                          : PARSER_STATUS_INCOMPLETE;
        assert(parser_feed(&parser, (const uint8_t*)message + index, 1) ==
               expected);
    }

    parser_deinit(&parser);
    return 0;
}

/* Verifies every two-chunk boundary, including boundaries inside CRLF. */
static int test_every_split(void) {
    const char message[] = "POST /items HTTP/1.1\r\n"
                           "Host: example.test\r\n"
                           "Content-Length: 4\r\n"
                           "\r\n"
                           "data";
    size_t length = sizeof(message) - 1;

    for (size_t split = 0; split <= length; split += 1) {
        struct parser parser;
        assert(parser_init(&parser) == 0);

        enum parser_status first =
            parser_feed(&parser, (const uint8_t*)message, split);
        if (split < length) {
            assert(first == PARSER_STATUS_INCOMPLETE);
        } else {
            assert(first == PARSER_STATUS_COMPLETE);
        }

        enum parser_status second = parser_feed(
            &parser, (const uint8_t*)message + split, length - split);
        assert(second == PARSER_STATUS_COMPLETE);
        assert(parser.request.body.length == 4);
        assert(memcmp(parser.request.body.data, "data", 4) == 0);
        parser_deinit(&parser);
    }

    return 0;
}

/* Verifies every method in the public method enum. */
static int test_methods(void) {
    static const struct {
        const char* name;
        enum request_method method;
    } methods[] = {
        {"GET", GET},
        {"POST", POST},
        {"PUT", PUT},
        {"PATCH", PATCH},
        {"DELETE", DELETE},
    };

    for (size_t index = 0; index < sizeof(methods) / sizeof(methods[0]);
        index += 1) {
        char message[128];
        int written = snprintf(message, sizeof(message),
            "%s /resource HTTP/1.0\r\n\r\n", methods[index].name);
        assert(written > 0);
        assert((size_t)written < sizeof(message));

        struct parser parser;
        assert(parser_init(&parser) == 0);
        assert(parser_feed(&parser, (const uint8_t*)message, (size_t)written) ==
               PARSER_STATUS_COMPLETE);
        assert(parser.request.method == methods[index].method);
        assert(parser.request.version == HTTP_1_0);
        parser_deinit(&parser);
    }

    return 0;
}

/* Parses one malformed request and verifies the parser enters a sticky error.
 */
static int assert_invalid_request(const char* message) {
    struct parser parser;

    assert(parser_init(&parser) == 0);
    assert(parser_feed(&parser, (const uint8_t*)message, strlen(message)) ==
           PARSER_STATUS_INVALID);
    assert(parser_feed(&parser, NULL, 0) == PARSER_STATUS_INVALID);
    parser_deinit(&parser);
    return 0;
}

/* Verifies syntax and framing failures are rejected. */
static int test_invalid_requests(void) {
    static const char* invalid[] = {
        "OPTIONS / HTTP/1.1\r\nHost: example.test\r\n\r\n",
        "GET  HTTP/1.1\r\nHost: example.test\r\n\r\n",
        "GET / HTTP/2\r\nHost: example.test\r\n\r\n",
        "GET / HTTP/1.1\r\n\r\n",
        "GET / HTTP/1.1\r\nHost example.test\r\n\r\n",
        "GET / HTTP/1.1\r\nHost: one\r\nHost: two\r\n\r\n",
        "POST / HTTP/1.1\r\nHost: example.test\r\n"
        "Content-Length: 1\r\nContent-Length: 1\r\n\r\nx",
        "POST / HTTP/1.1\r\nHost: example.test\r\n"
        "Content-Length: no\r\n\r\n",
        "POST / HTTP/1.1\r\nHost: example.test\r\n"
        "Transfer-Encoding: chunked\r\n\r\n",
        "POST / HTTP/1.1\r\nHost: example.test\r\n"
        "Content-Length: 1048577\r\n\r\n",
    };

    for (size_t index = 0; index < sizeof(invalid) / sizeof(invalid[0]);
        index += 1) {
        assert(assert_invalid_request(invalid[index]) == 0);
    }

    return 0;
}

/* Verifies a body remains incomplete until Content-Length bytes arrive. */
static int test_body_framing(void) {
    const char headers[] = "POST / HTTP/1.1\r\nHost: example.test\r\n"
                           "Content-Length: 5\r\n\r\n";
    struct parser parser;

    assert(parser_init(&parser) == 0);
    assert(parser_feed(&parser, (const uint8_t*)headers, sizeof(headers) - 1) ==
           PARSER_STATUS_INCOMPLETE);
    assert(parser_feed(&parser, (const uint8_t*)"hel", 3) ==
           PARSER_STATUS_INCOMPLETE);
    assert(parser.request.body.length == 3);
    assert(parser_feed(&parser, (const uint8_t*)"loignored", 9) ==
           PARSER_STATUS_COMPLETE);
    assert(parser.request.body.length == 5);
    assert(memcmp(parser.request.body.data, "hello", 5) == 0);
    parser_deinit(&parser);
    return 0;
}

/* Verifies oversized start and header lines are bounded while streaming. */
static int test_line_limits(void) {
    struct parser parser;
    uint8_t* data = malloc(REQUEST_MAX_START_LINE_LENGTH + 1);
    assert(data != NULL);
    memset(data, 'x', REQUEST_MAX_START_LINE_LENGTH + 1);

    assert(parser_init(&parser) == 0);
    assert(parser_feed(&parser, data, REQUEST_MAX_START_LINE_LENGTH + 1) ==
           PARSER_STATUS_INVALID);
    parser_deinit(&parser);
    free(data);

    const char start[] = "GET / HTTP/1.1\r\n";
    data = malloc(REQUEST_MAX_HEADER_LINE_LENGTH + 1);
    assert(data != NULL);
    memset(data, 'x', REQUEST_MAX_HEADER_LINE_LENGTH + 1);

    assert(parser_init(&parser) == 0);
    assert(parser_feed(&parser, (const uint8_t*)start, sizeof(start) - 1) ==
           PARSER_STATUS_INCOMPLETE);
    assert(parser_feed(&parser, data, REQUEST_MAX_HEADER_LINE_LENGTH + 1) ==
           PARSER_STATUS_INVALID);
    parser_deinit(&parser);
    free(data);
    return 0;
}

/* Runs request representation and HTTP framing tests. */
int main(void) {
    assert(test_request_init() == 0);
    assert(test_complete_request() == 0);
    assert(test_byte_at_a_time() == 0);
    assert(test_every_split() == 0);
    assert(test_methods() == 0);
    assert(test_invalid_requests() == 0);
    assert(test_body_framing() == 0);
    assert(test_line_limits() == 0);
    return 0;
}
