#ifndef CROC_THREAD_H
#define CROC_THREAD_H


typedef struct {
    pthread_t tid; 
    Object croc;
    int delay;
    int stream_index;
    SharedBuffer *buffer;   
    atomic_bool running; 
} CrocArgs;

void *croc_thread(void *arg);

#endif