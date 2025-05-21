
#ifndef UTILS_H
#define UTILS_H

#include <semaphore.h>

typedef struct {
    int shared_pipe[2];     //Pipe per comunicare con la grafica
    int frog_pipe[2];
    int crocs_pipe[2];
    int projectile_pipe[2];
    sem_t *sync_sem;
    sem_t *crocs_sem;
} IPCHandles;

//Inizializza e restituisce un semaforo (Unnamed)
sem_t* init_shared_semaphore(const char *sem_name);

//Funzione per distruggere un semaforo
void clean_up_semaphore(sem_t *sem, char *name);

//Chiude tutti i descrittori di file di una pipe
void clean_up_pipe(int pipe_fd[2]);

//Funzione per settare una pipe come non bloccante
void set_nonblocking(int fd);

#endif