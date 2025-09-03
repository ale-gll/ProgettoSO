#include <curses.h>
#include <string.h>
#include "menu.h"
#include "graphics.h"

void print_title(WINDOW *win, const char *filename) {
    char line[256];

    FILE *file = fopen(filename, "r");
    if(file != NULL) {   
        int start_y = 2, start_x = 15;

        //Leggo dal file
        while(fgets(line, sizeof(line), file)) {
            // Rimuove il newline se presente
            line[strcspn(line, "\n")] = '\0';
            wattron(win, COLOR_PAIR(START_MENU_COLOR_PAIR));
            mvwprintw(win, start_y++, start_x, "%s", line);
            wattroff(win, COLOR_PAIR(START_MENU_COLOR_PAIR));
        }
        wrefresh(win);
    }
    fclose(file);
}

void print_how_to_play(WINDOW *win, const char *filename) {
    box(win, 0, 0);
    char line[256];

    FILE *file = fopen(filename, "r");
    if(file != NULL) {
        int start_y = 1, start_x = 2;

        //Leggo dal file
        while(fgets(line, sizeof(line), file)) {
            //Rimuovo il newline
            line[strcspn(line, "\n")] = '\0';
            mvwprintw(win, start_y++, start_x, "%s", line);
        }
    }
    fclose(file);

    //Istruzione per tornare indietro
    char *instr = " Press ENTER.. ";
    int win_height = getmaxy(win);
    wattron(win, COLOR_PAIR(START_MENU_COLOR_PAIR));
    mvwprintw(win, win_height-1, 2, "%s", instr);
    wattroff(win, COLOR_PAIR(START_MENU_COLOR_PAIR));
    wrefresh(win);
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

    const char *title_path = "res/title.txt";

    char *option[] = {
        "NEW GAME",
        "HOW TO PLAY",
        "EXIT GAME"
    };

    int num_options = sizeof(option)/sizeof(char*);
    int input, selected = 0, previous = 0;
    int start_x, start_y;
    int options_y[num_options];

    print_title(win, title_path);

    getyx(win, start_y, start_x);   //Posizione del cursor dopo il titolo
    start_y += 4;   //Aggiungo margine sopra
    start_x = 13;    //Aggiungo margine a sx

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