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
#include <string.h>

#include "http/header.h"

/*
 * Verifies an owned string matches a string literal.
 */
static void assert_string_equals(
    const struct string* string, const char* expected) {
    size_t expected_length = strlen(expected);

    assert(string->length == expected_length);
    assert(memcmp(string->data, expected, expected_length) == 0);
}

/*
 * Parses a string literal as one header line.
 */
static int parse_header(struct headers* headers, const char* line) {
    return headers_parse_line(headers, (const uint8_t*)line, strlen(line));
}

/*
 * Verifies known headers are promoted into direct fields.
 */
static int test_headers_parse_line(void) {
    struct headers headers;

    assert(headers_init(&headers) == 0);
    assert(parse_header(&headers, "Host: example.test") == 0);
    assert(parse_header(&headers, "Authorization: Bearer token") == 0);
    assert(parse_header(&headers, "Content-Type: application/json") == 0);
    assert(parse_header(&headers, "Content-Length: 7") == 0);
    assert(parse_header(&headers, "X-Eunha: custom") == 0);

    assert_string_equals(&headers.host, "example.test");
    assert_string_equals(&headers.authorization, "Bearer token");
    assert(headers.content_type == CONTENT_TYPE_APPLICATION_JSON);
    assert(headers.content_length == 7);
    assert(headers.has_host);
    assert(headers.has_authorization);
    assert(headers.has_content_type);
    assert(headers.has_content_length);

    headers_deinit(&headers);
    return 0;
}

/*
 * Verifies header names are case-insensitive and values trim HTTP whitespace.
 */
static int test_headers_parse_line_whitespace(void) {
    struct headers headers;

    assert(headers_init(&headers) == 0);
    assert(parse_header(&headers, "host:\t example.test \t") == 0);
    assert(parse_header(&headers, "content-length: \t7 ") == 0);

    assert_string_equals(&headers.host, "example.test");
    assert(headers.content_length == 7);

    headers_deinit(&headers);
    return 0;
}

/*
 * Verifies malformed header lines and lengths are rejected.
 */
static int test_headers_parse_line_errors(void) {
    struct headers headers;

    assert(headers_init(&headers) == 0);

    errno = 0;
    assert(parse_header(&headers, "Host example.test") == -1);
    assert(errno == EINVAL);

    errno = 0;
    assert(parse_header(&headers, ": value") == -1);
    assert(errno == EINVAL);

    errno = 0;
    assert(parse_header(&headers, "Content-Length: nope") == -1);
    assert(errno == EINVAL);

    errno = 0;
    assert(parse_header(&headers, "Host : example.test") == -1);
    assert(errno == EINVAL);

    errno = 0;
    assert(parse_header(&headers, " folded: value") == -1);
    assert(errno == EINVAL);

    headers_deinit(&headers);
    return 0;
}

/*
 * Verifies ambiguous framing and duplicate singleton headers are rejected.
 */
static int test_headers_reject_ambiguous_fields(void) {
    struct headers headers;

    assert(headers_init(&headers) == 0);

    assert(parse_header(&headers, "Content-Length: 3") == 0);
    errno = 0;
    assert(parse_header(&headers, "Content-Length: 3") == -1);
    assert(errno == EINVAL);

    headers_clear(&headers);
    errno = 0;
    assert(parse_header(&headers, "Transfer-Encoding: chunked") == -1);
    assert(errno == EINVAL);

    headers_clear(&headers);
    assert(parse_header(&headers, "Host: first.test") == 0);
    errno = 0;
    assert(parse_header(&headers, "Host: second.test") == -1);
    assert(errno == EINVAL);

    headers_clear(&headers);
    assert(parse_header(&headers, "Authorization: first") == 0);
    errno = 0;
    assert(parse_header(&headers, "Authorization: second") == -1);
    assert(errno == EINVAL);

    headers_clear(&headers);
    assert(parse_header(&headers, "Content-Type: application/json") == 0);
    errno = 0;
    assert(parse_header(&headers, "Content-Type: text/plain") == -1);
    assert(errno == EINVAL);

    headers_deinit(&headers);
    return 0;
}

/*
 * Verifies control bytes are rejected inside field values.
 */
static int test_headers_reject_control_bytes(void) {
    const uint8_t line[] = {'X', '-', 'T', 'e', 's', 't', ':', ' ', 0x00};
    struct headers headers;

    assert(headers_init(&headers) == 0);
    errno = 0;
    assert(headers_parse_line(&headers, line, sizeof(line)) == -1);
    assert(errno == EINVAL);

    headers_deinit(&headers);
    return 0;
}

/*
 * Verifies Content-Type parameters do not affect media type classification.
 */
static int test_headers_content_type(void) {
    struct headers headers;

    assert(headers_init(&headers) == 0);

    assert(parse_header(
               &headers, "Content-Type: application/json; charset=utf-8") == 0);
    assert(headers.content_type == CONTENT_TYPE_APPLICATION_JSON);

    headers_clear(&headers);
    assert(parse_header(&headers,
               "Content-Type: application/x-www-form-urlencoded") == 0);
    assert(headers.content_type == CONTENT_TYPE_APPLICATION_FORM_URLENCODED);

    headers_clear(&headers);
    assert(parse_header(&headers, "Content-Type: text/plain") == 0);
    assert(headers.content_type == CONTENT_TYPE_TEXT_PLAIN);

    headers_clear(&headers);
    assert(parse_header(&headers, "Content-Type: application/eunha") == 0);
    assert(headers.content_type == CONTENT_TYPE_CUSTOM);

    headers_deinit(&headers);
    return 0;
}

/*
 * Runs single-header parsing tests.
 */
int main(void) {
    assert(test_headers_parse_line() == 0);
    assert(test_headers_parse_line_whitespace() == 0);
    assert(test_headers_parse_line_errors() == 0);
    assert(test_headers_reject_ambiguous_fields() == 0);
    assert(test_headers_reject_control_bytes() == 0);
    assert(test_headers_content_type() == 0);
    return 0;
}
