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

#include "parser.h"

#define CR 0x0D
#define HTAB 0x09
#define LF 0x0A
#define SP 0x20

/*
 * Returns whether byte is HTTP optional whitespace.
 */
static int parser_is_whitespace(uint8_t byte) {
    return byte == SP || byte == HTAB;
}

/*
 * Initializes parser to read length octets from data.
 */
void parser_init(struct parser* parser, const uint8_t* data, size_t length) {
    assert(parser != NULL);
    assert(data != NULL || length == 0);

    parser->data = data;
    parser->length = length;
    parser->position = 0;
}

/*
 * Returns the next octet without advancing. Returns -1 at end-of-buffer.
 */
int parser_peek(const struct parser* parser) {
    assert(parser != NULL);
    assert(parser->position <= parser->length);

    if (parser->position == parser->length) {
        return -1;
    }

    return parser->data[parser->position];
}

/*
 * Advances past HTTP optional whitespace: SP and HTAB.
 */
void parser_skip_whitespace(struct parser* parser) {
    assert(parser != NULL);
    assert(parser->position <= parser->length);

    while (parser->position < parser->length &&
           parser_is_whitespace(parser->data[parser->position])) {
        parser->position += 1;
    }
}

/*
 * Reads one header line. Name is [line-start, ':'), value starts after ':' and
 * optional whitespace, and the parser advances past the terminating CRLF.
 */
int parser_read_header(struct parser* parser, struct parser_header* header) {
    assert(parser != NULL);
    assert(header != NULL);
    assert(parser->position <= parser->length);

    const uint8_t* line_start = parser->data + parser->position;
    size_t length = parser->length - parser->position;
    const uint8_t* line_end = memchr(line_start, CR, length);

    /* A complete header line must end with CRLF. */
    if (line_end == NULL || line_end + 1 >= parser->data + parser->length ||
        line_end[1] != LF) {
        errno = EINVAL;
        return -1;
    }

    size_t line_length = (size_t)(line_end - line_start);
    const uint8_t* colon = memchr(line_start, ':', line_length);

    /* Header fields need a non-empty name followed by ':'. */
    if (colon == NULL || colon == line_start) {
        errno = EINVAL;
        return -1;
    }

    parser->position += (size_t)(colon - line_start) + 1;
    parser_skip_whitespace(parser);

    header->name.data = line_start;
    header->name.length = (size_t)(colon - line_start);
    header->value.data = parser->data + parser->position;
    header->value.length = (size_t)(line_end - header->value.data);
    parser->position = (size_t)(line_end - parser->data) + 2;
    return 0;
}
