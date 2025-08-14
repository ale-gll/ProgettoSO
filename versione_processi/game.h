
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
    int spawn_time_interval;     // Tempo randomico di spawn
    time_t last_spawn_time;
    int delay;
    int direction;
    int num_crocs;  //Numero di coccodrilli presenti
    int num_projs;
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

void reset_frog(WINDOW *win, IPCHandles *ipc, pid_t *frog_pid, Object *frog, int *active_lane, 
    bool is_scared, int win_height, int win_width);

bool check_win(Stats stats, Burrow burrows[5]);

void init_streams(Stream *streams, int start_y);

int get_number_of_crocs(Stream *streams);

int get_number_of_projs(Stream *streams);

int get_free_pid_index(ActiveProcesses *ap, int size);

void print_game_result(WINDOW *win, int win_height, int win_width, bool is_winner);

bool spawn_single_croc(Stream *stream, int stream_index, IPCHandles *ipc, ActiveProcesses *ap, int window_width);

bool spawn_initial_crocs(Stream *streams, IPCHandles *ipc, ActiveProcesses *ap, int window_width);

void game_loop(WINDOW *win, int start_y, int start_x);

void log_crocs_state(const char *filename, ActiveProcesses *ap, Stream *streams, int num_streams);

#endif