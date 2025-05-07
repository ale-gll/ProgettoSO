#include <unistd.h>
#include <curses.h>
#include <sys/types.h>
#include <stdlib.h>
#include <semaphore.h>
#include "utils.h"
#include "shared.h"
#include "frog_process.h"


void close_used_pipe_ends_frog(int shared_write_fd, int private_read_fd) {
    close(shared_write_fd);
    close(private_read_fd);
}

pid_t frog_process(WINDOW *win, int shared_pipe_fd[2], int private_pipe_fd[2], sem_t *sync_sem, Object frog) {

    pid_t pid = fork();
    if(pid == -1) {
        exit(-1);
    } else if(pid == 0) //Processo figlio
    {
        bool on_croc = false;   //Rileva se la rana si trova sopra un coccodrillo
        keypad(win, true);  
        noecho();
        cbreak();
        nodelay(win, true);

        frog.pid = getpid();

        //Chiudo i lati delle pipe che non uso
        close(shared_pipe_fd[0]);
        close(private_pipe_fd[1]);
        set_nonblocking(private_pipe_fd[0]);    //lettura non bloccante
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
                set_message(&m_out, MSG_UPDATE_POS, frog, NULL, NULL);

                sem_wait(sync_sem);
                write(shared_pipe_fd[1], &m_out, sizeof(Message));
                sem_post(sync_sem);
            }


            // Controllo se arriva un messaggio dal processo principale
            Message m_in;
            ssize_t n = read(private_pipe_fd[0], &m_in, sizeof(Message));
            if (n > 0) {
                if(m_in.msg_type == MSG_FROG_ON_CROC) {
                    on_croc = !on_croc;
                }

                if(m_in.msg_type == MSG_SET_FROG) {
                    frog = m_in.obj;
                }

                if (m_in.msg_type == MSG_KILL) {
                    close_used_pipe_ends_frog(shared_pipe_fd[1], private_pipe_fd[0]);
                    exit(0);
                }                
            }
            usleep(FROG_PROCESS_COOLDOWN);
        }
    }

    //Processo padre
    return pid;
}

