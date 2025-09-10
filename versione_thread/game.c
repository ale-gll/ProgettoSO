#include <unistd.h>
#include <curses.h>
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
    if(args != NULL) {
        args->frog = frog;
    }
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

bool is_on_croc(Stream *stream, Object frog, pthread_t *hovered_croc) {
    EntityNode *curr = stream->crocs;

    while(curr) {
        Object *croc = &curr->data.obj;
        if(frog.x >= croc->x 
            && (frog.x + FROG_WIDTH) <= (croc->x + CROC_WIDTH))
        {
            *hovered_croc = curr->tid;
            return true;
        }

        // Avanzo nella lista
        curr = curr->next;
    }
    return false;
}

bool is_hovering_valid(const Object *frog, const Object *croc) {
    return (frog->x >= croc->x &&
            (frog->x + FROG_WIDTH) <=  (croc->x + CROC_WIDTH));
}


/**
 * Funzioni per la gestione di EntityNode
*/

EntityNode *find_entity_by_tid(EntityNode *head, pthread_t tid) {
    EntityNode *curr = head; 

    while (curr) {
        if(curr->tid == tid) return curr;
        curr = curr->next;
    }
    return NULL;    
}

void remove_and_join_node(EntityNode **head, EntityNode *node) { 
    if (!node) return;

    // 1. Segnala al thread di terminare
    switch (node->data.obj.type) {
        case OBJ_FROG: {
            FrogArgs *args = (FrogArgs *) node->data.args;
            atomic_store(&args->running, false);
            pthread_join(node->tid, NULL);
            free(args);
            break;
        }
        case OBJ_CROC: {
            CrocArgs *args = (CrocArgs *) node->data.args;
            atomic_store(&args->running, false);
            pthread_join(node->tid, NULL);
            free(args);
            break;
        }
        case OBJ_GRANADE:
        case OBJ_PROJECTILE: {
            ProjArgs *args = (ProjArgs *) node->data.args;
            atomic_store(&args->running, false);
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
 * Funzion per lo spawn e clean-up degli oggetti dinamici
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
        .running = ATOMIC_VAR_INIT(true),
        .stream_index = stream_index, 
    };

    // Imposta dati di base del nodo
    new_node->data.obj = croc;
    new_node->data.args = args;     // cast (void*) implicito
    new_node->on_grass = false;
    new_node->next = NULL;
    new_node->prev = NULL;

    // Avvia il thread
    if( pthread_create(&new_node->tid, NULL, &croc_thread, args) != 0 ) {
        free(args);
        free(new_node);
        return false;
    }

    // Inserisci in coda alla lista dei coccodrilli nello stream
    if (stream->crocs == NULL) {
        stream->crocs = new_node;
    } else {
        EntityNode *curr = stream->crocs;
        while (curr->next) curr = curr->next;
        curr->next = new_node;
        new_node->prev = curr;
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

bool spawn_granade(int start_x, int start_y, ObjectDirection dir, int active_lane, EntityNode **active_granades, SharedBuffer *buffer) {
    EntityNode *new_node = malloc(sizeof(EntityNode));
    if(!new_node) return false; 

    // Inizializzo l'oggetto da inviare al thread 
    Object granade = {
        .direction = dir, 
        .type = OBJ_GRANADE,
        .x = start_x,
        .y = start_y
    };

    bool on_grass = is_on_grass(active_lane);

    ProjArgs *args = malloc(sizeof(ProjArgs));
    *args = (ProjArgs) {
        .buffer = buffer,
        .proj = granade,
        .running = true,
        .stream_index = (on_grass) ? -1 : active_lane,
    };

    // Imposta dati di base del nodo 
    new_node->data.args = args;
    new_node->data.obj = granade;
    new_node->on_grass = on_grass;
    new_node->next = NULL;
    new_node->prev = NULL;

    // Avvia il thread 
    if( pthread_create(&new_node->tid, NULL, &proj_thread, args) != 0 ) {
        free(args);
        free(new_node);
        return false;
    }

    // Inserisci in coda alla lista delle granate attive 
    if (*active_granades == NULL) {
        *active_granades = new_node;
    } else {
        EntityNode *curr = *active_granades;
        while (curr->next) curr = curr->next;
        curr->next = new_node;
        new_node->prev = curr;
    }

    return true;
}

bool spawn_enemy_projectile(Stream *stream, int stream_index, int start_x, SharedBuffer *buffer) {
    // Alloca nodo 
    EntityNode *new_node = malloc(sizeof(EntityNode));
    if(!new_node) return false;

    //Inizializzo l'oggetto da inviare al thread
    Object proj = {
        .direction = stream->direction,
        .type = OBJ_PROJECTILE,
        .x = start_x,
        .y = stream->y
    };

    // Thread argument 
    ProjArgs *args = malloc(sizeof(ProjArgs));
    *args = (ProjArgs) {
        .buffer = buffer,
        .proj = proj,
        .running = ATOMIC_VAR_INIT(true),
        .stream_index = stream_index,
    };

    // Imposto i dati del nodo
    new_node->data.args = args;
    new_node->data.obj = proj;
    new_node->on_grass = false;
    new_node->next = NULL;
    new_node->prev = NULL;

    // Avvia il thread
    if( pthread_create(&new_node->tid, NULL, &proj_thread, args) != 0 ) {
        free(args);
        free(new_node);
        return false;
    }

    // Inserisci in coda alla lista dei proiettili nello stream
    if (stream->projs == NULL) {
        stream->projs = new_node;
    } else {
        EntityNode *curr = stream->projs;
        while (curr->next) curr = curr->next;
        curr->next = new_node;
        new_node->prev = curr;
    }

    // Aggiorna dati stream
    stream->proj_count++;
    return true;
}

void clean_all_stream_objects(WINDOW *win, Stream *streams) {

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
                free(args);
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
            if (curr->tid != 0) {
                // Termina il thread 
                ProjArgs *args = (ProjArgs *) curr->data.args;
                atomic_store(&args->running, false);
                pthread_join(curr->tid, NULL);
                Object *proj = &curr->data.obj;
                remove_enemy_projectile(win, proj->y, proj->x);
                free(args);
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

void clean_all_granades(WINDOW *win, EntityNode **active_granades) {
    EntityNode *curr = *active_granades;

    while(curr) {
        // Termina thread 
        if(curr->tid != 0) {
            ProjArgs *args = (ProjArgs *) curr->data.args;
            atomic_store(&args->running, false);
            pthread_join(curr->tid, NULL);
            Object *granade = &curr->data.obj;
            remove_granade(win, granade->y, granade->x, curr->on_grass);
            free(args);
        }

        EntityNode *tmp = curr;
        curr = curr->next;
        free(tmp);
    }

    *active_granades = NULL;
}



/**
 * Funzioni per la verifica delle collisioni 
*/

EntityNode* check_collision_granade_projectiles(EntityNode *obj, EntityNode *list) { 
    EntityNode *curr = list;

    while(curr) {
        // Condizione di collisione: stessa cella
        Object *obj_ptr = &obj->data.obj;
        Object *list_obj_ptr = &curr->data.obj;  

        if(obj_ptr->x == list_obj_ptr->x && obj_ptr->y == list_obj_ptr->y) {
            return curr; 
        }

        curr = curr->next;
    }

    return NULL;
}

bool check_collision_frog_projectile(Object *frog, Object *proj) {
    bool overlap_x = proj->x < frog->x + FROG_WIDTH &&
                    proj->x + 1 > frog->x;

    bool overlap_y = proj->y == frog->y;

    return overlap_x && overlap_y;
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
    buffer_init(&buffer, BUFFER_CAPACITY);

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
    pthread_t hovered_croc = (pthread_t) -1;  
    time_t last_update_time = time(NULL);
    EntityNode *active_granades = NULL;

    while(flag) {
        Message m; 

        if(consume_try(&buffer, &m)) {
            switch(m.obj.type)
            {
                case OBJ_FROG: {
                    Object new_frog = m.obj;

                    if (m.msg_type == MSG_UPDATE_POS) 
                    {
                        // 1. Uscita dallo schermo
                        bool invalid_move = (active_lane == 0 && new_frog.direction == DIR_DOWN) || 
                                            is_out_of_screen(win_width, new_frog.x, FROG_WIDTH);

                        if (invalid_move) {
                            set_frog(&frog_mutex, frog_args, frog);
                        }

                        // 2. Rana cerca di salire sulle tane
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
                                hovered_croc = (pthread_t) -1;
                                on_croc = false;

                                // sincronizzo il thread rana
                                set_frog(&frog_mutex, frog_args, frog);
                            }
                            else {
                                // Fiume -> controllo se c’è un coccodrillo sotto la rana
                                on_croc = is_on_croc(&streams[next_active_lane-1], new_frog, &hovered_croc);

                                if (on_croc) {
                                    pthread_mutex_lock(&win_mutex);
                                    remove_frog(win, frog.y, frog.x, on_grass);
                                    active_lane = next_active_lane;
                                    on_grass = false;
                                    draw_frog(win, new_frog, is_scared);
                                    pthread_mutex_unlock(&win_mutex);

                                    frog = new_frog;
                                    set_frog(&frog_mutex, frog_args, frog);
                                } 
                                else {
                                    has_lost_manche = true;
                                    reset = true;
                                }
                            }
                        }
                    }

                    // La rana spara dei proiettili
                    if(m.msg_type == MSG_FIRE) {
                        // Spawn granata sx e dx
                        flag &= spawn_granade(frog.x-1, frog.y, DIR_LEFT, active_lane, &active_granades, &buffer);
                        flag &= spawn_granade((frog.x + FROG_WIDTH), frog.y, DIR_RIGHT, active_lane, &active_granades, &buffer);
                    }

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
                            // Aggiorna coccodrillo 
                            pthread_mutex_lock(&win_mutex);
                            remove_croc(win, old_croc->y, old_croc->x);
                            draw_croc(win, m.obj);
                            pthread_mutex_unlock(&win_mutex);
                            curr->data.obj = m.obj;

                            bool frog_on_this_croc = (
                                hovered_croc == m.tid && 
                                on_croc &&
                                active_lane == m.stream_index + 1
                            );

                            // Se la rana è sopra questo coccodrillo
                            if (frog_on_this_croc && is_hovering_valid(&frog, &m.obj) && m.msg_type == MSG_UPDATE_POS) {
                                // Aggiorno grafica
                                pthread_mutex_lock(&win_mutex);
                                remove_frog(win, frog.y, frog.x, on_grass);
                                if(frog.x > 0 && (frog.x + FROG_WIDTH) < win_width) {
                                    frog.x += (s->direction == DIR_LEFT) ? -1 : 1;
                                }
                                draw_frog(win, frog, is_scared);
                                pthread_mutex_unlock(&win_mutex);


                                // Invio aggiornamento al processo rana
                                set_frog(&frog_mutex, frog_args, frog);
                            } 
                            else if (hovered_croc == m.tid && on_croc && !is_hovering_valid(&frog, &m.obj)) {
                                reset = true;
                                has_lost_manche = true;
                            }
                        }
                    }
                    
                    if (m.msg_type == MSG_FIRE && curr) {
                        Object *croc = &curr->data.obj;

                        if (s->proj_count < MAX_PROJ_PER_STREAM) {
                            // int direction = (croc->direction == DIR_LEFT) ? -1 : 1;
                            // int next_x;

                            // if (croc->direction == DIR_LEFT) {
                            //     next_x = croc->x - 1;   // appena a sinistra del coccodrillo
                            // } else {
                            //     next_x = croc->x + CROC_WIDTH; // appena a destra del coccodrillo
                            // }

                            // flag &= spawn_enemy_projectile(s, m.stream_index, next_x, &buffer);
                            pthread_mutex_lock(&win_mutex);
                            draw_frog(win, frog, is_scared);
                            pthread_mutex_unlock(&win_mutex);
                        }
                    }
                    break;
                }

                case OBJ_GRANADE: {
                    if (m.msg_type != MSG_UPDATE_POS) break;

                    EntityNode *curr = find_entity_by_tid(active_granades, m.tid);
                    if (!curr) break;

                    Object *granade = &curr->data.obj;
                    bool removed = false;

                    // 1. Fuori schermo
                    if (is_fully_out_of_screen(win_width, m.obj.x, 1)) {
                        pthread_mutex_lock(&win_mutex);
                        remove_granade(win, granade->y, granade->x, curr->on_grass);
                        pthread_mutex_unlock(&win_mutex);

                        remove_and_join_node(&active_granades, curr);
                        removed = true;
                    }

                    // 2. Collisione con proiettile (solo se nel fiume e non già rimossa)
                    if (!removed && !curr->on_grass && m.stream_index > 0) {
                        EntityNode *hit_proj = check_collision_granade_projectiles(curr, streams[m.stream_index-1].projs);

                        if (hit_proj) {
                            pthread_mutex_lock(&win_mutex);
                            remove_granade(win, granade->y, granade->x, curr->on_grass);
                            Object *proj = &hit_proj->data.obj;
                            remove_enemy_projectile(win, proj->y, proj->x);
                            pthread_mutex_unlock(&win_mutex);

                            remove_and_join_node(&active_granades, curr);

                            Stream *s = &streams[m.stream_index-1];
                            remove_and_join_node(&s->projs, hit_proj);
                            s->proj_count--;

                            removed = true;
                        }
                    }

                    // 3. Nessuna rimozione → aggiorna posizione
                    if (!removed) {
                        pthread_mutex_lock(&win_mutex);
                        remove_granade(win, granade->y, granade->x, curr->on_grass);
                        draw_granade(win, m.obj);
                        pthread_mutex_unlock(&win_mutex);

                        curr->data.obj = m.obj;
                    }
                    break;
                }

                case OBJ_PROJECTILE: {
                    if (m.msg_type != MSG_UPDATE_POS) break;

                    Stream *s = &streams[m.stream_index];
                    EntityNode *curr = find_entity_by_tid(s->projs, m.tid);

                    if (!curr) break; // Nodo già rimosso o non trovato

                    Object *proj = &curr->data.obj;
                    bool removed = false;

                    // 1. Se il proiettile è fuori schermo -> rimuovi
                    if (is_fully_out_of_screen(win_width, m.obj.x, 1)) {
                        pthread_mutex_lock(&win_mutex);
                        remove_enemy_projectile(win, proj->y, proj->x);
                        pthread_mutex_unlock(&win_mutex);

                        remove_and_join_node(&s->projs, curr);
                        s->proj_count--;
                        removed = true;
                    }

                    // 2. Collisione con granata
                    if (!removed) {
                        EntityNode *hit_grenade = check_collision_granade_projectiles(curr, active_granades);
                        if (hit_grenade) {
                            pthread_mutex_lock(&win_mutex);
                            remove_enemy_projectile(win, proj->y, proj->x);
                            Object *grenade_obj = &hit_grenade->data.obj;
                            remove_granade(win, grenade_obj->y, grenade_obj->x, hit_grenade->on_grass);
                            pthread_mutex_unlock(&win_mutex);

                            remove_and_join_node(&s->projs, curr);
                            s->proj_count--;
                            remove_and_join_node(&active_granades, hit_grenade);
                            removed = true;
                        }
                    }

                    // 3. Collisione con la rana
                    if (!removed && check_collision_frog_projectile(proj, &frog)) {
                        reset = true;
                        has_lost_manche = true;
                    }

                    // 4. Nessuna collisione, aggiorna posizione
                    if (!removed) {
                        pthread_mutex_lock(&win_mutex);
                        remove_enemy_projectile(win, proj->y, proj->x);
                        draw_enemy_projectile(win, m.obj);
                        pthread_mutex_unlock(&win_mutex);

                        *proj = m.obj;
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
            pthread_mutex_lock(&frog_mutex);
            atomic_store(&frog_args->running, false);
            pthread_join(frog_tid, NULL);
            free(frog_args);
            frog_args = NULL;
            pthread_mutex_unlock(&frog_mutex);

            pthread_mutex_lock(&win_mutex);
            remove_frog(win, frog.y, frog.x, on_grass);

            // Elimino coccodrilli, proiettili e granate
            clean_all_stream_objects(win, streams); 
            clean_all_granades(win, &active_granades);
            
            pthread_mutex_unlock(&win_mutex);

            // Pulizia del buffer
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
            hovered_croc = (pthread_t) -1;
            on_croc = false;
            is_scared = (stats.lives <= 2);

            // Reinizializzo il buffer
            buffer_init(&buffer, BUFFER_CAPACITY);

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
                free(frog_args); frog_args = NULL; flag = false;
            }

            /* reset dei flag della manche */
            has_lost_manche = false;
            reset = false;
        }

        /**
         * Spawn di nuovi coccodrilli 
        */
        time_t t = time(NULL);
        // --- Spawn nuovi coccodrilli ---
        for (int i = 0; i < NUM_STREAMS; i++) {
            Stream *s = &streams[i];
            
            // Controlla se è tempo di spawnare e se non supero il limite
            if (s->croc_count < s->max_crocs &&
                difftime(t, s->last_spawn_time) >= s->spawn_time_interval) {
                
                if (spawn_single_croc(s, i, win_width, &buffer)) {
                    s->last_spawn_time = t;  // aggiorna timestamp
                    s->spawn_time_interval = (rand() % 3) + 5;  // 5,6,7 secondi
                }
            }
        }

        pthread_mutex_lock(&win_mutex);
        wrefresh(win);
        wrefresh(stats_win);
        pthread_mutex_unlock(&win_mutex);
        usleep(2000);
    }

    /**
     * Chiusura del programma
     */

    if(frog_tid != 0) {
        atomic_store(&frog_args->running, false); 
        pthread_join(frog_tid, NULL);
    }

    clean_all_stream_objects(win, streams);
    clean_all_granades(win, &active_granades);

    //Libero le risorse
    buffer_destroy(&buffer);
    pthread_mutex_destroy(&win_mutex);
    pthread_mutex_destroy(&frog_mutex);

    close_window(stats_win);
    print_game_result(win, win_height, win_width, is_winner, stats.score);
}
