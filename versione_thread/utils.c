#include <stdlib.h>
#include <unistd.h> 
#include <stdbool.h>
#include <time.h>
#include <signal.h>
#include <ncurses.h>
#include "utils.h"


void buffer_init(SharedBuffer *buf, int capacity) {
    buf->head = NULL;
    buf->tail = NULL;
    buf->count = 0;
    buf->capacity = capacity;

    pthread_mutex_init(&buf->mutex, NULL);
}

void buffer_destroy(SharedBuffer *buf) {
    pthread_mutex_lock(&buf->mutex);

    BufferNode *curr = buf->head;
    while (curr) {
        BufferNode *tmp = curr;
        curr = curr->next;
        free(tmp);
    }

    buf->head = buf->tail = NULL;
    buf->count = 0;

    pthread_mutex_unlock(&buf->mutex);

    pthread_mutex_destroy(&buf->mutex);
}

bool produce_try(SharedBuffer *buf, Message *msg) {
    pthread_mutex_lock(&buf->mutex);

    if (buf->count >= buf->capacity) {
        pthread_mutex_unlock(&buf->mutex);
        return false; // buffer pieno
    }

    BufferNode *node = malloc(sizeof(BufferNode));
    if (!node) {
        pthread_mutex_unlock(&buf->mutex);
        return false; // allocazione fallita
    }

    node->msg = *msg;
    node->next = NULL;

    if (buf->tail) {
        buf->tail->next = node;     // C'è almeno un elemento
    } else {
        buf->head = node;           // Lista vuota
    }

    buf->tail = node;
    buf->count++;

    pthread_mutex_unlock(&buf->mutex);
    return true;
}

bool consume_try(SharedBuffer *buf, Message *msg) {
    pthread_mutex_lock(&buf->mutex);

    if (buf->count == 0) {
        pthread_mutex_unlock(&buf->mutex);
        return false; // vuoto
    }

    BufferNode *node = buf->head;
    *msg = node->msg;

    buf->head = node->next;
    if (!buf->head) buf->tail = NULL;

    free(node);
    buf->count--;

    pthread_mutex_unlock(&buf->mutex);
    return true;
}

void set_message(Message *m, pthread_t tid, int msg_type, Object *obj, int *stream_index) 
{
    m->tid = tid;
    m->msg_type = msg_type;
    if(obj) m->obj = *obj;
    m->stream_index = (stream_index == NULL) ? -1 : *stream_index;
}

