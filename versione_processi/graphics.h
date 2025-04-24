
#ifndef GRAPHICS_H
#define GRAPHICS_H

#include "shared.h"

#define NUM_STREAMS 8
#define MAX_CROCS_PER_STREAM 3
#define BURROW_WIDTH 6
#define NUM_BURROWS 5

//Sprite dei personaggi (dichiarazioni)
extern char *frog_sprite[];
extern char *croc_sprite_sx[];
extern char *croc_sprite_dx[];

//Definisce una corrente del fiume
typedef struct {
    int speed;
    int direction;
    int num_crocs;  //Numero di coccodrilli presenti
    Object objs[MAX_CROCS_PER_STREAM]; //coccodrilli/proiettili nemici nello Stream    
} Stream;

//Definisce dove inizia e finisce una tana (indica anche se è stata occupata)
typedef struct {
    bool is_empty;
    int start_x, end_x;
} Burrow;

//Statistiche di gioco
typedef struct {
    int score;      //Punteggio 
    int lives;      //Vite della rana
    int time;       //Tempo in sec
} Stats;

//Inizializza il campo da gioco
void init_playground(WINDOW *win, int win_height, int win_width, Object *frog, Burrow burrows[5], Stats *stats);

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

//Riempie una parte di sfondo
void fill_area_with_color(WINDOW *win, int start_y, int start_x, int height, int width, int color_pair);


/*----------- Funzioni per la rimozione di oggetti dallo schermo ------------*/

//Cancella la rana
void remove_frog(WINDOW *win, int y, int x, bool on_grass);



#endif