#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/types.h>
#include <stdio.h>
#include <time.h>
#include <signal.h>
#include "utils.h"
#include "croc_process.h"

/* volatile -> Evita ottimizzazione che ignorano modifiche asincrone (con signal)
    sig_atomic_t -> Garantisce accesso atomico anche durante l'interruzione di segnali */
volatile sig_atomic_t running = 1;


void handle_sigterm(int sig){
    running = 0;
}


pid_t croc_process(IPCHandles *ipc, Object croc, int stream_index, int stream_crocs_index, int delay, int pid_index) 
{
    pid_t pid = fork();
    if(pid == -1) {
        exit(EXIT_FAILURE);
    } else if(pid == 0) //Processo figlio
    {
        signal(SIGTERM, handle_sigterm);

        int dir;
        dir = (croc.direction == DIR_LEFT) ? (-1) : 1;
        
        croc.pid = getpid();

        close(ipc->shared_pipe[0]);     // Lato lettura
        clean_up_pipe(ipc->frog_pipe);   

        log_croc_event("res/check_create", "First message", stream_index, stream_crocs_index, 
            getpid(), croc.x, croc.y);
            
        while(running) {
            Message m_out, m_in;
            croc.x += dir;

            set_message(&m_out, MSG_UPDATE_POS, &croc, &stream_index, &stream_crocs_index, &pid_index);           
            sem_wait(ipc->sync_sem);
            if (write(ipc->shared_pipe[1], &m_out, sizeof(Message)) == -1) {
                // Scrittura fallita, ma comunque rilascio il semaforo
            }
            sem_post(ipc->sync_sem);
            usleep(delay);
        }
        
        // Pulizia risorse del figlio
        close(ipc->shared_pipe[1]);           // Chiudi lato scrittura pipe condivisa
        sem_close(ipc->sync_sem);             // Chiudi semaforo
        exit(0); 
    }

    return pid;
}




