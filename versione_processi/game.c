#include <curses.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdlib.h>
#include <time.h>
#include "shared.h"
#include "frog_process.h"
#include "utils.h"
#include "game.h"


// void init_crocs(Stream streams[], int y, int win_width) {
//     int timeout;
//     Object tmp;
//     int dir;

//     dir = rand() % 2;   // DIR_LEFT = 0 / DIR_RIGHT = 1
//     tmp.type = OBJ_CROC;

//     for(int i = 0; i < NUM_STREAMS; i++) {
//         Stream s;
//         s.num_crocs = 0;

//         for (int j = 0; j < MAX_CROCS_PER_STREAM; j++)
//         {
//             tmp.direction = dir;
//             tmp.x = (tmp.direction == DIR_LEFT) ? (win_width + CROC_WIDTH) : (-CROC_WIDTH);
//             tmp.y = y;

//             //Avvio il processo
//             pid_t pid = croc_process();
//             tmp.pid = pid;


//             s.objs[j] = tmp;
//             s.num_crocs++;
//             usleep(50000);
//         }
        
//         //Aggiorno l'array di Stream
//         streams[i] = s;
//         dir = (dir == DIR_LEFT) ? DIR_RIGHT : DIR_LEFT;

//         //Aggiorno la posizione y
//         y -= FROG_CROC_HEIGHT;
//     }
// }

void update_lane(int *active_lane, int direction) {
    switch (direction) {
        case DIR_UP: 
            if (*active_lane < NUM_STREAMS + 1) (*active_lane)++; 
            break;
        case DIR_DOWN: 
            if (*active_lane > 0) (*active_lane)--; 
            break;
        default:
            break;
    }
}


void game_loop(WINDOW *win) {

    int shared_pipe[2];
    int private_frog_pipe[2];
    int win_height, win_width;

    sem_t *sync_sem;     //Semaforo condiviso tra i processi per uso di shared_pipe
    char *sem_name = "/shared_semaphore";

    //Inizializzo il semaforo
    sync_sem = init_shared_semaphore(sem_name);
    if(sync_sem == NULL){
        clean_up_semaphore(sync_sem, sem_name);
        return;
    }

    //Inizializzo la pipe di comunicazione
    if(pipe(shared_pipe) == -1) return;

    //Prendo le dimensioni della finestra
    getmaxyx(win, win_height, win_width);

    srand(time(NULL));

    //Inizializzo il campo di gioco
    Object frog;
    Burrow burrows[NUM_BURROWS];
    Stats stats;
    init_playground(win, win_height, win_width, &frog, burrows, &stats);
    wrefresh(win);

    //Inizializzo i processi
    pid_t frog_pid = frog_process(win, shared_pipe, private_frog_pipe, sync_sem, frog);


    /*----------------- LOOP PRINCIPALE DEL GIOCO -----------------*/

    int active_lane = 0;
    bool on_grass = true;

    close(shared_pipe[1]);  //Chiudo estremità di scrittura
    set_nonblocking(shared_pipe[0]);    //Pipe non bloccanti

    while(1) {
        Message m;

        //Leggo dalla pipe
        ssize_t bytes = read(shared_pipe[0], &m, sizeof m);
        if(bytes > 0) {
            
            if(m.msg_type == MSG_UPDATE_POS) {

                switch (m.obj.type)
                {
                case OBJ_FROG:
                    remove_frog(win, frog.y, frog.x, on_grass);
                    draw_frog(win, m.obj, false);

                    frog = m.obj;   //Aggiorno la vecchia posizione della rana
                    update_lane(&active_lane, m.obj.direction);
                    on_grass = (active_lane >= 1 && active_lane <= NUM_STREAMS) ? false : true;
                    break;
                
                default:
                    break;
                }
            }
        }

        wrefresh(win);

        usleep(100000);
    }

    /* Chiudo le risorse utilizzate e termino i processi */

    kill(frog_pid, SIGTERM);
    waitpid(frog_pid, NULL, 0);

    clean_up_pipe(shared_pipe);

    //Distruggo il semaforo
    clean_up_semaphore(sync_sem, sem_name);
}


