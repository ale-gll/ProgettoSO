
#ifndef UTILS_H
#define UTILS_H

#include <semaphore.h>
#include <stdbool.h>

#define DEBUG_LOG_FILE "res/debug_log.txt"  // Nome del file di log

typedef struct {
    pid_t *croc_pids, *proj_pids;
    int total_crocs, total_projs;
} ActiveProcesses;

typedef struct {
    int shared_pipe[2];     //Pipe per comunicare con la grafica
    int frog_pipe[2];       //Per la comunicazione privata con la rana
    sem_t *sync_sem;        //Sincronizzazione processi produttori
} IPCHandles;

//Inizializza e restituisce un semaforo (Unnamed)
sem_t* init_shared_semaphore(const char *sem_name);

//Funzione per distruggere un semaforo
void clean_up_semaphore(sem_t *sem, char *name);

//Chiude tutti i descrittori di file di una pipe
void clean_up_pipe(int pipe_fd[2]);

//Funzione per settare una pipe come non bloccante
void set_nonblocking(int fd);

bool init_ipc_handles(IPCHandles *ipc, char *sync_sem_name);

void cleanup_ipc_handles(IPCHandles *ipc, char *sync_sem_name);

void kill_all(pid_t *pids, int size);

void debug_log(const char *func_name, int pid, const char *error_msg, const char *log_file);

void log_croc_event(const char *log_file,
                    const char *event,
                    int stream_index,
                    int croc_index,
                    pid_t pid,
                    int x,
                    int y)
;



//Dimensioni delle sprite
#define FROG_WIDTH 5
#define FROG_CROC_HEIGHT 2  //rana e coccodrillo hanno la stessa altezza
#define CROC_WIDTH 12


//Tipi di oggetti dinamici presenti in gioco
typedef enum { OBJ_FROG, OBJ_CROC, OBJ_PROJECTILE, OBJ_GRANADE, OBJ_NONE } ObjectType;

//I coccodrilli e i proiettili/granate si muovono in un unica direzione a parte la rana
typedef enum { DIR_LEFT, DIR_RIGHT, DIR_UP, DIR_DOWN, DIR_UNKNOWN } ObjectDirection;

typedef enum { 
    MSG_UPDATE_POS,         //Aggiorna posizione oggetto
    MSG_FIRE,               //Spara un proiettile
    MSG_SET_FROG,           //Imposta i parametri della rana
    MSG_TOGGLE_ON_CROC,       //Imposta la rana sul coccodrillo
    MSG_KILL                //Uccide il processo
} MessageType;

//Info di un oggetto dinamico
typedef struct {
    int pid;
    int y, x;
    int type;
    int direction;  //DIR_UNKNOWN nel caso della rana
} Object;

//Struttura di un messaggio inviabile al gestore della grafica
typedef struct {
    int msg_type;
    Object obj;
    int stream_index;   //Indice in Streams
    int stream_obj_index;  //Indice nell'array di coccodrilli di uno Stream
    int pid_index;
} Message;

void set_message(Message *m, int msg_type, Object *obj, int *stream_index, 
    int *stream_objs_index, int *pid_index)
;

#endif