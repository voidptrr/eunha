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
#include <errno.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "http/parser.h"
#include "http/request.h"

/*
 * Verifies an owned request string matches a string literal.
 */
static void assert_string_equals(
    const struct string* string, const char* expected) {
    size_t expected_length = strlen(expected);

    assert(string->length == expected_length);
    assert(memcmp(string->data, expected, expected_length) == 0);
    assert(string->data[string->length] == 0);
}

/*
 * Verifies a complete string is rejected as one invalid request.
 */
static void assert_invalid_request(const char* data) {
    struct parser parser = parser_init();
    struct request request;

    assert(request_init(&request) == 0);

    errno = 0;
    assert(request_parse(&parser, &request, (const uint8_t*)data,
               strlen(data)) == REQUEST_INVALID);
    assert(errno == EINVAL);
    assert(parser.state == PARSER_INVALID);

    request_deinit(&request);
}

/*
 * Verifies request-owned fields start empty and deinitialize cleanly.
 */
static int test_request_init_deinit(void) {
    struct request request;

    assert(request_init(&request) == 0);
    assert(request.target.data != NULL);
    assert(request.target.length == 0);
    assert(request.method == UNKNOWN);
    assert(request.version == HTTP_VERSION_UNKNOWN);
    assert(request.headers.content_type == CONTENT_TYPE_NONE);
    assert(request.headers.content_length == 0);
    assert(!request.headers.has_host);
    assert(!request.headers.has_content_length);

    request_deinit(&request);
    assert(request.target.data == NULL);
    assert(request.body.data == NULL);
    return 0;
}

/*
 * Verifies one call can parse an already complete request.
 */
static int test_request_parse_complete(void) {
    const char* data = "POST /token HTTP/1.1\r\n"
                       "Host: example.test\r\n"
                       "Authorization: Bearer token\r\n"
                       "Content-Type: application/json; charset=utf-8\r\n"
                       "Content-Length: 7\r\n"
                       "X-Eunha: custom\r\n"
                       "\r\n"
                       "grant=a";
    struct parser parser = parser_init();
    struct request request;

    assert(request_init(&request) == 0);

    assert(request_parse(&parser, &request, (const uint8_t*)data,
               strlen(data)) == REQUEST_COMPLETE);
    assert(parser.state == PARSER_COMPLETE);
    assert(parser.position == strlen(data));
    assert(request.method == POST);
    assert_string_equals(&request.target, "/token");
    assert(request.version == HTTP_1_1);
    assert_string_equals(&request.headers.host, "example.test");
    assert_string_equals(&request.headers.authorization, "Bearer token");
    assert(request.headers.content_type == CONTENT_TYPE_APPLICATION_JSON);
    assert(request.headers.content_length == 7);
    assert(request.headers.has_host);
    assert(request.headers.has_content_length);
    assert_string_equals(&request.body, "grant=a");

    request_deinit(&request);
    return 0;
}

/*
 * Verifies parsing resumes through split start-line, header, and body data.
 */
static int test_request_parse_incremental(void) {
    const char* data = "POST /token HTTP/1.1\r\n"
                       "Host: example.test\r\n"
                       "Content-Length: 7\r\n"
                       "\r\n"
                       "grant=a";
    size_t start_line_end = strlen("POST /token HTTP/1.1\r\n");
    size_t partial_host_end = strlen("POST /token HTTP/1.1\r\nHost: example");
    size_t headers_without_end = strlen("POST /token HTTP/1.1\r\n"
                                        "Host: example.test\r\n"
                                        "Content-Length: 7\r\n");
    size_t body_start = headers_without_end + 2;
    struct parser parser = parser_init();
    struct request request;

    assert(request_init(&request) == 0);

    assert(request_parse(&parser, &request, (const uint8_t*)data,
               start_line_end - 1) == REQUEST_INCOMPLETE);
    assert(parser.state == PARSER_START_LINE);
    assert(parser.position == 0);

    assert(request_parse(&parser, &request, (const uint8_t*)data,
               start_line_end) == REQUEST_INCOMPLETE);
    assert(parser.state == PARSER_HEADERS);
    assert(parser.position == start_line_end);
    assert(request.method == POST);
    assert_string_equals(&request.target, "/token");

    assert(request_parse(&parser, &request, (const uint8_t*)data,
               partial_host_end) == REQUEST_INCOMPLETE);
    assert(parser.position == start_line_end);

    assert(request_parse(&parser, &request, (const uint8_t*)data,
               headers_without_end) == REQUEST_INCOMPLETE);
    assert(parser.state == PARSER_HEADERS);
    assert(parser.position == headers_without_end);
    assert_string_equals(&request.headers.host, "example.test");
    assert(request.headers.content_length == 7);

    assert(request_parse(&parser, &request, (const uint8_t*)data,
               body_start + 3) == REQUEST_INCOMPLETE);
    assert(parser.state == PARSER_BODY);
    assert(parser.position == body_start + 3);
    assert_string_equals(&request.body, "gra");

    assert(request_parse(&parser, &request, (const uint8_t*)data,
               strlen(data)) == REQUEST_COMPLETE);
    assert(parser.position == strlen(data));
    assert_string_equals(&request.body, "grant=a");

    request_deinit(&request);
    return 0;
}

/*
 * Verifies supported methods and HTTP/1.0 are accepted.
 */
static int test_request_parse_start_line_values(void) {
    struct method_case {
        const char* data;
        enum request_method method;
    } cases[] = {
        {"GET / HTTP/1.0\r\n\r\n", GET},
        {"PUT / HTTP/1.1\r\nHost: example.test\r\n\r\n", PUT},
        {"PATCH / HTTP/1.1\r\nHost: example.test\r\n\r\n", PATCH},
        {"DELETE / HTTP/1.1\r\nHost: example.test\r\n\r\n", DELETE},
    };

    for (size_t index = 0; index < sizeof(cases) / sizeof(cases[0]);
        index += 1) {
        struct parser parser = parser_init();
        struct request request;

        assert(request_init(&request) == 0);
        assert(
            request_parse(&parser, &request, (const uint8_t*)cases[index].data,
                strlen(cases[index].data)) == REQUEST_COMPLETE);
        assert(request.method == cases[index].method);

        if (index == 0) {
            assert(request.version == HTTP_1_0);
        }

        request_deinit(&request);
    }

    return 0;
}

/*
 * Verifies malformed start lines and headers make parser failure sticky.
 */
static int test_request_parse_errors(void) {
    assert_invalid_request("OPTIONS / HTTP/1.1\r\n\r\n");
    assert_invalid_request("GET / HTTP/2.0\r\n\r\n");
    assert_invalid_request("POST  HTTP/1.1\r\n\r\n");
    assert_invalid_request("POST / HTTP/1.1 extra\r\n\r\n");
    assert_invalid_request("POST / HTTP/1.1\r\nHost value\r\n\r\n");
    assert_invalid_request("POST / HTTP/1.1\r\nContent-Length: nope\r\n\r\n");
    assert_invalid_request("GET / HTTP/1.1\r\n\r\n");
    assert_invalid_request("GET /bad\ttarget HTTP/1.1\r\n"
                           "Host: example.test\r\n\r\n");
    assert_invalid_request("POST / HTTP/1.1\r\n"
                           "Host: example.test\r\n"
                           "Transfer-Encoding: chunked\r\n\r\n");
    assert_invalid_request("POST / HTTP/1.1\r\n"
                           "Host: example.test\r\n"
                           "Content-Length: 3\r\n"
                           "Content-Length: 3\r\n\r\nabc");
    return 0;
}

/*
 * Verifies the body waits for Content-Length and ignores following bytes.
 */
static int test_request_parse_body_framing(void) {
    const char* data = "POST /token HTTP/1.1\r\n"
                       "Host: example.test\r\n"
                       "Content-Length: 3\r\n"
                       "\r\n"
                       "abcnext";
    size_t first_request_length = strlen("POST /token HTTP/1.1\r\n"
                                         "Host: example.test\r\n"
                                         "Content-Length: 3\r\n"
                                         "\r\n"
                                         "abc");
    struct parser parser = parser_init();
    struct request request;

    assert(request_init(&request) == 0);

    assert(request_parse(&parser, &request, (const uint8_t*)data,
               first_request_length - 1) == REQUEST_INCOMPLETE);
    assert(parser.state == PARSER_BODY);
    assert_string_equals(&request.body, "ab");

    assert(request_parse(&parser, &request, (const uint8_t*)data,
               strlen(data)) == REQUEST_COMPLETE);
    assert(parser.position == first_request_length);
    assert_string_equals(&request.body, "abc");

    request_deinit(&request);
    return 0;
}

/*
 * Verifies the same request succeeds across every possible receive split.
 */
static int test_request_parse_every_split(void) {
    const char* data = "POST /token HTTP/1.1\r\n"
                       "Host: example.test\r\n"
                       "Content-Type: application/json\r\n"
                       "Content-Length: 7\r\n"
                       "\r\n"
                       "grant=a";
    size_t length = strlen(data);

    for (size_t split = 0; split < length; split += 1) {
        struct parser parser = parser_init();
        struct request request;

        assert(request_init(&request) == 0);

        assert(request_parse(&parser, &request, (const uint8_t*)data, split) ==
               REQUEST_INCOMPLETE);
        assert(request_parse(&parser, &request, (const uint8_t*)data, length) ==
               REQUEST_COMPLETE);
        assert_string_equals(&request.body, "grant=a");

        request_deinit(&request);
    }

    struct parser parser = parser_init();
    struct request request;

    assert(request_init(&request) == 0);

    for (size_t received = 0; received < length; received += 1) {
        assert(request_parse(&parser, &request, (const uint8_t*)data,
                   received) == REQUEST_INCOMPLETE);
    }

    assert(request_parse(&parser, &request, (const uint8_t*)data, length) ==
           REQUEST_COMPLETE);
    assert_string_equals(&request.body, "grant=a");
    request_deinit(&request);

    return 0;
}

/*
 * Verifies configured start-line, header, and body limits are enforced.
 */
static int test_request_parse_limits(void) {
    struct parser parser = parser_init();
    struct request request;

    uint8_t* start_line = malloc(REQUEST_MAX_START_LINE_LENGTH + 1);
    assert(start_line != NULL);
    memset(start_line, 'A', REQUEST_MAX_START_LINE_LENGTH + 1);

    assert(request_init(&request) == 0);
    errno = 0;
    assert(request_parse(&parser, &request, start_line,
               REQUEST_MAX_START_LINE_LENGTH + 1) == REQUEST_INVALID);
    assert(errno == EMSGSIZE);
    request_deinit(&request);
    free(start_line);

    const char* prefix = "GET / HTTP/1.1\r\n";
    size_t prefix_length = strlen(prefix);
    size_t header_data_length =
        prefix_length + REQUEST_MAX_HEADER_LINE_LENGTH + 1;
    uint8_t* header_data = malloc(header_data_length);
    assert(header_data != NULL);
    memcpy(header_data, prefix, prefix_length);
    memset(
        header_data + prefix_length, 'X', REQUEST_MAX_HEADER_LINE_LENGTH + 1);

    assert(request_init(&request) == 0);
    parser = parser_init();
    errno = 0;
    assert(request_parse(&parser, &request, header_data, header_data_length) ==
           REQUEST_INVALID);
    assert(errno == EMSGSIZE);
    request_deinit(&request);
    free(header_data);

    const char* header_line = "X:a\r\n";
    size_t header_line_length = strlen(header_line);
    size_t oversized_headers_length = REQUEST_MAX_HEADERS_LENGTH + 10;
    size_t oversized_request_length = prefix_length + oversized_headers_length;
    uint8_t* oversized_headers = malloc(oversized_request_length);
    assert(oversized_headers != NULL);
    memcpy(oversized_headers, prefix, prefix_length);

    size_t offset = prefix_length;
    while (offset + header_line_length <= oversized_request_length) {
        memcpy(oversized_headers + offset, header_line, header_line_length);
        offset += header_line_length;
    }

    assert(request_init(&request) == 0);
    parser = parser_init();
    errno = 0;
    assert(request_parse(&parser, &request, oversized_headers, offset) ==
           REQUEST_INVALID);
    assert(errno == EMSGSIZE);
    request_deinit(&request);
    free(oversized_headers);

    char body_limit[256];
    int written = snprintf(body_limit, sizeof(body_limit),
        "POST / HTTP/1.1\r\nHost: example.test\r\nContent-Length: %zu\r\n\r\n",
        (size_t)REQUEST_MAX_BODY_LENGTH + 1);
    assert(written > 0);
    assert((size_t)written < sizeof(body_limit));

    assert(request_init(&request) == 0);
    parser = parser_init();
    errno = 0;
    assert(request_parse(&parser, &request, (const uint8_t*)body_limit,
               (size_t)written) == REQUEST_INVALID);
    assert(errno == EMSGSIZE);
    request_deinit(&request);
    return 0;
}

/*
 * Verifies a request without Content-Length completes with an empty body.
 */
static int test_request_parse_empty_body(void) {
    const char* data = "GET / HTTP/1.1\r\nHost: example.test\r\n\r\n";
    struct parser parser = parser_init();
    struct request request;

    assert(request_init(&request) == 0);

    assert(request_parse(&parser, &request, (const uint8_t*)data,
               strlen(data)) == REQUEST_COMPLETE);
    assert(request.body.length == 0);
    assert(parser.position == strlen(data));

    request_deinit(&request);
    return 0;
}

/*
 * Runs incremental request parsing tests.
 */
int main(void) {
    assert(test_request_init_deinit() == 0);
    assert(test_request_parse_complete() == 0);
    assert(test_request_parse_incremental() == 0);
    assert(test_request_parse_start_line_values() == 0);
    assert(test_request_parse_errors() == 0);
    assert(test_request_parse_body_framing() == 0);
    assert(test_request_parse_empty_body() == 0);
    assert(test_request_parse_every_split() == 0);
    assert(test_request_parse_limits() == 0);
    return 0;
}
