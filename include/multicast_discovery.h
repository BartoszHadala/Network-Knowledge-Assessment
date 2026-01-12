#ifndef MULTICAST_DISCOVERY
#define MULTICAST_DISCOVERY

#include    <sys/types.h>
#include    <sys/socket.h>
#include    <netinet/ip.h>
#include    <net/if.h>  
#include    <netinet/in.h>  
#include    <arpa/inet.h>   
#include    <errno.h>
#include    <signal.h>
#include    <stdio.h>
#include    <stdlib.h>
#include    <string.h>
#include 	<sys/ioctl.h>
#include 	<unistd.h>
#include    <syslog.h>
#include    <pthread.h>
#include 	<net/if.h>
#include	<netdb.h>
#include	<sys/utsname.h>
#include	<linux/un.h>

#define MAXLINE 1024
#define SA      struct sockaddr
#define IPV6 1

/**
 * @brief Structure holding server information for discovery announcement
 */
typedef struct {
    char *ip;    /**< Server IP address */
    int port;    /**< Server port number */
} server_info_t;

/**
 * @brief Creates a UDP socket for sending multicast messages
 * 
 * The function automatically detects whether the address is IPv4 or IPv6 and creates
 * the appropriate socket. Allocates memory for the sockaddr structure.
 * 
 * @param serv Multicast IP address (e.g., "239.0.0.1" for IPv4 or "ff02::1" for IPv6)
 * @param port Port number
 * @param saptr Pointer to sockaddr structure pointer (memory allocated by the function)
 * @param lenp Pointer to variable where the size of sockaddr structure will be stored
 * @return Socket descriptor (>= 0) on success, -1 on error
 */
int snd_udp_socket(const char *serv, int port, SA **saptr, socklen_t *lenp);

/**
 * @brief Joins a multicast group on a socket
 * 
 * This function allows the socket to receive multicast datagrams sent to the specified
 * multicast group address. Can optionally bind to a specific network interface.
 * 
 * @param sockfd Socket descriptor (must be UDP socket)
 * @param grp Pointer to sockaddr structure containing the multicast group address
 * @param grplen Length of the sockaddr structure
 * @param ifname Name of the network interface (e.g., "eth0", "wlan0"), or NULL for any interface
 * @param ifindex Interface index (use 0 if providing ifname, or use if_nametoindex() result)
 * @return 0 on success, -1 on error (sets errno)
 */
int mcast_join(int sockfd, const SA *grp, socklen_t grplen, const char *ifname, unsigned int ifindex);

/**
 * @brief Sets or disables multicast loopback on a socket
 * 
 * Controls whether multicast datagrams sent on this socket are looped back to the
 * local host (useful for testing or if sender also needs to receive).
 * 
 * @param sockfd Socket descriptor (must be UDP socket)
 * @param onoff 1 to enable loopback, 0 to disable
 * @return 0 on success, -1 on error (sets errno)
 */
int mcast_set_loop(int sockfd, int onoff);

/**
 * @brief Gets the address family of a socket
 * 
 * Determines whether a socket is IPv4 (AF_INET) or IPv6 (AF_INET6).
 * 
 * @param sockfd Socket descriptor
 * @return AF_INET, AF_INET6, or -1 on error
 */
int sockfd_to_family(int sockfd);

/**
 * @brief Thread function that periodically announces server presence via multicast
 * 
 * This function runs in a separate thread and broadcasts the server's IP address
 * and port to the multicast group 239.0.0.1:9999 every 5 seconds.
 * 
 * @param arg Pointer to server_info_t structure containing server IP and port
 * @return NULL (runs indefinitely)
 */
void* discovery_announce(void* arg);

/**
 * @brief Discovers a server on the network using multicast
 * 
 * Listens on the multicast group 239.0.0.1:9999 for server announcements.
 * Blocks until a server announcement is received, then extracts the server's
 * IP address and port.
 * 
 * @param server_ip Buffer to store discovered server IP address (must be at least INET_ADDRSTRLEN bytes)
 * @param server_port Pointer to store discovered server port number
 */
void discover_server(char *server_ip, int *server_port);

/**
 * @brief Initialize and start multicast discovery announcement service
 * 
 * Starts a background thread that periodically broadcasts server presence
 * via multicast to 239.0.0.1:9999 every 5 seconds, allowing clients to auto-discover the server.
 * The IP address is copied internally, so the caller's buffer can be freed after this call.
 * 
 * @param ip Server IP address to announce (e.g., "192.168.1.5")
 * @param port Server port to announce
 * @return 0 on success, -1 on error
 */
int start_discovery_service(const char *ip, int port);

#endif // MULTICAST_DISCOVERY
