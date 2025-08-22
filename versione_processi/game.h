
#ifndef GAME_H
#define GAME_H

#include "graphics.h"

#define FROG_START_Y(win_height) ((win_height) - FROG_CROC_HEIGHT)
#define FROG_START_X(win_width)  (((win_width) - 10) / 2)
#define MAX_CROCS_PER_STREAM 3
#define MAX_PROJ_PER_STREAM 2
#define OBJ_DUMMY (Object){ .pid = -1, .y = -1, .x = -1, .type = OBJ_NONE, .direction = DIR_UNKNOWN }


typedef struct ObjectNode {
    Object data;
    struct ObjectNode *next;
} ObjectNode;

//Definisce una corrente del fiume
typedef struct {
    int y;                    // ordinata dello stream
    int spawn_time_interval;  // tempo randomico di spawn
    time_t last_spawn_time;
    int delay;
    int direction;
    
    int croc_count;   // quanti coccodrilli ci sono attualmente
    int max_crocs;    // numero massimo di coccodrilli consentiti
    
    int proj_count;   // quanti proiettili ci sono attualmente
    int max_projs;    // numero massimo di proiettili consentiti

    ObjectNode *crocs;  // lista dei coccodrilli nello Stream    
    ObjectNode *projs;  // lista dei proiettili nello Stream
} Stream;


bool is_on_grass(int lane);

bool is_out_of_screen(int win_width, int obj_x, int obj_width);

// Controlla se un elemento è completamente fuori schermo
bool is_fully_out_of_screen(int win_width, int obj_x, int obj_width);

// Inizializza la posizione iniziale della rana
Object init_frog(int win_height, int win_width);

// Inizializza le tane 
void init_burrows(Burrow burrows[5]);

// Inizializza le statistiche del gioco
void init_stats(Stats *stats);

int update_lane(int active_lane, int direction);

bool check_burrows(Object frog, Burrow *burrows);

bool check_win(Stats stats, Burrow burrows[5]);

void init_streams(Stream *streams, int start_y);

bool spawn_single_croc(Stream *stream, int stream_index, IPCHandles *ipc, int window_width);

void clean_all_stream_objects(WINDOW *win, Stream *streams);

void print_game_result(WINDOW *win, int win_height, int win_width, bool is_winner);

void game_loop(WINDOW *win, int start_y, int start_x);


void print_game_result(WINDOW *win, int win_height, int win_width, bool is_winner);
#endif