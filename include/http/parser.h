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
#define REQUEST_MAX_HEADER_SECTION_LENGTH 32768
#define REQUEST_MAX_BODY_LENGTH 1048576

/* Current phase or terminal state of an incremental request parse. */
enum parser_state {
    PARSER_STATE_START_LINE,
    PARSER_STATE_HEADERS,
    PARSER_STATE_BODY,
    PARSER_STATE_COMPLETE,
    PARSER_STATE_INVALID,
    PARSER_STATE_ERROR,
};

/* Result of feeding one new chunk to the parser. */
enum parser_status {
    PARSER_STATUS_INCOMPLETE,
    PARSER_STATUS_COMPLETE,
    PARSER_STATUS_INVALID,
    PARSER_STATUS_ERROR,
};

/*
 * Owns one request and the temporary state required to parse it incrementally.
 * Only an unfinished line is retained from the wire representation.
 */
struct parser {
    struct request request;
    enum parser_state state;
    struct string partial_line;
    size_t header_section_length;
    size_t expected_body_length;
    bool awaiting_line_feed;
};

/* Initializes an incremental parser and its owned request. */
enum eunha_result parser_init(struct parser* parser);

/* Advances parsing using only newly received bytes. */
enum parser_status parser_feed(
    struct parser* parser, const uint8_t* data, size_t length);

/* Returns the completed borrowed request, or NULL before completion. */
const struct request* parser_get_request(const struct parser* parser);

/* Releases temporary parsing storage and the owned request. */
void parser_deinit(struct parser* parser);

#endif
