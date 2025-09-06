#include <curses.h>
#include <unistd.h>
#include <semaphore.h>
#include <stdlib.h>
#include <time.h>
#include <stdio.h>
#include <string.h>
#include "utils.h"
#include "frog_thread.h"
#include "croc_thread.h"
#include "projectile_thread.h"
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

FrogArgs *set_frog_args(SharedBuffer *buffer, Object frog, pthread_mutex_t *frog_mutex, WINDOW *win, pthread_mutex_t *win_mutex) {  
    FrogArgs *frog_args = malloc(sizeof(FrogArgs));

    *frog_args = (FrogArgs){
        .buffer = buffer,
        .frog = frog,
        .frog_mutex = frog_mutex,
        .running = ATOMIC_VAR_INIT(true),
        .win = win,
        .win_mutex = win_mutex
    };

    return frog_args;
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

bool is_fully_out_of_screen(int win_width, int obj_x, int obj_width) {
    return (obj_x + obj_width <= 0 || obj_x >= win_width);
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



/**
 * Funzion per lo spawn + clean-up degli oggetti dinamici
*/

void init_streams(Stream *streams, int start_y) {
    int delay[] = {180000, 240000};
    int direction = (rand() % 2 == 0) ? DIR_LEFT : DIR_RIGHT;

    for (int i = 0; i < NUM_STREAMS; i++) {
        int crocs_per_stream = (rand() % 2) + 2; // 2 o 3 coccodrilli per stream

        streams[i].y = start_y;
        streams[i].direction = direction;
        streams[i].spawn_time_interval = (rand() % 3) + 5; // 5,6,7 secondi
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

bool spawn_single_croc(Stream *stream, int stream_index, int window_width, SharedBuffer *buffer) {
    // Alloca nodo per il nuovo coccodrillo
    EntityNode *new_node = malloc(sizeof(EntityNode));
    if (!new_node) return false;

    // Inizializzo l'oggetto da inviare al thread
    Object croc = {
        .direction = stream->direction,
        .type = OBJ_CROC,
        .x = (stream->direction == DIR_LEFT) ? window_width : -CROC_WIDTH,
        .y = stream->y
    };

    // Thread argument 
    CrocArgs *args = malloc(sizeof(CrocArgs));
    *args = (CrocArgs) {
        .buffer = buffer,
        .croc = croc,
        .delay = stream->delay,
        .running = true,
        .stream_index = stream_index, 
    };

    // Imposta dati di base del nodo
    new_node->data.obj = croc;
    new_node->data.args = args;     // cast (void*) implicito
    new_node->on_grass = false;
    new_node->next = NULL;
    new_node->prev = NULL;

    // Inserisci in coda alla lista dei coccodrilli nello stream
    if (stream->crocs == NULL) {
        stream->crocs = new_node;
    } else {
        EntityNode *curr = stream->crocs;
        while (curr->next) curr = curr->next;
        curr->next = new_node;
        new_node->prev = curr;
    }

    // Avvia il thread
    if( pthread_create(&new_node->tid, NULL, &croc_thread, args) != 0 ) {
        // rollback se fallisce
        if (new_node->prev) new_node->prev->next = NULL;
        else stream->crocs = NULL;
        free(args);
        free(new_node);
        return false;
    }

    // Aggiorna dati stream
    stream->last_spawn_time = time(NULL);
    stream->croc_count++;

    return true;
}

bool spawn_initial_crocs(Stream *streams, int window_width, SharedBuffer *buffer) {
    bool flag = true;

    for (int i = 0; i < NUM_STREAMS; i++) {
        flag &= spawn_single_croc(&streams[i], i, window_width, buffer);
    }

    return flag;
}

void clean_all_stream_objects(WINDOW *win, Stream *streams){

    for (int i = 0; i < NUM_STREAMS; i++) {
        EntityNode *curr = streams[i].crocs;
        
        /**
         * Elimino i coccodrilli
         */
        while(curr) {
            if(curr->tid != 0) { 
                // Termina il thread 
                CrocArgs *args = (CrocArgs *) curr->data.args;
                atomic_store(&args->running, false); 
                pthread_join(curr->tid, NULL);
                Object *croc = &curr->data.obj;
                remove_croc(win, croc->y, croc->x);
            }

            EntityNode *tmp = curr; 
            curr = curr->next;

            tmp->next = NULL;
            tmp->prev = NULL;
            free(tmp);
        }

        /**
         * Elimino i proiettili
         */
        curr = streams[i].projs;

        while(curr) {
            if (curr->tid > 0) {
                // Termina il thread 
                ProjArgs *args = (ProjArgs *) curr->data.args;
                atomic_store(&args->running, false);
                pthread_join(curr->tid, NULL);
                Object *proj = &curr->data.obj;
                remove_enemy_projectile(win, proj->y, proj->x);
            }

            EntityNode *tmp = curr;
            curr = curr->next;

            tmp->next = NULL;
            tmp->prev = NULL;
            free(tmp);
        }

        // Azzero le liste e i contatori
        streams[i].crocs = NULL;
        streams[i].croc_count = 0;
        streams[i].projs = NULL;
        streams[i].proj_count = 0;
    }
}

/**
 * Funzione per la stampa del risultato
*/

void print_game_result(WINDOW *win, int win_height, int win_width, bool is_winner, int score){
    wclear(win);
    wbkgd(win, A_NORMAL);
    box(win, 0, 0);

    char *press_enter_str = "Press ENTER to exit...";
    char *win_message = is_winner ? "YOU WIN" : "YOU LOSE";
    char total_score_str[20];

    int message_len = strlen(win_message);
    int press_len = strlen(press_enter_str);
    int tot_score_len = snprintf(total_score_str, 20, "%s %d", "Total score:", score);

    wattron(win, COLOR_PAIR(START_MENU_COLOR_PAIR));
    mvwprintw(win, win_height/2, (win_width - message_len)/2, "%s", win_message);
    mvwprintw(win, (win_height/2)+1, (win_width - tot_score_len)/2, "%s", total_score_str);
    mvwprintw(win, (win_height/2)+3, (win_width - press_len)/2, "%s", press_enter_str);
    wattroff(win, COLOR_PAIR(START_MENU_COLOR_PAIR));
    wrefresh(win);
    while(wgetch(win) != '\n');
}


/**
 * Funzione principale: Loop di gioco
 */

void game_loop(WINDOW *win, int start_y, int start_x) {
    srand(time(NULL));

    /* Setting della finestra */
    int win_height, win_width;

    getmaxyx(win, win_height, win_width);
    WINDOW *stats_win = newwin(3, win_width, start_y-3, start_x);
    pthread_mutex_t win_mutex;
    pthread_mutex_init(&win_mutex, NULL);


    //Inizializzo gli Stream del fiume
    Stream streams[NUM_STREAMS];
    init_streams(streams, win_height-4);

    SharedBuffer buffer; 
    buffer_init(&buffer);

    /**
     * Inizializzo il campo da gioco 
     */
    Object frog;
    pthread_mutex_t frog_mutex;
    pthread_mutex_init(&frog_mutex, NULL);
    Burrow burrows[NUM_BURROWS];
    Stats stats;
    bool flag = true;

    frog = init_frog(win_height, win_width);
    init_burrows(burrows); 
    init_stats(&stats);
    init_playground(win, stats_win, win_height, win_width, frog, burrows, stats);


    // Inizializzo il thread argument della rana
    FrogArgs *frog_args = set_frog_args(&buffer, frog, &frog_mutex, win, &win_mutex);

    // Crea il thread rana
    pthread_t frog_tid;
    if(pthread_create(&frog_tid, NULL, &frog_thread, frog_args) != 0) {
        flag = false; 
        free(frog_args);
    }

    // Inizializzo i coccodrilli 
    flag &= spawn_initial_crocs(streams, win_width, &buffer);


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

        if(consume_try(&buffer, &m)) {
            switch(m.obj.type) {
                case OBJ_FROG: {
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
                                pthread_mutex_lock(&win_mutex);
                                draw_frog(win, new_frog, is_scared);    // Disegno la rana nella tana
                                pthread_mutex_unlock(&win_mutex);
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
                                pthread_mutex_lock(&win_mutex);

                                remove_frog(win, frog.y, frog.x, on_grass);
                                active_lane = next_active_lane;
                                on_grass = true;
                                draw_frog(win, new_frog, is_scared);

                                pthread_mutex_unlock(&win_mutex);
                                frog = new_frog;        // Aggiorno nuova posizione rana

                                // Reset logico: non sono più su un coccodrillo
                                hovered_croc = -1;
                                on_croc = false;
                                set_frog(&frog_mutex, frog_args, frog);
                            }
                            else {
                                // // Fiume -> controllo se c’è un coccodrillo sotto la rana
                                // on_croc = is_on_croc(&streams[next_active_lane-1], new_frog, &hovered_croc);

                                // if (on_croc) {
                                    pthread_mutex_lock(&win_mutex);

                                    remove_frog(win, frog.y, frog.x, on_grass);
                                    active_lane = next_active_lane;
                                    on_grass = false;
                                    draw_frog(win, new_frog, is_scared);

                                    pthread_mutex_unlock(&win_mutex);
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
                }

                case OBJ_CROC: {
                    Stream *s = &streams[m.stream_index];

                    EntityNode *curr = find_entity_by_tid(s->crocs, m.tid);
                    if (curr) {
                        Object *old_croc = &curr->data.obj;

                        if ( is_fully_out_of_screen(win_width, m.obj.x, CROC_WIDTH) ) {
                            pthread_mutex_lock(&win_mutex);
                            remove_croc(win, old_croc->y, old_croc->x);
                            pthread_mutex_unlock(&win_mutex);

                            remove_and_join_node(&s->crocs, curr);
                            s->croc_count--;
                        } else {
                            pthread_mutex_lock(&win_mutex);
                            remove_croc(win, old_croc->y, old_croc->x);
                            draw_croc(win, m.obj);
                            pthread_mutex_unlock(&win_mutex);

                            curr->data.obj = m.obj;
                        }
                    }
                    break;
                }


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
                pthread_mutex_lock(&win_mutex);
                remove_stats(stats_win);
                draw_stats(stats_win, stats);
                pthread_mutex_unlock(&win_mutex);
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


        /**
         * Reset della manche
        */
        if (reset) {

            // Elimino rana 
            atomic_store(&frog_args->running, false);
            if (frog_tid != 0) {
                pthread_join(frog_tid, NULL);
                frog_tid = 0;
            }
            free(frog_args);
            frog_args = NULL;

            pthread_mutex_lock(&win_mutex);
            remove_frog(win, frog.y, frog.x, on_grass);
            pthread_mutex_unlock(&win_mutex);

            // Elimino coccodrilli e proiettili
            clean_all_stream_objects(win, streams);

            // Elimino il buffer
            buffer_destroy(&buffer);


            // Aggiorna le statistiche
            pthread_mutex_lock(&win_mutex);
            remove_stats(stats_win);
            if (has_lost_manche) {
                stats.lives--;
                update_score(&stats, SCORE_LOSE_LIFE);
            } else {
                update_score(&stats, SCORE_REACH_BURROW);
            }
            stats.time = TIME_PER_ROUND;
            draw_stats(stats_win, stats);
            pthread_mutex_unlock(&win_mutex);


            // Reset logico 
            active_lane = 0;
            on_grass = true;
            hovered_croc = -1;
            on_croc = false;
            is_scared = (stats.lives <= 2);

            // Reinizializzo il buffer
            buffer_init(&buffer);

            init_streams(streams, win_height - 4);
            spawn_initial_crocs(streams, win_width, &buffer);

            // Ricreo la rana 
            frog = init_frog(win_height, win_width);
            frog_args = set_frog_args(&buffer, frog, &frog_mutex, win, &win_mutex);
            if (!frog_args) {
                flag = false;
                break;
            }

            pthread_mutex_lock(&win_mutex);
            draw_frog(win, frog, is_scared);
            wrefresh(win);
            wrefresh(stats_win);
            pthread_mutex_unlock(&win_mutex);

            if (pthread_create(&frog_tid, NULL, &frog_thread, frog_args) != 0) {
                free(frog_args);
                frog_args = NULL;
                flag = false;
            }

            /* reset dei marker della manche */
            has_lost_manche = false;
            reset = false;
        }

        pthread_mutex_lock(&win_mutex);
        wrefresh(win);
        pthread_mutex_unlock(&win_mutex);
        wrefresh(stats_win);
    }

    /**
     * Chiusura del programma
     */

    if(frog_tid != 0) {
        atomic_store(&frog_args->running, false); 
        pthread_join(frog_tid, NULL);
    }

    clean_all_stream_objects(win, streams);

    //Libero le risorse
    buffer_destroy(&buffer);
    pthread_mutex_destroy(&win_mutex);
    pthread_mutex_destroy(&frog_mutex);

    close_window(stats_win);
    print_game_result(win, win_height, win_width, is_winner, stats.score);
}
