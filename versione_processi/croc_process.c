#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/types.h>
#include <stdio.h>
#include <time.h>
#include <signal.h>
#include "utils.h"
#include "croc_process.h"

pid_t croc_process(IPCHandles *ipc, Object croc, int stream_index, int stream_crocs_index, int delay, int pipe_index) 
{
    pid_t pid = fork();
    if(pid == -1) {
        exit(EXIT_FAILURE);
    } else if(pid == 0) //Processo figlio
    {
        int dir;
        dir = (croc.direction == DIR_LEFT) ? (-1) : 1;
        
        croc.pid = getpid();

        close(ipc->shared_pipe[0]); // Lato lettura
        clean_up_pipe(ipc->frog_pipe);   // Lato lettura, se aperto
        close(ipc->croc_pipes[pipe_index].pipe[1]);
        while(1) {
            Message m_out, m_in;
            croc.x += dir;

            set_message(&m_out, MSG_UPDATE_POS, &croc, &stream_index, &stream_crocs_index, &pipe_index);           
            sem_wait(ipc->sync_sem);
            if (write(ipc->shared_pipe[1], &m_out, sizeof(Message)) == -1) {
                // Scrittura fallita, ma comunque rilascio il semaforo
            }
            sem_post(ipc->sync_sem);

            if(has_pending_kill(ipc->croc_pipes[pipe_index].pipe[0])) {
                ssize_t n = read(ipc->croc_pipes[pipe_index].pipe[0], &m_in, sizeof(Message));

                if (n > 0 && m_in.msg_type == MSG_KILL) break;                
            }
            usleep(delay);
        }

        // Pulizia risorse del figlio
        close(ipc->shared_pipe[1]);                       // Chiudi lato scrittura pipe condivisa
        close(ipc->croc_pipes[pipe_index].pipe[0]);       // Chiudi lato lettura pipe di controllo
        sem_close(ipc->sync_sem);                         // Chiudi semaforo
        exit(0); 
    }

    return pid;
}




