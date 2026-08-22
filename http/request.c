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
enum eunha_result request_init(struct request* request) {
    assert(request != NULL);

    if (string_init(&request->target) == EUNHA_ERROR) {
        return EUNHA_ERROR;
    }

    if (headers_init(&request->headers) == EUNHA_ERROR) {
        string_deinit(&request->target);
        return EUNHA_ERROR;
    }

    if (string_init(&request->body) == EUNHA_ERROR) {
        headers_deinit(&request->headers);
        string_deinit(&request->target);
        return EUNHA_ERROR;
    }

    request->method = REQUEST_METHOD_UNKNOWN;
    request->version = HTTP_VERSION_UNKNOWN;
    return EUNHA_OK;
}

/* Releases request fields in reverse ownership order. */
void request_deinit(struct request* request) {
    assert(request != NULL);

    string_deinit(&request->body);
    headers_deinit(&request->headers);
    string_deinit(&request->target);
    request->method = REQUEST_METHOD_UNKNOWN;
    request->version = HTTP_VERSION_UNKNOWN;
}
