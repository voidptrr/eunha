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

#ifndef EUNHA_PARSER_H
#define EUNHA_PARSER_H

#include <stddef.h>
#include <stdint.h>

#include "utils.h"

/*
 * Current section of an HTTP request being parsed.
 */
enum parser_state {
    PARSER_START_LINE,
    PARSER_HEADERS,
    PARSER_BODY,
    PARSER_COMPLETE,
    PARSER_INVALID,
};

/*
 * Stateful cursor that resumes parsing as more request bytes arrive.
 */
struct parser {
    enum parser_state state;
    size_t position;
    size_t scan_position;
    size_t header_length;
};

/*
 * Initializes a parser at the beginning of a request.
 */
struct parser parser_init(void);

/*
 * Adjusts parser offsets after the caller removes consumed leading bytes.
 */
void parser_discard(struct parser* parser, size_t length);

/*
 * Reads and consumes one CRLF-terminated line. Returns an empty pointer and
 * EAGAIN when more bytes are needed, or EINVAL for malformed line endings.
 */
struct buffer parser_read_line(
    struct parser* parser, const uint8_t* data, size_t length);

#endif
