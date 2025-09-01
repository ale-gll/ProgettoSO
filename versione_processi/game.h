
#ifndef GAME_H
#define GAME_H

#include "graphics.h"

#define MAX_CROCS_PER_STREAM 3
#define MAX_PROJ_PER_STREAM 2
#define OBJ_DUMMY (Object){ .pid = -1, .y = -1, .x = -1, .type = OBJ_NONE, .direction = DIR_UNKNOWN }
#define NUM_STREAMS 8
#define BURROW_WIDTH 8
#define NUM_BURROWS 5
#define NUM_LIVES 5
#define TIME_PER_ROUND 50 
#define FROG_START_Y(win_height) ((win_height) - FROG_CROC_HEIGHT)
#define FROG_START_X(win_width)  (((win_width) - 10) / 2)

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


// Eventi di gioco che modificano il punteggio
typedef enum {
    SCORE_REACH_BURROW,
    SCORE_MOVE_UP,
    SCORE_MOVE_DOWN,
    SCORE_COMPLETE_ALL_BURROWS,
    SCORE_LOSE_LIFE
} ScoreEvent;

void update_score(Stats *stats, ScoreEvent e);

Object init_frog(int win_height, int win_width);

void init_burrows(Burrow burrows[5]);

void init_stats(Stats *stats);

bool is_on_grass(int lane);

bool is_out_of_screen(int win_width, int obj_x, int obj_width);

bool is_fully_out_of_screen(int win_width, int obj_x, int obj_width);

int update_lane(int active_lane, int direction);

bool check_burrows(Object frog, Burrow *burrows);

bool check_win(Stats stats, Burrow burrows[5]);

void init_streams(Stream *streams, int start_y);

bool spawn_single_croc(Stream *stream, int stream_index, IPCHandles *ipc, int window_width);

bool spawn_granade(IPCHandles *ipc, int start_x, int start_y, 
    ObjectDirection dir, int active_lane, ObjectNode **active_granades);

void clean_all_stream_objects(WINDOW *win, Stream *streams);

void clean_all_granades(WINDOW *win, ObjectNode **granades);

void game_loop(WINDOW *win, int start_y, int start_x);


void print_game_result(WINDOW *win, int win_height, int win_width, bool is_winner, int score);

#endif