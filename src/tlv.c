#include "tlv.h"

// Create LOGIN_REQUEST message
ssize_t tlv_create_login_request(uint8_t *buffer, const char *nick) {
    // Header access, buffer projection onto structure
    struct tlv_header *header = (struct tlv_header *)buffer;
    uint8_t nick_len = strlen(nick);
    
    // Nick length validation
    if (nick_len > MAX_NICK_LENGTH) {
        return -1;
    }
    
    header->type = htons(TLV_LOGIN_REQUEST);
    header->length = htons(1 + nick_len);  // 1 byte for length + nick
    
    buffer[4] = nick_len;  // After header
    memcpy(buffer + 5, nick, nick_len);
    
    return 4 + 1 + nick_len;  // header + length byte + nick
}

// Create LOGIN_RESPONSE message
ssize_t tlv_create_login_response(uint8_t *buffer, uint8_t status, const char *message) {
    // Header access, buffer projection onto structure
    struct tlv_header *header = (struct tlv_header *)buffer;
    uint8_t msg_len = strlen(message);
    
    // Message length validation
    if (msg_len > MAX_MESSAGE_LENGTH) {
        return -1;
    }
    
    header->type = htons(TLV_LOGIN_RESPONSE);
    header->length = htons(2 + msg_len);  // 1 status + 1 length + message
    
    buffer[4] = status;
    buffer[5] = msg_len;
    memcpy(buffer + 6, message, msg_len);
    
    return 4 + 2 + msg_len;
}

// Create REQUEST_QUESTION message
ssize_t tlv_create_request_question(uint8_t *buffer, uint8_t mode, uint8_t question_index) {
    struct tlv_header *header = (struct tlv_header *)buffer;
    
    header->type = htons(TLV_REQUEST_QUESTION);
    header->length = htons(2);  // mode + question_index
    
    buffer[4] = mode;
    buffer[5] = question_index;
    
    return 6;  // header (4) + 2 bytes data
}

// Create QUESTION_DATA message
ssize_t tlv_create_question_data(uint8_t *buffer, uint16_t question_id,
                                  const char *question_text,
                                  const char **answers, uint8_t num_answers) {
    struct tlv_header *header = (struct tlv_header *)buffer;
    
    if (num_answers > MAX_ANSWERS || num_answers == 0) {
        return -1;
    }
    
    uint16_t question_len = strlen(question_text);
    if (question_len > MAX_QUESTION_LENGTH) {
        return -1;
    }
    
    // Calculate total length needed
    uint16_t total_value_len = 2 + 1 + 2;  // question_id + num_answers + question_length
    total_value_len += question_len;  // question text
    
    // Add answer lengths
    for (int i = 0; i < num_answers; i++) {
        uint16_t ans_len = strlen(answers[i]);
        if (ans_len > MAX_ANSWER_LENGTH) {
            return -1;
        }
        total_value_len += 1 + 2 + ans_len;  // answer_id + answer_length + answer_text
    }
    
    // Write header
    header->type = htons(TLV_QUESTION_DATA);
    header->length = htons(total_value_len);
    
    // Write body
    size_t offset = 4;
    
    // question_id (2 bytes)
    uint16_t *qid = (uint16_t *)(buffer + offset);
    *qid = htons(question_id);
    offset += 2;
    
    // num_answers (1 byte)
    buffer[offset++] = num_answers;
    
    // question_length (2 bytes)
    uint16_t *qlen = (uint16_t *)(buffer + offset);
    *qlen = htons(question_len);
    offset += 2;
    
    // question_text
    memcpy(buffer + offset, question_text, question_len);
    offset += question_len;
    
    // answers
    for (int i = 0; i < num_answers; i++) {
        uint16_t ans_len = strlen(answers[i]);
        
        // answer_id (1 byte)
        buffer[offset++] = i;
        
        // answer_length (2 bytes)
        uint16_t *alen = (uint16_t *)(buffer + offset);
        *alen = htons(ans_len);
        offset += 2;
        
        // answer_text
        memcpy(buffer + offset, answers[i], ans_len);
        offset += ans_len;
    }
    
    return offset;
}

// Create ANSWER_SUBMIT message
ssize_t tlv_create_answer_submit(uint8_t *buffer, uint16_t question_id, uint8_t answer_id) {
    struct tlv_header *header = (struct tlv_header *)buffer;
    
    header->type = htons(TLV_ANSWER_SUBMIT);
    header->length = htons(3);  // 2 bytes question_id + 1 byte answer_id
    
    uint16_t *qid = (uint16_t *)(buffer + 4);
    *qid = htons(question_id);
    buffer[6] = answer_id;
    
    return 7;  // header (4) + 3 bytes data
}

// Create ANSWER_RESULT message
ssize_t tlv_create_answer_result(uint8_t *buffer, uint16_t question_id,
                                  uint8_t is_correct, uint8_t correct_answer_id,
                                  uint8_t test_mode, uint8_t questions_answered,
                                  uint8_t correct_count) {
    struct tlv_header *header = (struct tlv_header *)buffer;
    
    header->type = htons(TLV_ANSWER_RESULT);
    header->length = htons(7);  // 2 + 5 bytes
    
    uint16_t *qid = (uint16_t *)(buffer + 4);
    *qid = htons(question_id);
    buffer[6] = is_correct;
    buffer[7] = correct_answer_id;
    buffer[8] = test_mode;
    buffer[9] = questions_answered;
    buffer[10] = correct_count;
    
    return 11;  // header (4) + 7 bytes data
}

// Parse TLV header
int tlv_parse_header(const uint8_t *buffer, uint16_t *type, uint16_t *length) {
    // Casting a buffer to a structure
    const struct tlv_header *header = (const struct tlv_header *)buffer;
    
    *type = ntohs(header->type);
    *length = ntohs(header->length);
    
    return 0;
}

// Parse LOGIN_REQUEST
int tlv_parse_login_request(const uint8_t *buffer, char *nick, size_t nick_size) {
    // Read the nickname length
    uint8_t nick_len = buffer[0];
    
    if (nick_len >= nick_size || nick_len > MAX_NICK_LENGTH) {
        return -1;
    }
    
    // buffer +1 because it starts with the nickname lenght
    memcpy(nick, buffer + 1, nick_len);
    nick[nick_len] = '\0';
    
    return 0;
}

// Parse LOGIN_RESPONSE
int tlv_parse_login_response(const uint8_t *buffer, uint8_t *status,
                              char *message, size_t msg_size) {
    *status = buffer[0];
    uint8_t msg_len = buffer[1];
    
    if (msg_len >= msg_size || msg_len > MAX_MESSAGE_LENGTH) {
        return -1;
    }
    
    memcpy(message, buffer + 2, msg_len);
    message[msg_len] = '\0';
    
    return 0;
}

// Parse REQUEST_QUESTION
int tlv_parse_request_question(const uint8_t *buffer, uint8_t *mode,
                                uint8_t *question_index) {
    *mode = buffer[0];
    *question_index = buffer[1];
    
    return 0;
}

// Parse ANSWER_SUBMIT
int tlv_parse_answer_submit(const uint8_t *buffer, uint16_t *question_id,
                             uint8_t *answer_id) {
    const uint16_t *qid = (const uint16_t *)buffer;
    *question_id = ntohs(*qid);
    *answer_id = buffer[2];
    
    return 0;
}

// Parse ANSWER_RESULT
int tlv_parse_answer_result(const uint8_t *buffer, uint16_t *question_id,
                             uint8_t *is_correct, uint8_t *correct_answer_id,
                             uint8_t *test_mode, uint8_t *questions_answered,
                             uint8_t *correct_count) {
    const uint16_t *qid = (const uint16_t *)buffer;
    *question_id = ntohs(*qid);
    *is_correct = buffer[2];
    *correct_answer_id = buffer[3];
    *test_mode = buffer[4];
    *questions_answered = buffer[5];
    *correct_count = buffer[6];
    
    return 0;
}

// Create SUBMIT_SCORE message
ssize_t tlv_create_submit_score(uint8_t *buffer, uint8_t score, uint32_t time_seconds) {
    struct tlv_header *header = (struct tlv_header *)buffer;
    
    header->type = htons(TLV_SUBMIT_SCORE);
    header->length = htons(5);  // 1 byte score + 4 bytes time
    
    buffer[4] = score;
    uint32_t *time_ptr = (uint32_t *)(buffer + 5);
    *time_ptr = htonl(time_seconds);
    
    return 4 + 5;
}

// Parse SUBMIT_SCORE message
int tlv_parse_submit_score(const uint8_t *buffer, uint8_t *score, uint32_t *time_seconds) {
    *score = buffer[0];
    const uint32_t *time_ptr = (const uint32_t *)(buffer + 1);
    *time_seconds = ntohl(*time_ptr);
    
    return 0;
}

// Create REQUEST_RANKING message
ssize_t tlv_create_request_ranking(uint8_t *buffer) {
    struct tlv_header *header = (struct tlv_header *)buffer;
    
    header->type = htons(TLV_REQUEST_RANKING);
    header->length = htons(0);  // No payload
    
    return 4;
}

// Create RANKING_DATA message
ssize_t tlv_create_ranking_data(uint8_t *buffer, uint8_t count, 
                                 const char nicks[][MAX_NICK_LENGTH],
                                 const uint8_t *scores, const uint32_t *times) {
    struct tlv_header *header = (struct tlv_header *)buffer;
    
    size_t pos = 4;
    buffer[pos++] = count;  // Number of entries
    
    for (int i = 0; i < count; i++) {
        uint8_t nick_len = strlen(nicks[i]);
        buffer[pos++] = nick_len;
        memcpy(buffer + pos, nicks[i], nick_len);
        pos += nick_len;
        
        buffer[pos++] = scores[i];
        
        uint32_t *time_ptr = (uint32_t *)(buffer + pos);
        *time_ptr = htonl(times[i]);
        pos += 4;
    }
    
    header->type = htons(TLV_RANKING_DATA);
    header->length = htons(pos - 4);
    
    return pos;
}

// Parse RANKING_DATA message
int tlv_parse_ranking_data(const uint8_t *buffer, uint8_t *count,
                            char nicks[][MAX_NICK_LENGTH], 
                            uint8_t *scores, uint32_t *times) {
    size_t pos = 0;
    *count = buffer[pos++];
    
    for (int i = 0; i < *count; i++) {
        uint8_t nick_len = buffer[pos++];
        if (nick_len >= MAX_NICK_LENGTH) {
            return -1;
        }
        memcpy(nicks[i], buffer + pos, nick_len);
        nicks[i][nick_len] = '\0';
        pos += nick_len;
        
        scores[i] = buffer[pos++];
        
        const uint32_t *time_ptr = (const uint32_t *)(buffer + pos);
        times[i] = ntohl(*time_ptr);
        pos += 4;
    }
    
    return 0;
}

// Create REQUEST_SERVER_INFO message
ssize_t tlv_create_request_server_info(uint8_t *buffer) {
    struct tlv_header *header = (struct tlv_header *)buffer;
    
    header->type = htons(TLV_REQUEST_SERVER_INFO);
    header->length = htons(0);  // No payload
    
    return 4;
}

// Create SERVER_INFO_DATA message
ssize_t tlv_create_server_info_data(uint8_t *buffer, uint32_t uptime_seconds,
                                     uint16_t active_connections, uint32_t total_connections,
                                     uint16_t num_questions, uint32_t tests_completed,
                                     uint32_t questions_asked, uint8_t avg_score,
                                     uint8_t best_score, uint32_t best_time,
                                     const char *best_player, uint16_t port) {
    struct tlv_header *header = (struct tlv_header *)buffer;
    size_t pos = 4;
    
    // Uptime (4 bytes)
    uint32_t *uptime_ptr = (uint32_t *)(buffer + pos);
    *uptime_ptr = htonl(uptime_seconds);
    pos += 4;
    
    // Active connections (2 bytes)
    uint16_t *active_ptr = (uint16_t *)(buffer + pos);
    *active_ptr = htons(active_connections);
    pos += 2;
    
    // Total connections (4 bytes)
    uint32_t *total_ptr = (uint32_t *)(buffer + pos);
    *total_ptr = htonl(total_connections);
    pos += 4;
    
    // Num questions (2 bytes)
    uint16_t *questions_ptr = (uint16_t *)(buffer + pos);
    *questions_ptr = htons(num_questions);
    pos += 2;
    
    // Tests completed (4 bytes)
    uint32_t *tests_ptr = (uint32_t *)(buffer + pos);
    *tests_ptr = htonl(tests_completed);
    pos += 4;
    
    // Questions asked (4 bytes)
    uint32_t *asked_ptr = (uint32_t *)(buffer + pos);
    *asked_ptr = htonl(questions_asked);
    pos += 4;
    
    // Average score (1 byte)
    buffer[pos++] = avg_score;
    
    // Best score (1 byte)
    buffer[pos++] = best_score;
    
    // Best time (4 bytes)
    uint32_t *best_time_ptr = (uint32_t *)(buffer + pos);
    *best_time_ptr = htonl(best_time);
    pos += 4;
    
    // Best player (length + string)
    uint8_t player_len = strlen(best_player);
    if (player_len > MAX_NICK_LENGTH - 1) {
        player_len = MAX_NICK_LENGTH - 1;
    }
    buffer[pos++] = player_len;
    memcpy(buffer + pos, best_player, player_len);
    pos += player_len;
    
    // Port (2 bytes)
    uint16_t *port_ptr = (uint16_t *)(buffer + pos);
    *port_ptr = htons(port);
    pos += 2;
    
    header->type = htons(TLV_SERVER_INFO_DATA);
    header->length = htons(pos - 4);
    
    return pos;
}

// Parse SERVER_INFO_DATA message
int tlv_parse_server_info_data(const uint8_t *buffer, uint32_t *uptime_seconds,
                                uint16_t *active_connections, uint32_t *total_connections,
                                uint16_t *num_questions, uint32_t *tests_completed,
                                uint32_t *questions_asked, uint8_t *avg_score,
                                uint8_t *best_score, uint32_t *best_time,
                                char *best_player, uint16_t *port) {
    size_t pos = 0;
    
    // Uptime
    const uint32_t *uptime_ptr = (const uint32_t *)(buffer + pos);
    *uptime_seconds = ntohl(*uptime_ptr);
    pos += 4;
    
    // Active connections
    const uint16_t *active_ptr = (const uint16_t *)(buffer + pos);
    *active_connections = ntohs(*active_ptr);
    pos += 2;
    
    // Total connections
    const uint32_t *total_ptr = (const uint32_t *)(buffer + pos);
    *total_connections = ntohl(*total_ptr);
    pos += 4;
    
    // Num questions
    const uint16_t *questions_ptr = (const uint16_t *)(buffer + pos);
    *num_questions = ntohs(*questions_ptr);
    pos += 2;
    
    // Tests completed
    const uint32_t *tests_ptr = (const uint32_t *)(buffer + pos);
    *tests_completed = ntohl(*tests_ptr);
    pos += 4;
    
    // Questions asked
    const uint32_t *asked_ptr = (const uint32_t *)(buffer + pos);
    *questions_asked = ntohl(*asked_ptr);
    pos += 4;
    
    // Average score
    *avg_score = buffer[pos++];
    
    // Best score
    *best_score = buffer[pos++];
    
    // Best time
    const uint32_t *best_time_ptr = (const uint32_t *)(buffer + pos);
    *best_time = ntohl(*best_time_ptr);
    pos += 4;
    
    // Best player
    uint8_t player_len = buffer[pos++];
    if (player_len >= MAX_NICK_LENGTH) {
        return -1;
    }
    memcpy(best_player, buffer + pos, player_len);
    best_player[player_len] = '\0';
    pos += player_len;
    
    // Port
    const uint16_t *port_ptr = (const uint16_t *)(buffer + pos);
    *port = ntohs(*port_ptr);
    pos += 2;
    
    return 0;
}
