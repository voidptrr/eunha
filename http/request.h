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

#ifndef EUNHA_REQUEST_H
#define EUNHA_REQUEST_H

#include <stddef.h>
#include <stdint.h>

#include "datastruct/vector.h"

/*
 * HTTP methods recognized by the parser.
 */
enum request_method {
    UNKNOWN,
    GET,
    POST,
    PUT,
    PATCH,
    DELETE,
};

/*
 * Byte slice into request-owned storage. It is not null-terminated and must be
 * interpreted as octets during message parsing.
 */
struct request_buffer {
    const uint8_t* data;
    size_t length;
};

/*
 * One parsed HTTP header field. Name and value point into request.raw.
 */
struct request_header {
    struct request_buffer name;
    struct request_buffer value;
};

/*
 * Owns one HTTP request. raw stores the received octets; method, target,
 * version, and body point into raw after message parsing. headers stores
 * struct request_header items.
 */
struct request {
    struct vector raw;
    enum request_method method;
    struct request_buffer target;
    struct request_buffer version;
    struct vector headers;
    struct request_buffer body;
};

/*
 * Initializes request-owned storage.
 */
int request_init(struct request* request);

/*
 * Parses a received byte chunk into request state. For now this stores raw
 * bytes until HTTP framing is implemented.
 */
int request_parse(struct request* request, const uint8_t* data, size_t length);

/*
 * Releases request-owned storage.
 */
void request_deinit(struct request* request);

#endif
