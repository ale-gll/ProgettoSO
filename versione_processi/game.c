#include <curses.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "utils.h"
#include "frog_process.h"
#include "croc_process.h"
#include "game.h"

void init_streams(Stream *streams, int start_y) {
    int delay[] = {150000, 200000};
    int direction = (rand() % 2 == 0) ? DIR_LEFT : DIR_RIGHT;

    for (int i = 0; i < NUM_STREAMS; i++) {
        int crocs_per_stream = (rand() % 2) + 2; // 2 o 3 crocs per stream

        streams[i].y = start_y;
        streams[i].direction = direction;
        streams[i].delay = delay[i % 2];

        streams[i].next_croc_index = 0;
        streams[i].next_proj_index = 0;

        streams[i].num_crocs = crocs_per_stream;
        streams[i].num_projs = MAX_PROJ_PER_STREAM;

        streams[i].crocs = malloc(sizeof(Object) * crocs_per_stream);
        streams[i].projs = malloc(sizeof(Object) * MAX_PROJ_PER_STREAM);

        for (int j = 0; j < crocs_per_stream; j++) {
            streams[i].crocs[j].pid = -1;
        }
        for (int j = 0; j < MAX_PROJ_PER_STREAM; j++) {
            streams[i].projs[j].pid = -1;
        }

        direction = (direction == DIR_LEFT) ? DIR_RIGHT : DIR_LEFT;
        start_y -= 2;
    }
}


void free_streams(Stream *streams) {
    for (int i = 0; i < NUM_STREAMS; i++) {
        free(streams[i].crocs);
        free(streams[i].projs);
        streams[i].crocs = NULL;
        streams[i].projs = NULL;
    }
}


int get_number_of_crocs(Stream *streams) {
    int acc = 0;

    for (int i = 0; i < NUM_STREAMS; i++) acc += streams[i].num_crocs;

    return acc;
}


int get_number_of_projs(Stream *streams) {
    int acc = 0;

    for (int i = 0; i < NUM_STREAMS; i++) acc += streams[i].num_projs;
    return acc;
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
    set_message(&kill_msg, MSG_KILL, NULL, NULL, NULL, NULL);
    write(ipc->frog_pipe[1], &kill_msg, sizeof(Message)) == -1;
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
    return res && (stats.lives > 0) && (stats.time > 0);
}


bool check_is_on_croc(Object frog, Object *crocs, int num_crocs) {
    for(int i = 0; i < num_crocs; i++) {
        if(crocs[i].pid > 0) {
            if(frog.y == crocs[i].y &&
               frog.x + FROG_WIDTH > crocs[i].x &&
               frog.x < crocs[i].x + CROC_WIDTH) {
                return true; // Rana sopra un coccodrillo valido
            }
        }
    }
    return false;
}


void remove_all_stream_objects(WINDOW *win, Stream *stream) {
    for (int i = 0; i < NUM_STREAMS; i++) {
        for (int j = 0; j < stream[i].num_crocs; j++) {
            Object croc = stream[i].crocs[j];
            if(stream[i].crocs[j].pid > 0) {
                remove_croc(win, croc.y, croc.x);
            }
        }  
    }
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


bool spawn_initial_crocs(Stream *streams, IPCHandles *ipc, int window_width){

    for (int i = 0; i < NUM_STREAMS; i++) {
        Object croc;
        croc.direction = streams[i].direction;
        croc.type = OBJ_CROC;
        croc.x = (streams[i].direction == DIR_LEFT) ? window_width : -CROC_WIDTH;
        croc.y = streams[i].y;

        //Trova una pipe libera
        int pipe_index = find_free_croc_pipe_slot(ipc->croc_pipes, ipc->total_crocs);
        if (pipe_index == -1) {
            return false;
        }
        
        //Inizializza pipe
        if(pipe(ipc->croc_pipes[pipe_index].pipe) == -1) {
            return false;
        }
        
        //Avvio processo
        pid_t pid = croc_process(ipc, croc, i, streams[i].next_croc_index, streams[i].delay, pipe_index);
        if (pid == -1) {
            return false;
        }
        close(ipc->croc_pipes[pipe_index].pipe[0]); //Lato lettura

        ipc->croc_pipes[pipe_index].pid = pid;      
        streams[i].crocs[streams[i].next_croc_index] = croc;
        streams[i].next_croc_index += 1;
    }
    return true;
}


void kill_object(int write_fd, pid_t pid) {
    Message m; 
    set_message(&m, MSG_KILL, NULL, NULL, NULL, NULL);
    ssize_t w = write(write_fd, &m, sizeof m);
    if (w == -1) debug_log("kill_object", pid, "Write failed", DEBUG_LOG_FILE);
    else {
        debug_log("kill_object", pid, "Successful write", DEBUG_LOG_FILE);
    }
}


void kill_all(ProcessComm *obj_pipe_array, int arr_size) {

    for (int i = 0; i < arr_size; i++) {
        if (obj_pipe_array->pid > 0) {
            kill_object(obj_pipe_array[i].pipe[1], obj_pipe_array[i].pid);
            waitpid(obj_pipe_array[i].pid, NULL, 0);
        }
    }
    
}


void game_loop(WINDOW *win, int start_y, int start_x) {

    IPCHandles ipc;
    char *sync_sem_name = "/shared_semaphore";
    int win_height, win_width;

    getmaxyx(win, win_height, win_width);
    WINDOW *stats_win = newwin(3, win_width, start_y-3, start_x);

    //Inizializzo gli Stream del fiume
    Stream streams[NUM_STREAMS];
    init_streams(streams, win_height-4);
    ipc.total_crocs = get_number_of_crocs(streams);
    ipc.total_projs = get_number_of_projs(streams);
    

    //Inizializzo gli strumenti per la comunicazione interprocesso
    bool flag = init_ipc_handles(&ipc, sync_sem_name);


    srand(time(NULL));

    //Inizializzo il campo di gioco
    Object frog;
    Burrow burrows[NUM_BURROWS];
    Stats stats;

    frog = init_frog(win_height, win_width); 
    init_burrows(burrows); 
    init_stats(&stats);
    init_playground(win, stats_win, win_height, win_width, frog, burrows, stats);

    //Inizializzo i coccodrilli e le loro pipe
    flag &= spawn_initial_crocs(streams, &ipc, win_width);

    //Inizializzo il processo rana
    pid_t frog_pid = frog_process(win, &ipc, frog);



    //Processo principale

    int active_lane = 0;
    bool on_grass = true;
    bool is_scared = false;
    bool reset = false;
    bool is_winner = false;
    bool on_croc = false;
    int hovered_croc = -1;  //pid del coccodrillo dove si trova la rana
    time_t last_update_time = time(NULL);
    

    close(ipc.shared_pipe[1]);
    close(ipc.frog_pipe[0]);
    set_nonblocking(ipc.shared_pipe[0]);    //Pipe non bloccante

    while(flag) {
        Message m;
        Object old_croc; 
        bool has_stats_changed = false;

        ssize_t bytes = read(ipc.shared_pipe[0], &m, sizeof m);
        if(bytes > 0) {
            
            switch (m.obj.type)
            {
            case OBJ_FROG:
                if(m.msg_type == MSG_UPDATE_POS) 
                {
                    // 1. Rana esegue una mossa non valida
                    if( (active_lane == 0 && m.obj.direction == DIR_DOWN)
                    || is_out_of_screen(win_width, m.obj.x, FROG_WIDTH) ) {
                        Message reset_msg;
                        set_message(&reset_msg, MSG_SET_FROG, &frog, NULL, NULL, NULL);
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
                            has_stats_changed = true;
                        }
                        reset = true;   //Reset per nuova manche
                    }
                    
                    // 3. Rana si muove in orizzontale su marciapiede/argine o nel fiume
                    else {
                        remove_frog(win, frog.y, frog.x, on_grass); //Rimuovo vecchia rana
                        update_lane(&active_lane, m.obj.direction); //Aggiorno active_lane
                        on_grass = is_on_grass(active_lane);

                        //La rana si trova sull'acqua
                        if(active_lane > 0 && active_lane <= NUM_STREAMS) {
                            on_croc = check_is_on_croc(m.obj, streams[active_lane-1].crocs, streams[active_lane-1].num_crocs);
                        }
                        
                        if(!on_grass && !on_croc) reset = true;
                        draw_frog(win, m.obj, is_scared);
                        frog = m.obj;
                    }
                }
            break;
        
            case OBJ_CROC:
                if(m.msg_type == MSG_UPDATE_POS) 
                {
                    old_croc = streams[m.stream_index].crocs[m.stream_obj_index];

                    //Se il processo esiste ancora disegno
                    if(ipc.croc_pipes[m.pipe_index].pid > 0) {
                        if(is_fully_out_of_screen(win_width, m.obj.x, CROC_WIDTH)) {
                            remove_croc(win, old_croc.y, old_croc.x);
                            kill_object(ipc.croc_pipes[m.pipe_index].pipe[1], (pid_t) old_croc.pid);
                            waitpid((pid_t) old_croc.pid, NULL, 0);
                            close(ipc.croc_pipes[m.pipe_index].pipe[1]);    //Lato scrittura

                            ipc.croc_pipes[m.pipe_index].pid = -1;
                            streams[m.stream_index].crocs[m.stream_obj_index] = OBJ_DUMMY;
                            streams[m.stream_index].next_croc_index = m.stream_obj_index;
                        } 
                        else {
                            remove_croc(win, old_croc.y, old_croc.x);
                            draw_croc(win, m.obj);
                            streams[m.stream_index].crocs[m.stream_obj_index] = m.obj;
                        }
                    }
                }
            break;

            
                            
            default:
                break;
            }
        }

        //Reset della manche 
        if(reset) {
            //Cancello e ricreo i coccodrilli
            kill_all(ipc.croc_pipes, ipc.total_crocs);
            remove_all_stream_objects(win, streams);
            free_streams(streams);
            init_streams(streams, start_y);

            // for (int i = 0; i < ipc.total_crocs; i++)
            // {
            //     clean_up_pipe()
            // }
                      

            
            //Cancello e resetto la rana
            remove_frog(win, frog.y, frog.x, on_grass);
            reset_frog(win, &ipc, &frog_pid, &frog, &active_lane, is_scared, win_height, win_width);

            reset = false;
        }

        //Aggiorno il tempo 
        time_t now = time(NULL);
        if(now - last_update_time  > 1) {
            stats.time--;
            remove_stats(stats_win);
            draw_stats(stats_win, stats);
            has_stats_changed = true;
            last_update_time  = now;
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
        if(has_stats_changed) wrefresh(stats_win);
        usleep(15000);
    }

    /* Chiudo le risorse utilizzate e termino i processi */

    kill(frog_pid, SIGTERM);
    waitpid(frog_pid, NULL, 0);

    kill_all(ipc.croc_pipes, ipc.total_crocs);
    kill_all(ipc.proj_pipes, ipc.total_projs);

    cleanup_ipc_handles(&ipc, sync_sem_name);
    free_streams(streams);

    close_window(stats_win);    //Elimino la finestra delle statistiche
    print_game_result(win, win_height, win_width, is_winner);   //Stampa schermata di fine
}