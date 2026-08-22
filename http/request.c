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

#include "http/request.h"

/* Initializes request-owned strings and headers. */
int request_init(struct request* request) {
    assert(request != NULL);

    if (string_init(&request->target) == -1) {
        return -1;
    }

    if (headers_init(&request->headers) == -1) {
        string_deinit(&request->target);
        return -1;
    }

    if (string_init(&request->body) == -1) {
        headers_deinit(&request->headers);
        string_deinit(&request->target);
        return -1;
    }

    request->method = UNKNOWN;
    request->version = HTTP_VERSION_UNKNOWN;
    return 0;
}

/* Releases all storage owned by a request. */
void request_deinit(struct request* request) {
    assert(request != NULL);

    string_deinit(&request->body);
    headers_deinit(&request->headers);
    string_deinit(&request->target);
    request->method = UNKNOWN;
    request->version = HTTP_VERSION_UNKNOWN;
}
