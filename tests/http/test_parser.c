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
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "datastruct/vector.h"
#include "eunha.h"
#include "http/header.h"
#include "http/parser.h"
#include "http/request.h"

void* __real_malloc(size_t size);
void* __real_realloc(void* data, size_t size);
void* __wrap_malloc(size_t size);
void* __wrap_realloc(void* data, size_t size);

static size_t allocations_before_failure = SIZE_MAX;

/* Enables one deterministic allocation failure after successful_count calls. */
static void allocator_fail_after(size_t successful_count) {
    allocations_before_failure = successful_count;
}

/* Disables deterministic allocation failures. */
static void allocator_reset(void) {
    allocations_before_failure = SIZE_MAX;
}

/* Returns whether the next wrapped allocation must fail. */
static bool allocator_should_fail(void) {
    if (allocations_before_failure == SIZE_MAX) {
        return false;
    }

    if (allocations_before_failure == 0) {
        errno = ENOMEM;
        return true;
    }

    allocations_before_failure -= 1;
    return false;
}

/* Linker wrapper used to exercise malloc failures without production hooks. */
void* __wrap_malloc(size_t size) {
    if (allocator_should_fail()) {
        return NULL;
    }

    return __real_malloc(size);
}

/* Linker wrapper used to exercise growth failures without production hooks. */
void* __wrap_realloc(void* data, size_t size) {
    if (allocator_should_fail()) {
        return NULL;
    }

    return __real_realloc(data, size);
}

/* Verifies initialization includes an empty request and parser state. */
static int test_parser_lifecycle(void) {
    struct parser parser;

    assert(parser_init(&parser) == EUNHA_OK);
    assert(parser.request.method == REQUEST_METHOD_UNKNOWN);
    assert(parser.request.version == HTTP_VERSION_UNKNOWN);
    assert(parser.state == PARSER_STATE_START_LINE);
    assert(parser.request.target.length == 0);
    assert(parser.request.body.length == 0);
    assert(parser.partial_line.length == 0);
    assert(vector_len(&parser.request.headers.fields) == 0);
    assert(parser.header_section_length == 0);
    assert(parser.expected_body_length == 0);
    assert(!parser.awaiting_line_feed);
    assert(parser_get_request(&parser) == NULL);

    parser_deinit(&parser);
    assert(parser.request.target.data == NULL);
    assert(parser.request.body.data == NULL);
    assert(parser.partial_line.data == NULL);
    assert(parser.request.headers.fields.data == NULL);
    return 0;
}

/* Verifies a complete request populates all application-facing fields. */
static int test_complete_request(void) {
    const char message[] = "POST /oauth2/token HTTP/1.1\r\n"
                           "Host: auth.example\r\n"
                           "Content-Type: application/json\r\n"
                           "X-Request-ID: test-1\r\n"
                           "Content-Length: 5\r\n"
                           "\r\n"
                           "hello";
    struct parser parser;

    assert(parser_init(&parser) == EUNHA_OK);
    assert(parser_feed(&parser, (const uint8_t*)message, sizeof(message) - 1) ==
           PARSER_STATUS_COMPLETE);
    assert(parser.request.method == REQUEST_METHOD_POST);
    assert(parser.request.version == HTTP_VERSION_1_1);
    assert(
        strcmp((const char*)parser.request.target.data, "/oauth2/token") == 0);
    assert(parser.request.body.length == 5);
    assert(memcmp(parser.request.body.data, "hello", 5) == 0);
    assert(vector_len(&parser.request.headers.fields) == 4);

    const struct header* content_type =
        headers_get(&parser.request.headers, "content-type");
    assert(content_type != NULL);
    assert(
        strcmp((const char*)content_type->value.data, "application/json") == 0);
    assert(headers_get(&parser.request.headers, "x-request-id") != NULL);
    assert(parser_get_request(&parser) == &parser.request);
    assert(parser_feed(&parser, NULL, 0) == PARSER_STATUS_COMPLETE);

    parser_deinit(&parser);
    return 0;
}

/* Verifies callers can feed each byte exactly once. */
static int test_byte_at_a_time(void) {
    const char message[] = "GET / HTTP/1.1\r\nHost: example.test\r\n\r\n";
    size_t length = sizeof(message) - 1;
    struct parser parser;

    assert(parser_init(&parser) == EUNHA_OK);
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
        assert(parser_init(&parser) == EUNHA_OK);

        enum parser_status first =
            parser_feed(&parser, (const uint8_t*)message, split);
        assert(first == (split == length ? PARSER_STATUS_COMPLETE
                                         : PARSER_STATUS_INCOMPLETE));
        assert(parser_feed(&parser, (const uint8_t*)message + split,
                   length - split) == PARSER_STATUS_COMPLETE);
        assert(parser.request.body.length == 4);
        assert(memcmp(parser.request.body.data, "data", 4) == 0);
        parser_deinit(&parser);
    }

    return 0;
}

/* Verifies a partial line and split CRLF resume using only new bytes. */
static int test_partial_lines(void) {
    const char first[] = "GET /health HTTP/1.1\r";
    const char second[] = "\nHost: example";
    const char third[] = ".test\r\n\r";
    const char fourth[] = "\n";
    struct parser parser;

    assert(parser_init(&parser) == EUNHA_OK);
    assert(parser_feed(&parser, (const uint8_t*)first, strlen(first)) ==
           PARSER_STATUS_INCOMPLETE);
    assert(parser.awaiting_line_feed);
    assert(parser_feed(&parser, (const uint8_t*)second, strlen(second)) ==
           PARSER_STATUS_INCOMPLETE);
    assert(parser.state == PARSER_STATE_HEADERS);
    assert(parser_feed(&parser, (const uint8_t*)third, strlen(third)) ==
           PARSER_STATUS_INCOMPLETE);
    assert(parser.awaiting_line_feed);
    assert(parser_feed(&parser, (const uint8_t*)fourth, strlen(fourth)) ==
           PARSER_STATUS_COMPLETE);
    assert(strcmp((const char*)parser.request.target.data, "/health") == 0);
    parser_deinit(&parser);
    return 0;
}

/* Verifies every supported method and HTTP version. */
static int test_methods_and_versions(void) {
    static const struct {
        const char* name;
        enum request_method method;
    } methods[] = {
        {"GET", REQUEST_METHOD_GET},
        {"POST", REQUEST_METHOD_POST},
        {"PUT", REQUEST_METHOD_PUT},
        {"PATCH", REQUEST_METHOD_PATCH},
        {"DELETE", REQUEST_METHOD_DELETE},
    };

    for (size_t index = 0; index < sizeof(methods) / sizeof(methods[0]);
        index += 1) {
        char message[128];
        int written = snprintf(message, sizeof(message),
            "%s /resource HTTP/1.0\r\n\r\n", methods[index].name);
        assert(written > 0);
        assert((size_t)written < sizeof(message));

        struct parser parser;
        assert(parser_init(&parser) == EUNHA_OK);
        assert(parser_feed(&parser, (const uint8_t*)message, (size_t)written) ==
               PARSER_STATUS_COMPLETE);
        assert(parser.request.method == methods[index].method);
        assert(parser.request.version == HTTP_VERSION_1_0);
        parser_deinit(&parser);
    }

    return 0;
}

/* Parses one malformed message and verifies INVALID remains sticky. */
static int assert_invalid_request(const char* message) {
    struct parser parser;

    assert(parser_init(&parser) == EUNHA_OK);
    assert(parser_feed(&parser, (const uint8_t*)message, strlen(message)) ==
           PARSER_STATUS_INVALID);
    assert(parser_get_request(&parser) == NULL);
    assert(parser_feed(&parser, NULL, 0) == PARSER_STATUS_INVALID);
    parser_deinit(&parser);
    return 0;
}

/* Verifies syntax, known-header, and framing failures are rejected. */
static int test_invalid_requests(void) {
    static const char* invalid[] = {
        "OPTIONS / HTTP/1.1\r\nHost: example.test\r\n\r\n",
        "GET  HTTP/1.1\r\nHost: example.test\r\n\r\n",
        "GET / HTTP/2\r\nHost: example.test\r\n\r\n",
        "GET / HTTP/1.1\r\n\r\n",
        "GET / HTTP/1.1\r\nHost:\r\n\r\n",
        "GET / HTTP/1.1\r\nHost example.test\r\n\r\n",
        "GET / HTTP/1.1\r\nHost: one\r\nHost: two\r\n\r\n",
        "POST / HTTP/1.1\r\nHost: example.test\r\n"
        "Content-Length: 1\r\nContent-Length: 1\r\n\r\nx",
        "POST / HTTP/1.1\r\nHost: example.test\r\n"
        "Content-Length:\r\n\r\n",
        "POST / HTTP/1.1\r\nHost: example.test\r\n"
        "Content-Length: no\r\n\r\n",
        "POST / HTTP/1.1\r\nHost: example.test\r\n"
        "Transfer-Encoding: chunked\r\n\r\n",
        "POST / HTTP/1.1\r\nHost: example.test\r\n"
        "Content-Length: 1048577\r\n\r\n",
        "GET / HTTP/1.1\n",
        "GET / HTTP/1.1\rX",
    };

    for (size_t index = 0; index < sizeof(invalid) / sizeof(invalid[0]);
        index += 1) {
        assert(assert_invalid_request(invalid[index]) == 0);
    }

    return 0;
}

/* Verifies non-framing duplicates remain application-owned request data. */
static int test_non_framing_headers(void) {
    const char message[] = "GET / HTTP/1.1\r\n"
                           "Host: example.test\r\n"
                           "Content-Type: application\r\n"
                           "content-type:\r\n"
                           "Authorization:\r\n"
                           "Authorization: second\r\n"
                           "X-Duplicate: first\r\n"
                           "X-Duplicate: second\r\n"
                           "\r\n";
    struct parser parser;

    assert(parser_init(&parser) == EUNHA_OK);
    assert(parser_feed(&parser, (const uint8_t*)message, sizeof(message) - 1) ==
           PARSER_STATUS_COMPLETE);
    assert(vector_len(&parser.request.headers.fields) == 7);

    const struct header* header =
        headers_get(&parser.request.headers, "CONTENT-TYPE");
    assert(header != NULL);
    assert(strcmp((const char*)header->value.data, "application") == 0);

    header = headers_get(&parser.request.headers, "authorization");
    assert(header != NULL);
    assert(header->value.length == 0);

    header = headers_get(&parser.request.headers, "x-duplicate");
    assert(header != NULL);
    assert(strcmp((const char*)header->value.data, "first") == 0);
    parser_deinit(&parser);
    return 0;
}

/* Verifies a body remains incomplete until exactly Content-Length bytes. */
static int test_body_framing(void) {
    const char headers[] = "POST / HTTP/1.1\r\nHost: example.test\r\n"
                           "Content-Length: 5\r\n\r\n";
    struct parser parser;

    assert(parser_init(&parser) == EUNHA_OK);
    assert(parser_feed(&parser, (const uint8_t*)headers, sizeof(headers) - 1) ==
           PARSER_STATUS_INCOMPLETE);
    assert(parser_feed(&parser, (const uint8_t*)"hel", 3) ==
           PARSER_STATUS_INCOMPLETE);
    assert(parser.request.body.length == 3);
    assert(parser_feed(&parser, (const uint8_t*)"lo", 2) ==
           PARSER_STATUS_COMPLETE);
    assert(parser.request.body.length == 5);
    assert(memcmp(parser.request.body.data, "hello", 5) == 0);
    parser_deinit(&parser);
    return 0;
}

/* Verifies bytes after a framed message are rejected, including later calls. */
static int test_trailing_bytes(void) {
    static const char no_body[] =
        "GET / HTTP/1.1\r\nHost: example.test\r\n\r\nextra";
    static const char body[] = "POST / HTTP/1.1\r\nHost: example.test\r\n"
                               "Content-Length: 2\r\n\r\nOKextra";
    static const char complete[] =
        "GET / HTTP/1.1\r\nHost: example.test\r\n\r\n";
    struct parser parser;

    assert(assert_invalid_request(no_body) == 0);
    assert(assert_invalid_request(body) == 0);

    assert(parser_init(&parser) == EUNHA_OK);
    assert(parser_feed(&parser, (const uint8_t*)complete,
               sizeof(complete) - 1) == PARSER_STATUS_COMPLETE);
    assert(
        parser_feed(&parser, (const uint8_t*)"x", 1) == PARSER_STATUS_INVALID);
    parser_deinit(&parser);
    return 0;
}

/* Verifies start-line, field-line, and aggregate-header limits while streaming.
 */
static int test_line_and_header_limits(void) {
    uint8_t* data = malloc(REQUEST_MAX_START_LINE_LENGTH + 1);
    assert(data != NULL);
    memset(data, 'x', REQUEST_MAX_START_LINE_LENGTH + 1);

    struct parser parser;
    assert(parser_init(&parser) == EUNHA_OK);
    assert(parser_feed(&parser, data, REQUEST_MAX_START_LINE_LENGTH + 1) ==
           PARSER_STATUS_INVALID);
    parser_deinit(&parser);
    free(data);

    const char start[] = "GET / HTTP/1.1\r\n";
    data = malloc(REQUEST_MAX_HEADER_LINE_LENGTH + 1);
    assert(data != NULL);
    memset(data, 'x', REQUEST_MAX_HEADER_LINE_LENGTH + 1);

    assert(parser_init(&parser) == EUNHA_OK);
    assert(parser_feed(&parser, (const uint8_t*)start, sizeof(start) - 1) ==
           PARSER_STATUS_INCOMPLETE);
    assert(parser_feed(&parser, data, REQUEST_MAX_HEADER_LINE_LENGTH + 1) ==
           PARSER_STATUS_INVALID);
    parser_deinit(&parser);
    free(data);

    size_t line_length = REQUEST_MAX_HEADER_LINE_LENGTH;
    data = malloc(line_length + 2);
    assert(data != NULL);
    data[0] = 'X';
    data[1] = ':';
    memset(data + 2, 'a', line_length - 2);
    data[line_length] = '\r';
    data[line_length + 1] = '\n';

    assert(parser_init(&parser) == EUNHA_OK);
    assert(parser_feed(&parser, (const uint8_t*)start, sizeof(start) - 1) ==
           PARSER_STATUS_INCOMPLETE);
    for (size_t index = 0; index < 3; index += 1) {
        assert(parser_feed(&parser, data, line_length + 2) ==
               PARSER_STATUS_INCOMPLETE);
    }
    assert(
        parser_feed(&parser, data, line_length + 2) == PARSER_STATUS_INVALID);
    parser_deinit(&parser);
    free(data);
    return 0;
}

/* Verifies INCOMPLETE transitions to COMPLETE only at the framing boundary. */
static int test_status_transitions(void) {
    const char start[] = "POST / HTTP/1.1\r\n";
    const char fields[] = "Host: example.test\r\nContent-Length: 1\r\n";
    struct parser parser;

    assert(parser_init(&parser) == EUNHA_OK);
    assert(parser_feed(&parser, NULL, 0) == PARSER_STATUS_INCOMPLETE);
    assert(parser_feed(&parser, (const uint8_t*)start, sizeof(start) - 1) ==
           PARSER_STATUS_INCOMPLETE);
    assert(parser_feed(&parser, (const uint8_t*)fields, sizeof(fields) - 1) ==
           PARSER_STATUS_INCOMPLETE);
    assert(parser_feed(&parser, (const uint8_t*)"\r", 1) ==
           PARSER_STATUS_INCOMPLETE);
    assert(parser_feed(&parser, (const uint8_t*)"\n", 1) ==
           PARSER_STATUS_INCOMPLETE);
    assert(
        parser_feed(&parser, (const uint8_t*)"x", 1) == PARSER_STATUS_COMPLETE);
    parser_deinit(&parser);
    return 0;
}

/* Verifies initialization cleanup and parse-time allocation error transitions.
 */
static int test_allocation_errors(void) {
    for (size_t successful = 0; successful < 4; successful += 1) {
        struct parser parser;
        allocator_fail_after(successful);
        assert(parser_init(&parser) == EUNHA_ERROR);
        assert(errno == ENOMEM);
        allocator_reset();
    }

    struct parser parser;
    const char long_start[] = "GET /abcdefghijklmnopq HTTP/1.1";
    assert(parser_init(&parser) == EUNHA_OK);
    assert(parser_feed(&parser, (const uint8_t*)long_start,
               sizeof(long_start) - 1) == PARSER_STATUS_INCOMPLETE);
    allocator_fail_after(0);
    assert(
        parser_feed(&parser, (const uint8_t*)"\r\n", 2) == PARSER_STATUS_ERROR);
    allocator_reset();
    assert(parser_get_request(&parser) == NULL);
    assert(parser_feed(&parser, NULL, 0) == PARSER_STATUS_ERROR);
    parser_deinit(&parser);

    const char start[] = "GET / HTTP/1.1\r\n";
    assert(parser_init(&parser) == EUNHA_OK);
    assert(parser_feed(&parser, (const uint8_t*)start, sizeof(start) - 1) ==
           PARSER_STATUS_INCOMPLETE);
    allocator_fail_after(0);
    assert(parser_feed(&parser, (const uint8_t*)"Host: x\r\n", 9) ==
           PARSER_STATUS_ERROR);
    allocator_reset();
    parser_deinit(&parser);

    const char headers[] = "POST / HTTP/1.1\r\nHost: example.test\r\n"
                           "Content-Length: 17\r\n\r\n";
    assert(parser_init(&parser) == EUNHA_OK);
    assert(parser_feed(&parser, (const uint8_t*)headers, sizeof(headers) - 1) ==
           PARSER_STATUS_INCOMPLETE);
    allocator_fail_after(0);
    assert(parser_feed(&parser, (const uint8_t*)"0123456789abcdefg", 17) ==
           PARSER_STATUS_ERROR);
    allocator_reset();
    parser_deinit(&parser);
    return 0;
}

/* Runs parser ownership, streaming, framing, and limit tests. */
int main(void) {
    assert(test_parser_lifecycle() == 0);
    assert(test_complete_request() == 0);
    assert(test_byte_at_a_time() == 0);
    assert(test_every_split() == 0);
    assert(test_partial_lines() == 0);
    assert(test_methods_and_versions() == 0);
    assert(test_invalid_requests() == 0);
    assert(test_non_framing_headers() == 0);
    assert(test_body_framing() == 0);
    assert(test_trailing_bytes() == 0);
    assert(test_line_and_header_limits() == 0);
    assert(test_status_transitions() == 0);
    assert(test_allocation_errors() == 0);
    return 0;
}
