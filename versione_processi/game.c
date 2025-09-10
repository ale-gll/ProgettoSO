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
#include "projectile_process.h"
#include "game.h"

/**
 * Funzioni helper
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
    return (lane == 0 || lane == NUM_STREAMS+1);
}

bool is_on_croc(Stream *stream, Object frog, int *hovered_croc){
    ObjectNode *curr = stream->crocs;

    while(curr) {
        if(frog.x >= curr->data.x 
            && (frog.x + FROG_WIDTH) <= (curr->data.x + CROC_WIDTH))
        {
            *hovered_croc = curr->data.pid;
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

void set_frog(int write_fd, Object frog, bool *flag) {
    Message reset;

    set_message(&reset, MSG_SET_FROG, &frog, NULL);
    if( write(write_fd, &reset, sizeof(Message)) == -1 ) {
        *flag = false;
    }
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

bool is_spawn_time(time_t now, Stream s) {
    return (now - s.last_spawn_time > s.spawn_time_interval);
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


/**
 * Funzioni per l'inizializzazione di Stats, Burrows e rana
*/

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


/**
 * Funzioni per lo spawn degli oggetti dinamici 
*/

bool spawn_single_croc(Stream *stream, int stream_index, IPCHandles *ipc, int window_width) {
    // Alloca nodo per il nuovo coccodrillo
    ObjectNode *new_node = malloc(sizeof(ObjectNode));
    if (!new_node) return false;

    // Imposta dati di base del nodo
    new_node->data.direction = stream->direction;
    new_node->data.type = OBJ_CROC;
    new_node->data.x = (stream->direction == DIR_LEFT) ? window_width : -CROC_WIDTH;
    new_node->data.y = stream->y;
    new_node->on_grass = false;   // valore di default
    new_node->next = NULL;
    new_node->prev = NULL;

    // Avvia processo figlio
    pid_t pid = croc_process(ipc, new_node->data, stream_index, stream->delay);
    if (pid == -1) {
        free(new_node);
        return false;
    }

    // Aggiorna dati del nodo
    new_node->data.pid = (int) pid;

    // Inserisci in coda alla lista dei coccodrilli nello stream
    if (stream->crocs == NULL) {
        stream->crocs = new_node;
    } else {
        ObjectNode *curr = stream->crocs;
        while (curr->next) curr = curr->next;
        curr->next = new_node;
        new_node->prev = curr;
    }

    // Aggiorna dati stream
    stream->last_spawn_time = time(NULL);
    stream->croc_count++;
    return true;
}

bool spawn_initial_crocs(Stream *streams, IPCHandles *ipc, int window_width) {

    bool flag = true;

    for (int i = 0; i < NUM_STREAMS; i++) {
        flag &= spawn_single_croc(&streams[i], i, ipc, window_width);
    }   
    
    return flag;
}

bool spawn_granade(IPCHandles *ipc, int start_x, int start_y, ObjectDirection dir, int active_lane, ObjectNode **active_granades) 
{
    ObjectNode *new_node = malloc(sizeof(ObjectNode));
    if (!new_node) return false;

    // Imposta dati granata
    new_node->data.direction = dir;
    new_node->data.type = OBJ_GRANADE;
    new_node->data.y = start_y;
    new_node->data.x = start_x;
    new_node->on_grass = false;  // valore di default
    new_node->next = NULL;
    new_node->prev = NULL;

    /**
     * Se la rana è sull'erba -> stream_index = -1
     * Se è nel fiume -> stream_index = active_lane 
    */
    bool on_grass = is_on_grass(active_lane);
    int stream_index = (on_grass) ? -1 : active_lane;
    new_node->on_grass = on_grass;

    // Avvia il processo 
    pid_t pid = proj_process(ipc, new_node->data, stream_index);
    if (pid == -1) {
        free(new_node);
        return false;
    }

    new_node->data.pid = pid;

    // Inserisci in coda alla lista delle granate attive 
    if (*active_granades == NULL) {
        *active_granades = new_node;
    } else {
        ObjectNode *curr = *active_granades;
        while (curr->next) curr = curr->next;
        curr->next = new_node;
        new_node->prev = curr;
    }

    return true;
}

bool spawn_enemy_projectile(Stream *stream, int stream_index, IPCHandles *ipc, int start_x) {
    // Alloca nodo per il nuovo proiettile
    ObjectNode *new_node = malloc(sizeof(ObjectNode));
    if(!new_node) return false;

    // Imposta dati di base del nodo
    new_node->data.direction = stream->direction;
    new_node->data.type = OBJ_PROJECTILE;
    new_node->data.x = start_x;
    new_node->data.y = stream->y;
    new_node->on_grass = false;
    new_node->next = NULL;
    new_node->prev = NULL;

    // Avvia processo figlio
    pid_t pid = proj_process(ipc, new_node->data, stream_index);
    if(pid == -1) {
        free(new_node);
        return false;
    }

    // Aggiorna dati nodo
    new_node->data.pid = pid;

    // Inserisci in coda alla lista di proiettili
    if(stream->projs == NULL) {
        stream->projs = new_node;
    } else {
        ObjectNode *curr = stream->projs;
        while(curr->next) curr = curr->next;
        curr->next = new_node;
        new_node->prev = curr;
    }

    // Aggiorna dati stream
    stream->proj_count++;
    return true;
}


/**
 * Funzioni per pulizia grafica e kill processi
 */

void clean_all_stream_objects(WINDOW *win, Stream *streams) {
    for (int i = 0; i < NUM_STREAMS; i++) {
        ObjectNode *curr_croc = streams[i].crocs;
        while (curr_croc) {
            // Termina il processo se ancora attivo
            if (curr_croc->data.pid > 0) {
                kill(curr_croc->data.pid, SIGTERM);
                waitpid(curr_croc->data.pid, NULL, 0);
                remove_croc(win, curr_croc->data.y, curr_croc->data.x);
            }

            // Salva il prossimo nodo e libera il corrente
            ObjectNode *tmp = curr_croc;
            curr_croc = curr_croc->next;

            tmp->next = NULL;
            tmp->prev = NULL;
            free(tmp);
        }

        // Elimino anche i proiettili
        ObjectNode *curr_proj = streams[i].projs;
        while(curr_proj) {
            // Termina il processo se ancora attivo
            if (curr_proj->data.pid > 0) {
                kill(curr_proj->data.pid, SIGTERM);
                waitpid(curr_proj->data.pid, NULL, 0);
                remove_enemy_projectile(win, curr_proj->data.y, curr_proj->data.x);
            }

            ObjectNode *tmp = curr_proj;
            curr_proj = curr_proj->next;

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

void clean_all_granades(WINDOW *win, ObjectNode **granades) {
    ObjectNode *curr = *granades;

    while (curr) {
        // Termina il processo e cancella dalla grafica
        if (curr->data.pid > 0) {
            kill(curr->data.pid, SIGTERM);
            waitpid(curr->data.pid, NULL, 0);
            remove_granade(win, curr->data.y, curr->data.x, curr->on_grass);
        }

        // Passa al nodo successivo e libera il corrente
        ObjectNode *tmp = curr;
        curr = curr->next;

        tmp->next = NULL;
        tmp->prev = NULL;
        free(tmp);
    }

    *granades = NULL; // lista svuotata
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
 * Funzione principale del loop di gioco
*/

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


    /**
     * Processo principale
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
    ObjectNode *active_granades = NULL;


    // Chiudo i file descriptor che non uso
    close(ipc.frog_pipe[0]);
    set_nonblocking(ipc.shared_pipe[0]);    //Pipe non bloccante

    while(flag) {
        Message m;

        ssize_t bytes = read(ipc.shared_pipe[0], &m, sizeof m);
        if(bytes > 0) {
            switch (m.obj.type)
            {
            case OBJ_FROG:
                Object new_frog = m.obj;

                // La rana si muove nella mappa
                if (m.msg_type == MSG_UPDATE_POS) 
                {
                    // 1. Utente cerca di uscire fuori dallo schermo
                    if ((active_lane == 0 && new_frog.direction == DIR_DOWN)
                        || is_out_of_screen(win_width, new_frog.x, FROG_WIDTH)) 
                    {
                        set_frog(ipc.frog_pipe[1], frog, &flag);
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
                            set_frog(ipc.frog_pipe[1], new_frog, &flag);
                        }
                        else {
                            // Fiume -> controllo se c’è un coccodrillo sotto la rana
                            on_croc = is_on_croc(&streams[next_active_lane-1], new_frog, &hovered_croc);

                            if (on_croc) {
                                remove_frog(win, frog.y, frog.x, on_grass);
                                active_lane = next_active_lane;
                                on_grass = false;
                                draw_frog(win, new_frog, is_scared);
                                frog = new_frog;
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
                    flag &= spawn_granade(&ipc, frog.x-1, frog.y, DIR_LEFT, active_lane, &active_granades);
                    flag &= spawn_granade(&ipc, (frog.x + FROG_WIDTH +1), frog.y, DIR_RIGHT, active_lane, &active_granades);
                }
            break;
        
            case OBJ_CROC:
                Stream *s = &streams[m.stream_index];

                ObjectNode *curr = find_node_by_pid(s->crocs, m.obj.pid);

                if (curr) {
                    // Processo trovato -> aggiorno o rimuovo
                    if(is_fully_out_of_screen(win_width, m.obj.x, CROC_WIDTH)) {
                        // Rimuovo dal rendering
                        remove_croc(win, curr->data.y, curr->data.x);

                        // Rimuovo dalla lista e kill processo
                        remove_and_kill_node(&s->crocs, curr);

                        // Decremento contatore coccodrilli
                        s->croc_count--;
                    } 
                    else {
                        // Aggiorna coccodrillo
                        remove_croc(win, curr->data.y, curr->data.x);
                        draw_croc(win, m.obj);
                        curr->data = m.obj;

                        bool frog_on_this_croc = (
                            hovered_croc == m.obj.pid && 
                            on_croc &&
                            active_lane == m.stream_index + 1
                        );

                        // Se la rana è sopra questo coccodrillo
                        if (frog_on_this_croc && is_hovering_valid(&frog, &m.obj) && m.msg_type == MSG_UPDATE_POS) {
                            // Aggiorno grafica
                            remove_frog(win, frog.y, frog.x, on_grass);
                            if(frog.x > 0 && (frog.x + FROG_WIDTH) < win_width) frog.x += (s->direction == DIR_LEFT) ? -1 : 1;
                            draw_frog(win, frog, is_scared);

                            // Invio aggiornamento al processo rana
                            set_frog(ipc.frog_pipe[1], frog, &flag);
                        } 
                        else if (hovered_croc == m.obj.pid && on_croc && !is_hovering_valid(&frog, &m.obj)) {
                            reset = true;
                            has_lost_manche = true;
                        }
                    }
                }
        
                if (m.msg_type == MSG_FIRE && curr) {
                    if (s->proj_count < MAX_PROJ_PER_STREAM) {
                        int direction = (curr->data.direction == DIR_LEFT) ? -1 : 1;
                        int next_x;

                        if (curr->data.direction == DIR_LEFT) {
                            next_x = curr->data.x - 1;   // appena a sinistra del coccodrillo
                        } else {
                            next_x = curr->data.x + CROC_WIDTH; // appena a destra del coccodrillo
                        }

                        flag &= spawn_enemy_projectile(s, m.stream_index, &ipc, next_x);
                        draw_frog(win, frog, is_scared);
                    }
                }

                break;

            case OBJ_GRANADE:
                if (m.msg_type == MSG_UPDATE_POS) {
                    // Cerca l'oggetto nella lista di granate
                    ObjectNode *curr = find_node_by_pid(active_granades, m.obj.pid);

                    // Aggiorna la sua posizione
                    if (curr) {
                        if (is_fully_out_of_screen(win_width, curr->data.x, 1)) {
                            remove_granade(win, curr->data.y, curr->data.x, curr->on_grass);    // Rimuovo dal rendering
                            remove_and_kill_node(&active_granades, curr);                       // Rimuovo dalla lista
                        } 
                        else {
                            // Controllo che non ci sia collisione con un proiettile
                            if(!curr->on_grass) {
                                // La granata si trova nel fiume
                                ObjectNode *hit_proj = check_collision_granade_projectiles(curr, streams[m.stream_index-1].projs);

                                if(hit_proj) {
                                    // Rimuovi graficamente entrambi
                                    remove_granade(win, curr->data.y, curr->data.x, curr->on_grass);
                                    remove_enemy_projectile(win, hit_proj->data.y, hit_proj->data.x);

                                    remove_and_kill_node(&active_granades, curr);
                                    curr = NULL;

                                    Stream *s = &streams[m.stream_index-1];
                                    remove_and_kill_node(&s->projs, hit_proj);
                                    s->proj_count--;
                                }
                            }

                            if(curr){
                                // Aggiorna posizione granata
                                remove_granade(win, curr->data.y, curr->data.x, curr->on_grass);
                                draw_granade(win, m.obj);
                                curr->data = m.obj;
                            }
                        }
                    }
                }
                break;

            case OBJ_PROJECTILE:
                if(m.msg_type == MSG_UPDATE_POS) {
                    Stream *s = &streams[m.stream_index];

                    ObjectNode *curr = find_node_by_pid(s->projs, m.obj.pid);

                    if(curr) {
                        if(is_fully_out_of_screen(win_width, curr->data.x, 1)) {
                            remove_enemy_projectile(win, curr->data.y, curr->data.x);   // Rimuovo dal rendering
                            remove_and_kill_node(&s->projs, curr);                      // Rimuovo dalla lista
                
                            s->proj_count--;
                        } 
                        else {
                            // *** Collisione con granata ***
                            ObjectNode *hit_grenade = check_collision_granade_projectiles(curr, active_granades);
                            if (hit_grenade) {
                                remove_enemy_projectile(win, curr->data.y, curr->data.x);
                                remove_granade(win, hit_grenade->data.y, hit_grenade->data.x, hit_grenade->on_grass);

                                remove_and_kill_node(&s->projs, curr);
                                s->proj_count--;

                                remove_and_kill_node(&active_granades, hit_grenade);
                            } 
                            // *** Collisione con rana ***
                            else if(check_collision_frog_projectile(&frog, &curr->data)) {
                                // Reset manche
                                reset = true;
                                has_lost_manche = true;
                            }
                            else {
                                //Nessuna collisione -> aggiorna posizione normalmente
                                remove_enemy_projectile(win, curr->data.y, curr->data.x);
                                draw_enemy_projectile(win, m.obj);
                                curr->data = m.obj;
                            }
                        }
                    }
                }
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


        /**
         * Reset della manche
        */
        if(reset) {
            // Elimino tutti gli elementi del fiume
            clean_all_stream_objects(win, streams);     // Esegue wait()
            clean_all_granades(win, &active_granades);


            // Eliminazione e kill della rana
            remove_frog(win, frog.y, frog.x, on_grass);
            kill(frog_pid, SIGTERM);
            waitpid(frog_pid, NULL, 0);
            wrefresh(win);


            // Aggiorno le statistiche 
            remove_stats(stats_win);
            if(has_lost_manche) {
                stats.lives--;
                update_score(&stats, SCORE_LOSE_LIFE);
            } else {
                update_score(&stats, SCORE_REACH_BURROW);
            }
            stats.time = TIME_PER_ROUND;
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
                
                if (spawn_single_croc(s, i, &ipc, win_width)) {
                    s->last_spawn_time = t;  // aggiorna timestamp
                    s->spawn_time_interval = (rand() % 3) + 4;
                }
            }
        }

        wrefresh(win);
        wrefresh(stats_win);
        usleep(1000);
    }


    /* Chiudo le risorse utilizzate e termino i processi */

    if(frog_pid > 0){
        kill(frog_pid, SIGTERM);
        waitpid(frog_pid, NULL, 0);
    }

    clean_all_stream_objects(win, streams);
    clean_all_granades(win, &active_granades);

    cleanup_ipc_handles(&ipc, sync_sem_name);

    close_window(stats_win);    //Elimino la finestra delle statistiche
    print_game_result(win, win_height, win_width, is_winner, stats.score);   //Stampa schermata di fine
}
