
#ifndef FROG_THREAD_H
#define FROG_THREAD_H

#define FROG_THREAD_COOLDOWN 12000

typedef struct {
    pthread_t tid;                      // thread ID 
    WINDOW *win;
    pthread_mutex_t *win_mutex;         // Protegge la window in chiamate come wgetch()
    Object frog;                        // copia locale dell'oggetto
    pthread_mutex_t *frog_mutex;        // protegge la rana se viene resettata la vecchia posizione
    SharedBuffer *buffer;               // buffer di comunicazione
    atomic_bool running;                // atomic_bool per operazioni atomiche 
} FrogArgs;

void *frog_thread(void *arg);

#endif