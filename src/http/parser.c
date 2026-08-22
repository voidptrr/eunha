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

#define CR 0x0D
#define LF 0x0A
#define SP 0x20

/* Changes the parser into a sticky malformed-input state. */
static enum parser_status parser_set_invalid(struct parser* parser) {
    parser->state = PARSER_STATE_INVALID;
    return PARSER_STATUS_INVALID;
}

/* Changes the parser into a sticky allocation/runtime error state. */
static enum parser_status parser_set_error(struct parser* parser) {
    parser->state = PARSER_STATE_ERROR;
    return PARSER_STATUS_ERROR;
}

/* Converts a request mutation failure into the appropriate parse result. */
static enum parser_status parser_fail_from_errno(struct parser* parser) {
    if (errno == ENOMEM) {
        return parser_set_error(parser);
    }

    return parser_set_invalid(parser);
}

/* Maps one supported method string without allocating another lookup table. */
static enum request_method parser_parse_method(const struct string* method) {
    if (string_equals(method, "GET")) {
        return REQUEST_METHOD_GET;
    }

    if (string_equals(method, "POST")) {
        return REQUEST_METHOD_POST;
    }

    if (string_equals(method, "PUT")) {
        return REQUEST_METHOD_PUT;
    }

    if (string_equals(method, "PATCH")) {
        return REQUEST_METHOD_PATCH;
    }

    if (string_equals(method, "DELETE")) {
        return REQUEST_METHOD_DELETE;
    }

    return REQUEST_METHOD_UNKNOWN;
}

/* Maps one supported protocol version. */
static enum request_version parser_parse_version(const struct string* version) {
    if (string_equals(version, "HTTP/1.0")) {
        return HTTP_VERSION_1_0;
    }

    if (string_equals(version, "HTTP/1.1")) {
        return HTTP_VERSION_1_1;
    }

    return HTTP_VERSION_UNKNOWN;
}

/* Parses method SP request-target SP HTTP-version into the owned request. */
static enum parser_status parser_parse_start_line(
    struct parser* parser, const struct string* line) {
    struct string_split method_and_rest;
    if (string_split_once(line, SP, &method_and_rest) == EUNHA_ERROR) {
        return parser_set_error(parser);
    }

    /* The method must be non-empty and followed by one separator. */
    if (method_and_rest.length != 2 || method_and_rest.values[0].length == 0) {
        string_split_deinit(&method_and_rest);
        errno = EINVAL;
        return parser_set_invalid(parser);
    }

    struct string_split target_and_version;
    if (string_split_once(&method_and_rest.values[1], SP,
            &target_and_version) == EUNHA_ERROR) {
        string_split_deinit(&method_and_rest);
        return parser_set_error(parser);
    }

    /* The request target must also be present before the second separator. */
    if (target_and_version.length != 2 ||
        target_and_version.values[0].length == 0 ||
        target_and_version.values[1].length == 0 ||
        memchr(target_and_version.values[1].data, SP,
            target_and_version.values[1].length) != NULL) {
        string_split_deinit(&target_and_version);
        string_split_deinit(&method_and_rest);
        errno = EINVAL;
        return parser_set_invalid(parser);
    }

    /*
     * Spaces and control bytes are not valid inside this request-target form.
     */
    const struct string* target = &target_and_version.values[0];
    for (size_t index = 0; index < target->length; index += 1) {
        uint8_t byte = (uint8_t)target->data[index];
        if (byte < 0x21 || byte > 0x7E) {
            string_split_deinit(&target_and_version);
            string_split_deinit(&method_and_rest);
            errno = EINVAL;
            return parser_set_invalid(parser);
        }
    }

    enum request_method parsed_method =
        parser_parse_method(&method_and_rest.values[0]);
    enum request_version parsed_version =
        parser_parse_version(&target_and_version.values[1]);

    /* Only explicitly supported methods and HTTP versions pass. */
    if (parsed_method == REQUEST_METHOD_UNKNOWN ||
        parsed_version == HTTP_VERSION_UNKNOWN) {
        string_split_deinit(&target_and_version);
        string_split_deinit(&method_and_rest);
        errno = EINVAL;
        return parser_set_invalid(parser);
    }

    string_move(&parser->request.target, &target_and_version.values[0]);
    parser->request.method = parsed_method;
    parser->request.version = parsed_version;
    parser->state = PARSER_STATE_HEADERS;

    string_split_deinit(&target_and_version);
    string_split_deinit(&method_and_rest);
    return PARSER_STATUS_INCOMPLETE;
}

/* Appends one validated header line to the owned request. */
static enum parser_status parser_parse_header_line(
    struct parser* parser, const struct string* line) {
    if (headers_parse_line(&parser->request.headers, line) == EUNHA_ERROR) {
        return parser_fail_from_errno(parser);
    }

    return PARSER_STATUS_INCOMPLETE;
}

/* Validates framing and selects either body parsing or completion. */
static enum parser_status parser_finish_headers(struct parser* parser) {
    const struct string* host =
        headers_get(&parser->request.headers, HTTP_HEADER_HOST);
    size_t host_count =
        headers_value_count(&parser->request.headers, HTTP_HEADER_HOST);

    /* Host is a singleton field and an empty value is unusable. */
    if (host_count > 1 || (host != NULL && host->length == 0)) {
        errno = EINVAL;
        return parser_set_invalid(parser);
    }

    /* HTTP/1.1 requires Host even when the request has no body. */
    if (parser->request.version == HTTP_VERSION_1_1 && host == NULL) {
        errno = EINVAL;
        return parser_set_invalid(parser);
    }

    if (headers_get(&parser->request.headers, HTTP_HEADER_TRANSFER_ENCODING) !=
        NULL) {
        /* Chunked and other transfer codings are intentionally unsupported. */
        errno = EINVAL;
        return parser_set_invalid(parser);
    }

    const struct string* content_length =
        headers_get(&parser->request.headers, HTTP_HEADER_CONTENT_LENGTH);
    size_t content_length_count = headers_value_count(
        &parser->request.headers, HTTP_HEADER_CONTENT_LENGTH);

    /* Multiple lengths can disagree and make message framing unsafe. */
    if (content_length_count > 1) {
        errno = EINVAL;
        return parser_set_invalid(parser);
    }

    parser->expected_body_length = 0;
    if (content_length != NULL) {
        parser->expected_body_length = string_parse_size(content_length);
        if (parser->expected_body_length == SIZE_MAX) {
            return parser_set_invalid(parser);
        }
    }

    /* Bound allocation before accepting any body bytes. */
    if (parser->expected_body_length > REQUEST_MAX_BODY_LENGTH) {
        errno = EMSGSIZE;
        return parser_set_invalid(parser);
    }

    parser->state = parser->expected_body_length == 0 ? PARSER_STATE_COMPLETE
                                                      : PARSER_STATE_BODY;
    return parser->state == PARSER_STATE_COMPLETE ? PARSER_STATUS_COMPLETE
                                                  : PARSER_STATUS_INCOMPLETE;
}

/* Applies one complete request or header line. */
static enum parser_status parser_parse_line(struct parser* parser) {
    if (parser->state == PARSER_STATE_START_LINE) {
        return parser_parse_start_line(parser, &parser->partial_line);
    }

    assert(parser->state == PARSER_STATE_HEADERS);
    /* The section limit includes the CRLF terminating every header line. */
    parser->header_section_length += parser->partial_line.length + 2;

    if (parser->partial_line.length == 0) {
        return parser_finish_headers(parser);
    }

    return parser_parse_header_line(parser, &parser->partial_line);
}

/* Returns whether appending a run would exceed its current line limit. */
static bool parser_line_is_too_long(
    const struct parser* parser, size_t run_length) {
    size_t line_limit = parser->state == PARSER_STATE_START_LINE
                            ? REQUEST_MAX_START_LINE_LENGTH
                            : REQUEST_MAX_HEADER_LINE_LENGTH;

    /* Subtract first so the check cannot overflow on partial + incoming. */
    return parser->partial_line.length > line_limit ||
           run_length > line_limit - parser->partial_line.length;
}

/* Returns whether the current partial header would exceed the block limit. */
static bool parser_header_section_is_too_long(
    const struct parser* parser, size_t run_length) {
    if (parser->state != PARSER_STATE_HEADERS) {
        return false;
    }

    /* Reserve two bytes for this line's CRLF without unsigned underflow. */
    if (parser->header_section_length > REQUEST_MAX_HEADER_SECTION_LENGTH ||
        REQUEST_MAX_HEADER_SECTION_LENGTH - parser->header_section_length < 2) {
        return true;
    }

    size_t remaining =
        REQUEST_MAX_HEADER_SECTION_LENGTH - parser->header_section_length - 2;
    return parser->partial_line.length > remaining ||
           run_length > remaining - parser->partial_line.length;
}

/* Consumes body bytes and rejects any bytes beyond the declared framing. */
static enum parser_status parser_parse_body(
    struct parser* parser, const uint8_t* data, size_t length, size_t* offset) {
    size_t remaining =
        parser->expected_body_length - parser->request.body.length;
    size_t available = length - *offset;

    /* One connection carries one request, so extra framed bytes are invalid. */
    if (available > remaining) {
        errno = EINVAL;
        return parser_set_invalid(parser);
    }

    if (string_append(&parser->request.body, data + *offset, available) ==
        EUNHA_ERROR) {
        return parser_set_error(parser);
    }

    *offset = length;
    if (parser->request.body.length < parser->expected_body_length) {
        return PARSER_STATUS_INCOMPLETE;
    }

    parser->state = PARSER_STATE_COMPLETE;
    return PARSER_STATUS_COMPLETE;
}

/* Initializes the owned request before temporary parser storage. */
enum eunha_result parser_init(struct parser* parser) {
    assert(parser != NULL);

    if (request_init(&parser->request) == EUNHA_ERROR) {
        return EUNHA_ERROR;
    }

    if (string_init(&parser->partial_line, "") == EUNHA_ERROR) {
        request_deinit(&parser->request);
        return EUNHA_ERROR;
    }

    parser->state = PARSER_STATE_START_LINE;
    parser->header_section_length = 0;
    parser->expected_body_length = 0;
    parser->awaiting_line_feed = false;
    return EUNHA_OK;
}

/*
 * Consumes each new chunk once. Complete lines are applied immediately, while
 * an unfinished line remains parser-owned for the next call.
 */
enum parser_status parser_feed(
    struct parser* parser, const uint8_t* data, size_t length) {
    assert(parser != NULL);
    assert(data != NULL || length == 0);

    if (parser->state == PARSER_STATE_INVALID) {
        return PARSER_STATUS_INVALID;
    }

    if (parser->state == PARSER_STATE_ERROR) {
        return PARSER_STATUS_ERROR;
    }

    if (parser->state == PARSER_STATE_COMPLETE) {
        if (length == 0) {
            return PARSER_STATUS_COMPLETE;
        }

        /* Feeding more data after completion would amount to pipelining. */
        errno = EINVAL;
        return parser_set_invalid(parser);
    }

    size_t offset = 0;

    while (offset < length) {
        if (parser->state == PARSER_STATE_BODY) {
            return parser_parse_body(parser, data, length, &offset);
        }

        if (parser->awaiting_line_feed) {
            /* The preceding chunk ended after CR; HTTP lines require CRLF. */
            if (data[offset] != LF) {
                errno = EINVAL;
                return parser_set_invalid(parser);
            }

            parser->awaiting_line_feed = false;
            offset += 1;

            enum parser_status status = parser_parse_line(parser);
            string_clear(&parser->partial_line);

            if (status == PARSER_STATUS_INVALID ||
                status == PARSER_STATUS_ERROR) {
                return status;
            }

            if (status == PARSER_STATUS_COMPLETE) {
                /* Bytes after the terminating CRLF are unsupported pipelining.
                 */
                if (offset != length) {
                    errno = EINVAL;
                    return parser_set_invalid(parser);
                }

                return PARSER_STATUS_COMPLETE;
            }

            continue;
        }

        size_t run_start = offset;
        /* Copy a contiguous non-line-ending run in one allocation operation. */
        while (offset < length && data[offset] != CR && data[offset] != LF) {
            offset += 1;
        }

        size_t run_length = offset - run_start;
        if (parser_line_is_too_long(parser, run_length) ||
            parser_header_section_is_too_long(parser, run_length)) {
            errno = EMSGSIZE;
            return parser_set_invalid(parser);
        }

        if (string_append(&parser->partial_line, data + run_start,
                run_length) == EUNHA_ERROR) {
            return parser_set_error(parser);
        }

        if (offset == length) {
            return PARSER_STATUS_INCOMPLETE;
        }

        /* A bare LF is invalid; CR is retained as state until LF arrives. */
        if (data[offset] == LF) {
            errno = EINVAL;
            return parser_set_invalid(parser);
        }

        parser->awaiting_line_feed = true;
        offset += 1;
    }

    return PARSER_STATUS_INCOMPLETE;
}

/* Returns a borrowed request only after successful completion. */
const struct request* parser_get_request(const struct parser* parser) {
    assert(parser != NULL);

    if (parser->state != PARSER_STATE_COMPLETE) {
        return NULL;
    }

    return &parser->request;
}

/* Releases temporary parser storage, then the owned request. */
void parser_deinit(struct parser* parser) {
    assert(parser != NULL);

    string_deinit(&parser->partial_line);
    request_deinit(&parser->request);
    parser->state = PARSER_STATE_ERROR;
    parser->header_section_length = 0;
    parser->expected_body_length = 0;
    parser->awaiting_line_feed = false;
}
