#include <curses.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "shared.h"
#include "utils.h"
#include "frog_process.h"
#include "croc_process.h"
#include "game.h"


void init_crocs(IPCHandles *ipc, Stream *streams, int y, int x, int win_width) {
    Object croc_obj;
    int dir;

    dir = rand() % 2;   // DIR_LEFT = 0 / DIR_RIGHT = 1
    croc_obj.type = OBJ_CROC;

    for(int i = 0; i < NUM_STREAMS; i++) {
        streams[i].num_crocs = 0;
        streams[i].delay = (i % 2 == 0) ? 100000 : 200000;    //correnti dispari più lente

        for (int j = 0; j < MAX_CROCS_PER_STREAM; j++)
        {
            croc_obj.direction = dir;
            croc_obj.x = (croc_obj.direction == DIR_LEFT) 
                        ? (x + win_width + CROC_WIDTH) 
                        : x - CROC_WIDTH
            ;
            croc_obj.y = y;

            //Avvio il processo
            pid_t pid = croc_process(ipc, croc_obj, i, j, streams[i].delay);
            croc_obj.pid = pid;


            streams[i].objs[j] = croc_obj;
            streams[i].num_crocs++;
            
            // Random delay tra creazioni (distanza variabile)
            usleep(30000 + rand() % 50000);
        }
        
        //Aggiorno l'array di Stream
        dir = (dir == DIR_LEFT) ? DIR_RIGHT : DIR_LEFT;

        //Aggiorno la posizione y
        y -= FROG_CROC_HEIGHT;
    }
}

bool is_out_of_screen(int win_width, int obj_x, int obj_width) {
    return (obj_x < 0 || obj_x + obj_width > win_width);
}

bool is_fully_out_of_screen(int win_width, int obj_x, int obj_width) {
    return (obj_x + obj_width <= 0 || obj_x >= win_width);
}

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

void reset_frog(WINDOW *win, IPCHandles *ipc, pid_t *frog_pid, Object *frog, int *active_lane, bool is_scared, int win_height, int win_width) 
{
    // 1. Uccido la rana attuale
    Message kill_msg;
    set_message(&kill_msg, MSG_KILL, *frog, NULL, NULL);
    write(ipc->frog_pipe[1], &kill_msg, sizeof(Message));
    waitpid(*frog_pid, NULL, 0);

    // 2. Svuoto eventuali messaggi residui
    Message discard;
    while (read(ipc->shared_pipe[0], &discard, sizeof(discard)) > 0);

    //Chiudo le vecchie pipe
    close(ipc->shared_pipe[0]);
    close(ipc->frog_pipe[1]);

    if (pipe(ipc->shared_pipe) == -1 || pipe(ipc->frog_pipe) == -1) return;
    set_nonblocking(ipc->shared_pipe[0]);

    *frog = init_frog(win_height, win_width);
    *active_lane = 0;
    *frog_pid = frog_process(win, ipc, *frog);
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

void print_game_result(WINDOW *win, int win_height, int win_width, bool is_winner) {
    wclear(win);
    wbkgd(win, A_NORMAL);
    box(win, 0, 0);

    const char *press_enter_str = "Press ENTER to exit...";
    const char *message = is_winner ? "YOU WIN" : "YOU LOSE";

    int message_len = strlen(message);
    int press_len = strlen(press_enter_str);

    wattron(win, COLOR_PAIR(START_MENU_COLOR_PAIR));
    mvwprintw(win, win_height/2, (win_width - message_len)/2, "%s", message);
    mvwprintw(win, (win_height/2)+1, (win_width - press_len)/2, "%s", press_enter_str);
    wattroff(win, COLOR_PAIR(START_MENU_COLOR_PAIR));
    wrefresh(win);
    sleep(1);
}

bool init_ipc_handles(IPCHandles *ipc, char *sync_sem_name, char *crocs_sem_name) {

    if(pipe(ipc->shared_pipe) == -1 || pipe(ipc->frog_pipe) == -1 || pipe(ipc->crocs_pipe) == -1) {
        return false;
    }

    ipc->sync_sem = init_shared_semaphore(sync_sem_name);
    ipc->crocs_sem = init_shared_semaphore(crocs_sem_name);
    if(ipc->sync_sem == NULL || ipc->crocs_sem == NULL) {
        return false;
    }

    return true;
}

void game_loop(WINDOW *win, int start_y, int start_x) {

    IPCHandles ipc;
    char *sync_sem_name = "/shared_semaphore";
    char *crocs_sem_name = "/crocs_semaphore";
    int win_height, win_width;

    getmaxyx(win, win_height, win_width);
    WINDOW *stats_win = newwin(3, win_width, start_y-3, start_x);

    //Inizializzo gli strumenti per la comunicazione interprocesso
    bool flag = init_ipc_handles(&ipc, sync_sem_name, crocs_sem_name);


    srand(time(NULL));

    //Inizializzo il campo di gioco
    Object frog;
    Burrow burrows[NUM_BURROWS];
    Stats stats;

    frog = init_frog(win_height, win_width); 
    init_burrows(burrows); 
    init_stats(&stats);
    init_playground(win, stats_win, win_height, win_width, frog, burrows, stats);

    //Inizializzo i coccodrilli
    // Stream streams[NUM_STREAMS];
    // init_crocs(&ipc, streams, (start_y + win_height - 2), win_width);


    //Inizializzo il processo rana
    pid_t frog_pid = frog_process(win, &ipc, frog);


    //Processo principale

    int active_lane = 0;
    bool on_grass = true;
    bool is_scared = false;
    bool reset = false;
    bool is_winner = false;

    close(ipc.shared_pipe[1]);
    close(ipc.frog_pipe[0]);
    set_nonblocking(ipc.shared_pipe[0]);    //Pipe non bloccante

    while(flag) {
        Message m;

        ssize_t bytes = read(ipc.shared_pipe[0], &m, sizeof m);
        if(bytes > 0) {
            
            if(m.msg_type == MSG_UPDATE_POS) {

                switch (m.obj.type)
                {
                case OBJ_FROG:
                    // 1. Rana esegue una mossa non valida
                    if( (active_lane == 0 && m.obj.direction == DIR_DOWN)
                    || is_out_of_screen(win_width, m.obj.x, FROG_WIDTH) ) {
                        Message reset_msg;
                        set_message(&reset_msg, MSG_SET_FROG, frog, NULL, NULL);
                        if(write(ipc.frog_pipe[1], &reset_msg, sizeof(Message)) == -1) flag = false;
                    }

                    // 2. Rana cerca di salire oltre l'argine (tane)
                    else if(active_lane == (NUM_STREAMS+1) && m.obj.direction == DIR_UP) {
                        if (check_burrows(m.obj, burrows)) {
                            draw_frog(win, m.obj, is_scared);
                        } else {
                            stats.lives--;
                            remove_stats(stats_win);
                            draw_stats(stats_win, stats);
                        }
                        
                        reset = true;   //Reset per nuova manche
                    } 
                    
                    // 3. Rana si muove in orizzontale su marciapiede/argine o nel fiume
                    else {
                        remove_frog(win, frog.y, frog.x, on_grass);
                        draw_frog(win, m.obj, is_scared);
                
                        frog = m.obj;
                        update_lane(&active_lane, m.obj.direction);

                        //Se la rana è su una corsia d'acqua (1..NUM_STREAMS) allora non è sull'erba
                        on_grass = is_on_grass(active_lane);
                    }
                break;
            
                case OBJ_CROC:
                    if(is_fully_out_of_screen(win_width, m.obj.x, CROC_WIDTH)) {
                        //rimuovi coccodrillo nella vecchia posizione
                        kill(m.obj.pid, SIGTERM);
                        waitpid(m.obj.pid, NULL, 0);
                    } 
                    else {
                        //Ridisegna coccodrillo
                    }

                break;

                
                               
                default:
                    break;
                }
            }
        }

        //Reset della rana
        if(reset) {
            remove_frog(win, frog.y, frog.x, on_grass);
            reset_frog(win, &ipc, &frog_pid, &frog,
                        &active_lane, is_scared, win_height, win_width);
            reset = false;
        }

        //L'utente ha vinto
        if(check_win(stats, burrows)) {
            is_winner = true;
            break;
        } 
        //L'utente ha perso
        else if(stats.lives == 0) {
            break;
        }

        wrefresh(win);
        wrefresh(stats_win);
        usleep(100000);
    }

    /* Chiudo le risorse utilizzate e termino i processi */

    kill(frog_pid, SIGTERM);
    waitpid(frog_pid, NULL, 0);

    clean_up_pipe(ipc.shared_pipe);
    clean_up_pipe(ipc.frog_pipe);
    clean_up_semaphore(ipc.sync_sem, sync_sem_name);     //Distruggo il semaforo
    clean_up_semaphore(ipc.crocs_sem, crocs_sem_name);

    close_window(stats_win);    //Elimino la finestra delle statistiche
    print_game_result(win, win_height, win_width, is_winner);   //Stampa schermata di fine
}


