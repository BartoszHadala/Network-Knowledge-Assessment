#include "client_utils.h"
#include "tlv.h"
#include "menu.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define BUFFER_SIZE 4096

// Send LOGIN_REQUEST and receive LOGIN_RESPONSE
int client_login(int sockfd, const char *nick) {
    uint8_t buffer[BUFFER_SIZE];
    
    // Create and send LOGIN_REQUEST
    ssize_t len = tlv_create_login_request(buffer, nick);
    if ( len < 0 ) {
        fprintf(stderr, "Failed to create login request\n");
        return -1;
    }
    
    if ( send(sockfd, buffer, len, 0) != len ) {
        fprintf(stderr, "Failed to send login request: %s\n", strerror(errno));
        return -1;
    }
    
    // Receive LOGIN_RESPONSE
    ssize_t received = recv(sockfd, buffer, sizeof(buffer), 0);
    if ( received <= 0 ) {
        fprintf(stderr, "Failed to receive login response: %s\n", received == 0 ? "Connection closed" : strerror(errno));
        return -1;
    }
    
    // Parse response
    uint16_t type, length;
    if ( tlv_parse_header(buffer, &type, &length) < 0 || type != TLV_LOGIN_RESPONSE ) {
        fprintf(stderr, "Invalid response from server\n");
        return -1;
    }
    
    uint8_t status;
    char message[MAX_MESSAGE_LENGTH + 1];
    if ( tlv_parse_login_response(buffer + 4, &status, message, sizeof(message)) < 0 ) {
        fprintf(stderr, "Failed to parse login response\n");
        return -1;
    }
    
    // Check status
    if ( status != LOGIN_SUCCESS ) {
        fprintf(stderr, "✗ Login failed: %s\n", message);
        return -1;
    }
    
    printf("Status: %s\n", message);
    return 0;
}

// Request a question from server
int client_request_question(int sockfd, uint8_t mode, uint8_t question_index) {
    uint8_t buffer[BUFFER_SIZE];
    
    // Create and send REQUEST_QUESTION
    ssize_t len = tlv_create_request_question(buffer, mode, question_index);
    if ( len < 0 ) {
        fprintf(stderr, "Failed to create question request\n");
        return -1;
    }
    
    if ( send(sockfd, buffer, len, 0) != len ) {
        fprintf(stderr, "Failed to send question request: %s\n", strerror(errno));
        return -1;
    }
    
    return 0;
}

// Submit an answer to server
int client_submit_answer(int sockfd, uint16_t question_id, uint8_t answer_id) {
    uint8_t buffer[BUFFER_SIZE];
    
    // Create and send ANSWER_SUBMIT
    ssize_t len = tlv_create_answer_submit(buffer, question_id, answer_id);
    if ( len < 0 ) {
        fprintf(stderr, "Failed to create answer submit\n");
        return -1;
    }
    
    if ( send(sockfd, buffer, len, 0) != len ) {
        fprintf(stderr, "Failed to send answer: %s\n", strerror(errno));
        return -1;
    }
    
    // Receive ANSWER_RESULT
    ssize_t received = recv(sockfd, buffer, sizeof(buffer), 0);
    if (received <= 0) {
        fprintf(stderr, "Failed to receive answer result: %s\n", received == 0 ? "Connection closed" : strerror(errno));
        return -1;
    }
    
    // Parse result
    uint16_t type, length;
    if ( tlv_parse_header(buffer, &type, &length) < 0 || type != TLV_ANSWER_RESULT ) {
        fprintf(stderr, "Invalid response from server\n");
        return -1;
    }
    
    uint16_t qid;
    uint8_t is_correct, correct_answer_id, test_mode, questions_answered, correct_count;
    if ( tlv_parse_answer_result(buffer + 4, &qid, &is_correct, &correct_answer_id,
                                 &test_mode, &questions_answered, &correct_count) < 0 ) {
        fprintf(stderr, "Failed to parse answer result\n");
        return -1;
    }
    
    // Display result
    if ( is_correct ) {
        printf("✓ Correct answer!\n");
    } else {
        printf("✗ Wrong answer. Correct answer was: %c\n", 'A' + correct_answer_id);
    }
    
    if ( test_mode ) {
        printf("Progress: %d/%d questions, %d correct\n", 
               questions_answered, 10, correct_count);
    }
    
    return is_correct ? 1 : 0;
}

// Receive and handle a complete question flow
int client_handle_single_question(int sockfd) {
    uint8_t buffer[BUFFER_SIZE];
    
    // Receive QUESTION_DATA
    ssize_t received = recv(sockfd, buffer, sizeof(buffer), 0);
    if (received <= 0) {
        fprintf(stderr, "Failed to receive question: %s\n", received == 0 ? "Connection closed" : strerror(errno));
        return -1;
    }
    
    // Parse TLV header
    uint16_t type, length;
    if ( tlv_parse_header(buffer, &type, &length) < 0 || type != TLV_QUESTION_DATA ) {
        fprintf(stderr, "Invalid response from server (expected QUESTION_DATA)\n");
        return -1;
    }
    
    // Parse QUESTION_DATA manually
    size_t offset = 4;  // After header
    
    // question_id (2 bytes)
    uint16_t question_id = ntohs(*(uint16_t*)(buffer + offset));
    offset += 2;
    
    // num_answers (1 byte)
    uint8_t num_answers = buffer[offset++];
    
    // question_length (2 bytes)
    uint16_t question_len = ntohs(*(uint16_t*)(buffer + offset));
    offset += 2;
    
    // question_text
    char question_text[MAX_QUESTION_LENGTH + 1];
    if (question_len > MAX_QUESTION_LENGTH) {
        fprintf(stderr, "Question text too long\n");
        return -1;
    }
    memcpy(question_text, buffer + offset, question_len);
    question_text[question_len] = '\0';
    offset += question_len;
    
    // Display question
    printf("\n");
    printf("═══════════════════════════════════════════════════════════════════════\n");
    printf("  QUESTION #%d\n", question_id);
    printf("═══════════════════════════════════════════════════════════════════════\n");
    printf("\n");
    
    // Word wrap question text at 70 chars
    const int max_width = 70;
    char *text_ptr = question_text;
    
    while (*text_ptr) {
        int len = strlen(text_ptr);
        
        if (len <= max_width) {
            printf("  %s\n", text_ptr);
            break;
        }
        
        int break_pos = max_width;
        while (break_pos > 0 && text_ptr[break_pos] != ' ') {
            break_pos--;
        }
        
        if (break_pos == 0) {
            break_pos = max_width;
        }
        
        char line[max_width + 1];
        strncpy(line, text_ptr, break_pos);
        line[break_pos] = '\0';
        
        int line_len = strlen(line);
        while (line_len > 0 && line[line_len - 1] == ' ') {
            line[--line_len] = '\0';
        }
        
        printf("  %s\n", line);
        
        text_ptr += break_pos;
        while (*text_ptr == ' ') text_ptr++;
    }
    
    printf("\n");
    printf("───────────────────────────────────────────────────────────────────────\n");
    
    // Parse and display answers
    char answers[MAX_ANSWERS][MAX_ANSWER_LENGTH + 1];
    for (int i = 0; i < num_answers; i++) {
        // answer_id (1 byte)
        uint8_t ans_id = buffer[offset++];
        
        // answer_length (2 bytes)
        uint16_t ans_len = ntohs(*(uint16_t*)(buffer + offset));
        offset += 2;
        
        // answer_text
        if (ans_len > MAX_ANSWER_LENGTH) {
            fprintf(stderr, "Answer text too long\n");
            return -1;
        }
        memcpy(answers[ans_id], buffer + offset, ans_len);
        answers[ans_id][ans_len] = '\0';
        offset += ans_len;
        
        // Display answer with wrapping
        char *ans_ptr = answers[ans_id];
        int first_line = 1;
        const int ans_width = 66;
        
        while (*ans_ptr) {
            int len = strlen(ans_ptr);
            
            if (len <= ans_width) {
                if (first_line) {
                    printf("  %c)  %s\n", 'A' + ans_id, ans_ptr);
                } else {
                    printf("      %s\n", ans_ptr);
                }
                break;
            }
            
            int break_pos = ans_width;
            while (break_pos > 0 && ans_ptr[break_pos] != ' ') {
                break_pos--;
            }
            
            if (break_pos == 0) {
                break_pos = ans_width;
            }
            
            char line[ans_width + 1];
            strncpy(line, ans_ptr, break_pos);
            line[break_pos] = '\0';
            
            int line_len = strlen(line);
            while (line_len > 0 && line[line_len - 1] == ' ') {
                line[--line_len] = '\0';
            }
            
            if (first_line) {
                printf("  %c)  %s\n", 'A' + ans_id, line);
                first_line = 0;
            } else {
                printf("      %s\n", line);
            }
            
            ans_ptr += break_pos;
            while (*ans_ptr == ' ') ans_ptr++;
        }
    }
    
    printf("═══════════════════════════════════════════════════════════════════════\n");
    
    // Get user answer
    char answer_char;
    int answer_index;
    
    while (1) {
        printf("\nYour answer (A-D): ");
        if (scanf(" %c", &answer_char) != 1) {
            menu_clear_input_buffer();
            printf("Invalid input. Try again.\n");
            continue;
        }
        menu_clear_input_buffer();
        
        // Convert to uppercase
        if (answer_char >= 'a' && answer_char <= 'z') {
            answer_char = answer_char - 'a' + 'A';
        }
        
        // Validate
        answer_index = answer_char - 'A';
        if (answer_index >= 0 && answer_index < num_answers) {
            break;
        }
        
        printf("Invalid choice. Please select A-%c.\n", 'A' + num_answers - 1);
    }
    
    // Submit answer
    return client_submit_answer(sockfd, question_id, answer_index);
}

// Request and display rankings
int client_request_ranking(int sockfd) {
    uint8_t buffer[BUFFER_SIZE];
    
    // Create and send REQUEST_RANKING
    ssize_t len = tlv_create_request_ranking(buffer);
    if (len < 0) {
        fprintf(stderr, "Failed to create ranking request\n");
        return -1;
    }
    
    if (send(sockfd, buffer, len, 0) != len) {
        fprintf(stderr, "Failed to send ranking request: %s\n", strerror(errno));
        return -1;
    }
    
    // Receive RANKING_DATA
    ssize_t received = recv(sockfd, buffer, sizeof(buffer), 0);
    if (received <= 0) {
        fprintf(stderr, "Failed to receive ranking data: %s\n", 
                received == 0 ? "Connection closed" : strerror(errno));
        return -1;
    }
    
    // Parse header
    uint16_t type, length;
    if (tlv_parse_header(buffer, &type, &length) < 0 || type != TLV_RANKING_DATA) {
        fprintf(stderr, "Invalid response from server\n");
        return -1;
    }
    
    // Parse ranking data
    uint8_t count;
    char nicks[MAX_RANKINGS][MAX_NICK_LENGTH];
    uint8_t scores[MAX_RANKINGS];
    uint32_t times[MAX_RANKINGS];
    
    if (tlv_parse_ranking_data(buffer + 4, &count, nicks, scores, times) < 0) {
        fprintf(stderr, "Failed to parse ranking data\n");
        return -1;
    }
    
    // Display rankings
    printf("\n╔════════════════════════════════════════════════════════════════╗\n");
    printf("║                       TOP PLAYERS RANKING                      ║\n");
    printf("╠════════════════════════════════════════════════════════════════╣\n");
    
    if (count == 0) {
        printf("║  No scores recorded yet.                                       ║\n");
    } else {
        printf("║  Rank  Nick                Score    Time                      ║\n");
        printf("╠════════════════════════════════════════════════════════════════╣\n");
        
        for (int i = 0; i < count; i++) {
            int minutes = times[i] / 60;
            int seconds = times[i] % 60;
            
            printf("║  %-5d %-19s %2d/10    %02d:%02d                      ║\n", 
                   i + 1, nicks[i], scores[i], minutes, seconds);
        }
    }
    
    printf("╚════════════════════════════════════════════════════════════════╝\n");
    
    return 0;
}

// Request and display server information
int client_request_server_info(int sockfd) {
    uint8_t buffer[BUFFER_SIZE];
    
    // Create and send REQUEST_SERVER_INFO
    ssize_t len = tlv_create_request_server_info(buffer);
    if (len < 0) {
        fprintf(stderr, "Failed to create server info request\n");
        return -1;
    }
    
    if (send(sockfd, buffer, len, 0) != len) {
        fprintf(stderr, "Failed to send server info request: %s\n", strerror(errno));
        return -1;
    }
    
    // Receive SERVER_INFO_DATA
    ssize_t received = recv(sockfd, buffer, sizeof(buffer), 0);
    if (received <= 0) {
        fprintf(stderr, "Failed to receive server info: %s\n", 
                received == 0 ? "Connection closed" : strerror(errno));
        return -1;
    }
    
    // Parse header
    uint16_t type, length;
    if (tlv_parse_header(buffer, &type, &length) < 0 || type != TLV_SERVER_INFO_DATA) {
        fprintf(stderr, "Invalid response from server\n");
        return -1;
    }
    
    // Parse server info
    uint32_t uptime_seconds;
    uint16_t active_connections, num_questions, port;
    uint32_t total_connections, tests_completed, questions_asked;
    uint8_t avg_score, best_score;
    uint32_t best_time;
    char best_player[MAX_NICK_LENGTH];
    
    if (tlv_parse_server_info_data(buffer + 4, &uptime_seconds,
                                    &active_connections, &total_connections,
                                    &num_questions, &tests_completed,
                                    &questions_asked, &avg_score,
                                    &best_score, &best_time,
                                    best_player, &port) < 0) {
        fprintf(stderr, "Failed to parse server info\n");
        return -1;
    }
    
    // Calculate uptime components
    uint32_t days = uptime_seconds / 86400;
    uint32_t hours = (uptime_seconds % 86400) / 3600;
    uint32_t minutes = (uptime_seconds % 3600) / 60;
    
    // Calculate best time components
    uint32_t best_minutes = best_time / 60;
    uint32_t best_secs = best_time % 60;
    
    // Display server information
    printf("\n╔════════════════════════════════════════════════════════════════╗\n");
    printf("║                     SERVER INFORMATION                         ║\n");
    printf("╠════════════════════════════════════════════════════════════════╣\n");
    printf("║  Server Port:           %-6d                                 ║\n", port);
    
    if (days > 0) {
        printf("║  Uptime:                %dd %02dh %02dm                            ║\n", days, hours, minutes);
    } else if (hours > 0) {
        printf("║  Uptime:                %dh %02dm                                ║\n", hours, minutes);
    } else {
        printf("║  Uptime:                %dm                                     ║\n", minutes);
    }
    
    printf("║                                                                ║\n");
    printf("║  Active Connections:    %-6d                                 ║\n", active_connections);
    printf("║  Total Connections:     %-6d                                 ║\n", total_connections);
    printf("║                                                                ║\n");
    printf("║  Questions Database:    %-6d questions                       ║\n", num_questions);
    printf("║  Tests Completed:       %-6d                                 ║\n", tests_completed);
    printf("║  Questions Asked:       %-6d                                 ║\n", questions_asked);
    
    if (tests_completed > 0) {
        printf("║  Average Score:         %d/10                                   ║\n", avg_score);
    } else {
        printf("║  Average Score:         N/A                                    ║\n");
    }
    
    printf("║                                                                ║\n");
    
    if (best_score > 0) {
        printf("║  Best Score:            %-19s %d/10 (%02d:%02d)       ║\n", 
               best_player, best_score, best_minutes, best_secs);
    } else {
        printf("║  Best Score:            N/A                                    ║\n");
    }
    
    printf("║  Rankings Entries:      %-6d                                 ║\n", tests_completed);
    printf("╚════════════════════════════════════════════════════════════════╝\n");
    
    return 0;
}
