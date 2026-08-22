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

#include <errno.h>
#include <netdb.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "http/parser.h"
#include "net/server.h"

#define SERVER_BACKLOG 16
#define SERVER_RECEIVE_BUFFER_SIZE 4096
#define SERVER_RECEIVE_TIMEOUT_SECONDS 5
#define SERVER_REQUEST_TIMEOUT_SECONDS 15

static const char server_ok_response[] = "HTTP/1.1 200 OK\r\n"
                                         "Content-Type: text/plain\r\n"
                                         "Content-Length: 2\r\n"
                                         "Connection: close\r\n"
                                         "\r\n"
                                         "OK";

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
 * Sends every response byte unless the connection fails.
 */
static bool server_send_all(int socket_fd, const void* data, size_t length) {
    const uint8_t* bytes = data;

    while (length > 0) {
        ssize_t sent = send(socket_fd, bytes, length, 0);
        if (sent > 0) {
            bytes += (size_t)sent;
            length -= (size_t)sent;
            continue;
        }

        if (sent == -1 && errno == EINTR) {
            continue;
        }

        if (sent == 0) {
            errno = EPIPE;
        }

        return false;
    }

    return true;
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
static int server_receive_request(int client_fd, struct parser* parser) {
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

            enum parser_status status =
                parser_feed(parser, buffer, (size_t)bytes_received);

            if (status == PARSER_STATUS_COMPLETE) {
                return 0;
            }

            if (status == PARSER_STATUS_INVALID) {
                perror("parser_feed");
                return -1;
            }

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
static void server_handle_client(int client_fd) {
    struct parser parser;

    if (server_set_receive_timeout(client_fd) == -1) {
        perror("setsockopt");
        server_close_socket(client_fd);
        return;
    }

    if (parser_init(&parser) == -1) {
        perror("parser_init");
        server_close_socket(client_fd);
        return;
    }

    if (server_receive_request(client_fd, &parser) == 0) {
        if (!server_send_all(client_fd, server_ok_response,
                sizeof(server_ok_response) - 1)) {
            perror("send");
        }
    }

    server_close_socket(client_fd);
    parser_deinit(&parser);
}

/*
 * Opens the listening socket and handles clients one at a time.
 */
int server_listen(const char* service) {
    if (signal(SIGPIPE, SIG_IGN) == SIG_ERR) {
        perror("signal");
        return -1;
    }

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

        server_handle_client(client_fd);
    }
}
