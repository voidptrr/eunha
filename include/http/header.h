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

#ifndef EUNHA_HEADER_H
#define EUNHA_HEADER_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "datastruct/string.h"

/*
 * Content-Type media types this server handles directly.
 */
enum content_type {
    CONTENT_TYPE_NONE,
    CONTENT_TYPE_CUSTOM,
    CONTENT_TYPE_APPLICATION_FORM_URLENCODED,
    CONTENT_TYPE_APPLICATION_JSON,
    CONTENT_TYPE_TEXT_PLAIN,
};

/*
 * Known request headers promoted into direct fields.
 */
struct headers {
    struct string host;
    struct string authorization;
    enum content_type content_type;
    size_t content_length;
    bool has_host;
    bool has_authorization;
    bool has_content_type;
    bool has_content_length;
};

/*
 * Initializes storage owned by the header collection.
 */
int headers_init(struct headers* headers);

/*
 * Clears parsed header values while keeping allocations reusable.
 */
void headers_clear(struct headers* headers);

/*
 * Parses one header line without its trailing CRLF.
 */
int headers_parse_line(
    struct headers* headers, const uint8_t* data, size_t length);

/*
 * Releases header-owned storage.
 */
void headers_deinit(struct headers* headers);

#endif
