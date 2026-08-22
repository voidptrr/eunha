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

#include "eunha.h"
#include "http/request.h"

/* Verifies request storage contains only application-facing HTTP data. */
static int test_request_lifecycle(void) {
    struct request request;

    assert(request_init(&request) == EUNHA_OK);
    assert(request.method == REQUEST_METHOD_UNKNOWN);
    assert(request.version == HTTP_VERSION_UNKNOWN);
    assert(request.target.length == 0);
    assert(request.body.length == 0);
    assert(request.headers.entries != NULL);
    assert(request.headers.length == 0);

    request_deinit(&request);
    assert(request.target.data == NULL);
    assert(request.body.data == NULL);
    assert(request.headers.entries == NULL);
    return 0;
}

/* Runs request data ownership tests. */
int main(void) {
    assert(test_request_lifecycle() == 0);
    return 0;
}
