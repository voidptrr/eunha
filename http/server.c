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

/*
 * Exposes getaddrinfo and related POSIX networking declarations when compiling
 * in strict C mode.
 */
#define _POSIX_C_SOURCE 200112L

#include <assert.h>
#include <errno.h>
#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "datastruct/vector.h"
#include "http/parser.h"
#include "http/request.h"
#include "http/server.h"

#define SERVER_BACKLOG 16
#define SERVER_RECEIVE_BUFFER_SIZE 4096
#define SERVER_RECEIVE_TIMEOUT_SECONDS 5
#define SERVER_REQUEST_TIMEOUT_SECONDS 15

/*
 * Closes socket_fd and reports close failures without hiding the caller error.
 */
static void server_close_socket(int socket_fd) {
    if (close(socket_fd) == -1) {
        perror("close");
    }
}

/*
 * Limits how long recv can block without receiving another byte.
 */
static int server_set_receive_timeout(int socket_fd) {
    struct timeval timeout = {
        .tv_sec = SERVER_RECEIVE_TIMEOUT_SECONDS,
        .tv_usec = 0,
    };

    return setsockopt(
        socket_fd, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout));
}

/*
 * Rejects clients that keep a request open beyond the total deadline.
 */
static int server_check_request_deadline(time_t started) {
    time_t now = time(NULL);

    if (now == (time_t)-1) {
        return -1;
    }

    if (difftime(now, started) >= SERVER_REQUEST_TIMEOUT_SECONDS) {
        errno = ETIMEDOUT;
        return -1;
    }

    return 0;
}

/*
 * Removes bytes already copied into request-owned fields.
 */
static void server_discard_consumed_data(
    struct vector* data, struct parser* parser) {
    assert(data != NULL);
    assert(parser != NULL);
    assert(parser->position <= vector_len(data));

    size_t consumed = parser->position;
    if (consumed == 0) {
        return;
    }

    size_t remaining = vector_len(data) - consumed;
    memmove(data->data, (uint8_t*)data->data + consumed, remaining);
    data->length = remaining;
    parser_discard(parser, consumed);
}

/* Opens, binds, and starts listening on one resolved address. */
static int server_open_address(const struct addrinfo* address) {
    int enabled = 1;
    int socket_fd =
        socket(address->ai_family, address->ai_socktype, address->ai_protocol);

    if (socket_fd == -1) {
        return -1;
    }

    if (setsockopt(socket_fd, SOL_SOCKET, SO_REUSEADDR, &enabled,
            sizeof(enabled)) == -1) {
        server_close_socket(socket_fd);
        return -1;
    }

    if (bind(socket_fd, address->ai_addr, address->ai_addrlen) == -1) {
        server_close_socket(socket_fd);
        return -1;
    }

    if (listen(socket_fd, SERVER_BACKLOG) == -1) {
        server_close_socket(socket_fd);
        return -1;
    }

    return socket_fd;
}

/*
 * Resolves a numeric service such as "8080" and binds the first address that
 * works. AF_UNSPEC keeps the code ready for either IPv4 or IPv6.
 */
static int server_open_socket(const char* service) {
    struct addrinfo hints;
    struct addrinfo* addresses = NULL;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE | AI_NUMERICSERV;

    int result = getaddrinfo(NULL, service, &hints, &addresses);
    if (result != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(result));
        return -1;
    }

    for (struct addrinfo* address = addresses; address != NULL;
        address = address->ai_next) {
        int socket_fd = server_open_address(address);
        if (socket_fd != -1) {
            freeaddrinfo(addresses);
            return socket_fd;
        }
    }

    fprintf(stderr, "failed to listen on port %s\n", service);
    freeaddrinfo(addresses);
    return -1;
}

/*
 * Reads until HTTP framing says one complete request has arrived.
 */
static int server_receive_request(int client_fd, struct vector* data,
    struct parser* parser, struct request* request) {
    uint8_t buffer[SERVER_RECEIVE_BUFFER_SIZE];
    time_t started = time(NULL);

    if (started == (time_t)-1) {
        errno = EIO;
        perror("time");
        return -1;
    }

    for (;;) {
        if (server_check_request_deadline(started) == -1) {
            perror("request timeout");
            return -1;
        }

        ssize_t bytes_received = recv(client_fd, buffer, sizeof(buffer), 0);

        if (bytes_received > 0) {
            if (server_check_request_deadline(started) == -1) {
                perror("request timeout");
                return -1;
            }

            if (vector_extend(data, buffer, (size_t)bytes_received) == -1) {
                perror("vector_extend");
                return -1;
            }

            enum request_parse_status status =
                request_parse(parser, request, data->data, vector_len(data));

            if (status == REQUEST_COMPLETE) {
                server_discard_consumed_data(data, parser);
                return 0;
            }

            if (status == REQUEST_INVALID) {
                perror("request_parse");
                return -1;
            }

            server_discard_consumed_data(data, parser);
            continue;
        }

        if (bytes_received == 0) {
            fprintf(stderr, "connection closed before complete request\n");
            errno = EINVAL;
            return -1;
        }

        if (errno == EINTR) {
            continue;
        }

        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            errno = ETIMEDOUT;
        }

        perror("recv");
        return -1;
    }
}

/* Owns one accepted connection from read through close. */
static void server_handle_client(int client_fd, server_callback cb) {
    struct parser parser = parser_init();
    struct vector data;
    struct request parsed_request;

    if (server_set_receive_timeout(client_fd) == -1) {
        perror("setsockopt");
        server_close_socket(client_fd);
        return;
    }

    if (vector_init(&data, sizeof(uint8_t)) == -1) {
        perror("vector_init");
        server_close_socket(client_fd);
        return;
    }

    if (request_init(&parsed_request) == -1) {
        perror("request_init");
        vector_deinit(&data);
        server_close_socket(client_fd);
        return;
    }

    if (server_receive_request(client_fd, &data, &parser, &parsed_request) ==
        0) {
        cb(&parsed_request);
    }

    server_close_socket(client_fd);

    request_deinit(&parsed_request);
    vector_deinit(&data);
}

/*
 * Opens the listening socket and handles clients one at a time.
 */
int server_listen(const char* service, server_callback cb) {
    int socket_fd = server_open_socket(service);
    if (socket_fd == -1) {
        return -1;
    }

    printf("listening on port %s\n", service);

    for (;;) {
        int client_fd = accept(socket_fd, NULL, NULL);

        if (client_fd == -1) {
            if (errno == EINTR) {
                continue;
            }

            perror("accept");
            server_close_socket(socket_fd);
            return -1;
        }

        server_handle_client(client_fd, cb);
    }
}
