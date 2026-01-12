#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdio.h>
#include <errno.h>
#include <signal.h>
#include "tlv.h"
#include "server_utils.h"
#include "server_types.h"
#include "multicast_discovery.h"
#include "sock_options.h"
#include "deamon_init.h"
#include "quiz.h"

#define SA struct sockaddr
#define MAXEVENTS   2000
#define MAXLINE     1024
#define LISTENQ     4

// Structure containing the following: tructures: id, question, answers, answer_number, correct answer; counter
QuizDatabase quiz_db;

// Global rankings array: nick, score, time
struct score_entry rankings[MAX_RANKINGS];
int rankings_count = 0;
// Mutual exclusion, prevents racing
pthread_mutex_t rankings_mutex = PTHREAD_MUTEX_INITIALIZER;

// Connection nicknames mapping (fd -> nick)
char connection_nicks[MAXEVENTS][MAX_NICK_LENGTH];

// Server statistics
struct server_stats stats = {0};
pthread_mutex_t stats_mutex = PTHREAD_MUTEX_INITIALIZER;

int main(int argc, char **argv)
{
    int                     listenfd, connfd;
    int                     epollfd, currfd;
    int                     nready, activeconns = 0;
    uint16_t                port;
    socklen_t               len;
    char                    str[INET6_ADDRSTRLEN + 1];
    struct sockaddr_in6     servaddr, cliaddr;
    struct epoll_event      events[MAXEVENTS], ev;

    if ( argc == 2 ) {
        port = atoi(argv[1]);
    } else {
        printf("Enter port number: ");
        scanf("%hu", &port);
    }

    if ( (listenfd = socket(AF_INET6, SOCK_STREAM, 0)) < 0 ) {
        int serr = errno;
        fprintf(stderr, "socket error: %s\n", strerror(serr));
        return 1;
    }

    if ( set_socket_options(listenfd) == -1 ) {
        perror("set_socket_options\n");
    }
    
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin6_family = AF_INET6;
    servaddr.sin6_addr = in6addr_any;
    servaddr.sin6_port = htons(port);

    if ( bind(listenfd, (SA*)&servaddr, sizeof(servaddr)) < 0 ) {
        int serr = errno;
        fprintf(stderr, "bind error: %s\n", strerror(serr));
        return 1;
    }

    if ( listen(listenfd, LISTENQ) < 0 ) {
        int serr = errno;
        fprintf(stderr, "listen error: %s\n", strerror(serr));
        return 1;
    }

    // Load quiz questions
    const char *questions_paths[] = {
        "resources/questions.json",               // When run from project root
        "../resources/questions.json",            // When run from build/
        "/usr/share/networkexam/questions.json",  // System install
        NULL
    };
    
    int questions_loaded = 0;
    for (int i = 0; questions_paths[i] != NULL; i++) {
        if (quiz_load_questions(&quiz_db, questions_paths[i]) == 0) {
            questions_loaded = 1;
            break;
        }
    }
    
    if (!questions_loaded) {
        fprintf(stderr, "WARNING: Failed to load questions from any path - quiz will not work!\n");
    }

    // Initialize server statistics
    stats.start_time = time(NULL);
    stats.total_connections = 0;
    stats.active_connections = 0;
    stats.tests_completed = 0;
    stats.questions_asked = 0;
    stats.total_score = 0;
    stats.best_score = 0;
    stats.best_time = 0;
    strcpy(stats.best_player, "N/A");

    printf("Server initialized\n");

    // Demonization of the process
    if ( daemon_init(argv[0], LOG_LOCAL0, 1000, listenfd) < 0 ) {
        fprintf(stderr, "daemon_init failed\n");
        exit(EXIT_FAILURE);
    }

    syslog(LOG_NOTICE, "Program started by User %d\n", getuid());
    syslog(LOG_INFO, "Loaded %d quiz questions", quiz_db.count);


    // Setting the socket to non-blocking mode, required by epoll
    if ( set_nonblocking(listenfd) < 0 ) {
        int serr = errno;
        syslog(LOG_ERR, "set_nonblocking: %s\n", strerror(serr));
    }

    // Creating epoll instances
    if ( (epollfd = epoll_create1(EPOLL_CLOEXEC)) == -1 ) {
        int serr = errno;
        syslog(LOG_ERR, "epoll_create1() error: %s", strerror(serr));
        return -1;
    }

    // Waiting for accept, ready to read
    ev.events = EPOLLIN;
    // When epoll returns an event, it's shows which descriptor it was related to
    ev.data.fd = listenfd;

    // Setting listenfd to wait for an event
    if ( epoll_ctl(epollfd, EPOLL_CTL_ADD, listenfd, &ev) == -1 ) {
        int serr = errno;
        syslog(LOG_ERR, "listen error: %s\n", strerror(serr));
    }

    // Get local IP address for discovery
    char *local_ip = get_local_ip();
    
    if ( local_ip == NULL ) {
        local_ip = "127.0.0.1";
        syslog(LOG_WARNING, "Warning: Could not detect local IP, using localhost\n");
        
    } else {
        syslog(LOG_INFO, "Detected local IP: %s\n", local_ip);
    }
    
    // Start discovery announcement service
    if ( start_discovery_service(local_ip, port) < 0 ) {
        int serr = errno;
        syslog(LOG_WARNING, "Failed to start discovery service: %s", strerror(serr));
    }
    
    syslog(LOG_NOTICE, "Server listening on port %d\n", port);


    for (;;) {

        // Waiting for an event on a previously added descriptor
        if ( (nready = epoll_wait(epollfd, events, MAXEVENTS, -1)) == -1 ) {
            int serr = errno;
            syslog(LOG_ERR, "epoll_wait error: %s\n", strerror(serr));
            return 1;
        }

        // Check all clients for data
        for (int i = 0; i < nready; i++) {
            currfd = events[i].data.fd;

            if ( currfd == listenfd ) {
                // Accept loop
                while (1) {
                    len = sizeof(cliaddr);
                    connfd = accept(listenfd, (SA*)&cliaddr, &len);
                    if (connfd < 0) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK)
                            break; // no more incoming connections
                        int serr = errno;
                        syslog(LOG_ERR, "accept error: %s\n", strerror(serr));
                        break;
                    }

                    if ( set_nonblocking(connfd) < 0 ) {
                        int serr = errno;
                        syslog(LOG_ERR, "set_nonblocking: %s\n", strerror(serr));
                    }

                    memset(&str, 0, sizeof(str));
                    inet_ntop(AF_INET6, &cliaddr.sin6_addr, str, sizeof(str));

                    // Event definition: incoming data, connection closure, error
                    ev.events = EPOLLIN | EPOLLRDHUP | EPOLLERR;
                    ev.data.fd = connfd;
                    if ( (epoll_ctl(epollfd, EPOLL_CTL_ADD, connfd, &ev)) == -1 ) {
                        syslog(LOG_ERR, "epoll_ctl adding new connection error");
                        close(connfd);
                        continue;
                    }
                    activeconns++;
                    
                    // Update statistics
                    pthread_mutex_lock(&stats_mutex);
                    stats.total_connections++;
                    stats.active_connections = activeconns;
                    pthread_mutex_unlock(&stats_mutex);
                }
                continue;
            }

            // Handle client socket events
            if (events[i].events & (EPOLLERR | EPOLLHUP | EPOLLRDHUP)) {
                // error or hangup on the socket
                if (connection_nicks[currfd][0] != '\0') {
                    syslog(LOG_INFO, "User %s disconnected (fd=%d)", connection_nicks[currfd], currfd);
                    connection_nicks[currfd][0] = '\0';  // Clear nickname
                } else {
                    syslog(LOG_INFO, "Client disconnected (fd=%d)", currfd);
                }
                close(currfd);
                activeconns--;
                
                // Update statistics
                pthread_mutex_lock(&stats_mutex);
                stats.active_connections = activeconns;
                pthread_mutex_unlock(&stats_mutex);
                continue;
            }

            // Receive TLV message from client
            uint8_t buffer[4096];
            ssize_t received = recv(currfd, buffer, sizeof(buffer), 0);
            if ( received < 0 ) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) {
                    // no data right now
                    continue;
                }
                syslog(LOG_ERR, "recv\n");
                if (connection_nicks[currfd][0] != '\0') {
                    syslog(LOG_INFO, "User %s disconnected due to recv error (fd=%d)", connection_nicks[currfd], currfd);
                    connection_nicks[currfd][0] = '\0';  // Clear nickname
                }
                close(currfd);
                activeconns--;
                continue;
            }
            if ( received == 0 ) {
                // connection closed by peer
                if (connection_nicks[currfd][0] != '\0') {
                    syslog(LOG_INFO, "User %s disconnected (fd=%d)", connection_nicks[currfd], currfd);
                    connection_nicks[currfd][0] = '\0';  // Clear nickname
                } else {
                    syslog(LOG_INFO, "Client disconnected (fd=%d)", currfd);
                }
                close(currfd);
                activeconns--;
                continue;
            }

            // Parse TLV header
            uint16_t type, length;
            if ( received < TLV_HEADER_SIZE || tlv_parse_header(buffer, &type, &length) < 0 ) {
                syslog(LOG_ERR, "Invalid or incomplete TLV header from fd %d\n", currfd);
                if (connection_nicks[currfd][0] != '\0') {
                    syslog(LOG_INFO, "User %s disconnected due to invalid TLV (fd=%d)", connection_nicks[currfd], currfd);
                    connection_nicks[currfd][0] = '\0';  // Clear nickname
                }
                close(currfd);
                activeconns--;
                continue;
            }

            // Handle different message types
            if ( type == TLV_LOGIN_REQUEST ) {
                char nick[MAX_NICK_LENGTH];
                if (server_handle_login(currfd, buffer + TLV_HEADER_SIZE, nick) == 0) {
                    // Save nick for this connection
                    strncpy(connection_nicks[currfd], nick, MAX_NICK_LENGTH - 1);
                    connection_nicks[currfd][MAX_NICK_LENGTH - 1] = '\0';
                }
            } else if ( type == TLV_REQUEST_QUESTION ) {
                // Parse request
                uint8_t mode, question_index;
                if (tlv_parse_request_question(buffer + TLV_HEADER_SIZE, &mode, &question_index) < 0) {
                    syslog(LOG_ERR, "Failed to parse REQUEST_QUESTION from fd %d", currfd);
                    continue;
                }
                
                // Get random question
                Question *q = quiz_get_random_question(&quiz_db);
                if (!q) {
                    syslog(LOG_ERR, "No questions available for fd %d", currfd);
                    continue;
                }
                
                // Prepare answers as array of pointers
                const char *answers[MAX_ANSWERS_PER_Q];
                for (int i = 0; i < q->num_odpowiedzi; i++) {
                    answers[i] = q->odpowiedzi[i];
                }
                
                // Create QUESTION_DATA message
                uint8_t response[4096];
                ssize_t resp_len = tlv_create_question_data(response, q->id, q->pytanie, 
                                                             answers, q->num_odpowiedzi);
                if (resp_len > 0) {
                    send(currfd, response, resp_len, 0);
                    
                    // Update statistics
                    pthread_mutex_lock(&stats_mutex);
                    stats.questions_asked++;
                    pthread_mutex_unlock(&stats_mutex);
                }
            } else if ( type == TLV_ANSWER_SUBMIT ) {
                // Parse answer
                uint16_t question_id;
                uint8_t answer_id;
                if (tlv_parse_answer_submit(buffer + TLV_HEADER_SIZE, &question_id, &answer_id) < 0) {
                    syslog(LOG_ERR, "Failed to parse ANSWER_SUBMIT from fd %d", currfd);
                    continue;
                }
                
                // Find question
                Question *q = quiz_get_question_by_id(&quiz_db, question_id);
                if (!q) {
                    syslog(LOG_ERR, "Question %d not found for fd %d", question_id, currfd);
                    continue;
                }
                
                // Check answer
                int is_correct = quiz_check_answer(q, answer_id);
                uint8_t correct_answer_id = q->poprawna - 1;
                
                // Create ANSWER_RESULT message
                uint8_t response[4096];
                ssize_t resp_len = tlv_create_answer_result(response, question_id, 
                                                             is_correct, correct_answer_id,
                                                             0, 1, is_correct ? 1 : 0);
                if (resp_len > 0) {
                    send(currfd, response, resp_len, 0);
                }
            } else if ( type == TLV_SUBMIT_SCORE ) {
                // Parse score submission
                uint8_t score;
                uint32_t time_seconds;
                if (tlv_parse_submit_score(buffer + TLV_HEADER_SIZE, &score, &time_seconds) < 0) {
                    syslog(LOG_ERR, "Failed to parse SUBMIT_SCORE from fd %d", currfd);
                    continue;
                }
                
                // Get nickname for this connection
                if (connection_nicks[currfd][0] != '\0') {
                    // Add to rankings
                    pthread_mutex_lock(&rankings_mutex);
                    if (rankings_count < MAX_RANKINGS) {
                        strncpy(rankings[rankings_count].nick, connection_nicks[currfd], 32);
                        rankings[rankings_count].score = score;
                        rankings[rankings_count].time_seconds = time_seconds;
                        rankings_count++;
                        syslog(LOG_INFO, "Saved score for %s: %d/10 in %d seconds", 
                               connection_nicks[currfd], score, time_seconds);
                    }
                    pthread_mutex_unlock(&rankings_mutex);
                    
                    // Update statistics
                    pthread_mutex_lock(&stats_mutex);
                    stats.tests_completed++;
                    stats.total_score += score;
                    
                    // Update best score
                    if (score > stats.best_score || 
                        (score == stats.best_score && (stats.best_time == 0 || time_seconds < stats.best_time))) {
                        stats.best_score = score;
                        stats.best_time = time_seconds;
                        strncpy(stats.best_player, connection_nicks[currfd], 32);
                    }
                    pthread_mutex_unlock(&stats_mutex);
                }
            } else if ( type == TLV_REQUEST_RANKING ) {
                // Send ranking data
                pthread_mutex_lock(&rankings_mutex);
                
                char nicks[MAX_RANKINGS][MAX_NICK_LENGTH];
                uint8_t scores[MAX_RANKINGS];
                uint32_t times[MAX_RANKINGS];
                
                // Sort by score (descending), then by time (ascending)
                struct score_entry sorted[MAX_RANKINGS];
                memcpy(sorted, rankings, rankings_count * sizeof(struct score_entry));
                
                for (int i = 0; i < rankings_count - 1; i++) {
                    for (int j = 0; j < rankings_count - i - 1; j++) {
                        if (sorted[j].score < sorted[j+1].score || 
                            (sorted[j].score == sorted[j+1].score && sorted[j].time_seconds > sorted[j+1].time_seconds)) {
                            struct score_entry temp = sorted[j];
                            sorted[j] = sorted[j+1];
                            sorted[j+1] = temp;
                        }
                    }
                }
                
                // Copy to arrays (max 10 entries)
                int count = rankings_count > 10 ? 10 : rankings_count;
                for (int i = 0; i < count; i++) {
                    strncpy(nicks[i], sorted[i].nick, MAX_NICK_LENGTH);
                    scores[i] = sorted[i].score;
                    times[i] = sorted[i].time_seconds;
                }
                
                pthread_mutex_unlock(&rankings_mutex);
                
                // Create RANKING_DATA message
                uint8_t response[4096];
                ssize_t resp_len = tlv_create_ranking_data(response, count, nicks, scores, times);
                if (resp_len > 0) {
                    send(currfd, response, resp_len, 0);
                    syslog(LOG_INFO, "Sent ranking data to fd %d", currfd);
                }
            } else if ( type == TLV_REQUEST_SERVER_INFO ) {
                // Calculate server info
                pthread_mutex_lock(&stats_mutex);
                
                time_t current_time = time(NULL);
                uint32_t uptime = (uint32_t)difftime(current_time, stats.start_time);
                uint8_t avg_score = stats.tests_completed > 0 ? 
                                   (stats.total_score / stats.tests_completed) : 0;
                
                pthread_mutex_unlock(&stats_mutex);
                
                // Create SERVER_INFO_DATA message
                uint8_t response[4096];
                ssize_t resp_len = tlv_create_server_info_data(response, uptime,
                                                                stats.active_connections,
                                                                stats.total_connections,
                                                                quiz_db.count,
                                                                stats.tests_completed,
                                                                stats.questions_asked,
                                                                avg_score,
                                                                stats.best_score,
                                                                stats.best_time,
                                                                stats.best_player,
                                                                port);
                if (resp_len > 0) {
                    send(currfd, response, resp_len, 0);
                    syslog(LOG_INFO, "Sent server info to fd %d", currfd);
                }
            } else {
                syslog(LOG_WARNING, "Unexpected message type: 0x%04X\n", type);
            }
        }

    }

    return 0;

}