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
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "http/header.h"
#include "http/parser.h"
#include "http/request.h"
#include "utils.h"

#define SP 0x20

struct method_entry {
    const char* name;
    enum request_method method;
};

struct version_entry {
    const char* name;
    enum request_version version;
};

static const struct method_entry method_entries[] = {
    {
        .name = "GET",
        .method = GET,
    },
    {
        .name = "POST",
        .method = POST,
    },
    {
        .name = "PUT",
        .method = PUT,
    },
    {
        .name = "PATCH",
        .method = PATCH,
    },
    {
        .name = "DELETE",
        .method = DELETE,
    },
};

static const struct version_entry version_entries[] = {
    {
        .name = "HTTP/1.0",
        .version = HTTP_1_0,
    },
    {
        .name = "HTTP/1.1",
        .version = HTTP_1_1,
    },
};

/*
 * Parses method SP request-target SP HTTP-version into request fields.
 */
static int request_parse_start_line(
    struct request* request, struct buffer line) {
    assert(request != NULL);
    assert(line.data != NULL);

    const uint8_t* line_end = line.data + line.length;
    const uint8_t* first_space = memchr(line.data, SP, line.length);

    if (first_space == NULL || first_space == line.data) {
        errno = EINVAL;
        return -1;
    }

    const uint8_t* target = first_space + 1;
    const uint8_t* second_space =
        memchr(target, SP, (size_t)(line_end - target));

    if (second_space == NULL || second_space == target) {
        errno = EINVAL;
        return -1;
    }

    for (const uint8_t* byte = target; byte < second_space; byte += 1) {
        if (*byte < 0x21 || *byte > 0x7E) {
            errno = EINVAL;
            return -1;
        }
    }

    const uint8_t* version = second_space + 1;
    if (version == line_end ||
        memchr(version, SP, (size_t)(line_end - version)) != NULL) {
        errno = EINVAL;
        return -1;
    }

    enum request_method method = UNKNOWN;
    struct buffer method_buffer = {
        .data = line.data,
        .length = (size_t)(first_space - line.data),
    };
    for (size_t index = 0;
        index < sizeof(method_entries) / sizeof(method_entries[0]);
        index += 1) {
        if (buffer_equals(method_buffer, method_entries[index].name)) {
            method = method_entries[index].method;
            break;
        }
    }

    enum request_version request_version = HTTP_VERSION_UNKNOWN;
    struct buffer version_buffer = {
        .data = version,
        .length = (size_t)(line_end - version),
    };
    for (size_t index = 0;
        index < sizeof(version_entries) / sizeof(version_entries[0]);
        index += 1) {
        if (buffer_equals(version_buffer, version_entries[index].name)) {
            request_version = version_entries[index].version;
            break;
        }
    }

    if (method == UNKNOWN || request_version == HTTP_VERSION_UNKNOWN) {
        errno = EINVAL;
        return -1;
    }

    if (string_set(&request->target, target, (size_t)(second_space - target)) ==
        -1) {
        return -1;
    }

    request->method = method;
    request->version = request_version;
    return 0;
}

/*
 * Initializes request-owned storage.
 */
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

/*
 * Advances the request state machine over all bytes currently available.
 */
enum request_parse_status request_parse(struct parser* parser,
    struct request* request, const uint8_t* data, size_t length) {
    assert(parser != NULL);
    assert(request != NULL);
    assert(data != NULL || length == 0);
    assert(parser->position <= length);

    for (;;) {
        switch (parser->state) {
        case PARSER_START_LINE: {
            struct buffer line = parser_read_line(parser, data, length);
            if (line.data == NULL) {
                if (errno == EAGAIN) {
                    if (length - parser->position >
                        REQUEST_MAX_START_LINE_LENGTH) {
                        errno = EMSGSIZE;
                        goto invalid;
                    }

                    return REQUEST_INCOMPLETE;
                }

                goto invalid;
            }

            if (line.length > REQUEST_MAX_START_LINE_LENGTH) {
                errno = EMSGSIZE;
                goto invalid;
            }

            if (request_parse_start_line(request, line) == -1) {
                goto invalid;
            }

            parser->state = PARSER_HEADERS;
            break;
        }
        case PARSER_HEADERS: {
            struct buffer line = parser_read_line(parser, data, length);
            if (line.data == NULL) {
                if (errno == EAGAIN) {
                    size_t partial_length = length - parser->position;

                    if (partial_length > REQUEST_MAX_HEADER_LINE_LENGTH ||
                        partial_length > REQUEST_MAX_HEADERS_LENGTH -
                                             parser->header_length) {
                        errno = EMSGSIZE;
                        goto invalid;
                    }

                    return REQUEST_INCOMPLETE;
                }

                goto invalid;
            }

            if (line.length > REQUEST_MAX_HEADER_LINE_LENGTH ||
                line.length + 2 >
                    REQUEST_MAX_HEADERS_LENGTH - parser->header_length) {
                errno = EMSGSIZE;
                goto invalid;
            }

            parser->header_length += line.length + 2;

            if (line.length == 0) {
                if (request->version == HTTP_1_1 &&
                    !request->headers.has_host) {
                    errno = EINVAL;
                    goto invalid;
                }

                if (request->headers.content_length > REQUEST_MAX_BODY_LENGTH) {
                    errno = EMSGSIZE;
                    goto invalid;
                }

                parser->state = PARSER_BODY;
                break;
            }

            if (headers_parse_line(&request->headers, line.data, line.length) ==
                -1) {
                goto invalid;
            }

            break;
        }
        case PARSER_BODY: {
            assert(request->body.length <= request->headers.content_length);

            size_t body_remaining =
                request->headers.content_length - request->body.length;
            size_t available = length - parser->position;
            size_t consumed =
                available < body_remaining ? available : body_remaining;

            if (string_append(
                    &request->body, data + parser->position, consumed) == -1) {
                goto invalid;
            }

            parser->position += consumed;
            parser->scan_position = parser->position;

            if (request->body.length < request->headers.content_length) {
                return REQUEST_INCOMPLETE;
            }

            parser->state = PARSER_COMPLETE;
            return REQUEST_COMPLETE;
        }
        case PARSER_COMPLETE:
            return REQUEST_COMPLETE;
        case PARSER_INVALID:
            return REQUEST_INVALID;
        }
    }

invalid:
    parser->state = PARSER_INVALID;
    return REQUEST_INVALID;
}

/*
 * Releases request-owned storage.
 */
void request_deinit(struct request* request) {
    assert(request != NULL);

    string_deinit(&request->body);
    headers_deinit(&request->headers);
    string_deinit(&request->target);
    request->method = UNKNOWN;
    request->version = HTTP_VERSION_UNKNOWN;
}
