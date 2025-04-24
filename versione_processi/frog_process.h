
#ifndef FROG_PROCESS_H
#define FROG_PROCESS_H

#define FROG_PROCESS_COOLDOWN 100000

pid_t frog_process(WINDOW *win, int shared_pipe_fd[2], int private_pipe_fd[2], sem_t *sync_sem, Object frog);

#endif