#include "quiz.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <syslog.h>

// Load questions from JSON file
int quiz_load_questions(QuizDatabase *db, const char *filepath) {
    FILE *fp = fopen(filepath, "r");
    if (!fp) {
        syslog(LOG_ERR, "Failed to open questions file: %s", filepath);
        return -1;
    }

    // Get file size needed for memory allocation

    // Moves the pointer to the end of the file
    fseek(fp, 0, SEEK_END);
    // Getting the size in bytes
    long fsize = ftell(fp);
    // Moves the pointer to the begining of the file
    fseek(fp, 0, SEEK_SET);

    // Read file
    char *json_str = malloc(fsize + 1);
    if (!json_str) {
        fclose(fp);
        return -1;
    }

    // Load all bytes
    fread(json_str, 1, fsize, fp);
    json_str[fsize] = '\0';
    fclose(fp);

    // Using the CJSON library to convert JSON text into an object tree
    // The first element of the array is an object containing the fields id, question, answers, correct
    cJSON *root = cJSON_Parse(json_str);
    free(json_str);

    if (!root) {
        syslog(LOG_ERR, "JSON parse error");
        return -1;
    }

    // Validation - root should be array
    if (!cJSON_IsArray(root)) {
        syslog(LOG_ERR, "JSON root is not an array");
        cJSON_Delete(root);
        return -1;
    }

    db->count = 0;
    int array_size = cJSON_GetArraySize(root);

    for (int i = 0; i < array_size && db->count < MAX_QUESTIONS; i++) {
        cJSON *item = cJSON_GetArrayItem(root, i);
        if (!cJSON_IsObject(item)) continue;

        // Pointer in the questions array where the new question is saved
        Question *q = &db->questions[db->count];

        // Parse ID
        cJSON *id_json = cJSON_GetObjectItem(item, "id");
        if (!cJSON_IsNumber(id_json)) continue;
        q->id = id_json->valueint;

        // Parse question
        cJSON *pytanie_json = cJSON_GetObjectItem(item, "pytanie");
        if (!cJSON_IsString(pytanie_json)) continue;
        strncpy(q->pytanie, pytanie_json->valuestring, MAX_QUESTION_TEXT - 1);
        q->pytanie[MAX_QUESTION_TEXT - 1] = '\0';

        // Parse answers
        cJSON *odpowiedzi_json = cJSON_GetObjectItem(item, "odpowiedzi");
        if (!cJSON_IsArray(odpowiedzi_json)) continue;

        int ans_count = cJSON_GetArraySize(odpowiedzi_json);
        q->num_odpowiedzi = (ans_count > MAX_ANSWERS_PER_Q) ? MAX_ANSWERS_PER_Q : ans_count;

        for (int j = 0; j < q->num_odpowiedzi; j++) {
            cJSON *ans = cJSON_GetArrayItem(odpowiedzi_json, j);
            if (cJSON_IsString(ans)) {
                strncpy(q->odpowiedzi[j], ans->valuestring, MAX_ANSWER_TEXT - 1);
                q->odpowiedzi[j][MAX_ANSWER_TEXT - 1] = '\0';
            } else {
                q->odpowiedzi[j][0] = '\0';
            }
        }

        // Parse correct answer
        cJSON *poprawna_json = cJSON_GetObjectItem(item, "poprawna");
        if (!cJSON_IsNumber(poprawna_json)) continue;
        q->poprawna = poprawna_json->valueint;

        // Index increment
        db->count++;
    }

    cJSON_Delete(root);
    syslog(LOG_INFO, "Loaded %d questions from %s", db->count, filepath);
    
    // Seed random for quiz_get_random_question
    srand(time(NULL));
    
    return 0;
}

// Get random question
Question* quiz_get_random_question(QuizDatabase *db) {
    if (db->count == 0) {
        return NULL;
    }
    int index = rand() % db->count;
    return &db->questions[index];
}

// Get question by ID
Question* quiz_get_question_by_id(QuizDatabase *db, int id) {
    for (int i = 0; i < db->count; i++) {
        if (db->questions[i].id == id) {
            return &db->questions[i];
        }
    }
    return NULL;
}

// Check if answer is correct
int quiz_check_answer(Question *question, int answer_index) {
    return (answer_index == (question->poprawna - 1));
}
