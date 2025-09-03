#include <curses.h>
#include <unistd.h>
#include <semaphore.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "utils.h"
#include "frog_thread.h"
#include "croc_thread.h"
#include "game.h"

/**
 * Funzioni per l'inizializzazione di Stats, Burrows e rana
*/

Object init_frog(int win_height, int win_width) {
    Object frog;
    frog.direction = DIR_UNKNOWN;
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
    stats->lives = NUM_LIVES;
    stats->time = TIME_PER_ROUND;
}

void set_frog(pthread_mutex_t *frog_mutex, FrogArgs *args, Object frog) {
    pthread_mutex_lock(frog_mutex);
    args->frog = frog;
    pthread_mutex_unlock(frog_mutex);
}


/**
 * Funzioni helper 
*/

bool is_on_burrow(int active_lane, int dir) {
    return active_lane == (NUM_STREAMS+1) && dir == DIR_UP;
}

bool is_on_grass(int lane) {
    return (lane == 0 || lane == NUM_STREAMS+1);
}

bool is_out_of_screen(int win_width, int obj_x, int obj_width) {
    return (obj_x < 0 || obj_x + obj_width > win_width);
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

void update_score(Stats *stats, ScoreEvent e) {
    int delta = 0;

    switch (e)
    {
    case SCORE_REACH_BURROW:
        delta += 500 + (stats->time * 20);
        break;

    case SCORE_MOVE_UP:
        delta += 15;
        break;

    case SCORE_MOVE_DOWN:
        delta -= 10;
        break;

    case SCORE_LOSE_LIFE:
        delta -= 500;
        break;
    
    case SCORE_COMPLETE_ALL_BURROWS:
        if(stats->lives == NUM_LIVES) {
            delta += 1500;
        }
        break;

    default:
        break;
    }

    stats->score += delta;
}

bool check_win(Stats stats, Burrow burrows[5]) {
    bool res = true;
    int burrows_size = 5;

    for(int i = 0; i < burrows_size; i++) {
        res &= burrows[i].is_occupied;
    }
    return res && (stats.lives > 0) && (stats.time > 0);
}




/**
 * Funzioni per la gestione di EntityNode
*/

EntityNode *find_entity_by_tid(EntityNode *head, pthread_t tid) {
    EntityNode *curr = head; 

    while (curr && curr->tid != tid) {
        curr = curr->next;
    }
    return curr;    
}


void remove_and_join_node(EntityNode **head, EntityNode *node) { 
    if (!node) return;

    // 1. Segnala al thread di terminare
    switch (node->data.obj.type) {
        case OBJ_FROG: {
            FrogArgs *args = (FrogArgs *) node->data.args;
            args->running = false;
            pthread_join(node->tid, NULL);
            free(args);
            break;
        }
        case OBJ_CROC: {
            CrocArgs *args = (CrocArgs *) node->data.args;
            args->running = false;
            pthread_join(node->tid, NULL);
            free(args);
            break;
        }
        case OBJ_PROJECTILE: {
            ProjArgs *args = (ProjArgs *) node->data.args;
            args->running = false;
            pthread_join(node->tid, NULL);
            free(args);
            break;
        }
        default:
            break;
    }

    // 2. Rimuovi il nodo dalla lista doppiamente collegata
    if (node->prev) node->prev->next = node->next;
    else *head = node->next;  // era il primo nodo

    if (node->next) node->next->prev = node->prev;

    free(node); // libera il nodo stesso
}



void game_loop(WINDOW *win, int start_y, int start_x) {
    srand(time(NULL));

    /* Setting della finestra */
    int win_height, win_width;

    getmaxyx(win, win_height, win_width);
    WINDOW *stats_win = newwin(3, win_width, start_y-3, start_x);
    pthread_mutex_t win_mutex = PTHREAD_MUTEX_INITIALIZER;

    SharedBuffer buffer; 
    buffer_init(&buffer);

    /**
     * Inizializzo il campo da gioco 
     */
    Object frog;
    pthread_mutex_t frog_mutex = PTHREAD_MUTEX_INITIALIZER; 
    Burrow burrows[NUM_BURROWS];
    Stats stats;
    bool flag = false;

    frog = init_frog(win_height, win_width);
    init_burrows(burrows); 
    init_stats(&stats);
    init_playground(win, stats_win, win_height, win_width, frog, burrows, stats);


    // Inizializzo il thread argument della rana
    FrogArgs *frog_args = malloc(sizeof(FrogArgs));
    *frog_args = (FrogArgs){
        .buffer = &buffer,
        .frog = frog,
        .frog_mutex = &frog_mutex,
        .running = true,
        .win = win,
        .win_mutex = &win_mutex
    };

    // Crea il thread rana
    pthread_t frog_tid;
    if(pthread_create(&frog_tid, NULL, &frog_thread, frog_args) == 0) flag = true;


    /**
     * Loop di gioco 
     */

    int active_lane = 0;
    bool on_grass = true;
    bool is_scared = false;
    bool reset = false;
    bool is_winner = false;
    bool has_lost_manche = false;
    bool on_croc = false;
    int hovered_croc = -1;  //pid del coccodrillo dove si trova la rana
    time_t last_update_time = time(NULL);

    while(flag) {
        Message m; 

        if(consume_try(&buffer, &m)){
            switch(m.obj.type) {
                case OBJ_FROG:
                    Object new_frog = m.obj;

                    // La rana si muove nella mappa
                    if (m.msg_type == MSG_UPDATE_POS) 
                    {
                        // 1. Utente cerca di uscire fuori dallo schermo
                        bool invalid_move = (active_lane == 0 && new_frog.direction == DIR_DOWN) || 
                                            is_out_of_screen(win_width, new_frog.x, FROG_WIDTH);

                        if (invalid_move) {
                            set_frog(&frog_mutex, frog_args, frog);
                        }

                        // 2. Rana cerca di salire oltre l'argine (tane)
                        else if (is_on_burrow(active_lane, new_frog.direction)) 
                        {
                            if (check_burrows(new_frog, burrows)) {
                                draw_frog(win, new_frog, is_scared);
                            } else {
                                has_lost_manche = true;
                            }
                            reset = true;   // Reset per nuova manche
                        }

                        // 3. Movimento normale (marciapiede / fiume / coccodrilli)
                        else {

                            // Aggiorno il punteggio 
                            if(new_frog.direction == DIR_UP) { 
                                update_score(&stats, SCORE_MOVE_UP);
                            }
                            else if (new_frog.direction == DIR_DOWN && active_lane > 0) {
                                update_score(&stats, SCORE_MOVE_DOWN);
                            }

                            int next_active_lane = update_lane(active_lane, new_frog.direction);

                            if (is_on_grass(next_active_lane)) {
                                // Su marciapiede o argine
                                remove_frog(win, frog.y, frog.x, on_grass);
                                active_lane = next_active_lane;
                                on_grass = true;
                                draw_frog(win, new_frog, is_scared);
                                frog = new_frog;    // Aggiorno nuova posizione rana

                                // Reset logico: non sono più su un coccodrillo
                                hovered_croc = -1;
                                on_croc = false;
                                set_frog(&frog_mutex, frog_args, frog);
                            }
                            else {
                                // // Fiume -> controllo se c’è un coccodrillo sotto la rana
                                // on_croc = is_on_croc(&streams[next_active_lane-1], new_frog, &hovered_croc);

                                // if (on_croc) {
                                    remove_frog(win, frog.y, frog.x, on_grass);
                                    active_lane = next_active_lane;
                                    on_grass = false;
                                    draw_frog(win, new_frog, is_scared);
                                    frog = new_frog;
                                // } 
                                // else {
                                //     has_lost_manche = true;
                                //     reset = true;
                                // }
                            }
                        }
                    }

                    // // La rana spara dei proiettili
                    // if(m.msg_type == MSG_FIRE) {
                    //     // Spawn granata sx e dx
                    //     flag &= spawn_granade(&ipc, frog.x-1, frog.y, DIR_LEFT, active_lane, &active_granades);
                    //     flag &= spawn_granade(&ipc, (frog.x + FROG_WIDTH +1), frog.y, DIR_RIGHT, active_lane, &active_granades);
                    // }
                break;

                default:
                    break;
            }
        }
        

        /**
         * Controllo condizioni di fine manche / partita
        */
        if(stats.time < 0) {
            reset = true;
            has_lost_manche = true;
        } else {
            time_t now = time(NULL);
            if(now - last_update_time  >= 1) {
                stats.time--;
                remove_stats(stats_win);
                draw_stats(stats_win, stats);
                last_update_time  = now;
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


        pthread_mutex_lock(&win_mutex);
        wrefresh(win);
        pthread_mutex_unlock(&win_mutex);
        wrefresh(stats_win);
    }
}