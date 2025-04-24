#include <curses.h>
#include <stdlib.h>
#include <unistd.h> 
#include <sys/mman.h>
#include <sys/stat.h>   //ftruncate
#include <fcntl.h>      //shm_open
#include "utils.h"


void init_game_colors() {
    init_color(COLOR_BROWN, 245, 222, 179);
    
    //Inizializzo le coppie colore
    init_pair(FROG_COLOR_PAIR, COLOR_BLACK, COLOR_MAGENTA);    //Rana
    init_pair(CROC_COLOR_PAIR, COLOR_BLACK, COLOR_GREEN);   //Coccodrilli
    init_pair(SIDEWALK_COLOR_PAIR, COLOR_BLACK, COLOR_GREEN);   //Marciapiede, argine
    init_pair(START_MENU_COLOR_PAIR, COLOR_GREEN, COLOR_BLACK); //Testo del menu
    init_pair(RIVER_COLOR_PAIR, COLOR_BLACK, COLOR_BLUE);   //Fiume
    init_pair(BURROW_COLOR_PAIR, COLOR_BLACK, COLOR_BROWN); //Tana
    init_pair(BLACK_COLOR_PAIR, COLOR_BLACK, COLOR_BLACK);
    init_pair(SCARED_FROG_COLOR_PAIR, COLOR_BLACK, COLOR_RED);    //Rana spaventata
    init_pair(STATS_COLOR_PAIR, COLOR_WHITE, COLOR_BLACK);  //Statistiche
}

sem_t *init_shared_semaphore(const char *sem_name) {

    //Creo un oggetto di memoria condivisa
    int shm_fd = shm_open(sem_name, O_CREAT | O_RDWR, 0666);
    if(shm_fd == -1) 
    return NULL;

    //Ridimensiono l'oggetto di memoria creato
    ftruncate(shm_fd, sizeof(sem_t)); 

    //Mapping del semaforo nella memoria condivisa
    sem_t *sem = mmap(NULL, sizeof(sem_t), PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd, 0);
    if(sem == MAP_FAILED) 
    return NULL;

    //Inizializzo il semaforo
    sem_init(sem, 1, 1);

    return sem;
}

void clean_up_semaphore(sem_t *sem, char *sem_name) {
    if (sem != NULL) {  //Distruggo il semaforo se è stato creato
        sem_destroy(sem);
        munmap(sem, sizeof(sem_t));
    }

    if (sem_name != NULL) {
        shm_unlink(sem_name); //Provo comunque a rimuovere la shared memory
    }
}

void clean_up_pipe(int pipe_fd[2]) {
    close(pipe_fd[0]);
    close(pipe_fd[1]);
}

void set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}