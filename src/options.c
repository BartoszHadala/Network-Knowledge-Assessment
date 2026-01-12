#include "sock_options.h"

void set_socket_options(int listenfd) {
    // Disable IPV6_V6ONLY to allow IPv4 connections on IPv6 socket
    // This enables dual-stack operation: both IPv4 and IPv6 clients can connect
    // IPv4 addresses will be mapped to IPv6 format (::ffff:x.x.x.x)
    int no = 0;
    int yes = 1;

    if ( setsockopt(listenfd, IPPROTO_IPV6, IPV6_V6ONLY, &no, sizeof(no)) < 0 ) {
        fprintf(stderr, "setsockopt IPV6_V6ONLY error: %s\n", strerror(errno));
    }

    if ( setsockopt(listenfd, SOL_SOCKET, SO_KEEPALIVE, &yes, sizeof(yes)) < 0 ) {
        fprintf(stderr, "setsockopt SO_KEEPALIVE error: %s\n", strerror(errno));
    }

    if ( setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0 ) {
        fprintf(stderr, "setsockopt SO_REUSEADDR error: %s\n", strerror(errno));
    }    
}