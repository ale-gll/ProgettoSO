#include <stdlib.h>
#include <unistd.h> 
#include <stdbool.h>
#include <time.h>
#include <signal.h>
#include <ncurses.h>
#include "utils.h"


void buffer_init(SharedBuffer *buf) {
    buf->head = 0;
    buf->tail = 0;
    buf->count = 0;

    pthread_mutex_init(&buf->mutex, NULL);
    sem_init(&buf->empty, 0, BUFFER_SIZE);
    sem_init(&buf->full, 0, 0);
}

void buffer_destroy(SharedBuffer *buf) {
    pthread_mutex_destroy(&buf->mutex);
    sem_destroy(&buf->empty);
    sem_destroy(&buf->full);
}

void produce(SharedBuffer *buf, Message *msg) {
    sem_wait(&buf->empty);             // attende slot libero
    pthread_mutex_lock(&buf->mutex);

    buf->buffer[buf->head] = *msg;
    buf->head = (buf->head + 1) % BUFFER_SIZE;
    buf->count++;

    pthread_mutex_unlock(&buf->mutex);
    sem_post(&buf->full);              // segnala nuovo elemento
}

bool consume_try(SharedBuffer *buf, Message *msg) {
    if (sem_trywait(&buf->full) != 0) {
        return false; // nessun messaggio disponibile
    }

    pthread_mutex_lock(&buf->mutex);

    *msg = buf->buffer[buf->tail];
    buf->tail = (buf->tail + 1) % BUFFER_SIZE;
    buf->count--;

    pthread_mutex_unlock(&buf->mutex);
    sem_post(&buf->empty);

    return true; // messaggio consumato
}


void set_message(Message *m, pthread_t tid, int msg_type, Object *obj, int *stream_index) 
{
    m->tid = tid;
    m->msg_type = msg_type;
    if(obj != NULL) m->obj = *obj;
    m->stream_index = (stream_index == NULL) ? -1 : *stream_index;
}

