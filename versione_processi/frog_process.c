#include <unistd.h>
#include <curses.h>
#include <sys/types.h>
#include <stdlib.h>
#include <semaphore.h>
#include "utils.h"
#include "frog_process.h"


pid_t frog_process(WINDOW *win, IPCHandles *ipc, Object frog) {

    pid_t pid = fork();
    if(pid == -1) {
        exit(EXIT_FAILURE);
    } else if(pid == 0) //Processo figlio
    {
        bool on_croc = false;   //Rileva se la rana si trova sopra un coccodrillo
        keypad(win, true);  
        noecho();
        cbreak();
        nodelay(win, true);

        frog.pid = getpid();

        //Chiudo i lati delle pipe che non uso
        close(ipc->shared_pipe[0]);
        close(ipc->frog_pipe[1]);
        set_nonblocking(ipc->frog_pipe[0]);    //lettura non bloccante
        while (1)
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
                        frog.x -= (on_croc) ? FROG_WIDTH : 1; 
                        moved = true; 
                        frog.direction = DIR_LEFT;
                        break;
                    case KEY_RIGHT: 
                        frog.x += (on_croc) ? FROG_WIDTH : 1; 
                        moved = true; 
                        frog.direction = DIR_RIGHT;
                        break;
                }
            }

            //Scrivo nella pipe solo se il carattere si è mosso
            if(moved){
                Message m_out;
                set_message(&m_out, MSG_UPDATE_POS, &frog, NULL, NULL, NULL);

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

                if(m_in.msg_type == MSG_TOGGLE_ON_CROC) {
                    on_croc = !on_croc;
                }

                if (m_in.msg_type == MSG_KILL) {
                    clean_up_pipe(ipc->shared_pipe);
                    clean_up_pipe(ipc->frog_pipe);
                    exit(0);
                }                
            }
            usleep(FROG_PROCESS_COOLDOWN);
        }
    }

    //Processo padre
    return pid;
}

