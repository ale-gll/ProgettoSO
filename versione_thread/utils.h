
#ifndef UTILS_H
#define UTILS_H

#include <semaphore.h>
#include <stdbool.h>
#include <pthread.h>
#include <stdatomic.h>

#define FROG_WIDTH 5
#define FROG_CROC_HEIGHT 2  //rana e coccodrillo hanno la stessa altezza
#define CROC_WIDTH 12


#define BUFFER_SIZE 128

//Tipi di oggetti dinamici presenti in gioco
typedef enum { OBJ_FROG, OBJ_CROC, OBJ_PROJECTILE, OBJ_GRANADE, OBJ_NONE } ObjectType;

//I coccodrilli e i proiettili/granate si muovono in un unica direzione a parte la rana
typedef enum { DIR_LEFT, DIR_RIGHT, DIR_UP, DIR_DOWN, DIR_UNKNOWN } ObjectDirection;


//Info di un oggetto dinamico
typedef struct {
    int y, x;
    int type;
    int direction;  //DIR_UNKNOWN nel caso della rana
} Object;

//Definisce dove inizia e finisce una tana (indica anche se è stata occupata)
typedef struct {
    bool is_occupied;
    int start_x, end_x;
} Burrow;

//Statistiche di gioco
typedef struct {
    int score;      //Punteggio 
    int lives;      //Vite della rana
    int time;       //Tempo in sec
} Stats;


typedef enum { MSG_UPDATE_POS, MSG_FIRE, MSG_SET_FROG } MessageType;

//Struttura di un messaggio inviabile al gestore della grafica
typedef struct {
    pthread_t tid;
    int msg_type;
    Object obj;
    int stream_index;      //Indice in Streams (coccodrilli, proiettili, granate)
} Message;

void set_message(Message *m, pthread_t tid, int msg_type, Object *obj, int *stream_index);

// Buffer per la comunicazione con il thread di grafica 
typedef struct {
    Message buffer[BUFFER_SIZE];
    int head;                   // indice scrittura
    int tail;                   // indice lettura
    int count;                  // numero di messaggi presenti

    sem_t empty;                // slot liberi
    sem_t full;                 // slot pieni
    pthread_mutex_t mutex;      // protezione accessi al buffer
} SharedBuffer;

// Inizializzazione buffer
void buffer_init(SharedBuffer *buf);

// Distruzione buffer
void buffer_destroy(SharedBuffer *buf);

// Inserimento (produttore)
void produce(SharedBuffer *buf, Message *msg);

// Prelievo (consumatore)
bool consume_try(SharedBuffer *buf, Message *msg);


#endif