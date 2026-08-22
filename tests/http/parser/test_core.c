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

#include "http/parser.h"

/*
 * Verifies parser initialization starts at the request line.
 */
static int test_parser_init(void) {
    struct parser parser = parser_init();

    assert(parser.state == PARSER_START_LINE);
    assert(parser.position == 0);
    assert(parser.scan_position == 0);
    assert(parser.header_length == 0);
    return 0;
}

/*
 * Verifies line reads consume normal and empty CRLF-delimited lines.
 */
static int test_parser_read_line(void) {
    const char* data = "Host: example.test\r\n\r\nbody";
    struct parser parser = parser_init();

    struct buffer line =
        parser_read_line(&parser, (const uint8_t*)data, strlen(data));
    assert(line.length == strlen("Host: example.test"));
    assert(memcmp(line.data, "Host: example.test", line.length) == 0);

    line = parser_read_line(&parser, (const uint8_t*)data, strlen(data));
    assert(line.data != NULL);
    assert(line.length == 0);
    assert(parser.position == strlen("Host: example.test\r\n\r\n"));
    return 0;
}

/*
 * Verifies an incomplete line leaves the cursor ready to resume.
 */
static int test_parser_read_line_incomplete(void) {
    const char* data = "Host: example.test\r\n";
    struct parser parser = parser_init();

    errno = 0;
    struct buffer line = parser_read_line(
        &parser, (const uint8_t*)data, strlen("Host: example.test\r"));
    assert(line.data == NULL);
    assert(errno == EAGAIN);
    assert(parser.position == 0);
    assert(parser.scan_position == strlen("Host: example.test"));

    line = parser_read_line(&parser, (const uint8_t*)data, strlen(data));
    assert(line.data != NULL);
    assert(line.length == strlen("Host: example.test"));
    return 0;
}

/*
 * Verifies scanned and consumed offsets survive receive-buffer compaction.
 */
static int test_parser_discard(void) {
    const char* data = "Host: example.test\r\nPartial";
    struct parser parser = parser_init();

    struct buffer line =
        parser_read_line(&parser, (const uint8_t*)data, strlen(data));
    assert(line.data != NULL);

    line = parser_read_line(&parser, (const uint8_t*)data, strlen(data));
    assert(line.data == NULL);
    assert(errno == EAGAIN);
    assert(parser.scan_position == strlen(data));

    size_t consumed = parser.position;
    parser_discard(&parser, consumed);
    assert(parser.position == 0);
    assert(parser.scan_position == strlen("Partial"));
    return 0;
}

/*
 * Verifies bare CR and LF line endings are rejected.
 */
static int test_parser_read_line_errors(void) {
    struct parser parser = parser_init();
    errno = 0;
    struct buffer line = parser_read_line(
        &parser, (const uint8_t*)"Host: value\n", strlen("Host: value\n"));
    assert(line.data == NULL);
    assert(errno == EINVAL);

    parser = parser_init();
    errno = 0;
    line = parser_read_line(
        &parser, (const uint8_t*)"Host: value\rX", strlen("Host: value\rX"));
    assert(line.data == NULL);
    assert(errno == EINVAL);
    return 0;
}

/*
 * Runs parser cursor tests.
 */
int main(void) {
    assert(test_parser_init() == 0);
    assert(test_parser_read_line() == 0);
    assert(test_parser_read_line_incomplete() == 0);
    assert(test_parser_discard() == 0);
    assert(test_parser_read_line_errors() == 0);
    return 0;
}
