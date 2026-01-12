#ifndef SOCK_OPTIONS_H
#define SOCK_OPTIONS_H

#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>


/**
 * @brief Configure common socket options for a listening socket.
 *
 * Sets the following options on the given socket:
 *  - Disables IPV6_V6ONLY to allow both IPv4 and IPv6 connections
 *  - Enables SO_KEEPALIVE to detect dead peers
 *  - Enables SO_REUSEADDR to allow address reuse
 *
 * @param sockfd Socket file descriptor
 * @return 0 on success, -1 on error (one or more setsockopt calls failed)
 */

int set_socket_options(int sockfd);

int set_nonblocking(int fd);

#endif // SOCK_OPTIONS_H
