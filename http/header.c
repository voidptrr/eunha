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
#include <errno.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "http/header.h"
#include "utils.h"

/* Returns whether every field-name byte belongs to the HTTP token grammar. */
static bool header_name_is_valid(struct buffer name) {
    for (size_t index = 0; index < name.length; index += 1) {
        uint8_t byte = name.data[index];

        if ((byte >= '0' && byte <= '9') || (byte >= 'A' && byte <= 'Z') ||
            (byte >= 'a' && byte <= 'z')) {
            continue;
        }

        /* These punctuation bytes complete HTTP's permitted token alphabet. */
        switch (byte) {
        case '!':
        case '#':
        case '$':
        case '%':
        case '&':
        case '\'':
        case '*':
        case '+':
        case '-':
        case '.':
        case '^':
        case '_':
        case '`':
        case '|':
        case '~':
            break;
        default:
            return false;
        }
    }

    return name.length > 0;
}

/* Rejects control bytes that are not permitted inside a field value. */
static bool header_value_is_valid(struct buffer value) {
    for (size_t index = 0; index < value.length; index += 1) {
        uint8_t byte = value.data[index];

        /* HTAB is allowed, but other C0 controls and DEL are not. */
        if ((byte < 0x20 && byte != '\t') || byte == 0x7F) {
            return false;
        }
    }

    return true;
}

/* Releases all storage owned by one field. */
static void header_deinit(struct header* header) {
    string_deinit(&header->value);
    string_deinit(&header->name);
}

/* Initializes an empty header collection. */
enum eunha_result headers_init(struct headers* headers) {
    assert(headers != NULL);

    return vector_init(&headers->fields, sizeof(struct header));
}

/* Parses and stores one owned name/value field. */
enum eunha_result headers_parse_line(
    struct headers* headers, const uint8_t* data, size_t length) {
    assert(headers != NULL);
    assert(data != NULL || length == 0);

    struct buffer_split field = split_once((struct buffer){data, length}, ':');
    /* A field needs a colon, a non-empty token name, and a safe raw value. */
    if (field.after.data == NULL || !header_name_is_valid(field.before) ||
        !header_value_is_valid(field.after)) {
        errno = EINVAL;
        return EUNHA_ERROR;
    }

    /* Optional whitespace is syntax around the value, not part of its data. */
    struct buffer value = trim_whitespace(field.after);
    struct header header = {0};

    if (string_init(&header.name) == EUNHA_ERROR) {
        return EUNHA_ERROR;
    }

    if (string_init(&header.value) == EUNHA_ERROR) {
        /* The name was initialized first and remains owned on this path. */
        string_deinit(&header.name);
        return EUNHA_ERROR;
    }

    if (string_set(&header.name, field.before.data, field.before.length) ==
            EUNHA_ERROR ||
        string_set(&header.value, value.data, value.length) == EUNHA_ERROR) {
        header_deinit(&header);
        return EUNHA_ERROR;
    }

    if (vector_append(&headers->fields, &header) == EUNHA_ERROR) {
        /* Ownership transfers to the vector only after the copy succeeds. */
        header_deinit(&header);
        return EUNHA_ERROR;
    }

    return EUNHA_OK;
}

/* Returns each stored header in wire order. */
const struct header* header_iterator_next(struct header_iterator* iterator) {
    assert(iterator != NULL);
    assert(iterator->headers != NULL);

    if (iterator->index >= vector_len(&iterator->headers->fields)) {
        return NULL;
    }

    const struct header* fields = iterator->headers->fields.data;
    const struct header* header = &fields[iterator->index];
    iterator->index += 1;
    return header;
}

/* Finds the first field with the requested case-insensitive name. */
const struct header* headers_get(
    const struct headers* headers, const char* name) {
    assert(headers != NULL);
    assert(name != NULL);

    struct header_iterator iterator = {
        .headers = headers,
        .index = 0,
    };
    const struct header* header = NULL;

    while ((header = header_iterator_next(&iterator)) != NULL) {
        struct buffer stored_name = {
            .data = header->name.data,
            .length = header->name.length,
        };

        if (buffer_equals_case_insensitive(stored_name, name)) {
            return header;
        }
    }

    return NULL;
}

/* Releases each nested field before releasing the vector. */
void headers_deinit(struct headers* headers) {
    assert(headers != NULL);

    for (size_t index = 0; index < vector_len(&headers->fields); index += 1) {
        struct header* header = vector_get(&headers->fields, index);
        header_deinit(header);
    }

    vector_deinit(&headers->fields);
}
