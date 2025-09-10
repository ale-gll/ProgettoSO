#include <ncurses.h>
#include <unistd.h>
#include "utils.h"
#include "frog_thread.h"

void *frog_thread(void *arg) {
    FrogArgs *args = (FrogArgs *) arg;
    args->tid = pthread_self();

    pthread_mutex_lock(args->win_mutex);
    keypad(args->win, true);
    noecho();
    cbreak();
    nodelay(args->win, true);
    pthread_mutex_unlock(args->win_mutex);

    while (atomic_load(&args->running)) {
        bool moved = false;
        int old_x, old_y;   // <-- salvo la posizione originale

        pthread_mutex_lock(args->win_mutex);
        int ch = wgetch(args->win);
        pthread_mutex_unlock(args->win_mutex);

        pthread_mutex_lock(args->frog_mutex);
        Object *frog = &args->frog;
        old_x = frog->x;    // salvo posizione prima del movimento
        old_y = frog->y;

        if (ch != ERR) {
            switch (ch) {
                case KEY_UP:
                    frog->y -= FROG_CROC_HEIGHT;
                    moved = true;
                    frog->direction = DIR_UP;
                    break;

                case KEY_DOWN:
                    frog->y += FROG_CROC_HEIGHT;
                    moved = true;
                    frog->direction = DIR_DOWN;
                    break;

                case KEY_LEFT:
                    frog->x -= 1;
                    moved = true;
                    frog->direction = DIR_LEFT;
                    break;

                case KEY_RIGHT:
                    frog->x += 1;
                    moved = true;
                    frog->direction = DIR_RIGHT;
                    break;

                case (int) ' ': {
                    Message m;
                    set_message(&m, args->tid, MSG_FIRE, frog, NULL);
                    produce_try(args->buffer, &m);   // ignora se pieno
                    break;
                }
            }
        }

        if (moved) {
            Message m_out;
            set_message(&m_out, args->tid, MSG_UPDATE_POS, frog, NULL);

            if (!produce_try(args->buffer, &m_out)) {
                // Buffer pieno → rollback posizione
                frog->x = old_x;
                frog->y = old_y;
            }
        }

        pthread_mutex_unlock(args->frog_mutex);
        usleep(FROG_THREAD_COOLDOWN);
    }
    return NULL;
}
