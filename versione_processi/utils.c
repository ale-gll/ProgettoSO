#include <stdlib.h>
#include <unistd.h> 
#include <sys/mman.h>
#include <sys/stat.h>   //ftruncate
#include <fcntl.h>      //shm_open
#include "utils.h"


sem_t* init_shared_semaphore(const char *sem_name) {

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