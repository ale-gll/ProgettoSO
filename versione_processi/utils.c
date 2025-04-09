#include <curses.h>
#include "utils.h"


void init_game_colors() {
    
    //Inizializzo le coppie colore
    init_pair(FROG_COLOR_PAIR, COLOR_BLACK, COLOR_BLUE);    //Rana
    init_pair(CROC_COLOR_PAIR, COLOR_BLACK, COLOR_GREEN);   //Coccodrilli
    init_pair(SIDEWALK_COLOR_PAIR, COLOR_RED, COLOR_GREEN);   //Marciapiede, argine
    init_pair(4, COLOR_GREEN, COLOR_BLACK);

}

