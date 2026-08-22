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
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "http/parser.h"

/* Verifies parser initialization includes an empty owned request. */
static int test_parser_init(void) {
    struct parser parser;

    assert(parser_init(&parser) == 0);
    assert(parser.state == PARSER_START_LINE);
    assert(parser.line.length == 0);
    assert(parser.request.target.length == 0);
    assert(parser.request.body.length == 0);
    assert(!parser.saw_carriage_return);
    parser_deinit(&parser);
    return 0;
}

/* Verifies partial lines and a split CRLF resume from only new bytes. */
static int test_parser_partial_lines(void) {
    struct parser parser;
    const char first[] = "GET /health HTTP/1.1\r";
    const char second[] = "\nHost: example";
    const char third[] = ".test\r\n\r";
    const char fourth[] = "\n";

    assert(parser_init(&parser) == 0);
    assert(parser_feed(&parser, (const uint8_t*)first, strlen(first)) ==
           PARSER_STATUS_INCOMPLETE);
    assert(parser.saw_carriage_return);
    assert(parser_feed(&parser, (const uint8_t*)second, strlen(second)) ==
           PARSER_STATUS_INCOMPLETE);
    assert(parser.state == PARSER_HEADERS);
    assert(parser_feed(&parser, (const uint8_t*)third, strlen(third)) ==
           PARSER_STATUS_INCOMPLETE);
    assert(parser.saw_carriage_return);
    assert(parser_feed(&parser, (const uint8_t*)fourth, strlen(fourth)) ==
           PARSER_STATUS_COMPLETE);
    assert(strcmp((const char*)parser.request.target.data, "/health") == 0);
    parser_deinit(&parser);
    return 0;
}

/* Verifies malformed line endings fail immediately and remain failed. */
static int test_parser_line_endings(void) {
    const char bare_lf[] = "GET / HTTP/1.1\n";
    const char bare_cr[] = "GET / HTTP/1.1\rX";
    struct parser parser;

    assert(parser_init(&parser) == 0);
    assert(parser_feed(&parser, (const uint8_t*)bare_lf, strlen(bare_lf)) ==
           PARSER_STATUS_INVALID);
    assert(parser_feed(&parser, NULL, 0) == PARSER_STATUS_INVALID);
    parser_deinit(&parser);

    assert(parser_init(&parser) == 0);
    assert(parser_feed(&parser, (const uint8_t*)bare_cr, strlen(bare_cr)) ==
           PARSER_STATUS_INVALID);
    parser_deinit(&parser);
    return 0;
}

/* Runs parser lifecycle and line streaming tests. */
int main(void) {
    assert(test_parser_init() == 0);
    assert(test_parser_partial_lines() == 0);
    assert(test_parser_line_endings() == 0);
    return 0;
}
