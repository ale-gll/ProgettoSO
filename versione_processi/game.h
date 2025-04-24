
#ifndef GAME_H
#define GAME_H

#include "graphics.h"

// void init_crocs(Stream streams[], int y, int win_width);

void update_lane(int *active_lane, int direction);

void game_loop(WINDOW *win);


#endif