#include <unistd.h>
#include <curses.h>
#include <sys/types.h>
#include <stdlib.h>
#include <semaphore.h>
#include "shared.h"
#include "frog_process.h"

pid_t frog_process(WINDOW *win, int shared_pipe_fd[2], int private_pipe_fd[2], sem_t *sync_sem, Object frog) {

    pid_t pid = fork();
    if(pid == -1) {
        exit(-1);
    } else if(pid == 0) //Processo figlio
    {
        initscr();
        keypad(win, true);  
        noecho();
        cbreak();
        nodelay(win, true);

        frog.pid = getpid();

        //Chiudo i lati delle pipe che non uso
        close(shared_pipe_fd[0]);
        close(private_pipe_fd[1]);
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
                        frog.x -= FROG_WIDTH; 
                        moved = true; 
                        frog.direction = DIR_LEFT;
                        break;
                    case KEY_RIGHT: 
                        frog.x += FROG_WIDTH; 
                        moved = true; 
                        frog.direction = DIR_RIGHT;
                        break;
                }
            }

            //Scrivo nella pipe solo se il carattere si è mosso
            if(moved){
                Message m;
                set_message(&m, MSG_UPDATE_POS, frog, NULL, NULL);

                sem_wait(sync_sem);
                write(shared_pipe_fd[1], &m, sizeof(Message));
                sem_post(sync_sem);
            }

            usleep(FROG_PROCESS_COOLDOWN);
        }
    }

    //Processo padre
    return pid;
}

