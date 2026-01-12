#ifndef TLV_H
#define TLV_H

/**
 * TLV (Type-Length-Value) Protocol Implementation
 * 
 * TLV is a simple encoding scheme where each message consists of:
 *   - Type (2 bytes):   Message type identifier (network byte order)
 *   - Length (2 bytes): Length of the value field (network byte order)
 *   - Value (variable): Actual message data
 * 
 * Example: LOGIN_REQUEST with nick "User123"
 *   Bytes: [0x00, 0x01, 0x00, 0x08, 0x07, 'U', 's', 'e', 'r', '1', '2', '3']
 *          |Type=0x0001| |Len=8|  |nick_len| |------- nick ---------|
 * 
 * Usage:
 *   Client side:
 *     uint8_t buffer[1024];
 *     ssize_t len = tlv_create_login_request(buffer, "User123");
 *     send(sockfd, buffer, len, 0);
 * 
 *   Server side:
 *     recv(connfd, buffer, sizeof(buffer), 0);
 *     uint16_t type, length;
 *     tlv_parse_header(buffer, &type, &length);
 *     if (type == TLV_LOGIN_REQUEST) {
 *         char nick[64];
 *         tlv_parse_login_request(buffer + 4, nick, sizeof(nick));
 *     }
 */

#include <stdint.h>
#include <sys/types.h>
#include <string.h>
#include <arpa/inet.h>

// TLV Protocol constants
#define TLV_HEADER_SIZE         4  // Type (2 bytes) + Length (2 bytes)

// TLV Message Types
#define TLV_LOGIN_REQUEST       0x0001
#define TLV_LOGIN_RESPONSE      0x0002
#define TLV_REQUEST_QUESTION    0x0003
#define TLV_QUESTION_DATA       0x0004
#define TLV_ANSWER_SUBMIT       0x0005
#define TLV_ANSWER_RESULT       0x0006
#define TLV_SUBMIT_SCORE        0x0007
#define TLV_REQUEST_RANKING     0x0008
#define TLV_RANKING_DATA        0x0009
#define TLV_REQUEST_SERVER_INFO 0x000A
#define TLV_SERVER_INFO_DATA    0x000B

// Login response status codes
#define LOGIN_SUCCESS           0
#define LOGIN_ERROR_NICK_TAKEN  1
#define LOGIN_ERROR_INVALID     2

// Question modes
#define MODE_RANDOM             0
#define MODE_TEST               1

// Maximum sizes
#define MAX_NICK_LENGTH         32
#define MAX_MESSAGE_LENGTH      128
#define MAX_QUESTION_LENGTH     1024
#define MAX_ANSWER_LENGTH       256
#define MAX_ANSWERS             4
#define MAX_RANKINGS            100

// Basic TLV header
struct tlv_header {
    uint16_t type;      // Message type (network byte order)
    uint16_t length;    // Length of value field (network byte order)
    uint8_t value[];    // Variable length data
} __attribute__((packed));

// LOGIN_REQUEST (0x0001)
struct login_request {
    uint8_t nick_length;
    char nick[];
} __attribute__((packed));

// LOGIN_RESPONSE (0x0002)
struct login_response {
    uint8_t status;
    uint8_t message_length;
    char message[];
} __attribute__((packed));

// REQUEST_QUESTION (0x0003)
struct request_question {
    uint8_t mode;
    uint8_t question_index;
} __attribute__((packed));

// QUESTION_DATA (0x0004)
struct question_data {
    uint16_t question_id;
    uint8_t num_answers;
    uint16_t question_length;
    // Followed by: char question_text[]
    // Then num_answers blocks of answer_data
} __attribute__((packed));

struct answer_data {
    uint8_t answer_id;
    uint16_t answer_length;
    // Followed by: char answer_text[]
} __attribute__((packed));

// ANSWER_SUBMIT (0x0005)
struct answer_submit {
    uint16_t question_id;
    uint8_t answer_id;
} __attribute__((packed));

// ANSWER_RESULT (0x0006)
struct answer_result {
    uint16_t question_id;
    uint8_t is_correct;
    uint8_t correct_answer_id;
    uint8_t test_mode;
    uint8_t questions_answered;
    uint8_t correct_count;
} __attribute__((packed));

// Function prototypes

/**
 * Create TLV LOGIN_REQUEST message
 * @param buffer Output buffer
 * @param nick User nickname
 * @return Total message length in bytes
 */
ssize_t tlv_create_login_request(uint8_t *buffer, const char *nick);

/**
 * Create TLV LOGIN_RESPONSE message
 * @param buffer Output buffer
 * @param status Login status code
 * @param message Status message
 * @return Total message length in bytes
 */
ssize_t tlv_create_login_response(uint8_t *buffer, uint8_t status, const char *message);

/**
 * Create TLV REQUEST_QUESTION message
 * @param buffer Output buffer
 * @param mode Question mode (MODE_RANDOM or MODE_TEST)
 * @param question_index Question index (0-9 for test mode)
 * @return Total message length in bytes
 */
ssize_t tlv_create_request_question(uint8_t *buffer, uint8_t mode, uint8_t question_index);

/**
 * Create TLV QUESTION_DATA message
 * @param buffer Output buffer
 * @param question_id Question ID
 * @param question_text Question text
 * @param answers Array of answer strings (up to MAX_ANSWERS)
 * @param num_answers Number of answers
 * @return Total message length in bytes, -1 on error
 */
ssize_t tlv_create_question_data(uint8_t *buffer, uint16_t question_id,
                                  const char *question_text,
                                  const char **answers, uint8_t num_answers);

/**
 * Create TLV ANSWER_SUBMIT message
 * @param buffer Output buffer
 * @param question_id Question ID
 * @param answer_id Selected answer ID (0-3)
 * @return Total message length in bytes
 */
ssize_t tlv_create_answer_submit(uint8_t *buffer, uint16_t question_id, uint8_t answer_id);

/**
 * Create TLV ANSWER_RESULT message
 * @param buffer Output buffer
 * @param question_id Question ID
 * @param is_correct Whether answer was correct
 * @param correct_answer_id ID of correct answer
 * @param test_mode Test mode flag
 * @param questions_answered Number of questions answered
 * @param correct_count Number of correct answers
 * @return Total message length in bytes
 */
ssize_t tlv_create_answer_result(uint8_t *buffer, uint16_t question_id, 
                                  uint8_t is_correct, uint8_t correct_answer_id,
                                  uint8_t test_mode, uint8_t questions_answered,
                                  uint8_t correct_count);

/**
 * Parse 4-byte TLV header and converts from network byte order to host byte order
 * @param buffer Input buffer
 * @param type Output: message type
 * @param length Output: value length
 * @return 0 on success, -1 on error
 */
int tlv_parse_header(const uint8_t *buffer, uint16_t *type, uint16_t *length);

/**
 * Parse LOGIN_REQUEST message
 * @param buffer Input buffer (after header)
 * @param nick Output buffer for nickname
 * @param nick_size Size of nick buffer
 * @return 0 on success, -1 on error
 */
int tlv_parse_login_request(const uint8_t *buffer, char *nick, size_t nick_size);

/**
 * Parse LOGIN_RESPONSE message
 * @param buffer Input buffer (after header)
 * @param status Output: login status
 * @param message Output buffer for message
 * @param msg_size Size of message buffer
 * @return 0 on success, -1 on error
 */
int tlv_parse_login_response(const uint8_t *buffer, uint8_t *status, 
                              char *message, size_t msg_size);

/**
 * Parse REQUEST_QUESTION message
 * @param buffer Input buffer (after header)
 * @param mode Output: question mode
 * @param question_index Output: question index
 * @return 0 on success, -1 on error
 */
int tlv_parse_request_question(const uint8_t *buffer, uint8_t *mode, 
                                uint8_t *question_index);

/**
 * Parse ANSWER_SUBMIT message
 * @param buffer Input buffer (after header)
 * @param question_id Output: question ID
 * @param answer_id Output: answer ID
 * @return 0 on success, -1 on error
 */
int tlv_parse_answer_submit(const uint8_t *buffer, uint16_t *question_id, 
                             uint8_t *answer_id);

/**
 * Parse ANSWER_RESULT message
 * @param buffer Input buffer (after header)
 * @param question_id Output: question ID
 * @param is_correct Output: correctness flag
 * @param correct_answer_id Output: correct answer ID
 * @param test_mode Output: test mode flag
 * @param questions_answered Output: questions answered count
 * @param correct_count Output: correct answers count
 * @return 0 on success, -1 on error
 */
int tlv_parse_answer_result(const uint8_t *buffer, uint16_t *question_id,
                             uint8_t *is_correct, uint8_t *correct_answer_id,
                             uint8_t *test_mode, uint8_t *questions_answered,
                             uint8_t *correct_count);

/**
 * Create SUBMIT_SCORE message
 * @param buffer Output buffer
 * @param score Score (0-10)
 * @param time_seconds Time in seconds
 * @return Length of message on success, -1 on error
 */
ssize_t tlv_create_submit_score(uint8_t *buffer, uint8_t score, uint32_t time_seconds);

/**
 * Parse SUBMIT_SCORE message
 * @param buffer Input buffer (after header)
 * @param score Output: score
 * @param time_seconds Output: time in seconds
 * @return 0 on success, -1 on error
 */
int tlv_parse_submit_score(const uint8_t *buffer, uint8_t *score, uint32_t *time_seconds);

/**
 * Create REQUEST_RANKING message
 * @param buffer Output buffer
 * @return Length of message on success, -1 on error
 */
ssize_t tlv_create_request_ranking(uint8_t *buffer);

/**
 * Create RANKING_DATA message
 * @param buffer Output buffer
 * @param count Number of entries
 * @param nicks Array of nicknames
 * @param scores Array of scores
 * @param times Array of times in seconds
 * @return Length of message on success, -1 on error
 */
ssize_t tlv_create_ranking_data(uint8_t *buffer, uint8_t count, 
                                 const char nicks[][MAX_NICK_LENGTH],
                                 const uint8_t *scores, const uint32_t *times);

/**
 * Parse RANKING_DATA message
 * @param buffer Input buffer (after header)
 * @param count Output: number of entries
 * @param nicks Output: array of nicknames
 * @param scores Output: array of scores
 * @param times Output: array of times
 * @return 0 on success, -1 on error
 */
int tlv_parse_ranking_data(const uint8_t *buffer, uint8_t *count,
                            char nicks[][MAX_NICK_LENGTH], 
                            uint8_t *scores, uint32_t *times);

/**
 * Create REQUEST_SERVER_INFO message
 * @param buffer Output buffer
 * @return Length of message on success, -1 on error
 */
ssize_t tlv_create_request_server_info(uint8_t *buffer);

/**
 * Create SERVER_INFO_DATA message
 * @param buffer Output buffer
 * @param uptime_seconds Server uptime in seconds
 * @param active_connections Number of active connections
 * @param total_connections Total connections since start
 * @param num_questions Number of questions in database
 * @param tests_completed Number of completed tests
 * @param questions_asked Total questions asked
 * @param avg_score Average score (0-10)
 * @param best_score Best score achieved
 * @param best_time Best time in seconds
 * @param best_player Nickname of best player
 * @param port Server port number
 * @return Length of message on success, -1 on error
 */
ssize_t tlv_create_server_info_data(uint8_t *buffer, uint32_t uptime_seconds,
                                     uint16_t active_connections, uint32_t total_connections,
                                     uint16_t num_questions, uint32_t tests_completed,
                                     uint32_t questions_asked, uint8_t avg_score,
                                     uint8_t best_score, uint32_t best_time,
                                     const char *best_player, uint16_t port);

/**
 * Parse SERVER_INFO_DATA message
 * @param buffer Input buffer (after header)
 * @param uptime_seconds Output: server uptime
 * @param active_connections Output: active connections
 * @param total_connections Output: total connections
 * @param num_questions Output: number of questions
 * @param tests_completed Output: tests completed
 * @param questions_asked Output: questions asked
 * @param avg_score Output: average score
 * @param best_score Output: best score
 * @param best_time Output: best time
 * @param best_player Output: best player nickname
 * @param port Output: server port
 * @return 0 on success, -1 on error
 */
int tlv_parse_server_info_data(const uint8_t *buffer, uint32_t *uptime_seconds,
                                uint16_t *active_connections, uint32_t *total_connections,
                                uint16_t *num_questions, uint32_t *tests_completed,
                                uint32_t *questions_asked, uint8_t *avg_score,
                                uint8_t *best_score, uint32_t *best_time,
                                char *best_player, uint16_t *port);

#endif // TLV_H
