#ifndef MENU_H
#define MENU_H

#include <stdio.h>

/**
 * Display application banner (title/logo)
 */
void menu_display_banner(void);

/**
 * Display main menu and get user choice
 * @return Selected option (1-7), or -1 on error
 */
int menu_display_and_get_choice(void);

/**
 * Clear input buffer (for invalid input handling)
 */
void menu_clear_input_buffer(void);

#endif // MENU_H
