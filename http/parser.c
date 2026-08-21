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
 * Returns whether two parser bytes match a string literal.
 */
static int parser_buffer_equals(
    const struct parser_buffer* buffer, const char* expected) {
    size_t expected_length = strlen(expected);

    return buffer->length == expected_length &&
           memcmp(buffer->data, expected, expected_length) == 0;
}

/*
 * Returns whether byte is HTTP optional whitespace.
 */
static int parser_is_whitespace(uint8_t byte) {
    return byte == SP || byte == HTAB;
}

/*
 * Maps a request-line method token to one of the methods supported here.
 */
static enum request_method parser_method_from_buffer(
    const struct parser_buffer* method) {
    if (parser_buffer_equals(method, "GET")) {
        return GET;
    }

    if (parser_buffer_equals(method, "POST")) {
        return POST;
    }

    if (parser_buffer_equals(method, "PUT")) {
        return PUT;
    }

    if (parser_buffer_equals(method, "PATCH")) {
        return PATCH;
    }

    if (parser_buffer_equals(method, "DELETE")) {
        return DELETE;
    }

    return UNKNOWN;
}

/*
 * Reads a CRLF-terminated line without advancing the parser.
 */
static int parser_read_line(const struct parser* parser,
    const uint8_t** line_start, const uint8_t** line_end) {
    assert(parser != NULL);
    assert(line_start != NULL);
    assert(line_end != NULL);
    assert(parser->position <= parser->length);

    const uint8_t* start = parser->data + parser->position;
    const uint8_t* data_end = parser->data + parser->length;
    size_t length = parser->length - parser->position;
    const uint8_t* end = memchr(start, CR, length);

    /* HTTP lines are delimited by CRLF, not bare CR or LF. */
    if (end == NULL || end + 1 >= data_end || end[1] != LF) {
        errno = EINVAL;
        return -1;
    }

    *line_start = start;
    *line_end = end;
    return 0;
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
 * Reads one request line: method SP request-target SP HTTP-version CRLF.
 */
int parser_read_request_line(
    struct parser* parser, struct parser_request_line* line) {
    assert(parser != NULL);
    assert(line != NULL);
    assert(parser->position <= parser->length);

    const uint8_t* line_start = NULL;
    const uint8_t* line_end = NULL;

    if (parser_read_line(parser, &line_start, &line_end) != 0) {
        return -1;
    }

    size_t line_length = (size_t)(line_end - line_start);
    const uint8_t* first_space = memchr(line_start, SP, line_length);

    /* The method must be followed by SP and cannot be empty. */
    if (first_space == NULL || first_space == line_start) {
        errno = EINVAL;
        return -1;
    }

    size_t remaining_length = (size_t)(line_end - first_space - 1);
    const uint8_t* target_start = first_space + 1;
    const uint8_t* second_space = memchr(target_start, SP, remaining_length);

    /* The request target must be followed by SP and cannot be empty. */
    if (second_space == NULL || second_space == target_start) {
        errno = EINVAL;
        return -1;
    }

    const uint8_t* version_start = second_space + 1;

    /* The HTTP version is the final field and cannot be empty. */
    if (version_start == line_end) {
        errno = EINVAL;
        return -1;
    }

    /* HTTP-version is a single field, so it cannot contain another SP. */
    if (memchr(version_start, SP, (size_t)(line_end - version_start)) != NULL) {
        errno = EINVAL;
        return -1;
    }

    struct parser_buffer method = {
        .data = line_start,
        .length = (size_t)(first_space - line_start),
    };

    enum request_method request_method = parser_method_from_buffer(&method);

    /* For now the parser only accepts the methods we explicitly support. */
    if (request_method == UNKNOWN) {
        errno = EINVAL;
        return -1;
    }

    line->method = request_method;
    line->target.data = target_start;
    line->target.length = (size_t)(second_space - target_start);
    line->version.data = version_start;
    line->version.length = (size_t)(line_end - version_start);
    parser->position = (size_t)(line_end - parser->data) + 2;
    return 0;
}

/*
 * Reads one header line. Name is [line-start, ':'), value starts after ':' and
 * optional whitespace, and the parser advances past the terminating CRLF.
 */
int parser_read_header(struct parser* parser, struct parser_header* header) {
    assert(parser != NULL);
    assert(header != NULL);
    assert(parser->position <= parser->length);

    const uint8_t* line_start = NULL;
    const uint8_t* line_end = NULL;

    if (parser_read_line(parser, &line_start, &line_end) != 0) {
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
