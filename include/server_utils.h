#ifndef SERVER_UTILS_H
#define SERVER_UTILS_H

#include <stdint.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <sys/socket.h>
#include <unistd.h>
#include <syslog.h>
#include <ifaddrs.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include "tlv.h"

#define BUFFER_SIZE 4096
#define MAX_ACTIVE_NICKS 5000

/**
 * Handle LOGIN_REQUEST from client and send LOGIN_RESPONSE
 * @param connfd Client connection file descriptor
 * @param buffer Buffer containing parsed nickname
 * @param nick_out Output buffer to store the nickname (must be at least MAX_NICK_LENGTH)
 * @return 0 on success, -1 on error
 */
int server_handle_login(int connfd, const uint8_t *buffer, char *nick_out);

/**
 * Validate nickname (length and allowed characters)
 * @param nick Nickname to validate
 * @return 0 if valid, -1 if invalid
 */
int server_validate_nick(const char *nick);

/**
 * Check if nickname is already taken by another client
 * @param nick Nickname to check
 * @return 1 if taken, 0 if available
 */
int server_is_nick_taken(const char *nick);

/**
 * Get the local IP address of the server (non-loopback interface)
 * 
 * Scans network interfaces and returns the first non-loopback IPv4 address found.
 * Uses getifaddrs() to enumerate interfaces and skips the loopback interface.
 * 
 * @return Pointer to static buffer containing IP address string (e.g., "192.168.1.5"),
 *         or NULL if no suitable address found or error occurred
 * 
 * @note The returned pointer points to static storage and should not be freed.
 *       The value may change on subsequent calls to this function.
 */
char* get_local_ip();

#endif // SERVER_UTILS_H
