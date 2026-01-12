#include "menu.h"

// Display application banner
void menu_display_banner(void) {
    printf("\n");
    printf("╔════════════════════════════════════════════╗\n");
    printf("║         IT EXAM SYSTEM APPLICATION         ║\n");
    printf("╚════════════════════════════════════════════╝\n");
    printf("\n");
}

// Clear input buffer
void menu_clear_input_buffer(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF);
}

// Display menu and get user choice
int menu_display_and_get_choice(void) {
    int choice;
    
    printf("\n");
    printf("╔════════════════════════════════════════════╗\n");
    printf("║         IT EXAM SYSTEM - MAIN MENU         ║\n");
    printf("╠════════════════════════════════════════════╣\n");
    printf("║  1. Training - Random question             ║\n");
    printf("║  2. Knowledge test - Set of 10 questions   ║\n");
    printf("║  3. Ranking / User statistics              ║\n");
    printf("║  4. Server information                     ║\n");
    printf("║  5. Exit                                   ║\n");
    printf("╚════════════════════════════════════════════╝\n");
    printf("\nEnter your choice (1-5): ");
    
    if (scanf("%d", &choice) != 1) {
        menu_clear_input_buffer();
        return -1;  // Invalid input
    }
    
    menu_clear_input_buffer();  // Clear remaining newline
    
    if (choice < 1 || choice > 5) {
        return -1;  // Out of range
    }
    
    return choice;
}
