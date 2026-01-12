#ifndef SERVER_TYPES_H
#define SERVER_TYPES_H

#include <stdbool.h>
#include <stdint.h>
#include <time.h>

#define RBUF_SIZE 8192
#define WBUF_SIZE 8192

enum parse_state { READ_HEADER, READ_VALUE };

// Player score entry
struct score_entry {
    char nick[32];
    uint8_t score;
    uint32_t time_seconds;
};

// Server statistics
struct server_stats {
    time_t start_time;           // Server start timestamp
    uint32_t total_connections;  // Total connections since start
    uint32_t active_connections; // Current active connections
    uint32_t tests_completed;    // Number of completed tests
    uint32_t questions_asked;    // Total questions asked
    uint32_t total_score;        // Sum of all scores for average
    uint8_t best_score;          // Best score achieved
    uint32_t best_time;          // Best time (for perfect or high score)
    char best_player[32];        // Nick of best player
};

struct connection {
    int fd;
    bool is_listener;
    uint8_t rbuf[RBUF_SIZE];
    size_t rlen; // bytes in buffer
    size_t rpos;
    uint8_t wbuf[WBUF_SIZE];
    size_t wlen;
    size_t wpos;
    enum parse_state state;
    uint16_t tlv_type;
    uint16_t tlv_len;
};

#endif // SERVER_TYPES_H
