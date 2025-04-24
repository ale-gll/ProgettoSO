/* 
    Questo file contiene funzioni di supporto secondarie
*/

#ifndef UTILS_H
#define UTILS_H

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

#include <semaphore.h>

//Inizializza i colori usati nel gioco
void init_game_colors();

//Inizializza e restituisce un semaforo (Unnamed)
sem_t* init_shared_semaphore(const char *sem_name);

//Funzione per distruggere un semaforo
void clean_up_semaphore(sem_t *sem, char *name);

void clean_up_pipe(int pipe_fd[2]);

//Funzione per settare una pipe come non bloccante
void set_nonblocking(int fd);

#endif