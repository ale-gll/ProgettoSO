#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/types.h>
#include <signal.h>
#include <time.h>
#include "utils.h"
#include "croc_process.h"

/* volatile -> Evita ottimizzazione che ignorano modifiche asincrone (con signal)
    sig_atomic_t -> Garantisce accesso atomico anche durante l'interruzione di segnali */
volatile sig_atomic_t croc_running = 1;


void handle_croc_sigterm(int sig){
    croc_running = 0;
}

pid_t croc_process(IPCHandles *ipc, Object croc, int stream_index, int delay) 
{
    pid_t pid = fork();
    if(pid == -1) {
        exit(EXIT_FAILURE);
    } else if(pid == 0) //Processo figlio
    {
        signal(SIGTERM, handle_croc_sigterm);

        int dir;
        dir = (croc.direction == DIR_LEFT) ? (-1) : 1;
        
        croc.pid = getpid();

        // Inizializza random seed nel figlio
        srand(croc.pid ^ time(NULL));

        time_t fire_time = time(NULL);
        int fire_time_interval = rand() % 5 + 4;  // 4-8 secondi


        close(ipc->shared_pipe[0]);     // Lato lettura
        clean_up_pipe(ipc->frog_pipe);          
        while(croc_running) {
            Message m_out;

            time_t now = time(NULL);

            
            if(difftime(now, fire_time) > fire_time_interval) {
                set_message(&m_out, MSG_FIRE, &croc, &stream_index);

                // reset timer e nuovo intervallo random
                fire_time = now;
                fire_time_interval = rand() % 5 + 4; // 4-8 secondi
            } 
            else {
                croc.x += dir;
                set_message(&m_out, MSG_UPDATE_POS, &croc, &stream_index); 
            }
                      
            sem_wait(ipc->sync_sem);
            ssize_t w = write(ipc->shared_pipe[1], &m_out, sizeof(m_out));
            if(w == -1) {
                // Errore in scrittura
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




