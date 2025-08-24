#include <unistd.h>
#include <sys/types.h>
#include <signal.h>
#include <stdlib.h>
#include <semaphore.h>
#include "utils.h"
#include "projectile_process.h"

volatile sig_atomic_t proj_running = 1;

void handle_proj_sigterm() {
    proj_running = 0;
}

pid_t proj_process(IPCHandles *ipc, Object proj, int stream_index) {
    pid_t pid = fork();
    if(pid == -1) {
        exit(EXIT_FAILURE);
    } else if(pid == 0) // Processo figlio
    {
        signal(SIGTERM, handle_proj_sigterm);

        int dir;
        dir = (proj.direction == DIR_LEFT) ? -1 : 1;

        proj.pid = getpid();

        // Chiudo le risorse non utilizzate
        close(ipc->shared_pipe[0]);
        clean_up_pipe(ipc->frog_pipe);
        while(proj_running) {
            Message m_out;
            proj.x += dir;

            set_message(&m_out, MSG_UPDATE_POS, &proj, &stream_index);
            // Scrivo nella pipe condivisa
            sem_wait(ipc->sync_sem);
            ssize_t w = write(ipc->shared_pipe[1], & m_out, sizeof(Message));
            if(w == -1) {
                // Errore in scrittura
            }
            sem_post(ipc->sync_sem);

            usleep(PROJECTILE_PROCESS_COOLDOWN);
        }

        // Pulizia risorse del figlio
        close(ipc->shared_pipe[1]);           // Chiudi lato scrittura pipe condivisa
        sem_close(ipc->sync_sem);             // Chiudi semaforo
        exit(0); 
    }

    return pid;
}