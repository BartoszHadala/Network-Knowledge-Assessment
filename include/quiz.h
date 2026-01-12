#ifndef QUIZ_H
#define QUIZ_H

#include <stdint.h>
#include <cjson/cJSON.h>

#define MAX_QUESTIONS 200
#define MAX_QUESTION_TEXT 1024
#define MAX_ANSWER_TEXT 256
#define MAX_ANSWERS_PER_Q 4

typedef struct {
    int id;
    char pytanie[MAX_QUESTION_TEXT];
    char odpowiedzi[MAX_ANSWERS_PER_Q][MAX_ANSWER_TEXT];
    int num_odpowiedzi;
    int poprawna;  // 1-based index (from JSON)
} Question;

typedef struct {
    Question questions[MAX_QUESTIONS];
    int count;
} QuizDatabase;

/**
 * Load questions from JSON file
 * @param db Quiz database to fill
 * @param filepath Path to questions.json
 * @return 0 on success, -1 on error
 */
int quiz_load_questions(QuizDatabase *db, const char *filepath);

/**
 * Get random question from database
 * @param db Quiz database
 * @return Pointer to random question, NULL if database empty
 */
Question* quiz_get_random_question(QuizDatabase *db);

/**
 * Get question by ID
 * @param db Quiz database
 * @param id Question ID
 * @return Pointer to question, NULL if not found
 */
Question* quiz_get_question_by_id(QuizDatabase *db, int id);

/**
 * Check if answer is correct
 * @param question Question to check
 * @param answer_index 0-based answer index
 * @return 1 if correct, 0 if incorrect
 */
int quiz_check_answer(Question *question, int answer_index);

#endif // QUIZ_H
