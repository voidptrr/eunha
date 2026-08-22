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
#include <string.h>

#include "http/header.h"
#include "http/parser.h"
#include "http/request.h"
#include "utils.h"

#define CR 0x0D
#define LF 0x0A
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
    {.name = "GET", .method = GET},
    {.name = "POST", .method = POST},
    {.name = "PUT", .method = PUT},
    {.name = "PATCH", .method = PATCH},
    {.name = "DELETE", .method = DELETE},
};

static const struct version_entry version_entries[] = {
    {.name = "HTTP/1.0", .version = HTTP_1_0},
    {.name = "HTTP/1.1", .version = HTTP_1_1},
};

/* Parses method SP request-target SP HTTP-version into request fields. */
static int parser_parse_start_line(
    struct request* request, struct buffer line) {
    const uint8_t* end = line.data + line.length;
    const uint8_t* first_space = memchr(line.data, SP, line.length);

    if (first_space == NULL || first_space == line.data) {
        errno = EINVAL;
        return -1;
    }

    const uint8_t* target = first_space + 1;
    const uint8_t* second_space = memchr(target, SP, (size_t)(end - target));

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

    const uint8_t* version_start = second_space + 1;
    if (version_start == end ||
        memchr(version_start, SP, (size_t)(end - version_start)) != NULL) {
        errno = EINVAL;
        return -1;
    }

    struct buffer method = {
        .data = line.data,
        .length = (size_t)(first_space - line.data),
    };
    struct buffer version = {
        .data = version_start,
        .length = (size_t)(end - version_start),
    };

    enum request_method parsed_method = UNKNOWN;
    for (size_t index = 0;
        index < sizeof(method_entries) / sizeof(method_entries[0]);
        index += 1) {
        if (buffer_equals(method, method_entries[index].name)) {
            parsed_method = method_entries[index].method;
            break;
        }
    }

    enum request_version parsed_version = HTTP_VERSION_UNKNOWN;
    for (size_t index = 0;
        index < sizeof(version_entries) / sizeof(version_entries[0]);
        index += 1) {
        if (buffer_equals(version, version_entries[index].name)) {
            parsed_version = version_entries[index].version;
            break;
        }
    }

    if (parsed_method == UNKNOWN || parsed_version == HTTP_VERSION_UNKNOWN) {
        errno = EINVAL;
        return -1;
    }

    if (string_set(&request->target, target, (size_t)(second_space - target)) ==
        -1) {
        return -1;
    }

    request->method = parsed_method;
    request->version = parsed_version;
    return 0;
}

/* Validates headers that determine HTTP message framing. */
static int parser_finish_headers(struct parser* parser) {
    struct header_iterator iterator = {
        .headers = &parser->request.headers,
        .index = 0,
    };
    const struct header* header = NULL;
    bool has_host = false;
    bool has_content_length = false;
    size_t content_length = 0;

    while ((header = header_next(&iterator)) != NULL) {
        struct buffer name = {
            .data = header->name.data,
            .length = header->name.length,
        };
        struct buffer value = {
            .data = header->value.data,
            .length = header->value.length,
        };

        if (buffer_equals(name, "host")) {
            if (has_host || value.length == 0) {
                errno = EINVAL;
                return -1;
            }

            has_host = true;
            continue;
        }

        if (buffer_equals(name, "content-length")) {
            if (has_content_length) {
                errno = EINVAL;
                return -1;
            }

            content_length = buffer_to_digit(value);
            if (content_length == SIZE_MAX) {
                return -1;
            }

            has_content_length = true;
            continue;
        }

        if (buffer_equals(name, "transfer-encoding")) {
            errno = EINVAL;
            return -1;
        }
    }

    if (parser->request.version == HTTP_1_1 && !has_host) {
        errno = EINVAL;
        return -1;
    }

    if (content_length > REQUEST_MAX_BODY_LENGTH) {
        errno = EMSGSIZE;
        return -1;
    }

    parser->content_length = content_length;
    parser->state = content_length == 0 ? PARSER_COMPLETE : PARSER_BODY;
    return 0;
}

/* Applies one complete line according to the parser's current section. */
static int parser_process_line(struct parser* parser) {
    struct buffer line = {
        .data = parser->line.data,
        .length = parser->line.length,
    };

    if (parser->state == PARSER_START_LINE) {
        if (parser_parse_start_line(&parser->request, line) == -1) {
            return -1;
        }

        parser->state = PARSER_HEADERS;
        return 0;
    }

    assert(parser->state == PARSER_HEADERS);
    parser->headers_length += line.length + 2;

    if (line.length == 0) {
        return parser_finish_headers(parser);
    }

    return headers_append(&parser->request.headers, line.data, line.length);
}

/* Initializes an HTTP parser and the request it owns. */
int parser_init(struct parser* parser) {
    assert(parser != NULL);

    if (request_init(&parser->request) == -1) {
        return -1;
    }

    if (string_init(&parser->line) == -1) {
        request_deinit(&parser->request);
        return -1;
    }

    parser->state = PARSER_START_LINE;
    parser->headers_length = 0;
    parser->content_length = 0;
    parser->saw_carriage_return = false;
    return 0;
}

/*
 * Consumes each new chunk once. Complete lines are applied immediately, while
 * an unfinished line remains in parser->line for the next call.
 */
enum parser_status parser_feed(
    struct parser* parser, const uint8_t* data, size_t length) {
    assert(parser != NULL);
    assert(data != NULL || length == 0);

    if (parser->state == PARSER_COMPLETE) {
        return PARSER_STATUS_COMPLETE;
    }

    if (parser->state == PARSER_INVALID) {
        return PARSER_STATUS_INVALID;
    }

    size_t position = 0;

    while (position < length) {
        if (parser->state == PARSER_BODY) {
            size_t remaining =
                parser->content_length - parser->request.body.length;
            size_t available = length - position;
            size_t consumed = available < remaining ? available : remaining;

            if (string_append(
                    &parser->request.body, data + position, consumed) == -1) {
                goto invalid;
            }

            if (parser->request.body.length < parser->content_length) {
                return PARSER_STATUS_INCOMPLETE;
            }

            parser->state = PARSER_COMPLETE;
            return PARSER_STATUS_COMPLETE;
        }

        if (parser->saw_carriage_return) {
            if (data[position] != LF) {
                errno = EINVAL;
                goto invalid;
            }

            parser->saw_carriage_return = false;
            position += 1;

            if (parser_process_line(parser) == -1) {
                goto invalid;
            }

            string_clear(&parser->line);
            if (parser->state == PARSER_COMPLETE) {
                return PARSER_STATUS_COMPLETE;
            }

            continue;
        }

        size_t start = position;
        while (
            position < length && data[position] != CR && data[position] != LF) {
            position += 1;
        }

        size_t run_length = position - start;
        size_t line_limit = parser->state == PARSER_START_LINE
                                ? REQUEST_MAX_START_LINE_LENGTH
                                : REQUEST_MAX_HEADER_LINE_LENGTH;

        if (parser->line.length > line_limit ||
            run_length > line_limit - parser->line.length) {
            errno = EMSGSIZE;
            goto invalid;
        }

        size_t next_line_length = parser->line.length + run_length;
        if (parser->state == PARSER_HEADERS) {
            if (parser->headers_length >
                REQUEST_MAX_HEADERS_LENGTH - next_line_length - 2) {
                errno = EMSGSIZE;
                goto invalid;
            }
        }

        if (string_append(&parser->line, data + start, run_length) == -1) {
            goto invalid;
        }

        if (position == length) {
            return PARSER_STATUS_INCOMPLETE;
        }

        if (data[position] == LF) {
            errno = EINVAL;
            goto invalid;
        }

        parser->saw_carriage_return = true;
        position += 1;
    }

    return PARSER_STATUS_INCOMPLETE;

invalid:
    parser->state = PARSER_INVALID;
    return PARSER_STATUS_INVALID;
}

/* Releases the partial line and parsed request. */
void parser_deinit(struct parser* parser) {
    assert(parser != NULL);

    string_deinit(&parser->line);
    request_deinit(&parser->request);
    parser->state = PARSER_INVALID;
    parser->headers_length = 0;
    parser->content_length = 0;
    parser->saw_carriage_return = false;
}
