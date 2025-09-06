
#ifndef PROJECTILE_THREAD_H
#define PROJECTILE_THREAD_H

#define PROJECTILE_THREAD_COOLDOWN 65000 

typedef struct {
    pthread_t tid; 
    Object proj;
    int stream_index;
    SharedBuffer *buffer;   
    atomic_bool running; 
} ProjArgs;

void *proj_thread(void *arg);

#endif