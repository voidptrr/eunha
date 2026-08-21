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
 * Verifies parser_init and parser_peek preserve the current position.
 */
static int test_parser_init_peek(void) {
    struct parser parser;

    init_parser_text(&parser, "GET");
    assert(parser.data != NULL);
    assert(parser.length == 3);
    assert(parser.position == 0);
    assert(parser_peek(&parser) == 'G');
    assert(parser.position == 0);

    parser_init(&parser, NULL, 0);
    assert(parser_peek(&parser) == -1);
    return 0;
}

/*
 * Verifies parser_skip_whitespace consumes only SP and HTAB.
 */
static int test_parser_skip_whitespace(void) {
    struct parser parser;

    init_parser_text(&parser, " \tGET");
    parser_skip_whitespace(&parser);

    assert(parser.position == 2);
    assert(parser_peek(&parser) == 'G');
    return 0;
}

/*
 * Verifies parser_read_header returns name and value slices.
 */
static int test_parser_read_header(void) {
    struct parser parser;
    struct parser_header header;

    init_parser_text(&parser, "Content-Length: 123\r\n");
    assert(parser_read_header(&parser, &header) == 0);

    assert(header.name.length == strlen("Content-Length"));
    assert(memcmp(header.name.data, "Content-Length", header.name.length) == 0);
    assert(header.value.length == strlen("123"));
    assert(memcmp(header.value.data, "123", header.value.length) == 0);
    assert(parser.position == strlen("Content-Length: 123\r\n"));
    return 0;
}

/*
 * Verifies optional whitespace after ':' is skipped.
 */
static int test_parser_read_header_skip_value_whitespace(void) {
    struct parser parser;
    struct parser_header header;

    init_parser_text(&parser, "Host:\t example.com\r\n");
    assert(parser_read_header(&parser, &header) == 0);

    assert(header.name.length == strlen("Host"));
    assert(memcmp(header.name.data, "Host", header.name.length) == 0);
    assert(header.value.length == strlen("example.com"));
    assert(memcmp(header.value.data, "example.com", header.value.length) == 0);
    return 0;
}

/*
 * Verifies empty header values are accepted.
 */
static int test_parser_read_header_empty_value(void) {
    struct parser parser;
    struct parser_header header;

    init_parser_text(&parser, "X:\r\n");
    assert(parser_read_header(&parser, &header) == 0);

    assert(header.name.length == strlen("X"));
    assert(memcmp(header.name.data, "X", header.name.length) == 0);
    assert(header.value.length == 0);
    assert(parser.position == strlen("X:\r\n"));
    return 0;
}

/*
 * Verifies malformed header lines fail with EINVAL.
 */
static int test_parser_read_header_errors(void) {
    struct parser parser;
    struct parser_header header;

    init_parser_text(&parser, "Host: value");
    errno = 0;
    assert(parser_read_header(&parser, &header) == -1);
    assert(errno == EINVAL);

    init_parser_text(&parser, "Host: value\rX");
    errno = 0;
    assert(parser_read_header(&parser, &header) == -1);
    assert(errno == EINVAL);

    init_parser_text(&parser, "Host value\r\n");
    errno = 0;
    assert(parser_read_header(&parser, &header) == -1);
    assert(errno == EINVAL);

    init_parser_text(&parser, ": value\r\n");
    errno = 0;
    assert(parser_read_header(&parser, &header) == -1);
    assert(errno == EINVAL);
    return 0;
}

/*
 * Runs parser unit tests.
 */
int main(void) {
    assert(test_parser_init_peek() == 0);
    assert(test_parser_skip_whitespace() == 0);
    assert(test_parser_read_header() == 0);
    assert(test_parser_read_header_skip_value_whitespace() == 0);
    assert(test_parser_read_header_empty_value() == 0);
    assert(test_parser_read_header_errors() == 0);
    return 0;
}
