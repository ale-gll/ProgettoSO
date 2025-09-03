
#ifndef GRAPHICS_H
#define GRAPHICS_H

#include "utils.h"

//Define dei colori
#define COLOR_BROWN 11
#define FROG_COLOR_PAIR 1       //ID colore rana
#define CROC_COLOR_PAIR 2       //ID colore coccodrilli
#define WALKABLE_COLOR_PAIR 3           //ID colore marciapiede
#define START_MENU_COLOR_PAIR 4   //ID colore titolo
#define RIVER_COLOR_PAIR 5  //ID colore fiume
#define BURROW_COLOR_PAIR 6
#define BLACK_COLOR_PAIR 7
#define SCARED_FROG_COLOR_PAIR 8
#define STATS_COLOR_PAIR 9
#define GRANADE_COLOR_PAIR 12
#define ENEMY_PROJ_COLOR_PAIR 13

//Sprite dei personaggi (dichiarazioni)
extern char *frog_sprite[];
extern char *croc_sprite_sx[];
extern char *croc_sprite_dx[];


// Inizializza i colori usati nel gioco
void init_game_colors();

// Inizializza il campo da gioco
void init_playground(WINDOW *win, WINDOW *stats_win, int win_height, int win_width, 
    Object frog, Burrow burrows[5], Stats stats)
;

/* Funzioni per il disegno degli oggetti dinamici */

void draw_frog(WINDOW *win, Object frog, bool scared);

void draw_croc(WINDOW *win, Object croc);

void draw_walkable(WINDOW *win, int y, int x);

void draw_burrows(WINDOW *win, int y, int x);

void draw_stats(WINDOW *win, Stats stats);

void draw_granade(WINDOW *win, Object granade);

void draw_enemy_projectile(WINDOW *win, Object proj);


/* Funzioni per la cancellazione dalla grafica degli oggetti dinamici */

void remove_frog(WINDOW *win, int y, int x, bool on_grass);

void remove_stats(WINDOW *win);

void remove_croc(WINDOW *win, int y, int x);

void remove_granade(WINDOW *win, int y, int x, bool on_grass);

void remove_enemy_projectile(WINDOW *win, int y, int x);


/* Funzioni per il rilevamento delle collisioni */

//ObjectNode* check_collision_granade_projectiles(ObjectNode *obj, ObjectNode *list );

bool check_collision_frog_projectile(Object *frog, Object *proj);


// Rimuove completamente una finestra
void close_window(WINDOW *win);
#endif