
#ifndef GRAPHICS_H
#define GRAPHICS_H

#include "shared.h"

//Define dei colori
#define COLOR_BROWN 11
#define FROG_COLOR_PAIR 1       //ID colore rana
#define CROC_COLOR_PAIR 2       //ID colore coccodrilli
#define SIDEWALK_COLOR_PAIR 3           //ID colore marciapiede
#define START_MENU_COLOR_PAIR 4   //ID colore titolo
#define RIVER_COLOR_PAIR 5  //ID colore fiume
#define BURROW_COLOR_PAIR 6
#define BLACK_COLOR_PAIR 7
#define SCARED_FROG_COLOR_PAIR 8
#define STATS_COLOR_PAIR 9


#define NUM_STREAMS 8
#define MAX_CROCS_PER_STREAM 3
#define BURROW_WIDTH 8
#define NUM_BURROWS 5


//Sprite dei personaggi (dichiarazioni)
extern char *frog_sprite[];
extern char *croc_sprite_sx[];
extern char *croc_sprite_dx[];

//Definisce dove inizia e finisce una tana (indica anche se è stata occupata)
typedef struct {
    bool is_occupied;
    int start_x, end_x;
} Burrow;

//Statistiche di gioco
typedef struct {
    int score;      //Punteggio 
    int lives;      //Vite della rana
    int time;       //Tempo in sec
} Stats;


//Inizializza i colori usati nel gioco
void init_game_colors();

//Inizializza il campo da gioco
void init_playground(WINDOW *win, WINDOW *stats_win, int win_height, int win_width, 
    Object frog, Burrow burrows[5], Stats stats)
;

//Disegna l'oggetto rana
void draw_frog(WINDOW *win, Object frog, bool scared);

//Disegna l'oggetto coccodrillo
void draw_croc(WINDOW *win, Object croc);

//Disegna la sponda dell'argine o il marciapiede
void draw_walkable(WINDOW *win, int y, int x);

//Disegna le tane
void draw_burrows(WINDOW *win, int y, int x);

//Disegna le statistiche di gioco
void draw_stats(WINDOW *win, Stats stats);

//Cancella la rana
void remove_frog(WINDOW *win, int y, int x, bool on_grass);

void remove_stats(WINDOW *win);

void remove_croc(WINDOW *win, int y, int x);

//Rimuove completamente una finestra
void close_window(WINDOW *win);
#endif