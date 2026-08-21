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

/*
 * Tokenizer state for parsing HTTP as octets. data is not owned by the parser;
 * position is the current byte offset into data.
 */
struct parser {
    const uint8_t* data;
    size_t length;
    size_t position;
};

/*
 * Byte slice returned by parser read functions. It points into parser.data and
 * is not null-terminated.
 */
struct parser_buffer {
    const uint8_t* data;
    size_t length;
};

/*
 * Parsed header line. Name and value point into parser.data.
 */
struct parser_header {
    struct parser_buffer name;
    struct parser_buffer value;
};

/*
 * Initializes parser to read length octets from data.
 */
void parser_init(struct parser* parser, const uint8_t* data, size_t length);

/*
 * Returns the next octet without advancing. Returns -1 at end-of-buffer.
 */
int parser_peek(const struct parser* parser);

/*
 * Advances past HTTP optional whitespace: SP and HTAB.
 */
void parser_skip_whitespace(struct parser* parser);

/*
 * Reads one header line. Name is [line-start, ':'), value starts after ':' and
 * optional whitespace, and the parser advances past the terminating CRLF.
 */
int parser_read_header(struct parser* parser, struct parser_header* header);

#endif
