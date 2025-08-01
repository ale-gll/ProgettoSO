#include <stdlib.h>
#include <stdio.h>      /* ****************************** */
#include <unistd.h> 
#include <sys/mman.h>
#include <sys/stat.h>   //ftruncate
#include <fcntl.h>      //shm_open
#include <stdbool.h>
#include <poll.h>
#include <time.h>
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

    ipc->croc_pipes = malloc(sizeof(ProcessComm) * ipc->total_crocs);
    ipc->proj_pipes = malloc(sizeof(ProcessComm) * ipc->total_projs);

    for (int i = 0; i < ipc->total_crocs; i++) ipc->croc_pipes[i].pid = -1;
    for (int i = 0; i < ipc->total_projs; i++) ipc->proj_pipes[i].pid = -1;

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

    for (int i = 0; i < ipc->total_crocs; i++) {
        clean_up_pipe(ipc->croc_pipes[i].pipe);
    }

    for (int i = 0; i < ipc->total_projs; i++) {
        clean_up_pipe(ipc->proj_pipes[i].pipe);
    }

    //Libero la memoria allocata
    free(ipc->croc_pipes);
    free(ipc->proj_pipes);    
}


int find_free_croc_pipe_slot(ProcessComm *pipes, int total) {
    for (int i = 0; i < total; i++) {
        if (pipes[i].pid == -1) return i;
    }
    return -1; // Nessuno slot disponibile
}


void set_message(Message *m, int msg_type, Object *obj, int *stream_index, int *stream_objs_index, int *pipe_index) {
    m->msg_type = msg_type;
    if(obj != NULL) m->obj = *obj;
    m->stream_index = (stream_index == NULL) ? -1 : *stream_index;
    m->stream_obj_index = (stream_objs_index == NULL) ? -1 : *stream_objs_index;
    m->pipe_index = (pipe_index == NULL) ? -1 : *pipe_index;
}


void debug_log(const char *func_name, int pid, const char *error_msg, const char *log_file) {
    FILE *fp = fopen(log_file, "a");
    if (!fp) return;

    time_t now = time(NULL);
    struct tm *t = localtime(&now);

    // Format: [HH:MM:SS] (PID) FUNC: error message
    fprintf(fp, "[%02d:%02d:%02d] (%d) %s: %s\n", 
            t->tm_hour, t->tm_min, t->tm_sec, pid, func_name, error_msg);

    fclose(fp);
}


int has_pending_kill(int fd) {
    struct pollfd pfd = { .fd = fd, .events = POLLIN };
    int ret = poll(&pfd, 1, 0); // timeout 0ms → non blocca mai
    return (ret > 0);
}