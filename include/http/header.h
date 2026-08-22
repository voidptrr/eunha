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

#include <stddef.h>

#include "datastruct/string.h"

#define HTTP_HEADER_AUTHORIZATION "Authorization"
#define HTTP_HEADER_CONTENT_LENGTH "Content-Length"
#define HTTP_HEADER_CONTENT_TYPE "Content-Type"
#define HTTP_HEADER_HOST "Host"
#define HTTP_HEADER_TRANSFER_ENCODING "Transfer-Encoding"

struct header_entry;

/*
 * Case-insensitive header table. Each entry groups values for one field name
 * in arrival order.
 */
struct headers {
    struct header_entry* entries;
    size_t length;
    size_t capacity;
};

/* Initializes an empty header collection. */
enum eunha_result headers_init(struct headers* headers);

/* Parses and stores one header line without its trailing CRLF. */
enum eunha_result headers_parse_line(
    struct headers* headers, const struct string* line);

/* Returns the first value matching name with ASCII case ignored. */
const struct string* headers_get(
    const struct headers* headers, const char* name);

/* Returns how many values are stored for a case-insensitive name. */
size_t headers_value_count(const struct headers* headers, const char* name);

/* Returns one value by arrival index, or NULL when absent or out of bounds. */
const struct string* headers_get_at(
    const struct headers* headers, const char* name, size_t index);

/* Releases every header and the collection storage. */
void headers_deinit(struct headers* headers);

#endif
