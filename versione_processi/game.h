
#ifndef GAME_H
#define GAME_H

#include "graphics.h"

#define FROG_START_Y(win_height) ((win_height) - FROG_CROC_HEIGHT)
#define FROG_START_X(win_width)  (((win_width) - 10) / 2)
#define TIME_PER_ROUND 60
#define STATS_WIN_HEIGHT 2

// void init_crocs(Stream streams[], int y, int win_width);

bool is_on_grass(int lane);

Object init_frog(int win_height, int win_width);

void init_burrows(Burrow burrows[5]);

void init_stats(Stats *stats);

void update_lane(int *active_lane, int direction);

bool check_burrows(Object frog, Burrow *burrows);

void reset_frog(WINDOW *win, int shared_pipe[2], int private_pipe[2], sem_t *sem, 
    pid_t *frog_pid, Object *frog, int *active_lane, bool is_scared, int win_height, int win_width)
;

bool check_win(Stats stats, Burrow burrows[5]);

void game_loop(WINDOW *pg_win, int start_y, int start_x);


#endif