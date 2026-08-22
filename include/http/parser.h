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

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "datastruct/string.h"
#include "http/request.h"

#define REQUEST_MAX_START_LINE_LENGTH 8192
#define REQUEST_MAX_HEADER_LINE_LENGTH 8192
#define REQUEST_MAX_HEADERS_LENGTH 32768
#define REQUEST_MAX_BODY_LENGTH 1048576

/* Current section of an HTTP request being parsed. */
enum parser_state {
    PARSER_START_LINE,
    PARSER_HEADERS,
    PARSER_BODY,
    PARSER_COMPLETE,
    PARSER_INVALID,
};

/* Result of feeding one new chunk of request bytes to the parser. */
enum parser_status {
    PARSER_STATUS_INVALID,
    PARSER_STATUS_INCOMPLETE,
    PARSER_STATUS_COMPLETE,
};

/*
 * Stateful HTTP/1.x parser. It owns the request and only retains an unfinished
 * line between parser_feed calls.
 */
struct parser {
    enum parser_state state;
    struct request request;
    struct string line;
    size_t headers_length;
    size_t content_length;
    bool saw_carriage_return;
};

/* Initializes an HTTP parser and its request storage. */
int parser_init(struct parser* parser);

/*
 * Advances parsing with newly received bytes; previous chunks are not needed.
 */
enum parser_status parser_feed(
    struct parser* parser, const uint8_t* data, size_t length);

/* Releases the parser line and parsed request. */
void parser_deinit(struct parser* parser);

#endif
