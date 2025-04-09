#include <curses.h>
#include <string.h>
#include "menu.h"
#include "utils.h"


int print_title(WINDOW *win, char *filename) {

    char line[256];

    //Apro il file
    FILE *file = fopen(filename, "r");
    if(!file) return ERR;

    int start_y = 1, start_x = 11;  //Posizione 11 per centrare il titolo

    //Leggo dal file
    while(fgets(line, sizeof(line), file)) {
        // Rimuove il newline se presente
        line[strcspn(line, "\n")] = '\0';
        wattron(win, COLOR_PAIR(START_MENU_COLOR_PAIR));
        mvwprintw(win, start_y++, start_x, "%s", line);
        wattroff(win, COLOR_PAIR(START_MENU_COLOR_PAIR));
    }
    wrefresh(win);


    //Chiudo la risorsa
    fclose(file);
    return 0;
}

void draw_options(WINDOW *win, char *option[], int num_options, int start_y, int start_x, int options_y[]) {
    
    for (int i = 0; i < num_options; i++) {
        options_y[i] = start_y + i * 2;
        mvwprintw(win, options_y[i], start_x, "  %s", option[i]);  // "  " spazio per lasciare posto a "> "
    }
    wrefresh(win);
}

void highlight_option(WINDOW *win, int current, int previous, int options_y[], int x, char *option[]) {
    // Ripristina la precedente opzione (senza evidenziarla)
    mvwprintw(win, options_y[previous], x, "  %s", option[previous]);

    // Evidenzia quella nuova
    wattron(win, COLOR_PAIR(START_MENU_COLOR_PAIR) | A_BOLD);
    mvwprintw(win, options_y[current], x, "> %s", option[current]);
    wattroff(win, COLOR_PAIR(START_MENU_COLOR_PAIR) | A_BOLD);

    wrefresh(win);
}

int start_menu(WINDOW *win) {
    noecho();
    keypad(win, true);

    char *title_path = "res/title.txt";

    char *option[] = {
        "NEW GAME",
        "HOW TO PLAY",
        "EXIT GAME"
    };

    int num_options = sizeof(option)/sizeof(char*);
    int input, selected = 0, previous = 0;
    int start_x, start_y;
    int options_y[num_options];

    if (print_title(win, title_path) == ERR) return ERR;

    getyx(win, start_y, start_x);   //Posizione del cursor dopo il titolo
    start_y += 3;   //Aggiungo margine sopra
    start_x = 8;    //Aggiungo margine a sx

    draw_options(win, option, num_options, start_y, start_x, options_y);
    highlight_option(win, selected, selected, options_y, start_x, option);  //Evidenzio NEW GAME

    do {
        input = wgetch(win);
        previous = selected;    //Aggiorno l'opzione precedente a quella appena inserita

        switch (input) {
            case KEY_UP:
                if (selected > 0) selected--;
                break;
            case KEY_DOWN:
                if (selected < num_options - 1) selected++;
                break;
            default:
                continue;
        }

        if (selected != previous)
            highlight_option(win, selected, previous, options_y, start_x, option);

    } while (input != '\n');

    return selected;    //Ritorno l'opzione selezionata
}

