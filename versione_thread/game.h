
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

/* da mettere in proj_thread */
typedef struct {
    pthread_t tid; 
    Object proj;
    int stream_index;
    SharedBuffer *buffer;   
    bool running; 
} ProjArgs;

typedef struct {
    Object obj;          // posizione e tipo dell’oggetto
    void *args;          // puntatore al suo ThreadArgs (es. CrocArgs*, ProjArgs*)
} ActiveEntity;

// Elemento della lista di thread attivi
typedef struct EntityNode { 
    pthread_t tid;
    ActiveEntity data;
    bool on_grass;
    struct EntityNode *next;
    struct EntityNode *prev;
} EntityNode;


// Eventi di gioco che modificano il punteggio
typedef enum {
    SCORE_REACH_BURROW,
    SCORE_MOVE_UP,
    SCORE_MOVE_DOWN,
    SCORE_COMPLETE_ALL_BURROWS,
    SCORE_LOSE_LIFE
} ScoreEvent;


/**
 * Funzioni helper 
 */


/**
 * Funzione principale: loop di gioco
 */

void game_loop(WINDOW *win, int start_y, int start_x);


#endif