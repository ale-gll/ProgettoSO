/*
    Questo file contiene le funzioni relative al menu di gioco
*/

#ifndef MENU_H
#define MENU_H

#define MAX_TITLE_LENGTH 38


typedef enum { NEW_GAME, HOW_TO_PLAY, EXIT_GAME } StartMenuOptions;


int start_menu(WINDOW *win);

void print_title(WINDOW *win, const char *filename);

void draw_options(WINDOW *win, char *option[], int num_options, int start_y, int start_x, int options_y[]);

void highlight_option(WINDOW *win, int current, int previous, int options_y[], int x, char *option[]);

void print_how_to_play(WINDOW *win, const char *filename);

#endif