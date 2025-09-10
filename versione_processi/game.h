
#ifndef GAME_H
#define GAME_H

#include "graphics.h"

#define MAX_CROCS_PER_STREAM 3
#define MAX_PROJ_PER_STREAM 2
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

void game_loop(WINDOW *win, int start_y, int start_x);

#endif