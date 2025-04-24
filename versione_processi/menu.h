/*
    Questo file contiene le funzioni relative al menu di gioco
*/

#ifndef MENU_H
#define MENU_H

#define MAX_TITLE_LENGTH 38


typedef enum { NEW_GAME, HOW_TO_PLAY, EXIT_GAME } StartMenuOptions;

/*
    Restituisce:
    > 0     -> l'utente crea una nuova partita
    > ERR   -> l'utente vuole uscire dal programma
*/
int start_menu(WINDOW *win);

/*
    Funzione per stampare il titolo e le istruzioni di gioco.
    Ritorna:
    > ERR   -> in caso di errore
    > 0     -> altrimenti

    Il nome del file è in sola lettura
*/
int print_title(WINDOW *win, const char *filename);

void draw_options(WINDOW *win, char *option[], int num_options, int start_y, int start_x, int options_y[]);

void highlight_option(WINDOW *win, int current, int previous, int options_y[], int x, char *option[]);

// void print_how_to_play(WINDOW *win);

#endif