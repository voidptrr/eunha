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

#include "datastruct/string.h"
#include "http/header.h"

/* HTTP methods currently recognized by the parser. */
enum request_method {
    REQUEST_METHOD_UNKNOWN,
    REQUEST_METHOD_GET,
    REQUEST_METHOD_POST,
    REQUEST_METHOD_PUT,
    REQUEST_METHOD_PATCH,
    REQUEST_METHOD_DELETE,
};

/* HTTP versions currently accepted by the parser. */
enum request_version {
    HTTP_VERSION_UNKNOWN,
    HTTP_VERSION_1_0,
    HTTP_VERSION_1_1,
};

/* Parsed HTTP data whose allocation-backed fields are owned by the request. */
struct request {
    enum request_method method;
    struct string target;
    enum request_version version;
    struct headers headers;
    struct string body;
};

/* Initializes storage owned by a request. */
enum eunha_result request_init(struct request* request);

/* Releases all request-owned storage. */
void request_deinit(struct request* request);

#endif
