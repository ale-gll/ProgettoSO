
#ifndef GAME_H
#define GAME_H

#include "graphics.h"

#define FROG_START_Y(win_height) ((win_height) - FROG_CROC_HEIGHT)
#define FROG_START_X(win_width)  (((win_width) - 10) / 2)
#define TIME_PER_ROUND 60 
#define MAX_CROCS_PER_STREAM 3
#define MAX_PROJ_PER_STREAM 2
#define OBJ_DUMMY (Object){ .pid = -1, .y = -1, .x = -1, .type = OBJ_NONE, .direction = DIR_UNKNOWN }


//Definisce una corrente del fiume
typedef struct {
    int y;  //ordinata dello stream
    int delay;
    int direction;
    int num_crocs;  //Numero di coccodrilli presenti
    int next_croc_index;
    int num_projs;
    int next_proj_index;
    Object *crocs;   //coccodrilli nello Stream    
    Object *projs;   //Proiettili nello Stream
} Stream;


bool is_on_grass(int lane);

bool is_out_of_screen(int win_width, int obj_x, int obj_width);

bool is_fully_out_of_screen(int win_width, int obj_x, int obj_width);

Object init_frog(int win_height, int win_width);

void init_burrows(Burrow burrows[5]);

void init_stats(Stats *stats);

void update_lane(int *active_lane, int direction);

bool check_burrows(Object frog, Burrow *burrows);

void reset_frog(WINDOW *win, IPCHandles *ipc, pid_t *frog_pid, Object *frog, int *active_lane, bool is_scared, int win_height, int win_width);

bool check_win(Stats stats, Burrow burrows[5]);

void init_streams(Stream *streams, int start_y);

void free_streams(Stream *streams);

int get_number_of_crocs(Stream *streams);

int get_number_of_projs(Stream *streams);

void print_game_result(WINDOW *win, int win_height, int win_width, bool is_winner);

void kill_object(int write_fd, pid_t pid);

void game_loop(WINDOW *win, int start_y, int start_x);


#endif