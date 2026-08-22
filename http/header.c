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

/*
 * Header fields retained by the request representation.
 */
enum header_type {
    HEADER_CUSTOM,
    HEADER_AUTHORIZATION,
    HEADER_CONTENT_LENGTH,
    HEADER_CONTENT_TYPE,
    HEADER_HOST,
    HEADER_TRANSFER_ENCODING,
};

struct header_entry {
    const char* name;
    enum header_type type;
};

struct content_type_entry {
    const char* name;
    enum content_type type;
};

static const struct header_entry header_entries[] = {
    {
        .name = "authorization",
        .type = HEADER_AUTHORIZATION,
    },
    {
        .name = "content-length",
        .type = HEADER_CONTENT_LENGTH,
    },
    {
        .name = "content-type",
        .type = HEADER_CONTENT_TYPE,
    },
    {
        .name = "host",
        .type = HEADER_HOST,
    },
    {
        .name = "transfer-encoding",
        .type = HEADER_TRANSFER_ENCODING,
    },
};

static const struct content_type_entry content_type_entries[] = {
    {
        .name = "application/x-www-form-urlencoded",
        .type = CONTENT_TYPE_APPLICATION_FORM_URLENCODED,
    },
    {
        .name = "application/json",
        .type = CONTENT_TYPE_APPLICATION_JSON,
    },
    {
        .name = "text/plain",
        .type = CONTENT_TYPE_TEXT_PLAIN,
    },
};

/*
 * Returns whether every field-name byte belongs to the HTTP token grammar.
 */
static bool header_name_is_valid(struct buffer name) {
    for (size_t index = 0; index < name.length; index += 1) {
        uint8_t byte = name.data[index];

        if ((byte >= '0' && byte <= '9') || (byte >= 'A' && byte <= 'Z') ||
            (byte >= 'a' && byte <= 'z')) {
            continue;
        }

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

/*
 * Rejects control bytes that are not permitted inside a field value.
 */
static bool header_value_is_valid(struct buffer value) {
    for (size_t index = 0; index < value.length; index += 1) {
        uint8_t byte = value.data[index];

        if ((byte < 0x20 && byte != '\t') || byte == 0x7F) {
            return false;
        }
    }

    return true;
}

/*
 * Initializes header-owned storage.
 */
int headers_init(struct headers* headers) {
    assert(headers != NULL);

    if (string_init(&headers->host) == -1) {
        return -1;
    }

    if (string_init(&headers->authorization) == -1) {
        string_deinit(&headers->host);
        return -1;
    }

    headers_clear(headers);
    return 0;
}

/*
 * Clears parsed header values while keeping allocations reusable.
 */
void headers_clear(struct headers* headers) {
    assert(headers != NULL);

    string_clear(&headers->host);
    string_clear(&headers->authorization);
    headers->content_type = CONTENT_TYPE_NONE;
    headers->content_length = 0;
    headers->has_host = false;
    headers->has_authorization = false;
    headers->has_content_type = false;
    headers->has_content_length = false;
}

/*
 * Parses one header line and stores fields retained by the request.
 */
int headers_parse_line(
    struct headers* headers, const uint8_t* data, size_t length) {
    assert(headers != NULL);
    assert(data != NULL || length == 0);

    struct buffer line = {
        .data = data,
        .length = length,
    };
    struct buffer_split field = split_once(line, ':');

    if (field.after.data == NULL || !header_name_is_valid(field.before) ||
        !header_value_is_valid(field.after)) {
        errno = EINVAL;
        return -1;
    }

    struct buffer name = field.before;
    struct buffer value = trim_whitespace(field.after);

    enum header_type type = HEADER_CUSTOM;
    for (size_t index = 0;
        index < sizeof(header_entries) / sizeof(header_entries[0]);
        index += 1) {
        if (buffer_equals_case_insensitive(name, header_entries[index].name)) {
            type = header_entries[index].type;
            break;
        }
    }

    switch (type) {
    case HEADER_HOST:
        if (headers->has_host || value.length == 0) {
            errno = EINVAL;
            return -1;
        }

        if (string_set(&headers->host, value.data, value.length) == -1) {
            return -1;
        }

        headers->has_host = true;
        return 0;
    case HEADER_AUTHORIZATION:
        if (headers->has_authorization) {
            errno = EINVAL;
            return -1;
        }

        if (string_set(&headers->authorization, value.data, value.length) ==
            -1) {
            return -1;
        }

        headers->has_authorization = true;
        return 0;
    case HEADER_CONTENT_LENGTH: {
        if (headers->has_content_length) {
            errno = EINVAL;
            return -1;
        }

        size_t content_length = buffer_to_digit(value);
        if (content_length == SIZE_MAX) {
            return -1;
        }

        headers->content_length = content_length;
        headers->has_content_length = true;
        return 0;
    }
    case HEADER_CONTENT_TYPE: {
        if (headers->has_content_type) {
            errno = EINVAL;
            return -1;
        }

        struct buffer_split content_type = split_once(value, ';');
        struct buffer media_type = trim_whitespace(content_type.before);

        headers->content_type = CONTENT_TYPE_CUSTOM;
        for (size_t index = 0; index < sizeof(content_type_entries) /
                                           sizeof(content_type_entries[0]);
            index += 1) {
            if (buffer_equals_case_insensitive(
                    media_type, content_type_entries[index].name)) {
                headers->content_type = content_type_entries[index].type;
                break;
            }
        }

        headers->has_content_type = true;
        return 0;
    }
    case HEADER_TRANSFER_ENCODING:
        errno = EINVAL;
        return -1;
    case HEADER_CUSTOM:
        return 0;
    }

    return 0;
}

/*
 * Releases header-owned storage.
 */
void headers_deinit(struct headers* headers) {
    assert(headers != NULL);

    string_deinit(&headers->authorization);
    string_deinit(&headers->host);
    headers->content_type = CONTENT_TYPE_NONE;
    headers->content_length = 0;
    headers->has_host = false;
    headers->has_authorization = false;
    headers->has_content_type = false;
    headers->has_content_length = false;
}
