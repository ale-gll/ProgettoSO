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
    int delay[] = {180000, 220000};
    int direction = (rand() % 2 == 0) ? DIR_LEFT : DIR_RIGHT;

    for (int i = 0; i < NUM_STREAMS; i++) {
        int crocs_per_stream = (rand() % 2) + 2; // 2 o 3 crocs per stream

        streams[i].y = start_y;
        streams[i].direction = direction;
        streams[i].spawn_time_interval = (rand() % 3) + 4; // 4,5,6 secondi
        streams[i].delay = delay[i % 2];
        streams[i].last_spawn_time = 0; // inizialmente nessuno spawn

        streams[i].max_crocs = crocs_per_stream;
        streams[i].max_projs = MAX_PROJ_PER_STREAM;

        streams[i].croc_count = 0;
        streams[i].proj_count = 0;

        // Liste vuote all'inizio
        streams[i].crocs = NULL;
        streams[i].projs = NULL;

        direction = (direction == DIR_LEFT) ? DIR_RIGHT : DIR_LEFT;
        start_y -= 2;
    }
}


bool is_out_of_screen(int win_width, int obj_x, int obj_width) {
    return (obj_x < 0 || obj_x + obj_width > win_width);
}


bool is_fully_out_of_screen(int win_width, int obj_x, int obj_width) {
    return (obj_x + obj_width <= 0 || obj_x >= win_width);
}


bool is_on_burrow(int active_lane, int dir) {
    return active_lane == (NUM_STREAMS+1) && dir == DIR_UP;
}


bool is_on_grass(int lane) {
    return !(lane >= 1 && lane <= NUM_STREAMS);
}


bool is_on_croc(Stream *stream, Object frog, int *hovered_croc){
    ObjectNode *curr = stream->crocs;

    while(curr) {
        if(frog.x > curr->data.x 
            && (frog.x + FROG_WIDTH) < (curr->data.x + CROC_WIDTH))
        {
            *hovered_croc = curr->data.pid;
            return true;
        }

        // Avanzo nella lista
        curr = curr->next;
    }
    return false;
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

    burrows[0].start_x = 5;
    burrows[1].start_x = 18;
    burrows[2].start_x = 31;
    burrows[3].start_x = 44;
    burrows[4].start_x = 57;

    for(int i = 0; i < NUM_BURROWS; i++) {
        burrows[i].is_occupied = false;     //tana non occupata
        burrows[i].end_x = burrows[i].start_x + BURROW_WIDTH;
    }
}


void init_stats(Stats *stats) {
    stats->score = 0;
    stats->lives = 5;
    stats->time = TIME_PER_ROUND;
}


int update_lane(int active_lane, int direction) {
    switch (direction) {
        case DIR_UP: 
            active_lane += (active_lane < NUM_STREAMS+1) ? 1 : 0; 
            break;
        case DIR_DOWN: 
            active_lane -= (active_lane > 0) ? 1 : 0;
            break;
        default:
            break;
    }

    return active_lane;
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


bool check_win(Stats stats, Burrow burrows[5]) {
    bool res = true;
    int burrows_size = 5;

    for(int i = 0; i < burrows_size; i++) {
        res &= burrows[i].is_occupied;
    }
    return res && (stats.lives > 0) && (stats.time > 0);
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


bool spawn_initial_crocs(Stream *streams, IPCHandles *ipc, int window_width) {

    bool flag = true;

    for (int i = 0; i < NUM_STREAMS; i++) {
        flag &= spawn_single_croc(&streams[i], i, ipc, window_width);
    }   
    
    return flag;
}


bool is_spawn_time(time_t now, Stream s) {
    return (now - s.last_spawn_time > s.spawn_time_interval);
}


bool spawn_single_croc(Stream *stream, int stream_index, IPCHandles *ipc, int window_width) {
    // Alloca nodo per il nuovo coccodrillo
    ObjectNode *new_node = malloc(sizeof(ObjectNode));
    if(!new_node) return false;

    // Imposta dati di base del nodo
    new_node->data.direction = stream->direction;
    new_node->data.type = OBJ_CROC;
    new_node->data.x = (stream->direction == DIR_LEFT) ? window_width : -CROC_WIDTH;
    new_node->data.y = stream->y;

    // Avvia processo figlio
    pid_t pid = croc_process(ipc, new_node->data, stream_index, stream->delay);
    if(pid == -1) {
        free(new_node);
        return false;
    }

    // Aggiorna dati del nodo
    new_node->data.pid = (int) pid;
    new_node->next = NULL;

    // Inserisci in coda alla lista dei coccodrilli nello stream
    if(stream->crocs == NULL) {
        stream->crocs = new_node;
    } else {
        ObjectNode *curr = stream->crocs;
        while(curr->next) curr = curr->next;
        curr->next = new_node;
    }

    // Aggiorna dati stream
    stream->last_spawn_time = time(NULL);
    stream->croc_count++;
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


    // Avvio i coccodrilli
    flag &= spawn_initial_crocs(streams, &ipc, win_width);

    //Inizializzo il processo rana
    pid_t frog_pid = frog_process(win, &ipc, frog);



    //Processo principale

    int active_lane = 0;
    bool on_grass = true;
    bool is_scared = false;
    bool reset = false;
    bool is_winner = false;
    bool has_lost_manche = false;
    bool on_croc = false;
    int hovered_croc = -1;  //pid del coccodrillo dove si trova la rana
    time_t last_update_time = time(NULL);


    // Chiudo i file descriptor che non uso
    close(ipc.frog_pipe[0]);
    set_nonblocking(ipc.shared_pipe[0]);    //Pipe non bloccante

    while(flag) {
        Message m;
        Object old_croc; 

        ssize_t bytes = read(ipc.shared_pipe[0], &m, sizeof m);
        if(bytes > 0) {
            
            switch (m.obj.type)
            {
            case OBJ_FROG:
                Object new_frog = m.obj;

                if(m.msg_type == MSG_UPDATE_POS) 
                {
                    // 1. Rana esegue una mossa non valida
                    if( (active_lane == 0 && new_frog.direction == DIR_DOWN)
                    || is_out_of_screen(win_width, new_frog.x, FROG_WIDTH) ) {
                        Message reset_msg;
                        set_message(&reset_msg, MSG_SET_FROG, &frog, NULL);
                        if(write(ipc.frog_pipe[1], &reset_msg, sizeof(Message)) == -1) flag = false;
                    }

                    // 2. Rana cerca di salire oltre l'argine (tane)
                    else if(is_on_burrow(active_lane, new_frog.direction)) 
                    {
                        if (check_burrows(new_frog, burrows)) {
                            draw_frog(win, new_frog, is_scared);
                            wrefresh(win);
                        } else {
                            has_lost_manche = true;
                        }
                        reset = true;   //Reset per nuova manche
                    }
                    else {

                        int next_active_lane = update_lane(active_lane, new_frog.direction);

                        // Se la rana è nel fiume deve essere sopra un coccodrillo
                        if(is_on_grass(next_active_lane)) {
                            remove_frog(win, frog.y, frog.x, on_grass);
                            active_lane = next_active_lane;
                            on_grass = true;
                            draw_frog(win, new_frog, is_scared);
                            frog = new_frog;
                            hovered_croc = -1;
                        }
                        else {
                            on_croc = is_on_croc(&streams[next_active_lane-1], new_frog, &hovered_croc);

                            if (on_croc) {
                                remove_frog(win, frog.y, frog.x, on_grass);
                                active_lane = next_active_lane;
                                on_grass = false;
                                draw_frog(win, new_frog, is_scared);
                                frog = new_frog;
                            } else {
                                has_lost_manche = true;
                                reset = true;
                            }
                        }
                    }
                }
            break;
        
            case OBJ_CROC:
                if(m.msg_type == MSG_UPDATE_POS) 
                {
                    Stream *s = &streams[m.stream_index];

                    // Trova il nodo corrispondente al pid
                    ObjectNode *prev = NULL;
                    ObjectNode *curr = s->crocs; 

                    while(curr && curr->data.pid != m.obj.pid) {
                        prev = curr;
                        curr = curr->next;
                    }

                    if (curr) {
                        // Processo trovato -> aggiorno o rimuovo
                        if(is_fully_out_of_screen(win_width, m.obj.x, CROC_WIDTH)) {
                            // Rimuovo dal rendering
                            remove_croc(win, curr->data.y, curr->data.x);

                            // Uccido il processo
                            kill(curr->data.pid, SIGTERM);
                            waitpid(curr->data.pid, NULL, 0);

                            // Rimuovo dalla lista
                            if (prev) prev->next = curr->next;
                            else s->crocs = curr->next;

                            // Decremento contatore coccodrilli attivi
                            s->croc_count--;

                            // Libero la memoria allocata
                            free(curr);
                        } else {
                            // Aggiorna coccodrillo
                            remove_croc(win, curr->data.y, curr->data.x);
                            draw_croc(win, m.obj);
                            curr->data = m.obj;

                            // Se la rana è sopra questo coccodrillo
                            if(hovered_croc == m.obj.pid && on_croc) {
                                // Aggiorno grafica
                                remove_frog(win, frog.y, frog.x, on_grass);
                                frog.x += (s->direction == DIR_LEFT) ? -1 : 1;
                                draw_frog(win, frog, is_scared);

                                // Invio aggiornamento al processo rana
                                Message frog_update;
                                set_message(&frog_update, MSG_SET_FROG, &frog, NULL);
                                if (write(ipc.frog_pipe[1], &frog_update, sizeof(frog_update)) == -1) {
                                    flag = false;
                                }                                
                            }
                        }
                    }
                }
            break;

            
                            
            default:
                break;
            }
        
        }

        // Reset della manche 
        if(reset) {
            // Elimino tutti gli elementi del fiume
            clean_all_stream_objects(win, streams);     // Esegue wait()

            // Eliminazione e kill della rana
            remove_frog(win, frog.y, frog.x, on_grass);
            kill(frog_pid, SIGTERM);
            waitpid(frog_pid, NULL, 0);
            wrefresh(win);

            remove_stats(stats_win);
            // Aggiorno le statistiche
            if(has_lost_manche) {
                stats.lives--;
            }
            stats.time = TIME_PER_ROUND;
            
            // Aggiorna punteggio*************

            draw_stats(stats_win, stats);

            // Chiudo le risorse
            clean_up_pipe(ipc.shared_pipe);
            clean_up_pipe(ipc.frog_pipe);

            // Ricreo le pipe
            if (pipe(ipc.shared_pipe) == -1 || pipe(ipc.frog_pipe) == -1) {
                flag = false;
            }
            set_nonblocking(ipc.shared_pipe[0]);

            active_lane = 0;
            on_grass = true;
            hovered_croc = -1;
            on_croc = false;
            is_scared = (stats.lives <= 2);

            // Ricreo il processo rana
            frog = init_frog(win_height, win_width);
            frog_pid = frog_process(win, &ipc, frog);
            if(frog_pid == -1) {
                flag = false;
            }
            close(ipc.frog_pipe[0]);
            draw_frog(win, frog, is_scared);

            // Ricreo i coccodrilli
            init_streams(streams, win_height-4);
            spawn_initial_crocs(streams, &ipc, win_width);

            has_lost_manche = false;
            reset = false;
        }

        //Aggiorno il tempo rimanente
        time_t now = time(NULL);
        if(now - last_update_time  >= 1) {
            stats.time--;
            remove_stats(stats_win);
            draw_stats(stats_win, stats);
            last_update_time  = now;
        }

        time_t t = time(NULL);
        // --- Spawn nuovi coccodrilli ---
        for (int i = 0; i < NUM_STREAMS; i++) {
            Stream *s = &streams[i];
            
            // Controlla se è tempo di spawnare e se non supero il limite
            if (s->croc_count < s->max_crocs &&
                difftime(t, s->last_spawn_time) >= s->spawn_time_interval) {
                
                if (spawn_single_croc(s, i, &ipc, win_width)) {
                    s->last_spawn_time = t;  // aggiorna timestamp
                    s->spawn_time_interval = (rand() % 3) + 4;
                }
            }
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
        usleep(2000);
    }


    /* Chiudo le risorse utilizzate e termino i processi */

    if(frog_pid > 0){
        kill(frog_pid, SIGTERM);
        waitpid(frog_pid, NULL, 0);
    }

    clean_all_stream_objects(win, streams);

    cleanup_ipc_handles(&ipc, sync_sem_name);

    close_window(stats_win);    //Elimino la finestra delle statistiche
    print_game_result(win, win_height, win_width, is_winner);   //Stampa schermata di fine
}


void clean_all_stream_objects(WINDOW *win, Stream *streams) {
    for (int i = 0; i < NUM_STREAMS; i++) {
        ObjectNode *curr = streams[i].crocs;
        while (curr) {
            // Termina il processo se ancora attivo
            if (curr->data.pid > 0) {
                kill(curr->data.pid, SIGTERM);
                waitpid(curr->data.pid, NULL, 0);
                remove_croc(win, curr->data.y, curr->data.x);
            }

            // Passa al nodo successivo e libera il corrente
            ObjectNode *tmp = curr;
            curr = curr->next;
            free(tmp);
        }
        streams[i].crocs = NULL; // Lista vuota dopo la rimozione
        streams[i].croc_count = 0;
    }
    wrefresh(win);
}
