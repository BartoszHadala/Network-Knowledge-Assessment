#ifndef CLIENT_UTILS_H
#define CLIENT_UTILS_H

#include <stdint.h>
#include <sys/types.h>

/**
 * Send LOGIN_REQUEST and receive LOGIN_RESPONSE
 * @param sockfd Socket file descriptor
 * @param nick User nickname
 * @return 0 on success, -1 on error
 */
int client_login(int sockfd, const char *nick);

/**
 * Request a question from server
 * @param sockfd Socket file descriptor
 * @param mode Question mode (MODE_RANDOM or MODE_TEST)
 * @param question_index Question index for test mode
 * @return 0 on success, -1 on error
 */
int client_request_question(int sockfd, uint8_t mode, uint8_t question_index);

/**
 * Submit an answer to server
 * @param sockfd Socket file descriptor
 * @param question_id Question ID
 * @param answer_id Selected answer ID
 * @return 0 on success, -1 on error
 */
int client_submit_answer(int sockfd, uint16_t question_id, uint8_t answer_id);

/**
 * Receive and display question, get user answer, submit and show result
 * @param sockfd Socket file descriptor
 * @return 1 if correct answer, 0 if incorrect, -1 on error
 */
int client_handle_single_question(int sockfd);

/**
 * Request and display user rankings
 * @param sockfd Socket file descriptor
 * @return 0 on success, -1 on error
 */
int client_request_ranking(int sockfd);

/**
 * Request and display server information
 * @param sockfd Socket file descriptor
 * @return 0 on success, -1 on error
 */
int client_request_server_info(int sockfd);

#endif // CLIENT_UTILS_H
