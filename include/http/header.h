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
#include <stdint.h>

#include "datastruct/string.h"
#include "datastruct/vector.h"

/*
 * One owned HTTP field. Names retain their original spelling and values have
 * surrounding optional whitespace removed.
 */
struct header {
    struct string name;
    struct string value;
};

/*
 * Ordered collection of every header received in a request.
 */
struct headers {
    struct vector fields;
};

/*
 * Cursor used to visit headers in wire order.
 */
struct header_iterator {
    const struct headers* headers;
    size_t index;
};

/* Initializes an empty header collection. */
enum eunha_result headers_init(struct headers* headers);

/* Parses and stores one header line without its trailing CRLF. */
enum eunha_result headers_parse_line(
    struct headers* headers, const uint8_t* data, size_t length);

/* Returns the next header, or NULL after the final header. */
const struct header* header_iterator_next(struct header_iterator* iterator);

/* Returns the first header matching name with ASCII case ignored. */
const struct header* headers_get(
    const struct headers* headers, const char* name);

/* Releases every header and the collection storage. */
void headers_deinit(struct headers* headers);

#endif
