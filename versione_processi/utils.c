#include <stdlib.h>
#include <unistd.h> 
#include <sys/mman.h>
#include <sys/stat.h>   //ftruncate
#include <fcntl.h>      //shm_open
#include <sys/wait.h>
#include <stdbool.h>
#include <time.h>
#include <signal.h>
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


void clean_up_pipe(int pipe[2]) {
    if (pipe[0] >= 0) close(pipe[0]);
    if (pipe[1] >= 0) close(pipe[1]);
}


void set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}


bool init_ipc_handles(IPCHandles *ipc, char *sync_sem_name) {
    if(pipe(ipc->shared_pipe) == -1 || pipe(ipc->frog_pipe) == -1) {
        return false;
    }

    ipc->sync_sem = init_shared_semaphore(sync_sem_name);
    if(ipc->sync_sem == NULL) {
        return false;
    }

    return true;
}


void cleanup_ipc_handles(IPCHandles *ipc, char *sync_sem_name) {

    //Distruggo il semaforo
    clean_up_semaphore(ipc->sync_sem, sync_sem_name);

    //Chiudo le pipe
    clean_up_pipe(ipc->shared_pipe);
    clean_up_pipe(ipc->frog_pipe);   
}


void set_message(Message *m, int msg_type, Object *obj, int *stream_index) 
{
    m->msg_type = msg_type;
    if(obj != NULL) m->obj = *obj;
    m->stream_index = (stream_index == NULL) ? -1 : *stream_index;
}


/**
 * Funzioni per la gestione di ObjectNode
 */

ObjectNode *find_node_by_pid(ObjectNode *head, pid_t pid) {
    ObjectNode *curr = head;
    while (curr && curr->data.pid != pid) {
        curr = curr->next;
    }
    return curr;
}


void remove_and_kill_node(ObjectNode **head, ObjectNode *node) {
     if (!node) return;

    // Kill processo
    kill(node->data.pid, SIGTERM);
    waitpid(node->data.pid, NULL, 0);

    // Unlink dalla lista
    if (node->prev)
        node->prev->next = node->next;
    else
        *head = node->next;  // era la testa

    if (node->next)
        node->next->prev = node->prev;

    // Free nodo
    free(node);
}

