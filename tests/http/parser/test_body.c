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
 * Verifies parser_read_body consumes the empty line and returns body bytes.
 */
static int test_parser_read_body(void) {
    struct parser parser;
    struct parser_request_line request_line;
    struct parser_header header;
    struct parser_buffer body;

    init_parser_text(&parser, "POST /token HTTP/1.1\r\n"
                              "Content-Length: 7\r\n"
                              "\r\n"
                              "grant=a");

    assert(parser_read_request_line(&parser, &request_line) == 0);
    assert(parser_read_header(&parser, &header) == 0);
    assert(parser_read_body(&parser, 7, &body) == 0);

    assert_buffer_equals(body, "grant=a");
    assert(parser.position == parser.length);
    return 0;
}

/*
 * Verifies a message can have an empty body after the empty line.
 */
static int test_parser_read_body_empty(void) {
    struct parser parser;
    struct parser_buffer body;

    init_parser_text(&parser, "\r\n");
    assert(parser_read_body(&parser, 0, &body) == 0);

    assert(body.length == 0);
    assert(parser.position == parser.length);
    return 0;
}

/*
 * Verifies parser_read_body rejects data before the empty line.
 */
static int test_parser_read_body_errors(void) {
    struct parser parser;
    struct parser_buffer body;

    init_parser_text(&parser, "Content-Length: 7\r\nbody");
    errno = 0;
    assert(parser_read_body(&parser, 7, &body) == -1);
    assert(errno == EINVAL);

    init_parser_text(&parser, "\nbody");
    errno = 0;
    assert(parser_read_body(&parser, 4, &body) == -1);
    assert(errno == EINVAL);
    return 0;
}

/*
 * Verifies parser_read_body rejects bodies that do not match Content-Length.
 */
static int test_parser_read_body_length_errors(void) {
    struct parser parser;
    struct parser_buffer body;

    init_parser_text(&parser, "\r\nabc");
    errno = 0;
    assert(parser_read_body(&parser, 2, &body) == -1);
    assert(errno == EINVAL);

    init_parser_text(&parser, "\r\nabc");
    errno = 0;
    assert(parser_read_body(&parser, 4, &body) == -1);
    assert(errno == EINVAL);
    return 0;
}

/*
 * Runs body parser unit tests.
 */
int main(void) {
    assert(test_parser_read_body() == 0);
    assert(test_parser_read_body_empty() == 0);
    assert(test_parser_read_body_errors() == 0);
    assert(test_parser_read_body_length_errors() == 0);
    return 0;
}
