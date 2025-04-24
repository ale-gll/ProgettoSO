#include <curses.h>
#include <stdlib.h>
#include <stdbool.h>
#include "utils.h"
#include "graphics.h"

//Sprite tane
char *burrows_sprite[] = {
    "|     ______       ______       ______       ______       ______     |",
    "|    |      |     |      |     |      |     |      |     |      |    |",
    "|____|      |_____|      |_____|      |_____|      |_____|      |____|"
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
    " /ç'~~~~~|",
    "[^_<___>_|"
};

//Sprite del coccodrillo volto a dx
char *croc_sprite_dx[] = {
    "|~~~~~'ç\\ ",
    "|_<___>_^]"
};


void init_playground(WINDOW *win, int win_height, int win_width, Object *frog, Burrow burrows[5], Stats *stats) {
    
    int burrow_distance = 5, burrow_start_x = 6;

    //Inizializzo l'oggetto rana
    frog->y = win_height - FROG_CROC_HEIGHT;
    frog->x = (win_width - 10) /2;
    frog->type = OBJ_FROG;
    frog->direction = DIR_UNKNOWN;

    //Inizializzo le tane
    for(int i = 0; i < NUM_BURROWS; i++){
        burrows[i].is_empty = true;     //tana non occupata
        burrows[i].start_x = (i == 0) ? burrow_start_x : burrows[i-1].start_x + burrow_distance + BURROW_WIDTH;
        burrows[i].end_x = burrows[i].start_x + BURROW_WIDTH;
    }

    wbkgd(win, COLOR_PAIR(RIVER_COLOR_PAIR));   //Sfondo blu
    fill_area_with_color(win, 0, 0, 5, win_width, BLACK_COLOR_PAIR);    //Elimino lo sfondo blu in alto

    draw_burrows(win, 2, 0);    //Tane
    draw_walkable(win, 5, 0);   //Argine in alto
    draw_walkable(win, win_height-FROG_CROC_HEIGHT, 0); //Marciapiede
    draw_frog(win, *frog, false);  //Rana al centro del marciapiede

    //Disegno le statistiche
    stats->score = 0;
    stats->lives = 5;
    stats->time = 60;
    draw_stats(win, *stats);
}

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
    if(croc.type != OBJ_CROC) return;

    //Assegno la sprite da disegnare secondo la direzione del coccodrillo
    char **sprite = (croc.direction == DIR_LEFT) ? croc_sprite_sx : croc_sprite_dx;

    for(int i = 0; i < FROG_CROC_HEIGHT; i++){
        wattron(win, COLOR_PAIR(CROC_COLOR_PAIR));
        mvwprintw(win, (croc.y + i), croc.x, "%s", sprite[i]);
        wattroff(win, COLOR_PAIR(CROC_COLOR_PAIR));
    }
}

void draw_walkable(WINDOW *win, int y, int x) {
    for(int i = 0; i < FROG_CROC_HEIGHT; i++) {
        wattron(win, COLOR_PAIR(SIDEWALK_COLOR_PAIR));
        mvwprintw(win, y + i, x, "%s", walkable_sprite[i]);
        wattroff(win, COLOR_PAIR(SIDEWALK_COLOR_PAIR));
    }
}

void fill_area_with_color(WINDOW *win, int start_y, int start_x, int height, int width, int color_pair) {
    wattron(win, COLOR_PAIR(color_pair));

    for (int y = 0; y < height; y++) {
        for (int x = 0; x < width; x++) {
            mvwprintw(win, start_y + y, start_x + x, " ");
        }
    }

    wattroff(win, COLOR_PAIR(color_pair));
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
    mvwprintw(win, 0, section / 2 - 4, "Time: %d", stats.time);
    mvwprintw(win, 0, section + section / 2 - 5, "Score: %d", stats.score);
    mvwprintw(win, 0, 2 * section + section / 2 - 5, "Lives: %d", stats.lives);

    wattroff(win, COLOR_PAIR(STATS_COLOR_PAIR));
}


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
            wattron(win, COLOR_PAIR(SIDEWALK_COLOR_PAIR));
            mvwprintw(win, y + i, x, "%s", grass[i]); 
            wattroff(win, COLOR_PAIR(SIDEWALK_COLOR_PAIR));
        } else {
            mvwprintw(win, y + i, x, "%s", empty[i]);
        }
    }
}

