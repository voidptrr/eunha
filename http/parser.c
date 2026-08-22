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
#include <string.h>

#include "http/parser.h"

#define CR 0x0D
#define LF 0x0A

/*
 * Initializes a parser at the beginning of a request.
 */
struct parser parser_init(void) {
    return (struct parser){
        .state = PARSER_START_LINE,
        .position = 0,
        .scan_position = 0,
        .header_length = 0,
    };
}

/*
 * Adjusts parser offsets after the caller removes consumed leading bytes.
 */
void parser_discard(struct parser* parser, size_t length) {
    assert(parser != NULL);
    assert(length <= parser->position);
    assert(length <= parser->scan_position);

    parser->position -= length;
    parser->scan_position -= length;
}

/*
 * Reads and consumes one CRLF-terminated line. Incomplete lines do not advance
 * the parser, allowing the same line to resume after another receive.
 */
struct buffer parser_read_line(
    struct parser* parser, const uint8_t* data, size_t length) {
    assert(parser != NULL);
    assert(data != NULL || length == 0);
    assert(parser->position <= length);
    assert(parser->scan_position >= parser->position);
    assert(parser->scan_position <= length);

    if (parser->scan_position == length) {
        errno = EAGAIN;
        return (struct buffer){0};
    }

    const uint8_t* line_start = data + parser->position;
    const uint8_t* scan_start = data + parser->scan_position;
    size_t remaining = length - parser->scan_position;
    const uint8_t* cr = memchr(scan_start, CR, remaining);
    const uint8_t* lf = memchr(scan_start, LF, remaining);

    /* A bare LF cannot become valid after receiving more data. */
    if (lf != NULL && (cr == NULL || lf < cr)) {
        errno = EINVAL;
        return (struct buffer){0};
    }

    if (cr == NULL) {
        parser->scan_position = length;
        errno = EAGAIN;
        return (struct buffer){0};
    }

    if (cr + 1 == data + length) {
        parser->scan_position = (size_t)(cr - data);
        errno = EAGAIN;
        return (struct buffer){0};
    }

    /* HTTP lines require CRLF; a bare CR is invalid. */
    if (cr[1] != LF) {
        errno = EINVAL;
        return (struct buffer){0};
    }

    parser->position = (size_t)(cr - data) + 2;
    parser->scan_position = parser->position;
    return (struct buffer){
        .data = line_start,
        .length = (size_t)(cr - line_start),
    };
}
