/* 
    Questo file contiene funzioni di supporto secondarie
*/

#ifndef UTILS_H
#define UTILS_H

#define FROG_COLOR_PAIR 1       //ID colore rana
#define CROC_COLOR_PAIR 2       //ID colore coccodrilli
#define SIDEWALK_COLOR_PAIR 3           //ID colore marciapiede
#define START_MENU_COLOR_PAIR 4   //ID colore titolo

//Inizializza i colori usati nel gioco
void init_game_colors();

//Chiude i file descriptor di una pipe
void clean_up(int pipe_fd[2]);

#endif