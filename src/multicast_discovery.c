#define _GNU_SOURCE
#include "multicast_discovery.h"
#include <sys/types.h>
#include <sys/ioctl.h>
#include <net/if.h>

int snd_udp_socket(const char *serv, int port, SA **saptr, socklen_t *lenp)
{
	int sockfd;
	struct sockaddr_in6 *pservaddrv6;
	struct sockaddr_in *pservaddrv4;

    // Dynamic memory allocation, assuming we support IPv6 addresses
	*saptr = malloc( sizeof(struct sockaddr_in6));
	
	pservaddrv6 = (struct sockaddr_in6*)*saptr;

	memset(pservaddrv6, 0, sizeof(struct sockaddr_in6));

    // Check if we have an IPv6 address
	if (inet_pton(AF_INET6, serv, &pservaddrv6->sin6_addr) <= 0){
	
        // Freeing up old memory and creating new one
		free(*saptr);
		*saptr = malloc( sizeof(struct sockaddr_in));
		pservaddrv4 = (struct sockaddr_in*)*saptr;
		memset(pservaddrv4, 0, sizeof(struct sockaddr_in));

		// Check if we have an IPv4 address, otherwise return an error
		if (inet_pton(AF_INET, serv, &pservaddrv4->sin_addr) <= 0){
			syslog(LOG_ERR,"AF_INET inet_pton error for %s : %s \n", serv, strerror(errno));
			return -1;
		}else{
			pservaddrv4->sin_family = AF_INET;
			pservaddrv4->sin_port   = htons(port);
			*lenp =  sizeof(struct sockaddr_in);
            // Create an IPv4 UDP socket
			if ( (sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0){
				syslog(LOG_ERR,"AF_INET socket error : %s\n", strerror(errno));
				return -1;
			}
		}

	}else{
		pservaddrv6 = (struct sockaddr_in6*)*saptr;
		pservaddrv6->sin6_family = AF_INET6;
		pservaddrv6->sin6_port   = htons(port);	/* daytime server */
		*lenp =  sizeof(struct sockaddr_in6);

        // Creates an IPv6 UDP socket
		if ( (sockfd = socket(AF_INET6, SOCK_DGRAM, 0)) < 0){
			syslog(LOG_ERR,"AF_INET6 socket error : %s\n", strerror(errno));
			return -1;
		}

	}

	return(sockfd);
}

int mcast_join(int sockfd, const SA *grp, socklen_t grplen __attribute__((unused)), const char *ifname, unsigned int ifindex)
{
    // Request for IP_ADD_MEMBERSHIP
    // Contains the multicast group address and network interface
	struct ip_mreq mreq; 
    // Network interface info: name, IP
	struct ifreq ifreq;  

    // Check if the address is IPv4, because this function only supports it
	if ( grp->sa_family != AF_INET ) {
		errno = EAFNOSUPPORT;
		return -1;
	}

    // Copying to structure
	memcpy(&mreq.imr_multiaddr,
		   &((const struct sockaddr_in *) grp)->sin_addr,
		   sizeof(struct in_addr));

	if (ifindex > 0) {
        // Convert number → name (e.g. 2 → "eth0")
		if (if_indextoname(ifindex, ifreq.ifr_name) == NULL) {
			errno = ENXIO;
			return(-1);
		}
        // Get the IP address of this interface
		if (ioctl(sockfd, SIOCGIFADDR, &ifreq) < 0) {
			return(-1);
        }
        // Copy the interface IP address
		memcpy(&mreq.imr_interface,
			   &((struct sockaddr_in *) &ifreq.ifr_addr)->sin_addr,
			   sizeof(struct in_addr));
	} else if (ifname != NULL) {
        // If not present, use the given name (e.g. "wlan0")
		strncpy(ifreq.ifr_name, ifname, IFNAMSIZ);
		if (ioctl(sockfd, SIOCGIFADDR, &ifreq) < 0)
			return(-1);
		memcpy(&mreq.imr_interface,
			   &((struct sockaddr_in *) &ifreq.ifr_addr)->sin_addr,
			   sizeof(struct in_addr));
	} else {
        // Any interface
		mreq.imr_interface.s_addr = htonl(INADDR_ANY);
	}

	return(setsockopt(sockfd, IPPROTO_IP, IP_ADD_MEMBERSHIP,
					  &mreq, sizeof(mreq)));
}

int sockfd_to_family(int sockfd)
{
	struct sockaddr_storage ss;
	socklen_t len;

	len = sizeof(ss);
	if (getsockname(sockfd, (SA *) &ss, &len) < 0)
		return(-1);
	return(ss.ss_family);
}

int mcast_set_loop(int sockfd, int onoff)
{
	switch (sockfd_to_family(sockfd)) {
	case AF_INET: {
		unsigned char flag;

		flag = onoff;
		return(setsockopt(sockfd, IPPROTO_IP, IP_MULTICAST_LOOP,
						  &flag, sizeof(flag)));
	}

#ifdef	IPV6
	case AF_INET6: {
		unsigned int flag;

		flag = onoff;
		return(setsockopt(sockfd, IPPROTO_IPV6, IPV6_MULTICAST_LOOP,
						  &flag, sizeof(flag)));
	}
#endif

	default:
		errno = EAFNOSUPPORT;
		return(-1);
	}
}

// Every thread function must accept a void*
void* discovery_announce(void* arg) {
    // Unpacking the argument
    server_info_t *info = (server_info_t*)arg;
    
    // Creating a UDP multicast socket
    SA *sadest;
    socklen_t salen;
    int sendfd = snd_udp_socket("239.0.0.1", 9999, &sadest, &salen);
    
    // Infinite loop of announcements every 5 seconds
    char announcement[MAXLINE];
    while(1) {
	 snprintf(announcement, sizeof(announcement), 
		   "SERVER:%s:%d", info->ip, info->port);
        
	 sendto(sendfd, announcement, strlen(announcement), 0, 
		 sadest, salen);
        
        sleep(5);
    }
}

void discover_server(char *server_ip, int *server_port) {
    // // Creating a UDP multicast socket
    SA *sarecv;
    socklen_t salen;
    int recvfd = snd_udp_socket("239.0.0.1", 9999, &sarecv, &salen);
    
    // Allows multiple sockets to bind to the same port simultaneously
    const int on = 1;
    setsockopt(recvfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    
    bind(recvfd, sarecv, salen);

    // Join multicast group
    mcast_join(recvfd, sarecv, salen, NULL, 0);
    
	// Wait for the announcement
	char line[MAXLINE];
	ssize_t n = recvfrom(recvfd, line, MAXLINE - 1, 0, NULL, NULL);
	if (n <= 0) {
		server_ip[0] = '\0';
		*server_port = 0;
		close(recvfd);
		return;
	}
	line[n] = '\0';

	// Strip trailing newline/carriage return
	char *p = line + n - 1;
	while (p >= line && (*p == '\n' || *p == '\r')) { *p = '\0'; --p; }

	// Robust parse: handle IPv4 and IPv6 addresses. Use last ':' as separator
	const char *prefix = "SERVER:";
	if (strncmp(line, prefix, strlen(prefix)) != 0) {
		server_ip[0] = '\0';
		*server_port = 0;
		close(recvfd);
		return;
	}

	char *last_colon = strrchr(line, ':');
	if (!last_colon) {
		server_ip[0] = '\0';
		*server_port = 0;
		close(recvfd);
		return;
	}

	// Parse port
	*server_port = atoi(last_colon + 1);

	// Copy IP between prefix and last_colon
	size_t ip_len = (size_t)(last_colon - (line + strlen(prefix)));
	if (ip_len >= INET6_ADDRSTRLEN) ip_len = INET6_ADDRSTRLEN - 1;
	memcpy(server_ip, line + strlen(prefix), ip_len);
	server_ip[ip_len] = '\0';
    
    close(recvfd);
}

int start_discovery_service(const char *ip, int port) {
    // The structure contains the IP address and port
	static server_info_t server_info;
	/* Use INET6_ADDRSTRLEN to fit IPv6 and IPv4-mapped IPv6 strings */
	static char ip_copy[INET6_ADDRSTRLEN];
    static pthread_t discovery_thread;
    
	// Overwrite protection
	strncpy(ip_copy, ip, INET6_ADDRSTRLEN - 1);
	ip_copy[INET6_ADDRSTRLEN - 1] = '\0';
    
    server_info.ip = ip_copy;
    server_info.port = port;
    
    // Creating a thread discovery
    if (pthread_create(&discovery_thread, NULL, discovery_announce, &server_info) != 0) {
		syslog(LOG_ERR, "pthread_create discovery");
        return -1;
    }
    
    // The thread becomes a demon
    // The system automatically cleans up after the thread ends
    pthread_detach(discovery_thread);
	syslog(LOG_NOTICE, "Discovery service started on %s:%d\n", ip, port);
    return 0;
}