#include <curses.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include "graphics.h"

//Sprite tane
char *burrows_sprite[] = {
    "|    ________     ________     ________     ________     ________    |",
    "|   |        |   |        |   |        |   |        |   |        |   |",
    "|   |________|   |________|   |________|   |________|   |________|   |"
};

//Sprite marciapiede/argine
char *walkable_sprite[] = {
    "**********************************************************************",
    "**********************************************************************"
};

//Sprite della rana spaventata (quando ha poche vite)
char *scared_frog_sprite[] = {
    "[O_O]",
    ";[ ];"
};

//Sprite della rana
char *frog_sprite[] = {
    "[^v^]",
    ";[ ];"
};

//Sprite del coccodrillo volto a sx
char *croc_sprite_sx[] = {
    " _|O      |",
    "|^________|"
};

//Sprite del coccodrillo volto a dx
char *croc_sprite_dx[] = {
    "|      O|_ ",
    "|________^|"
};


/* Funzioni di inizializzazione del campo di gioco */

void init_game_colors() {
    init_color(COLOR_BROWN, 245, 222, 179);

    //Inizializzo le coppie colore
    init_pair(FROG_COLOR_PAIR, COLOR_BLACK, COLOR_MAGENTA);    //Rana
    init_pair(CROC_COLOR_PAIR, COLOR_BLACK, COLOR_CYAN);   //Coccodrilli
    init_pair(WALKABLE_COLOR_PAIR, COLOR_BLACK, COLOR_GREEN);   //Marciapiede, argine
    init_pair(START_MENU_COLOR_PAIR, COLOR_GREEN, COLOR_BLACK); //Testo del menu
    init_pair(RIVER_COLOR_PAIR, COLOR_BLACK, COLOR_BLUE);   //Fiume
    init_pair(BURROW_COLOR_PAIR, COLOR_BLACK, COLOR_BROWN); //Tana
    init_pair(SCARED_FROG_COLOR_PAIR, COLOR_BLACK, COLOR_RED);    //Rana spaventata
    init_pair(STATS_COLOR_PAIR, COLOR_WHITE, COLOR_BLACK);  //Statistiche
    init_pair(GRANADE_COLOR_PAIR, COLOR_MAGENTA, COLOR_BLACK);  // Granate rana
    init_pair(ENEMY_PROJ_COLOR_PAIR, COLOR_WHITE, COLOR_BLACK);   // Proiettili nemici
}

void init_playground(WINDOW *pg_win, WINDOW *stats_win, int win_height, int win_width, 
    Object frog, Burrow burrows[5], Stats stats) 
{

    wbkgd(pg_win, COLOR_PAIR(RIVER_COLOR_PAIR));   //Sfondo blu

    draw_burrows(pg_win, 0, 0);    //Tane
    draw_walkable(pg_win, 3, 0);   //Argine in alto
    draw_walkable(pg_win, win_height-FROG_CROC_HEIGHT, 0);     //Marciapiede
    draw_frog(pg_win, frog, false);    //Rana al centro del marciapiede
    draw_stats(stats_win, stats);     //Disegno le statistiche

    wrefresh(pg_win);
    wrefresh(stats_win);
}


/* Funzioni di disegno*/

void draw_frog(WINDOW *win, Object frog, bool scared) {
    if(frog.type != OBJ_FROG) return;

    char **sprite = (scared) ? scared_frog_sprite : frog_sprite;
    int color = (scared) ? SCARED_FROG_COLOR_PAIR : FROG_COLOR_PAIR;

    for(int i = 0; i < FROG_CROC_HEIGHT; i++){
        wattron(win, COLOR_PAIR(color));
        mvwprintw(win, (frog.y + i), frog.x, "%s", sprite[i]);
        wattroff(win, COLOR_PAIR(color));
    }
}

void draw_croc(WINDOW *win, Object croc) {
    if (croc.type != OBJ_CROC) return;

    // Seleziona la sprite a seconda della direzione
    char **sprite = (croc.direction == DIR_LEFT) ? croc_sprite_sx : croc_sprite_dx;

    int win_width = getmaxx(win);  // larghezza della finestra

    wattron(win, COLOR_PAIR(CROC_COLOR_PAIR));

    for (int i = 0; i < FROG_CROC_HEIGHT; i++) {
        int row = croc.y + i;
        int start_x = croc.x;
        int sprite_len = CROC_WIDTH;

        int visible_start = 0;           // indice da cui iniziare nella stringa
        int visible_len = sprite_len;    // quanti caratteri disegnare

        // Se parte della sprite va fuori a sinistra
        if (start_x < 0) {
            visible_start = -start_x;
            visible_len -= visible_start;
            start_x = 0;
        }

        // Se parte della sprite va fuori a destra
        if (start_x + visible_len > win_width) {
            visible_len = win_width - start_x;
        }

        // Disegna solo se c'è una parte visibile
        if (visible_len > 0) {
            mvwaddnstr(win, row, start_x, &sprite[i][visible_start], visible_len);
        }
    }

    wattroff(win, COLOR_PAIR(CROC_COLOR_PAIR));
}

void draw_granade(WINDOW *win, Object granade) {
    wattron(win, COLOR_PAIR(GRANADE_COLOR_PAIR));
    mvwprintw(win, granade.y, granade.x, "O");
    wattroff(win, COLOR_PAIR(GRANADE_COLOR_PAIR));
}

void draw_enemy_projectile(WINDOW *win, Object proj) {
    wattron(win, COLOR_PAIR(ENEMY_PROJ_COLOR_PAIR));
    mvwprintw(win, proj.y, proj.x, "+");
    wattroff(win, COLOR_PAIR(ENEMY_PROJ_COLOR_PAIR));
}

void draw_walkable(WINDOW *win, int y, int x) {
    for(int i = 0; i < FROG_CROC_HEIGHT; i++) {
        wattron(win, COLOR_PAIR(WALKABLE_COLOR_PAIR));
        mvwprintw(win, y + i, x, "%s", walkable_sprite[i]);
        wattroff(win, COLOR_PAIR(WALKABLE_COLOR_PAIR));
    }
}

void draw_burrows(WINDOW *win, int y, int x) {
    int height = sizeof(burrows_sprite) / sizeof(char*);

    for(int i = 0; i < height; i++) {
        wattron(win, COLOR_PAIR(BURROW_COLOR_PAIR));
        mvwprintw(win, y + i, x, "%s", burrows_sprite[i]);
        wattroff(win, COLOR_PAIR(BURROW_COLOR_PAIR));
    }
}

void draw_stats(WINDOW *win, Stats stats) {
    int width = getmaxx(win);

    wattron(win, COLOR_PAIR(STATS_COLOR_PAIR));
    
    int section = width / 3;
    mvwprintw(win, 1, section / 2 - 4, "Time: %d", stats.time);
    mvwprintw(win, 1, section + section / 2 - 5, "Score: %d", stats.score);
    mvwprintw(win, 1, 2 * section + section / 2 - 5, "Lives: %d", stats.lives);

    wattroff(win, COLOR_PAIR(STATS_COLOR_PAIR));
}


/* Funzioni di cancellazione */

void remove_frog(WINDOW *win, int y, int x, bool on_grass) { 
    char *grass[] = {
        "*****",
        "*****"
    };

    char *empty[] = {
        "     ",
        "     "
    };
    
    for (int i = 0; i < FROG_CROC_HEIGHT; i++) {
        if (on_grass) {
            wattron(win, COLOR_PAIR(WALKABLE_COLOR_PAIR));
            mvwprintw(win, y + i, x, "%s", grass[i]); 
            wattroff(win, COLOR_PAIR(WALKABLE_COLOR_PAIR));
        } else {
            mvwprintw(win, y + i, x, "%s", empty[i]);
        }
    }
}

void remove_stats(WINDOW *win) {
    mvwhline(win, 1, 0, ' ', 70);
}

void remove_croc(WINDOW *win, int y, int x) {
    int win_width = getmaxx(win);

    for (int i = 0; i < FROG_CROC_HEIGHT; i++) {
        int start = x;
        int len = CROC_WIDTH;

        // Clipping sinistro
        if (start < 0) {
            len += start; // riduce lunghezza
            start = 0;
        }

        // Clipping destro
        if (start + len > win_width) {
            len = win_width - start;
        }

        if (len > 0) {
            // "           " deve avere >= CROC_WIDTH spazi
            mvwaddnstr(win, y + i, start, "           ", len);
        }
    }
}

void remove_granade(WINDOW *win, int y, int x, bool on_grass) {
    if(!on_grass)
        mvwprintw(win, y, x, " ");
    else {
        wattron(win, COLOR_PAIR(WALKABLE_COLOR_PAIR));
        mvwprintw(win, y, x, "*");
        wattroff(win, COLOR_PAIR(WALKABLE_COLOR_PAIR));
    }
}

void remove_enemy_projectile(WINDOW *win, int y, int x) {
    mvwprintw(win, y, x, " ");
}


/* Chiusura di una finestra */

void close_window(WINDOW *win) {
    if (win != NULL) {
        wclear(win);
        wrefresh(win);
        delwin(win);
    }
}

