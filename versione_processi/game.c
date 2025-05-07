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

bool is_on_grass(int lane) {
    return !(lane >= 1 && lane <= NUM_STREAMS);
}

Object init_frog(int win_height, int win_width) {
    Object frog;
    frog.direction = DIR_UNKNOWN;
    frog.pid = -1;
    frog.type = OBJ_FROG;
    frog.y = FROG_START_Y(win_height);
    frog.x = FROG_START_X(win_width);
    return frog;
}

void init_burrows(Burrow burrows[5]) {
    
    int burrow_distance = 5, burrow_start_x = 5;

    for(int i = 0; i < NUM_BURROWS; i++) {
        burrows[i].is_occupied = false;     //tana non occupata
        burrows[i].start_x = burrow_start_x + i * (burrow_distance + BURROW_WIDTH);
        burrows[i].end_x = burrows[i].start_x + BURROW_WIDTH;
    }
}

void init_stats(Stats *stats) {
    stats->score = 0;
    stats->lives = 5;
    stats->time = TIME_PER_ROUND;
}

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

bool check_burrows(Object frog, Burrow *burrows) {

    for(int i = 0; i < NUM_BURROWS; i++) {
        if((!burrows[i].is_occupied) && 
            frog.x >= burrows[i].start_x &&
            (frog.x + FROG_WIDTH) <= burrows[i].end_x) {
            
            burrows[i].is_occupied = true;
            return true;
        }
    }

    return false;
}

void reset_frog(WINDOW *win, int shared_pipe[2], int private_pipe[2], sem_t *sem, 
                pid_t *frog_pid, Object *frog, int *active_lane, bool is_scared, int win_height, int win_width) 
{
    // 1. Uccido la rana attuale
    Message kill_msg;
    set_message(&kill_msg, MSG_KILL, *frog, NULL, NULL);
    write(private_pipe[1], &kill_msg, sizeof(Message));
    waitpid(*frog_pid, NULL, 0);

    // 2. Svuoto eventuali messaggi residui
    Message discard;
    while (read(shared_pipe[0], &discard, sizeof(discard)) > 0);

    //Chiudo le vecchie pipe
    close(shared_pipe[0]);
    close(private_pipe[1]);

    if (pipe(shared_pipe) == -1 || pipe(private_pipe) == -1) return;
    set_nonblocking(shared_pipe[0]);

    *frog = init_frog(win_height, win_width);
    *active_lane = 0;
    *frog_pid = frog_process(win, shared_pipe, private_pipe, sem, *frog);
    usleep(10000);

    draw_frog(win, *frog, is_scared);
}

bool check_win(Stats stats, Burrow burrows[5]) {
    bool res = true;
    int burrows_size = 5;

    for(int i = 0; i < burrows_size; i++) {
        res &= burrows[i].is_occupied;
    }
    return res && (stats.lives > 0);
}

void game_loop(WINDOW *pg_win, int start_y, int start_x) {

    int shared_pipe[2];
    int private_frog_pipe[2];
    int win_height, win_width;

    getmaxyx(pg_win, win_height, win_width);
    WINDOW *stats_win = newwin(STATS_WIN_HEIGHT, win_width, start_y-(STATS_WIN_HEIGHT+1), start_x);

    sem_t *sync_sem;
    char *sem_name = "/shared_semaphore";

    //Inizializzo le pipe di comunicazione
    if(pipe(shared_pipe) == -1 || pipe(private_frog_pipe) == -1) return;

    //Inizializzo il semaforo
    sync_sem = init_shared_semaphore(sem_name);
    if(sync_sem == NULL){
        clean_up_semaphore(sync_sem, sem_name);
        return;
    }

    srand(time(NULL));

    //Inizializzo il campo di gioco
    Object frog;
    Burrow burrows[NUM_BURROWS];
    Stats stats;

    frog = init_frog(win_height, win_width); 
    init_burrows(burrows); 
    init_stats(&stats);
    init_playground(pg_win, stats_win, win_height, win_width, frog, burrows, stats);

    //Inizializzo i processi figli
    pid_t frog_pid = frog_process(pg_win, shared_pipe, private_frog_pipe, sync_sem, frog);



    //Processo principale

    bool flag = true;
    int active_lane = 0;
    bool on_grass = true;
    bool is_scared = false;
    bool reset = false;

    close(shared_pipe[1]);
    close(private_frog_pipe[0]);
    set_nonblocking(shared_pipe[0]);    //Pipe non bloccante

    while(flag) {
        Message m;

        ssize_t bytes = read(shared_pipe[0], &m, sizeof m);
        if(bytes > 0) {
            
            if(m.msg_type == MSG_UPDATE_POS) {

                switch (m.obj.type)
                {
                case OBJ_FROG:
                    // 1. Rana cerca di andare giù dal marciapiede
                    if(active_lane == 0 && m.obj.direction == DIR_DOWN) {
                        Message reset_msg;
                        set_message(&reset_msg, MSG_SET_FROG, frog, NULL, NULL);
                        if(write(private_frog_pipe[1], &reset_msg, sizeof(Message)) == -1) flag = false;
                    }

                    // 2. Rana cerca di salire oltre l'argine (tane)
                    else if(active_lane == (NUM_STREAMS+1) && m.obj.direction == DIR_UP) {
                        if (check_burrows(m.obj, burrows)) {
                            draw_frog(pg_win, m.obj, is_scared);
                        } else {
                            stats.lives--;
                            remove_stats(stats_win);
                            draw_stats(stats_win, stats);
                        }
                        
                        reset = true;   //Reset per nuova manche
                    } 
                    
                    // 3. Rana si muove in orizzontale su marciapiede/argine o nel fiume
                    else {
                        remove_frog(pg_win, frog.y, frog.x, on_grass);
                        draw_frog(pg_win, m.obj, is_scared);
                
                        frog = m.obj;
                        update_lane(&active_lane, m.obj.direction);

                        //Se la rana è su una corsia d'acqua (1..NUM_STREAMS) allora non è sull'erba
                        on_grass = is_on_grass(active_lane);
                    }
                break;
            
                               
                default:
                    break;
                }
            }
        }

        //Reset della rana
        if(reset) {
            remove_frog(pg_win, frog.y, frog.x, on_grass);
            reset_frog(pg_win, shared_pipe, private_frog_pipe, sync_sem, &frog_pid, &frog,
                        &active_lane, is_scared, win_height, win_width);
            reset = false;
        }

        //L'utente ha vinto
        if(check_win(stats, burrows)) {
            break;
        }


        wrefresh(pg_win);
        wrefresh(stats_win);
        usleep(100000);
    }

    /* Chiudo le risorse utilizzate e termino i processi */
    delwin(stats_win);

    kill(frog_pid, SIGTERM);
    waitpid(frog_pid, NULL, 0);

    clean_up_pipe(shared_pipe);
    clean_up_pipe(private_frog_pipe);

    //Distruggo il semaforo
    clean_up_semaphore(sync_sem, sem_name);
}


