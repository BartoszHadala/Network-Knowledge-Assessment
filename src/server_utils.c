#include "server_utils.h"

// active_nicks[0]  → "Alice\0"    (33 byte)
// active_nicks[0][0] = 'A'
static char active_nicks[MAX_ACTIVE_NICKS][MAX_NICK_LENGTH + 1];
static int active_count = 0;

// Validate nickname (length and characters)
int server_validate_nick(const char *nick) {
    size_t len = strlen(nick);
    
    // Check length
    if ( len == 0 || len > MAX_NICK_LENGTH ) {
        return -1;
    }
    
    // Check allowed characters: a-z, A-Z, 0-9, _, -
    for (size_t i = 0; i < len; i++) {
        char c = nick[i];
        if ( !isalnum(c) && c != '_' && c != '-' ) {
            return -1;
        }
    }
    
    return 0;
}

// Check if nickname is already taken
int server_is_nick_taken(const char *nick) {
    for (int i = 0; i < active_count; i++) {
        if ( strcmp(active_nicks[i], nick) == 0 ) {
            return 1;  // Taken
        }
    }
    return 0;  // Available
}

// Handle LOGIN_REQUEST from client
int server_handle_login(int connfd, const uint8_t *buffer, char *nick_out) {
    uint8_t response_buffer[BUFFER_SIZE];
    char nick[64];
    
    // Parse login request
    if ( tlv_parse_login_request(buffer, nick, sizeof(nick)) < 0 ) {
        // Inform the client about incorrect format
        syslog(LOG_NOTICE, "Failed to parse login request\n");
        ssize_t len = tlv_create_login_response(response_buffer, LOGIN_ERROR_INVALID,
                                                "Invalid request format");
        send(connfd, response_buffer, len, 0);
        return -1;
    }
    
    syslog(LOG_INFO, "Login attempt: '%s'\n", nick);
    
    // Validate nickname
    if ( server_validate_nick(nick) < 0 ) {
        // Inform the client about incorrect length and characters
        syslog(LOG_ERR, "Invalid nickname: '%s'\n", nick);
        ssize_t len = tlv_create_login_response(response_buffer, LOGIN_ERROR_INVALID,
                                                "Invalid nickname format");
        send(connfd, response_buffer, len, 0);
        return -1;
    }
    
    // Check if nickname is taken
    if ( server_is_nick_taken(nick) ) {
        syslog(LOG_NOTICE, "Nickname already taken: '%s'\n", nick);
        ssize_t len = tlv_create_login_response(response_buffer, LOGIN_ERROR_NICK_TAKEN,
                                                "Nickname already in use");
        send(connfd, response_buffer, len, 0);
        return -1;
    }
    
    // Success - add to active list, increase the nick counter
    if ( active_count < MAX_ACTIVE_NICKS ) {
        strcpy(active_nicks[active_count], nick);
        active_count++;
    } else {
        syslog(LOG_WARNING, "Warning: Active nicks list full (%d)\n", MAX_ACTIVE_NICKS);
    }
    
    syslog(LOG_INFO, "✓ User '%s' logged in successfully\n", nick);
    
    // Copy nick to output parameter
    if (nick_out) {
        strncpy(nick_out, nick, MAX_NICK_LENGTH - 1);
        nick_out[MAX_NICK_LENGTH - 1] = '\0';
    }
    
    ssize_t len = tlv_create_login_response(response_buffer, LOGIN_SUCCESS,
                                            "Login successful!");
    send(connfd, response_buffer, len, 0);
    
    return 0;
}

char* get_local_ip() {
    // Creating a pointer to a list of network interfaces and an iterator over that list
    struct ifaddrs *ifaddr, *ifa;
    static char host[INET6_ADDRSTRLEN];
    
    // Getting the interface list
    if ( getifaddrs(&ifaddr) == -1 ) {
        syslog(LOG_WARNING, "getifaddrs");
        return NULL;
    }
    
    // Iterating through interfaces with null address filtering
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if ( ifa->ifa_addr == NULL )
            continue;
        
        // IPv6 address lookup, skips loopback    
        if ( ifa->ifa_addr->sa_family == AF_INET6 ) {
            if ( strcmp(ifa->ifa_name, "lo") != 0 ) {
                // Address conversion
                struct sockaddr_in6 *addr = (struct sockaddr_in6 *)ifa->ifa_addr;
                if ( !IN6_IS_ADDR_LINKLOCAL(&addr->sin6_addr) ) {
                    inet_ntop(AF_INET6, &addr->sin6_addr, host, sizeof(host));
                    // Cleaning
                    freeifaddrs(ifaddr);
                    return host;
                }
            }
        }
    }
    
    // Fallback: if no IPv6, use IPv4 (mapped to IPv6)
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if ( ifa->ifa_addr == NULL )
            continue;
            
        if ( ifa->ifa_addr->sa_family == AF_INET ) {
            if ( strcmp(ifa->ifa_name, "lo") != 0 ) {
                struct sockaddr_in *addr = (struct sockaddr_in *)ifa->ifa_addr;
                // Map IPv4 to IPv6 format: ::ffff:192.168.1.5
                const char *prefix = "::ffff:";
                snprintf(host, sizeof(host), "%s", prefix);
                inet_ntop(AF_INET, &addr->sin_addr, host + strlen(prefix), 
                         sizeof(host) - strlen(prefix));
                freeifaddrs(ifaddr);
                return host;
            }
        }
    }
    
    freeifaddrs(ifaddr);
    return NULL;
}