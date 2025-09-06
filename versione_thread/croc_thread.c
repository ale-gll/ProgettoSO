#include <stdlib.h>
#include <unistd.h>
#include "utils.h"
#include "croc_thread.h"

void *croc_thread(void *arg) {
    CrocArgs *args = (CrocArgs *) arg;
    args->tid = pthread_self();

    Object *croc = &args->croc;
    int dir;
    dir = (croc->direction == DIR_LEFT) ? (-1) : 1;

    time_t fire_time = time(NULL);
    int fire_time_interval = rand() % 5 + 4;  // 4-8 secondi


    while(atomic_load(&args->running)) {
        Message m_out;

        time_t now = time(NULL);

        
        if(difftime(now, fire_time) > fire_time_interval) {
            set_message(&m_out, args->tid, MSG_FIRE, croc, &args->stream_index);

            // reset timer e nuovo intervallo random
            fire_time = now;
            fire_time_interval = rand() % 5 + 4; // 4-8 secondi
        } 
        else {
            croc->x += dir;
            set_message(&m_out, args->tid, MSG_UPDATE_POS, croc, &args->stream_index); 
        }            
        produce(args->buffer, &m_out);

        usleep(args->delay);
    }
    return NULL;
}
