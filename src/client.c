#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <time.h>
#include "client_utils.h"
#include "multicast_discovery.h"
#include "tlv.h"
#include "menu.h"

#define SA struct sockaddr
#define MAXLINE 1024

int main(int argc, char **argv)
{
    int                     sockfd;
    struct sockaddr_in6     servaddr;
    char server_ip          [INET_ADDRSTRLEN];
    int                     port;

    if ( argc == 2 && strcmp(argv[1], "--discover") == 0 ) {
        printf("Searching for server...\n");
        // Multicast discovery, search for a server IP and port
        discover_server(server_ip, &port);
    } else if ( argc == 3) {
        // Manually specifying arguments
        strcpy(server_ip, argv[1]);
        port = atoi(argv[2]);
    } else {
        fprintf(stderr, "usage: %s <IPaddress> <Port> OR %s --discover\n", argv[0], argv[0]);
        return 1;
    }

    menu_display_banner();

    if ( (sockfd = socket(AF_INET6, SOCK_STREAM, 0)) < 0 ) {
        fprintf(stderr, "socket error: %s\n", strerror(errno));
        return 1;
    }

    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin6_family = AF_INET6;
    servaddr.sin6_port = htons(port);

    // Try IPv6 first, if it fails try IPv4-mapped-IPv6
    if ( inet_pton(AF_INET6, server_ip, &servaddr.sin6_addr) <= 0 ) {
        // Probably IPv4 address, convert to IPv4-mapped-IPv6
        struct in_addr ipv4_addr;
        if ( inet_pton(AF_INET, server_ip, &ipv4_addr) <= 0 ) {
            fprintf(stderr,"inet_pton error for %s : %s \n", server_ip, strerror(errno));
            return 1;
        }
        // Map IPv4 to IPv6: ::ffff:x.x.x.x
        memset(&servaddr.sin6_addr, 0, sizeof(servaddr.sin6_addr));
        servaddr.sin6_addr.s6_addr[10] = 0xff;
        servaddr.sin6_addr.s6_addr[11] = 0xff;
        memcpy(&servaddr.sin6_addr.s6_addr[12], &ipv4_addr, 4);
    }

    if ( connect(sockfd, (SA*)&servaddr, sizeof(servaddr)) < 0 ) {
        fprintf(stderr,"connect error : %s \n", strerror(errno));
        return 1;
    }

    
    char nick[MAX_NICK_LENGTH + 1];
    printf("Enter your nickname: ");
    if ( scanf("%32s", nick) != 1 ) {
        fprintf(stderr, "Failed to read nickname\n");
        close(sockfd);
        return 1;
    }

    if ( client_login(sockfd, nick) < 0 ) {
        close(sockfd);
        return 1;
    }

    printf("\n");

    // Main menu loop
    int running = 1;
    while (running) {
        int choice = menu_display_and_get_choice();
        
        if (choice < 0) {
            printf("Invalid choice. Please enter a number between 1 and 7.\n");
            continue;
        }
        
        switch (choice) {
            case 1:
                printf("\n====== Training Mode - Random Question ======\n");
                // Request random question
                if (client_request_question(sockfd, MODE_RANDOM, 0) == 0) {
                    // Receive, display question, get answer, submit, show result
                    client_handle_single_question(sockfd);
                }
                break;
                
            case 2:
                printf("\n====== Knowledge Test - 10 Questions ======\n");
                printf("Starting test...\n\n");
                // Test of 10 questions
                int correct_count = 0;
                int questions_asked = 0;
                
                // Start timer
                time_t start_time = time(NULL);
                
                for (int i = 0; i < 10; i++) {
                    printf("\n--- Question %d/10 ---\n", i + 1);
                    if (client_request_question(sockfd, MODE_TEST, i) == 0) {
                        int result = client_handle_single_question(sockfd);
                        if (result < 0) {
                            printf("Error during test. Aborting.\n");
                            break;
                        }
                        questions_asked++;
                        if (result == 1) {
                            correct_count++;
                        }
                    } else {
                        printf("Failed to request question. Aborting test.\n");
                        break;
                    }
                    
                    if (i < 9) {
                        printf("\nPress Enter to continue to next question...");
                        getchar();
                    }
                }
                
                // Calculate elapsed time
                time_t end_time = time(NULL);
                uint32_t elapsed_seconds = (uint32_t)difftime(end_time, start_time);
                
                printf("\n╔═══════════════════════════════════════╗\n");
                printf("║         TEST COMPLETE                 ║\n");
                printf("╠═══════════════════════════════════════╣\n");
                printf("║  Your Score: %d/%d                     ║\n", correct_count, questions_asked);
                printf("║  Time: %02d:%02d                        ║\n", elapsed_seconds / 60, elapsed_seconds % 60);
                printf("╚═══════════════════════════════════════╝\n");
                
                // Submit score to server
                if (questions_asked == 10) {
                    uint8_t score_buffer[256];
                    ssize_t len = tlv_create_submit_score(score_buffer, correct_count, elapsed_seconds);
                    if (len > 0) {
                        send(sockfd, score_buffer, len, 0);
                        printf("\n✓ Score submitted to server!\n");
                    }
                }
                break;
                
            case 3:
                printf("\n====== User Rankings ======\n");
                if (client_request_ranking(sockfd) < 0) {
                    printf("Failed to retrieve rankings.\n");
                }
                break;
                
            case 4:
                printf("\n====== Server Information ======\n");
                if (client_request_server_info(sockfd) < 0) {
                    printf("Failed to retrieve server information.\n");
                }
                break;
                
            case 5:
                printf("\nExiting...\n");
                running = 0;
                break;
                
            default:
                printf("Invalid option.\n");
        }
    }

    printf("\nDisconnecting...\n");
    close(sockfd);
    return 0;
}