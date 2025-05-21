#include <stdlib.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/types.h>
#include "utils.h"
#include "shared.h"
#include "croc_process.h"

pid_t croc_process(IPCHandles *ipc, Object croc, int stream_index, int stream_objs_index, int delay) 
{
    pid_t pid = fork();
    if(pid == -1) {
        exit(EXIT_FAILURE);
    } else if(pid == 0) //Processo figlio
    {
        int dir;
        dir = (croc.direction == DIR_LEFT) ? (-1) : 1;
        
        croc.pid = getpid();

        Message m_out, m_in;

        clean_up_pipe(ipc->frog_pipe);
        close(ipc->shared_pipe[0]);
        close(ipc->crocs_pipe[1]);
        set_nonblocking(ipc->crocs_pipe[0]);
        while(1) {
            croc.x += dir;

            set_message(&m_out, MSG_UPDATE_POS, croc, &stream_index, &stream_objs_index);           
            sem_wait(ipc->sync_sem);
            if( write(ipc->shared_pipe[1], &m_out, sizeof(Message)) == -1) {
                //Esegue solo sem_post (evita che si blocchi)
            }
            sem_post(ipc->sync_sem);

            ssize_t n = read(ipc->crocs_pipe[0], &m_in, sizeof(Message));
            if(n > 0) {
                if(m_in.msg_type == MSG_KILL) {
                    close(ipc->shared_pipe[1]);
                    close(ipc->frog_pipe[0]);
                    sem_close(ipc->sync_sem);
                    exit(0);
                }
            }

            usleep(delay);
        }
    }

    return pid;
}




