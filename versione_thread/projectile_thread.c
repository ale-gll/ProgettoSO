#include <unistd.h>
#include "utils.h"
#include "projectile_thread.h"

void *proj_thread(void *arg) {
    ProjArgs *args = (ProjArgs *) arg;
    args->tid = pthread_self();

    Object *proj = &args->proj;
    int dir = (proj->direction == DIR_LEFT) ? -1 : 1;

    while(atomic_load(&args->running)) {
        Message m_out; 
        proj->x += dir; 
        set_message(&m_out, args->tid, MSG_UPDATE_POS, proj, args->stream_index);
        produce(args->buffer, &m_out);

        usleep(PROJECTILE_THREAD_COOLDOWN);
    }

    return NULL; 
}
