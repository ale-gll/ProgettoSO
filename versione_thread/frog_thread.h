
#ifndef FROG_THREAD_H
#define FROG_THREAD_H

typedef struct {
    pthread_t tid;
    WINDOW *win;
    pthread_mutex_t *win_mutex;
    Object frog;
    pthread_mutex_t *frog_mutex; 
    SharedBuffer *buffer;
    bool running; 
} FrogArgs;

void *frog_thread(void *arg);

#endif