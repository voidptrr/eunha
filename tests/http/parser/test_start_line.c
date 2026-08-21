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

#include "http/parser.h"

/*
 * Initializes parser state over a string literal.
 */
static void init_parser_text(struct parser* parser, const char* text) {
    size_t length = strlen(text);

    parser_init(parser, (const uint8_t*)text, length);
}

/*
 * Verifies a parser buffer matches a string literal.
 */
static void assert_buffer_equals(
    struct parser_buffer buffer, const char* expected) {
    size_t expected_length = strlen(expected);

    assert(buffer.length == expected_length);
    assert(memcmp(buffer.data, expected, expected_length) == 0);
}

/*
 * Verifies parser_read_request_line returns method, target, and version slices.
 */
static int test_parser_read_request_line(void) {
    struct parser parser;
    struct parser_request_line line;

    init_parser_text(&parser, "POST /oauth/token HTTP/1.1\r\n");
    assert(parser_read_request_line(&parser, &line) == 0);

    assert(line.method == POST);
    assert_buffer_equals(line.target, "/oauth/token");
    assert_buffer_equals(line.version, "HTTP/1.1");
    assert(parser.position == strlen("POST /oauth/token HTTP/1.1\r\n"));
    return 0;
}

/*
 * Verifies parser_read_request_line recognizes the supported methods.
 */
static int test_parser_read_request_line_methods(void) {
    struct parser parser;
    struct parser_request_line line;

    init_parser_text(&parser, "GET / HTTP/1.1\r\n");
    assert(parser_read_request_line(&parser, &line) == 0);
    assert(line.method == GET);

    init_parser_text(&parser, "PUT / HTTP/1.1\r\n");
    assert(parser_read_request_line(&parser, &line) == 0);
    assert(line.method == PUT);

    init_parser_text(&parser, "PATCH / HTTP/1.1\r\n");
    assert(parser_read_request_line(&parser, &line) == 0);
    assert(line.method == PATCH);

    init_parser_text(&parser, "DELETE / HTTP/1.1\r\n");
    assert(parser_read_request_line(&parser, &line) == 0);
    assert(line.method == DELETE);
    return 0;
}

/*
 * Verifies malformed request lines fail with EINVAL.
 */
static int test_parser_read_request_line_errors(void) {
    struct parser parser;
    struct parser_request_line line;

    init_parser_text(&parser, "POST / HTTP/1.1");
    errno = 0;
    assert(parser_read_request_line(&parser, &line) == -1);
    assert(errno == EINVAL);

    init_parser_text(&parser, "POST  HTTP/1.1\r\n");
    errno = 0;
    assert(parser_read_request_line(&parser, &line) == -1);
    assert(errno == EINVAL);

    init_parser_text(&parser, "POST / \r\n");
    errno = 0;
    assert(parser_read_request_line(&parser, &line) == -1);
    assert(errno == EINVAL);

    init_parser_text(&parser, "POST / HTTP/1.1 extra\r\n");
    errno = 0;
    assert(parser_read_request_line(&parser, &line) == -1);
    assert(errno == EINVAL);

    init_parser_text(&parser, "OPTIONS / HTTP/1.1\r\n");
    errno = 0;
    assert(parser_read_request_line(&parser, &line) == -1);
    assert(errno == EINVAL);
    return 0;
}

/*
 * Runs request start-line parser unit tests.
 */
int main(void) {
    assert(test_parser_read_request_line() == 0);
    assert(test_parser_read_request_line_methods() == 0);
    assert(test_parser_read_request_line_errors() == 0);
    return 0;
}
