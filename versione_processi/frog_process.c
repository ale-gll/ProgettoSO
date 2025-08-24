#include <unistd.h>
#include <curses.h>
#include <sys/types.h>
#include <stdlib.h>
#include <semaphore.h>
#include <signal.h>
#include "utils.h"
#include "frog_process.h"

volatile sig_atomic_t frog_running = 1;

void handle_frog_sigterm(int sig) {
    frog_running = 0;
}

pid_t frog_process(WINDOW *win, IPCHandles *ipc, Object frog) {

    pid_t pid = fork();
    if(pid == -1) {
        exit(EXIT_FAILURE);
    } else if(pid == 0) //Processo figlio
    {
        signal(SIGTERM, handle_frog_sigterm);
        keypad(win, true);  
        noecho();
        cbreak();
        nodelay(win, true);

        frog.pid = getpid();

        //Chiudo i lati delle pipe che non uso
        close(ipc->shared_pipe[0]);
        close(ipc->frog_pipe[1]);
        set_nonblocking(ipc->frog_pipe[0]);    //lettura non bloccante
        while (frog_running)
        {
            bool moved = false;
            int ch = wgetch(win);   //Prendo l'input da stdscr

            if(ch != ERR) {
                switch (ch)
                {
                    case KEY_UP: 
                        frog.y -= FROG_CROC_HEIGHT; 
                        moved = true;
                        frog.direction = DIR_UP;
                        break;
                    case KEY_DOWN: 
                        frog.y += FROG_CROC_HEIGHT; 
                        moved = true; 
                        frog.direction = DIR_DOWN;
                        break;
                    case KEY_LEFT: 
                        frog.x -= 1; 
                        moved = true; 
                        frog.direction = DIR_LEFT;
                        break;
                    case KEY_RIGHT: 
                        frog.x += 1; 
                        moved = true; 
                        frog.direction = DIR_RIGHT;
                        break;
                    case (int) ' ':
                    {
                        Message m; 
                        set_message(&m, MSG_FIRE, &frog, NULL);
                        sem_wait(ipc->sync_sem);
                        if(write(ipc->shared_pipe[1], &m, sizeof(Message)) == -1) {
                            // Errore gestito silenziosamente
                        }
                        sem_post(ipc->sync_sem);
                        break;
                    }
                }
            }

            //Scrivo nella pipe solo se il carattere si è mosso
            if(moved){
                Message m_out;
                set_message(&m_out, MSG_UPDATE_POS, &frog, NULL);

                sem_wait(ipc->sync_sem);
                if(write(ipc->shared_pipe[1], &m_out, sizeof(Message)) == -1) {
                    //Errore gestito "silenziosamente"
                }
                sem_post(ipc->sync_sem);
            }


            // Controllo se arriva un messaggio dal processo principale
            Message m_in;
            ssize_t n = read(ipc->frog_pipe[0], &m_in, sizeof(Message));
            if (n > 0) {
                if(m_in.msg_type == MSG_SET_FROG) {
                    frog = m_in.obj;
                }               
            }
            usleep(FROG_PROCESS_COOLDOWN);
        }

        clean_up_pipe(ipc->shared_pipe);
        clean_up_pipe(ipc->frog_pipe);
        exit(0);
    }

    //Processo padre
    return pid;
}

