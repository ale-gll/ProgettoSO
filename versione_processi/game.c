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
        int crocs_per_stream = (rand() % 2) + 3; // 3 o 4 crocs per stream

        streams[i].y = start_y;
        streams[i].direction = direction;
        streams[i].spawn_time_interval = (rand() % 3) + 2;   //2,3,4 secondi
        streams[i].delay = delay[i % 2];

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


int get_free_pid_index(ActiveProcesses *ap, int size) {
    for (int i = 0; i < size; i++)
    {
        if(ap->croc_pids[i] == -1) return i;
    }
    return -1;  // Nessuno slot disponibile
}


int get_free_stream_slot_index(Object *objs, int size) {
    for (int i = 0; i < size; i++)
    {
        if(objs[i].pid == -1) return i;
    }
    return -1;    
}


void init_active_processes_struct(ActiveProcesses *ap, Stream *streams) {
    
    ap->total_crocs = get_number_of_crocs(streams);
    ap->total_projs = get_number_of_projs(streams);

    // Alloco la memoria
    ap->croc_pids = malloc(ap->total_crocs * sizeof(pid_t));
    ap->proj_pids = malloc(ap->total_projs * sizeof(pid_t));

    // Inizializzo gli array 
    for (int i = 0; i < ap->total_crocs; i++) {
        ap->croc_pids[i] = -1;
    }

    for (int i = 0; i < ap->total_projs; i++) {
        ap->proj_pids[i] = -1;
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


bool spawn_initial_crocs(Stream *streams, IPCHandles *ipc, ActiveProcesses *ap, int window_width) {

    bool flag = true;

    for (int i = 0; i < NUM_STREAMS; i++) {
        flag &= spawn_single_croc(&streams[i], i, ipc, ap, window_width);
    }   
    
    return flag;
}


bool is_spawn_time(time_t now, Stream s) {
    return (now - s.last_spawn_time > s.spawn_time_interval);
}


bool spawn_single_croc(Stream *stream, int stream_index, IPCHandles *ipc, ActiveProcesses *ap, int window_width) {
    Object croc;

    croc.direction = stream->direction;
    croc.type = OBJ_CROC;
    croc.x = (stream->direction == DIR_LEFT) ? window_width : -CROC_WIDTH;
    croc.y = stream->y;
    
    int pid_index = get_free_pid_index(ap, ap->total_crocs);
    if(pid_index == -1) {
        return false;   // nessuno slot processi libero
    }

    int croc_index = get_free_stream_slot_index(stream->crocs, stream->num_crocs);
    if(croc_index == -1) {
        return false;   // nessuno slot stream libero
    }

    // Avvio il processo
    pid_t pid = croc_process(ipc, croc, stream_index, croc_index, stream->delay, pid_index);
    if(pid == -1) {
        return false;   // errore fork
    }

    // Aggiorna struttura dati
    croc.pid = pid;
    ap->croc_pids[pid_index] = pid;
    stream->crocs[croc_index] = croc;
    stream->last_spawn_time = time(NULL);

    log_croc_event(DEBUG_LOG_FILE, "Spawned", stream_index, croc_index, pid, croc.x, croc.y);

    return true;
}


void game_loop(WINDOW *win, int start_y, int start_x) {
    srand(time(NULL));

    IPCHandles ipc;
    char *sync_sem_name = "/shared_semaphore";
    int win_height, win_width;

    getmaxyx(win, win_height, win_width);
    WINDOW *stats_win = newwin(3, win_width, start_y-3, start_x);

    //Inizializzo gli Stream del fiume
    Stream streams[NUM_STREAMS];
    init_streams(streams, win_height-4);

    //Inizializzo gli strumenti per la comunicazione interprocesso
    bool flag = init_ipc_handles(&ipc, sync_sem_name);


    //Inizializzo il campo di gioco
    Object frog;
    Burrow burrows[NUM_BURROWS];
    Stats stats;

    frog = init_frog(win_height, win_width);     
    init_burrows(burrows); 
    init_stats(&stats);
    init_playground(win, stats_win, win_height, win_width, frog, burrows, stats);


    // Inizializzo i coccodrilli
    ActiveProcesses active_processes;
    init_active_processes_struct(&active_processes, streams);

    // Avvio i coccodrilli
    flag &= spawn_initial_crocs(streams, &ipc, &active_processes, win_width);

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

                        draw_frog(win, m.obj, is_scared);                        
                        frog = m.obj;
                    }
                }
            break;
        
            case OBJ_CROC:
                if (m.msg_type == MSG_UPDATE_POS) 
                {
                    old_croc = streams[m.stream_index].crocs[m.stream_obj_index];
                    pid_t pid = active_processes.croc_pids[m.pid_index];

                    // Se il processo esiste ancora disegno
                    if (pid > 0)
                    {
                        if (is_fully_out_of_screen(win_width, m.obj.x, CROC_WIDTH)) // Fuori dallo schermo
                        {
                            // Rimuovo dal rendering
                            remove_croc(win, old_croc.y, old_croc.x);

                            // Uccido il processo
                            kill(pid, SIGTERM);
                            waitpid(pid, NULL, 0);                          

                            // Libero lo slot processi
                            active_processes.croc_pids[m.pid_index] = -1;

                            // Libero lo slot nello stream
                            streams[m.stream_index].crocs[m.stream_obj_index] = OBJ_DUMMY;
                        }
                        else {
                            remove_croc(win, old_croc.y, old_croc.x);
                            draw_croc(win, m.obj);
                            streams[m.stream_index].crocs[m.stream_obj_index] = m.obj;
                        }
                    }
                }

                // Dopo un intervallo casuale genero un nuovo coccodrillo
                time_t t = time(NULL);
                if(is_spawn_time(t, streams[m.stream_index])) {
                    bool success = spawn_single_croc(&streams[m.stream_index], m.stream_index, &ipc, &active_processes, win_width);

                    if(success) 
                    {
                        // Aggiorno lo stream
                        streams[m.stream_index].last_spawn_time = t;
                        streams[m.stream_index].spawn_time_interval = (rand() % 3) + 2;
                    }
                } 
            break;

            
                            
            default:
                break;
            }
        }

        // Reset della manche 
        if(reset) {         
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

    kill_all(active_processes.croc_pids, active_processes.total_crocs);

    cleanup_ipc_handles(&ipc, sync_sem_name);

    close_window(stats_win);    //Elimino la finestra delle statistiche
    print_game_result(win, win_height, win_width, is_winner);   //Stampa schermata di fine
}


void log_crocs_state(const char *filename, ActiveProcesses *ap, Stream *streams, int num_streams)
{
    FILE *f = fopen(filename, "a");
    if (!f) {
        perror("Errore apertura file log");
        return;
    }

    // Timestamp solo ora:min:sec
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char time_buf[9];
    strftime(time_buf, sizeof(time_buf), "%H:%M:%S", tm_info);

    fprintf(f, "[%s]\n", time_buf);

    // Array pid globali
    fprintf(f, "PIDs: %d [", ap->total_crocs);
    for (int i = 0; i < ap->total_crocs; i++) {
        fprintf(f, "%d", ap->croc_pids[i]);
        if (i < ap->total_crocs - 1) fprintf(f, ", ");
    }
    fprintf(f, "]\n");

    // Array per ogni stream
    for (int s = 0; s < num_streams; s++) {
        fprintf(f, "[%d] (", s);
        for (int j = 0; j < streams[s].num_crocs; j++) {
            fprintf(f, "%d", streams[s].crocs[j].pid);
            if (j < streams[s].num_crocs - 1) fprintf(f, ", ");
        }
        fprintf(f, ")\n");
    }

    fprintf(f, "\n");
    fclose(f);
}