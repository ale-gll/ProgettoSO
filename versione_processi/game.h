
#ifndef GAME_H
#define GAME_H

#include "graphics.h"

#define FROG_START_Y(win_height) ((win_height) - FROG_CROC_HEIGHT)
#define FROG_START_X(win_width)  (((win_width) - 10) / 2)
#define TIME_PER_ROUND 60 

//Definisce una corrente del fiume
typedef struct {
    int delay;
    int direction;
    int num_crocs;  //Numero di coccodrilli presenti
    Object objs[MAX_CROCS_PER_STREAM]; //coccodrilli/proiettili nemici nello Stream    
} Stream;


bool is_on_grass(int lane);

bool is_out_of_screen(int win_width, int obj_x, int obj_width);

bool is_fully_out_of_screen(int win_width, int obj_x, int obj_width);

Object init_frog(int win_height, int win_width);

void init_burrows(Burrow burrows[5]);

void init_stats(Stats *stats);

void update_lane(int *active_lane, int direction);

bool check_burrows(Object frog, Burrow *burrows);

void reset_frog(WINDOW *win, IPCHandles *ipc, pid_t *frog_pid, Object *frog, 
    int *active_lane, bool is_scared, int win_height, int win_width)
;

bool check_win(Stats stats, Burrow burrows[5]);

void print_game_result(WINDOW *win, int win_height, int win_width, bool is_winner);

void init_crocs(IPCHandles *ipc, Stream *streams, int y, int x, int win_width);

bool init_ipc_handles(IPCHandles *ipc, char *sync_sem_name, char *crocs_sem_name);

void game_loop(WINDOW *win, int start_y, int start_x);


#endif